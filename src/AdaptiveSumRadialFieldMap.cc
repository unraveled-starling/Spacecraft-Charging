#include "AdaptiveSumRadialFieldMap.hh"
#include "G4Exception.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4PhysicalConstants.hh" 


#include <cmath>
#include <algorithm>
#include <fstream>
#include <omp.h>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <vector>
#include <functional> 
#include <cstdint>   
#include <atomic>    
#include <fstream>
#include <iomanip> 
#include <numeric> 
#include <chrono> 

static const double epsilon0_SI = 8.8541878128e-12 * farad / meter; // F/m
static const G4double k_electric = 1.0 / (4.0 * CLHEP::pi * CLHEP::epsilon0);

namespace {
    struct FieldStats {
        double min;
        double max;
        double mean;
        double std_dev;
        double median;
        double iqr;
        double q1;
        double q3;
        int n_filtered;
    };

    FieldStats calculateFieldStats(std::vector<double>& data) {

        const double epsilon = 1e-12; 
        
        std::vector<double> nonzero_data;
        nonzero_data.reserve(data.size()); 

        std::copy_if(data.begin(), data.end(), 
                     std::back_inserter(nonzero_data), 
                     [&](double x){ return x > epsilon; });
        
        size_t n = nonzero_data.size();

        FieldStats stats = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0}; 

        stats.n_filtered = n;


        std::sort(nonzero_data.begin(), nonzero_data.end());

        stats.min = nonzero_data.front();
        stats.max = nonzero_data.back();

        double sum = std::accumulate(nonzero_data.begin(), nonzero_data.end(), 0.0);
        stats.mean = sum / n;

        double sq_sum = 0.0;
        for (double val : nonzero_data) {
            sq_sum += (val - stats.mean) * (val - stats.mean);
        }
        stats.std_dev = std::sqrt(sq_sum / n);

        if (n % 2 == 1) {
            stats.median = nonzero_data[n / 2];
        } else {
            stats.median = (nonzero_data[n / 2 - 1] + nonzero_data[n / 2]) / 2.0;
        }

        auto find_quantile = [&](double p) {
            if (p < 0.0 || p > 1.0) return 0.0;
            double pos = (n - 1.0) * p + 1.0; 
            double index = std::floor(pos);
            double delta = pos - index;
            
            if (index < 1) return nonzero_data.front();
            if (index >= n) return nonzero_data.back();

            return nonzero_data[static_cast<size_t>(index) - 1] * (1.0 - delta) + nonzero_data[static_cast<size_t>(index)] * delta;
        };


        stats.q1 = find_quantile(0.25);
        stats.q3 = find_quantile(0.75);
        stats.iqr = stats.q3 - stats.q1;

        return stats;
    }
} // end namespace

namespace {
    struct NodeChargeInfo {
        std::vector<int> pos_indices;
        std::vector<int> neg_indices;
    };
}



AdaptiveSumRadialFieldMap::AdaptiveSumRadialFieldMap(
    std::vector<G4ThreeVector>& positions,
    std::vector<G4double>& charges,
    const G4double& gradThreshold,
    G4VSolid* geometry,
    const G4double& dielectricConstant,
    const G4double& minStep,
    const G4double& time_step_dt,
    const G4double& material_temp_K,
    const std::string& state_filename,
    const std::string& filename,
    const G4ThreeVector& min_bounds, 
    const G4ThreeVector& max_bounds,
    int max_depth_param,   
    int initial_depth,
    bool dissipateCharge,       
    StorageType storage)
    : max_depth_(max_depth_param), minStepSize_(minStep),
      worldMin_(min_bounds), worldMax_(max_bounds), fieldGradThreshold_(gradThreshold), fStorage(storage),dissipateCharge_(dissipateCharge),
      fPositions(positions), fCharges(charges), fStateFilename(state_filename), initialDepth_(initial_depth), geometry_(geometry), dielectricConstant_(dielectricConstant) // Use references directly
{

    LoadPersistentState(fStateFilename, fPositions, fCharges);

    total_nodes_.store(0);
    leaf_nodes_.store(0);
    gradient_refinements_.store(0);
    max_depth_reached_.store(0); 
    barnes_hut_theta_ = 0.5;

    auto start_build1 = std::chrono::high_resolution_clock::now();

    G4cout << "Building initial charge octree..." << G4endl;
    buildChargeOctree(); 

    auto end_build1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_build1 - start_build1;
    double duration_in_minutes = duration.count() / 60.0;
    G4cout << "Building initial field octree..." << "(time: " << duration_in_minutes << " min)" << G4endl;
    all_leaves_.clear();
    root_ = buildFromScratch(); 

    std::vector<double> field_magnitudes;
    size_t num_leaves = all_leaves_.size(); 

    G4cout << "Computing field values at initial leaf node..." << G4endl;

    field_magnitudes.resize(num_leaves); 
    
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_leaves; ++i) { 
        Node* leaf = all_leaves_[i];
        if(leaf) {
            leaf->precomputed_field = computeFieldFromCharges(leaf->center);
            field_magnitudes[i] = leaf->precomputed_field.mag();
        } else {
             field_magnitudes[i] = 0.0;
        }
    }
    FieldStats stats = calculateFieldStats(field_magnitudes);
    
    G4cout << "   >>> Mean of Field Magnitude: " << G4BestUnit(stats.mean,"Electric field") << " <<<" << G4endl;
    G4cout << "   >>> Median of Field Magnitude: " << G4BestUnit(stats.median,"Electric field") << " <<<" << G4endl;
    G4cout << "   >>> Std of Field Magnitude: " << G4BestUnit(stats.std_dev,"Electric field") << " <<<" << G4endl;
    G4cout << "   >>> IQR of Field Magnitude: " << G4BestUnit(stats.iqr,"Electric field") << " <<<" << G4endl;
    G4cout << "   >>> Min of Field Magnitude: " << G4BestUnit(stats.min,"Electric field") << " <<<" << G4endl;
    G4cout << "   >>> Max of Field Magnitude: " << G4BestUnit(stats.max,"Electric field") << " <<<" << G4endl;

    fieldGradThreshold_ = stats.max*gradThreshold; 
    G4cout << "   --> Field Gradient Threshold (V/m): " << G4BestUnit(fieldGradThreshold_,"Electric field") << G4endl;
    
    auto end_computations = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration2 = end_computations - end_build1;
    double duration_in_minutes2 = duration2.count() / 60.0;
    G4cout << "Refining field map based on field gradients..." << "(time: " << duration_in_minutes2 << " min)" << G4endl;
    #pragma omp parallel
    {
        #pragma omp single
        refineMeshByGradient(root_.get(), 0);
    }
    if (dissipateCharge_) { 

        G4cout << "Applying one-time charge dissipation ..." << G4endl;
        ApplyChargeDissipation(time_step_dt, material_temp_K); 
        auto end_charge = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration3 = end_charge - end_build1;
        double duration_in_minutes3 = duration3.count() / 60.0;
        G4cout << "(time: " << duration_in_minutes3 << " min)" << G4endl;
    } 

    all_leaves_.clear();
    collectFinalLeaves(root_.get());
    leaf_nodes_.store(static_cast<int>(all_leaves_.size()));


    G4cout << "Applying dielectric scaling to final mesh..." << G4endl;
    num_leaves = all_leaves_.size();

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_leaves; ++i) { 
        Node* leaf = all_leaves_[i];
        if(leaf) {
            // Check boundary overlap for this specific leaf
            double half_width = (leaf->max.x() - leaf->min.x()) * 0.5;
            double f = GetDielectricFraction(leaf->center, half_width);

            // Apply scaling only if partially or fully inside
            if (f > 0.0) {
                double epsilon_eff_perp = 1.0 / ( (1.0 - f) + (f / dielectricConstant_) );
                 leaf->precomputed_field = leaf->precomputed_field / epsilon_eff_perp;
            }
        }
    }

    G4cout << "   --> Saving refined field to " <<filename << G4endl;
    if (!filename.empty()) {
        ExportFieldMapToFile(filename);
    }
    PrintMeshStatistics();
    SaveFinalParticleState(state_filename);

}

AdaptiveSumRadialFieldMap::~AdaptiveSumRadialFieldMap() {
}

void AdaptiveSumRadialFieldMap::LoadPersistentState(const std::string& filename,
                                                    std::vector<G4ThreeVector>& positions,
                                                    std::vector<G4double>& charges)
{
    size_t initial_count = positions.size();
    G4cout << "Loading persistent particle state from " << filename << " and appending..." << G4endl;
    std::ifstream infile(filename, std::ios::binary);

    if (infile.is_open()) {
        uint64_t num_particles = 0;
        infile.read(reinterpret_cast<char*>(&num_particles), sizeof(uint64_t));

        for (uint64_t i = 0; i < num_particles; ++i) {
            double pos[3];
            double charge;
            infile.read(reinterpret_cast<char*>(pos), sizeof(pos));
            infile.read(reinterpret_cast<char*>(&charge), sizeof(double));

            if (infile.fail()) {
                G4cerr << "Warning: Unexpected EOF or read failure while reading particle " << i << G4endl;
                break;
            }

            positions.push_back(G4ThreeVector(pos[0], pos[1], pos[2]));
            charges.push_back(charge);
        }

        infile.close();
        G4cout << "   Loaded " << positions.size() - initial_count << " persistent particles." << G4endl;
        G4cout << "   Total particles now: " << positions.size() << G4endl;
    } else {
        G4cout << "   " << filename << " not found. Using only initially provided particles." << G4endl;
    }

    if (positions.size() != charges.size()) {
        G4Exception("AdaptiveSumRadialFieldMap::LoadPersistentState", "SizeMismatch", FatalException,
                    "Position and charge vectors have different sizes after loading state.");
    }
}

void AdaptiveSumRadialFieldMap::SaveFinalParticleState(const std::string& filename) const
{
    G4cout << "Saving final particle state (non-zero charges) to " << filename << "..." << G4endl;
    std::ofstream outfile(filename, std::ios::trunc | std::ios::binary); // Overwrite, binary format
    if (!outfile.is_open()) {
        G4cerr << "Error: Could not open " << filename << " for writing particle state!" << G4endl;
        return;
    }

    uint64_t num_particles_to_save = 0; 
    auto count_position = outfile.tellp(); 
    outfile.write(reinterpret_cast<const char*>(&num_particles_to_save), sizeof(uint64_t));

    const G4double charge_threshold = 1e-21 * CLHEP::eplus; // Threshold to consider charge zero

    for (size_t i = 0; i < fPositions.size(); ++i) {
        if (std::abs(fCharges[i]) > charge_threshold) {
            double pos[3] = { fPositions[i].x(), fPositions[i].y(), fPositions[i].z() };
            outfile.write(reinterpret_cast<const char*>(pos), sizeof(pos));
            double charge = fCharges[i];
            outfile.write(reinterpret_cast<const char*>(&charge), sizeof(double));

            num_particles_to_save++; // Increment count of saved particles
        }
    }

    outfile.seekp(count_position); // Move write pointer back to the beginning
    outfile.write(reinterpret_cast<const char*>(&num_particles_to_save), sizeof(uint64_t)); // Write the correct count
    outfile.seekp(0, std::ios::end); // Go back to the end if writing more data later (optional)


    outfile.close();
    if (outfile.good()) {
        G4cout << "   Particle state saved (" << num_particles_to_save << " non-zero particles)." << G4endl;
    } else {
         G4cerr << "Error: Could not write successfully to " << filename << G4endl;
    }
}

void AdaptiveSumRadialFieldMap::ApplyChargeDissipation(G4double dt_internal, G4double temp_K) {

    G4cout << "    Building particle-to-leaf map (parallel map-reduce)..." << G4endl;

    int num_threads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        num_threads = omp_get_num_threads();
    }
    std::vector<std::map<Node*, NodeChargeInfo>> private_maps(num_threads);

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        
        auto& thread_local_map = private_maps[thread_id]; 

        #pragma omp for schedule(dynamic, 1000)
        for (size_t i = 0; i < fPositions.size(); ++i) {
            
            if (std::abs(fCharges[i]) < 1e-21 * CLHEP::eplus) continue;

            Node* leaf = findLeafNode(fPositions[i], root_.get()); 
            
            if (!leaf) continue; // Particle is outside the map, skip it.

            if (fCharges[i] > 0) {
                thread_local_map[leaf].pos_indices.push_back(static_cast<int>(i));
            } else {
                thread_local_map[leaf].neg_indices.push_back(static_cast<int>(i));
            }
        }
    } 


    
    G4cout << "    Merging " << private_maps.size() << " thread-local maps..." << G4endl;
    std::map<Node*, NodeChargeInfo> global_leaf_map; 

    for (const auto& local_map : private_maps) { // Loop over each thread's map
        for (const auto& pair : local_map) {     // Loop over each leaf in that map
            Node* leaf = pair.first;
            const NodeChargeInfo& info = pair.second;

            global_leaf_map[leaf].pos_indices.insert(
                global_leaf_map[leaf].pos_indices.end(),
                info.pos_indices.begin(), info.pos_indices.end()
            );
            global_leaf_map[leaf].neg_indices.insert(
                global_leaf_map[leaf].neg_indices.end(),
                info.neg_indices.begin(), info.neg_indices.end()
            );
        }
    }
    
    private_maps.clear(); 
    
    G4cout << "    Applying dissipation to " << global_leaf_map.size() << " active leaves..." << G4endl;

    double conductivity_SI = calculateConductivity(temp_K);
    const double epsilon0_SI_Value = CLHEP::epsilon0 / (farad/meter); // Get SI value
    double rate_constant_Value = 0.0;
    if (epsilon0_SI_Value > 1e-18) { // Avoid division by zero
        rate_constant_Value = conductivity_SI / epsilon0_SI_Value;
    } else {
        G4cerr << "Warning: Epsilon0 value is too small, dissipation rate set to 0." << G4endl;
    }

    G4double total_dissipated_charge = 0.0;

    std::vector<std::pair<Node*, NodeChargeInfo>> leaf_vector(global_leaf_map.begin(), global_leaf_map.end());

    #pragma omp parallel for schedule(dynamic) reduction(+:total_dissipated_charge)
    for (size_t i = 0; i < leaf_vector.size(); ++i) {
        
        const auto& pair = leaf_vector[i];

        const std::vector<int>& positive_particle_indices = pair.second.pos_indices;
        const std::vector<int>& negative_particle_indices = pair.second.neg_indices;

        double Q_node_net_Internal = 0.0;
        for (int idx : positive_particle_indices) {
             if (idx >= 0 && static_cast<size_t>(idx) < fCharges.size()) Q_node_net_Internal += fCharges[idx];
        }
        for (int idx : negative_particle_indices) {
             if (idx >= 0 && static_cast<size_t>(idx) < fCharges.size()) Q_node_net_Internal += fCharges[idx];
        }
        // --- End of replacement logic ---

        if (std::abs(Q_node_net_Internal) < 1e-21 * CLHEP::eplus) continue;

        double Q_node_net_Value = Q_node_net_Internal / CLHEP::eplus; // Dimensionless number of elementary charges

        double dt_Value = dt_internal; // / CLHEP::second; // Make sure dt_internal has time unit!
        
        double delta_Q_node_Value = -rate_constant_Value * Q_node_net_Value * dt_Value;

        // --- Re-apply the unit at the end ---
        double delta_Q_node = delta_Q_node_Value * CLHEP::eplus; // Result now has units of e+
        
        double charge_change_magnitude = std::abs(delta_Q_node); // Still in e+ units


        if (charge_change_magnitude > 1e-14 * CLHEP::eplus) { // Use a small threshold
            total_dissipated_charge += charge_change_magnitude; // Accumulate magnitude in e+

            if (delta_Q_node < 0 && !positive_particle_indices.empty()) {
                distributeChargeChange(positive_particle_indices, -charge_change_magnitude);
            } else if (delta_Q_node > 0 && !negative_particle_indices.empty()) {
                distributeChargeChange(negative_particle_indices, charge_change_magnitude);
            }
        }
    } 

    G4cout << "    --> Charge dissipation applied (neutralizing)." << G4endl;
    G4cout << "    >>> Total charge dissipated/neutralized this step: "
           << total_dissipated_charge / CLHEP::eplus << " e <<<" << G4endl;
}

double AdaptiveSumRadialFieldMap::calculateConductivity(double temp_K) const {
    if (temp_K <= 0) return 0.0;
    return 6.0e-18 * std::exp(0.0230 * temp_K);
}

void AdaptiveSumRadialFieldMap::distributeChargeChange(const std::vector<int>& particle_indices, G4double total_charge_change) {
     // Check for invalid inputs
     if (particle_indices.empty() || std::abs(total_charge_change) < 1e-25 * CLHEP::coulomb) return; // Use smaller threshold

     // Calculate change per particle for this specific group
     double charge_change_per_particle = total_charge_change / particle_indices.size();

     for (int idx : particle_indices) {
         // Check index bounds for safety
         if (idx >= 0 && static_cast<size_t>(idx) < fCharges.size()) {
             #pragma omp atomic update
             fCharges[idx] += charge_change_per_particle;

             G4double current_charge;
             #pragma omp atomic read
             current_charge = fCharges[idx];

             if ((charge_change_per_particle < 0 && current_charge < 1e-21 * CLHEP::eplus) || // Went negative
                 (charge_change_per_particle > 0 && current_charge > -1e-21 * CLHEP::eplus)) { // Went positive
                  #pragma omp atomic write
                  fCharges[idx] = 0.0;
             }
         } else {
            #pragma omp critical (error_log) // Use critical section for thread-safe error output
            G4cerr << "Warning: Invalid particle index (" << idx << ") in distributeChargeChange." << G4endl;
         }
     }
}


double AdaptiveSumRadialFieldMap::GetDielectricFraction(const G4ThreeVector& c, double hw) const {
    if (!geometry_) return 0.0;

    G4ThreeVector points[] = {
        c,
        c + G4ThreeVector( hw,  hw,  hw), c + G4ThreeVector( hw,  hw, -hw),
        c + G4ThreeVector( hw, -hw,  hw), c + G4ThreeVector( hw, -hw, -hw),
        c + G4ThreeVector(-hw,  hw,  hw), c + G4ThreeVector(-hw,  hw, -hw),
        c + G4ThreeVector(-hw, -hw,  hw), c + G4ThreeVector(-hw, -hw, -hw)
    };

    int inside_count = 0;
    for (const auto& p : points) {
        if (geometry_->Inside(p) != kOutside) { 
            inside_count++;
        }
    }

    return (double)inside_count / 9.0; 
}


void AdaptiveSumRadialFieldMap::buildChargeOctree() {
    charge_root_.reset(); // Important when rebuilding
    if (fPositions.empty()) return;
    G4ThreeVector min_box, max_box; calculateBoundingBox(min_box, max_box);
    charge_root_ = std::make_unique<ChargeNode>();
    charge_root_->is_leaf = true; charge_root_->particle_index = -1; charge_root_->total_charge = 0.0; charge_root_->center_of_mass = (min_box + max_box) * 0.5;
    for (size_t i = 0; i < fPositions.size(); ++i) {
        if (std::abs(fCharges[i]) > 1e-25 * CLHEP::coulomb && pointInside(min_box, max_box, fPositions[i])) {
            insertCharge(charge_root_.get(), static_cast<int>(i), min_box, max_box);
        }
    }
}

void AdaptiveSumRadialFieldMap::insertCharge(ChargeNode* node, int particle_index, const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds) {
    if (!node) return; if (particle_index < 0 || static_cast<size_t>(particle_index) >= fPositions.size()) {
        G4Exception("AdaptiveSumRadialFieldMap::insertCharge", "IndexOutOfBounds", FatalException, "Invalid particle index provided."); return;
    }
    G4double old_total_charge = node->total_charge; G4double charge_to_add = fCharges[particle_index]; node->total_charge += charge_to_add;
    if (std::abs(node->total_charge) > 1e-25 * CLHEP::coulomb) { if (std::abs(old_total_charge) > 1e-25 * CLHEP::coulomb) { node->center_of_mass = (node->center_of_mass * old_total_charge + fPositions[particle_index] * charge_to_add) / node->total_charge; } else { node->center_of_mass = fPositions[particle_index]; } } else { node->center_of_mass = (min_bounds + max_bounds) * 0.5; }
    if (node->is_leaf) {
        if (node->particle_index == -1) { node->particle_index = particle_index; }
        else {
            int old_particle_index = node->particle_index; bool aggregate = false;
            if (old_particle_index == -2) { aggregate = true; } else if (old_particle_index >= 0) { if (fPositions[particle_index] == fPositions[old_particle_index]) { aggregate = true; } else { const G4double minBoxSizeSq = minStepSize_ * minStepSize_; if ((max_bounds - min_bounds).mag2() < minBoxSizeSq) { aggregate = true; } } }
            if (aggregate && old_particle_index != -2) { node->particle_index = -2; return; }
            node->is_leaf = false; node->particle_index = -1; G4ThreeVector center = (min_bounds + max_bounds) * 0.5;
            if (old_particle_index >= 0) {
                 int old_child_idx = 0; if (fPositions[old_particle_index].x() >= center.x()) old_child_idx |= 1; if (fPositions[old_particle_index].y() >= center.y()) old_child_idx |= 2; if (fPositions[old_particle_index].z() >= center.z()) old_child_idx |= 4;
                 G4ThreeVector old_min, old_max; calculateChildBounds(min_bounds, max_bounds, center, old_child_idx, old_min, old_max); if (!node->children[old_child_idx]) node->children[old_child_idx] = std::make_unique<ChargeNode>(); insertCharge(node->children[old_child_idx].get(), old_particle_index, old_min, old_max);
            }
             int new_child_idx = 0; if (fPositions[particle_index].x() >= center.x()) new_child_idx |= 1; if (fPositions[particle_index].y() >= center.y()) new_child_idx |= 2; if (fPositions[particle_index].z() >= center.z()) new_child_idx |= 4;
             G4ThreeVector new_min, new_max; calculateChildBounds(min_bounds, max_bounds, center, new_child_idx, new_min, new_max); if (!node->children[new_child_idx]) node->children[new_child_idx] = std::make_unique<ChargeNode>(); insertCharge(node->children[new_child_idx].get(), particle_index, new_min, new_max);
        }
    } else { G4ThreeVector center = (min_bounds + max_bounds) * 0.5; int child_idx = 0; if (fPositions[particle_index].x() >= center.x()) child_idx |= 1; if (fPositions[particle_index].y() >= center.y()) child_idx |= 2; if (fPositions[particle_index].z() >= center.z()) child_idx |= 4; G4ThreeVector c_min, c_max; calculateChildBounds(min_bounds, max_bounds, center, child_idx, c_min, c_max); if (!node->children[child_idx]) node->children[child_idx] = std::make_unique<ChargeNode>(); insertCharge(node->children[child_idx].get(), particle_index, c_min, c_max); }
}



G4ThreeVector AdaptiveSumRadialFieldMap::computeFieldWithApproximation(
    const G4ThreeVector& point, 
    const ChargeNode* node, 
    const G4ThreeVector& node_min, 
    const G4ThreeVector& node_max) const 
{
    if (!node || std::abs(node->total_charge) < 1e-25 * CLHEP::coulomb) return G4ThreeVector(0,0,0);

    // --- FORCE ACCEPT AGGREGATED NODE ---
    if (node->particle_index == -2) { // aggregated node
        G4ThreeVector r = point - node->center_of_mass;
        G4double r2 = r.mag2() + (1.0*nm)*(1.0*nm); // softening
        return r * (node->total_charge * k_electric / std::pow(r2, 1.5));
    }

    G4ThreeVector displacement = point - node->center_of_mass;
    G4double d2 = displacement.mag2();
    const G4double softening_factor_sq = minStepSize_*minStepSize_;
    if (d2 < softening_factor_sq) d2 = softening_factor_sq;

    if (node->is_leaf) {
        if (node->particle_index != -1) {
            G4double inv_d3 = 1.0 / (std::sqrt(d2) * d2);
            return displacement * (node->total_charge * k_electric * inv_d3);
        } else return G4ThreeVector(0,0,0);
    }

    double width = (node_max.x() - node_min.x());
    double distance = std::sqrt(d2);

    if (width / distance < barnes_hut_theta_) {
        G4double inv_d3 = 1.0 / (distance * d2);
        return displacement * (node->total_charge * k_electric * inv_d3);
    }

    // recurse into children
    G4ThreeVector E_total(0,0,0);
    G4ThreeVector center = (node_min + node_max) * 0.5;
    for(int i = 0; i < 8; ++i) {
        if(node->children[i]) {
            G4ThreeVector c_min, c_max;
            calculateChildBounds(node_min, node_max, center, i, c_min, c_max);
            E_total += computeFieldWithApproximation(point, node->children[i].get(), c_min, c_max);
        }
    }
    return E_total;
}


G4ThreeVector AdaptiveSumRadialFieldMap::computeFieldFromCharges(const G4ThreeVector& point) const { if (!charge_root_) return G4ThreeVector(0,0,0); G4ThreeVector min_box, max_box; calculateBoundingBox(min_box, max_box); return computeFieldWithApproximation(point, charge_root_.get(), min_box, max_box); }

std::unique_ptr<AdaptiveSumRadialFieldMap::Node> AdaptiveSumRadialFieldMap::buildFromScratch() {
    
    G4cout << "   Building coarse grid to depth " << initialDepth_ << "..." << G4endl;

    G4ThreeVector min_box, max_box;
    calculateBoundingBox(min_box, max_box);
    auto root_node = std::make_unique<Node>();
    root_node->min = min_box;
    root_node->max = max_box;
    root_node->center = (min_box + max_box) * 0.5;

    total_nodes_.store(1); // Start with 1 (the root)
    max_depth_reached_.store(0);
    
    buildUniformGrid(root_node.get(), 0);

    all_leaves_.clear();
    collectFinalLeaves(root_node.get());
    leaf_nodes_.store(static_cast<int>(all_leaves_.size()));
    
    G4cout << "   Coarse grid built with " << leaf_nodes_.load() << " leaf nodes." << G4endl;

    return root_node;
}


void AdaptiveSumRadialFieldMap::buildUniformGrid(Node* node, int depth)
{
    if (depth == initialDepth_) {
        node->is_leaf = true;
        return;
    }

    // Recursive Case: Not deep enough. Subdivide.
    node->is_leaf = false;
    for (int i = 0; i < 8; ++i) {
        G4ThreeVector child_min, child_max;
        calculateChildBounds(node->min, node->max, node->center, i, child_min, child_max);

        auto child = std::make_unique<Node>();
        child->min = child_min;
        child->max = child_max;
        child->center = (child_min + child_max) * 0.5;

        total_nodes_++; // Atomically increment total node count

        buildUniformGrid(child.get(), depth + 1);

        node->children[i] = std::move(child);
    }
}

std::unique_ptr<AdaptiveSumRadialFieldMap::Node> AdaptiveSumRadialFieldMap::createOctreeFromScratch(const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds, int depth) {
    auto node = std::make_unique<Node>(); node->min = min_bounds; node->max = max_bounds; node->center = (min_bounds + max_bounds) * 0.5; node->is_leaf = true; total_nodes_++;
    int current_max = max_depth_reached_.load(std::memory_order_relaxed);
    while (depth > current_max) {
        if (max_depth_reached_.compare_exchange_weak(current_max, depth, std::memory_order_relaxed)) break;
    }
    leaf_nodes_++; return node;
}


AdaptiveSumRadialFieldMap::Node* AdaptiveSumRadialFieldMap::findLeafNode(const G4ThreeVector& point, Node* node) const
{
    if (!node) return nullptr;
    if (node->is_leaf) return node; // Found the leaf

    G4ThreeVector center = node->center;
    int child_idx = 0;
    if (point.x() >= center.x()) child_idx |= 1;
    if (point.y() >= center.y()) child_idx |= 2;
    if (point.z() >= center.z()) child_idx |= 4;

    if (node->children[child_idx]) {
        // Recurse into the correct child
        return findLeafNode(point, node->children[child_idx].get());
    }
    #pragma omp critical (error_log)
    { // Add braces for proper critical section scope
        G4cerr << "Warning: findLeafNode reached internal node with missing child for point "
               << point << " within node bounds [" << node->min << ", " << node->max << "]" << G4endl;
    }
    return nullptr;
}

void AdaptiveSumRadialFieldMap::refineMeshByGradient(Node* node, int depth) {
    if (!node) return;

    int current_max = max_depth_reached_.load(std::memory_order_relaxed);
    while (depth > current_max) {
        if (max_depth_reached_.compare_exchange_weak(current_max, depth, std::memory_order_relaxed)) break;
    }

    if (node->is_leaf) {
        double size = (node->max.x() - node->min.x());
        if (depth < max_depth_ && size > minStepSize_ && hasHighFieldGradient(node->center, node->precomputed_field, size * 0.2)) {

            gradient_refinements_++;
            node->is_leaf = false;
            leaf_nodes_--; // This leaf is no longer a leaf

            // Create 8 new children
            for (int i = 0; i < 8; ++i) {
                G4ThreeVector c_min, c_max;
                calculateChildBounds(node->min, node->max, node->center, i, c_min, c_max);
                auto child = std::make_unique<Node>();
                child->min = c_min;
                child->max = c_max;
                child->center = (c_min + c_max) * 0.5;
                child->is_leaf = true; // The new children are leaves
                child->precomputed_field = computeFieldFromCharges(child->center);
                total_nodes_++;
                leaf_nodes_++; // Add new leaf
                node->children[i] = std::move(child);
            }

            for (int i = 0; i < 8; ++i) {
                if(node->children[i]) {
                    #pragma omp task
                    refineMeshByGradient(node->children[i].get(), depth + 1);
                }
            }

            #pragma omp taskwait
        }

    } else {

        for (int i = 0; i < 8; ++i) {
            if(node->children[i]) {
                #pragma omp task
                refineMeshByGradient(node->children[i].get(), depth + 1);
            }
        }

        #pragma omp taskwait
    }
}


bool AdaptiveSumRadialFieldMap::hasHighFieldGradient(
    const G4ThreeVector& center, 
    const G4ThreeVector& parent_field, 
    double leaf_size) const 
{
    if (fPositions.empty()) return false;

    // Step proportional to leaf size, clamped to reasonable limits
    double dx = std::clamp(0.2 * leaf_size, 0.1*nm, 1.0*um);

    // Diagonal directions for sampling: +diag, -diag, +X, just 3 points total
    std::vector<G4ThreeVector> sample_points = {
        center + G4ThreeVector( dx,  dx,  dx),
        center + G4ThreeVector(-dx, -dx, -dx),
        center + G4ThreeVector( dx, -dx,  dx) // arbitrary third diagonal
    };

    std::vector<double> field_magnitudes(sample_points.size());

    #pragma omp parallel for
    for (size_t i = 0; i < sample_points.size(); ++i) {
        field_magnitudes[i] = computeFieldFromCharges(sample_points[i]).mag();
    }

    // Approximate gradient using finite differences along diagonal
    double grad_sq = 0.0;
    for (double f_mag : field_magnitudes) {
        double df = f_mag - parent_field.mag();
        grad_sq += (df*df);
    }

    grad_sq /= (sample_points.size() * dx * dx); // normalize by step^2

    return grad_sq > (fieldGradThreshold_ * fieldGradThreshold_);
}



void AdaptiveSumRadialFieldMap::collectFinalLeaves(Node* node) { if (!node) return; if (node->is_leaf) { all_leaves_.push_back(node); } else { for (int i = 0; i < 8; ++i) { if(node->children[i]) collectFinalLeaves(node->children[i].get()); } } }

void AdaptiveSumRadialFieldMap::GetFieldValue(const G4double point[4], G4double field[6]) const { const G4ThreeVector r(point[0], point[1], point[2]); G4ThreeVector E = evaluateField(r); field[0]=0; field[1]=0; field[2]=0; field[3]=E.x(); field[4]=E.y(); field[5]=E.z(); }
G4ThreeVector AdaptiveSumRadialFieldMap::evaluateField(const G4ThreeVector& point) const { if (!pointInside(worldMin_, worldMax_, point)) return G4ThreeVector(0,0,0); if (!root_) return G4ThreeVector(0,0,0); return evaluateFieldRecursive(point, root_.get()); }
G4ThreeVector AdaptiveSumRadialFieldMap::evaluateFieldRecursive(const G4ThreeVector& point, const Node* node) const {
    if (!node) return G4ThreeVector(0,0,0);
    if (node->is_leaf) return node->precomputed_field;
    G4ThreeVector center = node->center;
    int child_idx = 0;
    if (point.x() >= center.x()) child_idx |= 1;
    if (point.y() >= center.y()) child_idx |= 2;
    if (point.z() >= center.z()) child_idx |= 4;
    if (node->children[child_idx]) {
        return evaluateFieldRecursive(point, node->children[child_idx].get());
    } else {
        // Return zero instead of G4Exception
        #pragma omp critical (error_log)
        G4cerr << "Warning: Point in internal node missing child during eval." << G4endl;
        return G4ThreeVector(0,0,0);
    }
}

bool AdaptiveSumRadialFieldMap::pointInside(const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds, const G4ThreeVector& point) const { return (point.x() >= min_bounds.x() && point.x() <= max_bounds.x() && point.y() >= min_bounds.y() && point.y() <= max_bounds.y() && point.z() >= min_bounds.z() && point.z() <= max_bounds.z()); }
void AdaptiveSumRadialFieldMap::calculateBoundingBox(G4ThreeVector& min_box, G4ThreeVector& max_box) const { min_box = worldMin_; max_box = worldMax_; }
void AdaptiveSumRadialFieldMap::PrintMeshStatistics() const { G4cout << "\n=== Adaptive Mesh Statistics ===" << G4endl; G4cout << "Total nodes created:      " << total_nodes_.load() << G4endl; G4cout << "Final leaf nodes:         " << leaf_nodes_.load() << G4endl; G4cout << "Max octree depth reached: " << max_depth_reached_.load() << G4endl; G4cout << "Gradient refinements:     " << gradient_refinements_.load() << G4endl; G4cout << "=================================\n" << G4endl; }
void AdaptiveSumRadialFieldMap::calculateChildBounds(const G4ThreeVector& p_min, const G4ThreeVector& p_max, const G4ThreeVector& p_cen, int c_idx, G4ThreeVector& c_min, G4ThreeVector& c_max) const { c_min.setX((c_idx & 1) ? p_cen.x() : p_min.x()); c_min.setY((c_idx & 2) ? p_cen.y() : p_min.y()); c_min.setZ((c_idx & 4) ? p_cen.z() : p_min.z()); c_max.setX((c_idx & 1) ? p_max.x() : p_cen.x()); c_max.setY((c_idx & 2) ? p_max.y() : p_cen.y()); c_max.setZ((c_idx & 4) ? p_max.z() : p_cen.z()); }

void AdaptiveSumRadialFieldMap::ExportFieldMapToFile(const std::string& filename) const {
    G4cout << "Exporting adaptive binary field map (all " << total_nodes_.load() << " nodes)..." << G4endl;

    std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
    if (!outfile.is_open()) {
        G4Exception("AdaptiveSumRadialFieldMap::ExportFieldMapToFile", "FileOpenError", FatalException, ("Failed to open file for writing: " + filename).c_str());
        return;
    }

    // Octree Parameters
    uint32_t max_d = static_cast<uint32_t>(max_depth_);
    double min_s = minStepSize_;

    uint64_t total_node_count = static_cast<uint64_t>(total_nodes_.load());
    uint64_t final_leaf_count = static_cast<uint64_t>(leaf_nodes_.load()); 

    outfile.write(reinterpret_cast<const char*>(&max_d), sizeof(max_d));
    outfile.write(reinterpret_cast<const char*>(&min_s), sizeof(min_s));
    outfile.write(reinterpret_cast<const char*>(&total_node_count), sizeof(total_node_count));
    outfile.write(reinterpret_cast<const char*>(&final_leaf_count), sizeof(final_leaf_count)); 

    writeFieldPointsToFileRecursive(outfile, root_.get()); 
    
    uint32_t grad_ref = static_cast<uint32_t>(gradient_refinements_.load());
    uint32_t max_depth_r = static_cast<uint32_t>(max_depth_reached_.load());
    outfile.write(reinterpret_cast<const char*>(&grad_ref), sizeof(grad_ref));
    outfile.write(reinterpret_cast<const char*>(&max_depth_r), sizeof(max_depth_r));

    outfile.close();
}


void AdaptiveSumRadialFieldMap::writeFieldPointsToFileRecursive(std::ofstream& outfile, const Node* node) const {
    if (!node) return;

    float pos[3] = { static_cast<float>(node->center.x()), static_cast<float>(node->center.y()), static_cast<float>(node->center.z()) };
    outfile.write(reinterpret_cast<const char*>(pos), sizeof(pos));
    
    float field[3] = { static_cast<float>(node->precomputed_field.x()), static_cast<float>(node->precomputed_field.y()), static_cast<float>(node->precomputed_field.z()) };
    outfile.write(reinterpret_cast<const char*>(field), sizeof(field));
    

    if (!node->is_leaf) {
        for (int i = 0; i < 8; ++i) {
            writeFieldPointsToFileRecursive(outfile, node->children[i].get());
        }
    }
}