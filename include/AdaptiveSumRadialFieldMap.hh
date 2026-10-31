#ifndef AdaptiveSumRadialFieldMap_h
#define AdaptiveSumRadialFieldMap_h 1

#include "G4ElectricField.hh"
#include "G4ThreeVector.hh"
#include "G4Types.hh"          // For G4double
#include "G4SystemOfUnits.hh"  // Defines units like m, nm, um
#include "G4PhysicalConstants.hh" // Defines eplus, epsilon0, pi
#include "G4VSolid.hh"

#include <vector>
#include <string>
#include <memory>              // For std::unique_ptr
#include <fstream>             // For std::ofstream
#include <atomic>              // For std::atomic counters
#include <cstdint>             // For uint32_t, uint64_t in enums/export


class G4VSolid;

class AdaptiveSumRadialFieldMap : public G4ElectricField {

public:
    // Enum to choose storage precision
    enum class StorageType : uint32_t { Double = 0, Float = 1 };

    AdaptiveSumRadialFieldMap(
        std::vector<G4ThreeVector>& positions,
        std::vector<G4double>& charges,
        const G4double& gradThreshold,
        G4VSolid* geometry,
        const G4double& dielectricConstant,
        const G4double& minStep,
        const G4double& time_step_dt,
        const G4double& material_temp_K,
        const std::string& state_filename = "charges.txt",
        const std::string& filename = "",
        const G4ThreeVector& min_bounds_default = G4ThreeVector(-1*m, -1*m, -1*m),
        const G4ThreeVector& max_bounds_default = G4ThreeVector( 1*m,  1*m,  1*m),
        int max_depth_default = 10,
        int initial_depth_default = 5,
        bool dissipateCharge_default = true,
        StorageType storage = StorageType::Double
    );

    ~AdaptiveSumRadialFieldMap() override;

    void GetFieldValue(const G4double point[4], G4double field[6]) const override;

    void ExportFieldMapToFile(const std::string& filename) const;
    void SaveFinalParticleState(const std::string& filename) const;
    void PrintMeshStatistics() const;
    G4ThreeVector evaluateField(const G4ThreeVector& point) const;


private:
    struct Node {
        G4ThreeVector min, max, center;
        G4ThreeVector precomputed_field = G4ThreeVector(0,0,0);
        std::unique_ptr<Node> children[8] = {nullptr};
        bool is_leaf = true;
    };

    struct ChargeNode {
        std::unique_ptr<ChargeNode> children[8] = {nullptr};
        G4ThreeVector center_of_mass = G4ThreeVector(0,0,0);
        G4double total_charge = 0.0;
        int particle_index = -1; 
        bool is_leaf = true;
    };

    int max_depth_;
    G4double minStepSize_;
    G4ThreeVector worldMin_;
    G4ThreeVector worldMax_;
    G4double fieldGradThreshold_;
    StorageType fStorage;
    G4double barnes_hut_theta_;
    int initialDepth_;
    bool dissipateCharge_;
    G4VSolid* geometry_;
    G4double dielectricConstant_;

    std::vector<G4ThreeVector>& fPositions;
    std::vector<G4double>& fCharges;
    std::string fStateFilename;


    std::unique_ptr<Node> root_;
    std::unique_ptr<ChargeNode> charge_root_;


    std::vector<Node*> all_leaves_;


    std::atomic<int> total_nodes_;
    std::atomic<int> leaf_nodes_; // Tracks final leaf count
    std::atomic<int> gradient_refinements_;
    std::atomic<int> max_depth_reached_; // Use atomic for max during parallel refine



    
    void LoadPersistentState(const std::string& filename,
                               std::vector<G4ThreeVector>& positions, // Modify external vectors
                               std::vector<G4double>& charges);

    std::unique_ptr<Node> buildFromScratch();

    std::unique_ptr<Node> createOctreeFromScratch(const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds, int depth);
    void refineMeshByGradient(Node* node, int depth);
    bool hasHighFieldGradient(const G4ThreeVector& center, const G4ThreeVector& center_field, double sample_distance) const;
    void collectFinalLeaves(Node* node);
    G4ThreeVector evaluateFieldRecursive(const G4ThreeVector& point, const Node* node) const;

    void buildChargeOctree();
    void insertCharge(ChargeNode* node, int particle_index, const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds);
    G4ThreeVector computeFieldWithApproximation(const G4ThreeVector& point, const ChargeNode* node, const G4ThreeVector& node_min, const G4ThreeVector& node_max) const;
    G4ThreeVector computeFieldFromCharges(const G4ThreeVector& point) const;
    void buildUniformGrid(Node* node, int depth);
    Node* findLeafNode(const G4ThreeVector& point, Node* node) const; // <-- Add this
    void ApplyChargeDissipation(G4double dt, G4double temp_K);
    double calculateConductivity(double temp_K) const;
    void distributeChargeChange(const std::vector<int>& particle_indices, G4double total_charge_change);
    double GetDielectricFraction(const G4ThreeVector& center, double half_width) const;

    bool pointInside(const G4ThreeVector& min_bounds, const G4ThreeVector& max_bounds, const G4ThreeVector& point) const;
    void calculateBoundingBox(G4ThreeVector& min_box, G4ThreeVector& max_box) const;
    void calculateChildBounds(const G4ThreeVector& parent_min, const G4ThreeVector& parent_max,
                              const G4ThreeVector& parent_center, int child_index,
                              G4ThreeVector& child_min, G4ThreeVector& child_max) const;
    void collectStatistics(const Node* node, int depth);

    void writeFieldPointsToFileRecursive(std::ofstream& outfile, const Node* node) const;

}; 

#endif 