//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class
//
//

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorConstruction.hh"
#include "G4VUserDetectorConstruction.hh"
#include "DetectorMessenger.hh"

#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <chrono>  // Make sure this is included

#include "G4MaterialPropertiesTable.hh"
#include "G4FieldManager.hh"
#include "G4ThreeVector.hh"
#include "G4ChordFinder.hh"
#include "G4EqMagElectricField.hh" 
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4DormandPrince745.hh"
#include "G4IntegrationDriver.hh"  
#include "G4UniformElectricField.hh"
#include "G4SystemOfUnits.hh"
#include "SDManager.hh"
#include "G4SDManager.hh"
#include "G4RunManager.hh"

#include "G4Material.hh"
#include "G4Element.hh"

#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4FieldManager.hh"

#include "G4UserLimits.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "G4UniformElectricField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4NistManager.hh"
#include "G4PeriodicBoundaryBuilder.hh"
#include "G4UserLimits.hh"

#include "CADMesh.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction():G4VUserDetectorConstruction()
, boolPBC_(false), worldX_(0), worldY_(0), worldZ_(0), Epsilon_(0), fieldMinimumStep_(0),sphereSolid_(0), equivalentIterationTime_(0.02),density_(2.1),
fieldGradThreshold_(0), CADFile_(""), RootInput_(""), Scale_(1), filename_(""), octreeDepth_(8), materialTemperature_(450), charges_filename_(""), 
initial_depth_(6), boolDissipationModel_(true)

{
  // create commands for interactive definition of the detector 
  detectorMessenger_ = new DetectorMessenger(this);

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{

  return ConstructVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::ConstructVolumes() {
G4GeometryManager::GetInstance()->OpenGeometry();
G4PhysicalVolumeStore::GetInstance()->Clean();
G4LogicalVolumeStore::GetInstance()->Clean();
G4SolidStore::GetInstance()->Clean();


  // ----------------------------------- //
 //           DEFINE MATERIAL           //
// ----------------------------------- // 


G4int ncomponents,natoms;

//G4Material* SiO2 = new G4Material("SiO2", density_, ncomponents=2,
//    kStateSolid, materialTemperature_);

//G4Element* Si = new G4Element("Silicon","Si", 14., 28.0855*g/mole);
//G4Element* O = new G4Element("Oxygen", "O", 8., 16.00*g/mole);

//SiO2->AddElement(Si, natoms=1);
//SiO2->AddElement(O, natoms=2);

G4NistManager* nist = G4NistManager::Instance();

G4Material* Kapton = nist->FindOrBuildMaterial("G4_KAPTON");
G4Material* SiO2 = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
G4Material* Silicon = nist->FindOrBuildMaterial("G4_Si");
G4Material* Al = nist->FindOrBuildMaterial("G4_Al");

G4Material* Silicone = new G4Material("Silicone",1.03*g/cm3,4);

	G4Element* H  = nist->FindOrBuildElement("H");
	G4Element* C  = nist->FindOrBuildElement("C");
	G4Element* O  = nist->FindOrBuildElement("O");
	G4Element* Si = nist->FindOrBuildElement("Si");
	
	Silicone->AddElement(C ,0.324);
	Silicone->AddElement(H ,0.081);
	Silicone->AddElement(O ,0.216);
	Silicone->AddElement(Si,0.379);



//G4cout << "Density of " << Kapton->GetName() << ": " << G4BestUnit(Kapton->GetDensity(), "Volumic Mass")  
//       << "at a temperature of " << Kapton->GetTemperature() << " K" << G4endl
//       << "with a dielectric constant of " << Epsilon_ << G4endl;

G4bool checkOverlaps = true;

// define vacuum 
G4Material* world_mat = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");

G4Box* solidWorld =    
  new G4Box("World",                       
      worldX_/2, worldY_/2, worldZ_/2);     

G4cout << "World size: "
      << "X: " << G4BestUnit(worldX_, "Length") << ", "
      << "Y: " << G4BestUnit(worldY_, "Length") << ", "
      << "Z: " << G4BestUnit(worldZ_, "Length") << G4endl;


logicWorld_ =                         
  new G4LogicalVolume(solidWorld,          
                      world_mat,           
                      "World");           

G4VPhysicalVolume* physWorld = 
  new G4PVPlacement(0,                     
                    G4ThreeVector(),       
                    logicWorld_,           
                    "World",               
                    0,                     
                    false,                 
                    0,                     
                    checkOverlaps);        
if (boolPBC_)
{
  G4PeriodicBoundaryBuilder* pbb = new G4PeriodicBoundaryBuilder();
  logicWorld_ = pbb->Construct(logicWorld_);
  std::cout << "PBC set to logicWorld" << std::endl;
}


 if (!CADFile_.empty()) {

 std::string folder_name = "geometry";

  std::ostringstream oss;
  oss << folder_name << "/" << CADFile_;
  std::string full_path = oss.str();

  auto sphere_mesh = CADMesh::TessellatedMesh::FromSTL(full_path);
  sphere_mesh->SetScale(Scale_);
  sphereSolid_ = sphere_mesh->GetSolid();
}


  // -------------------------------- //
 //           CREATE SLABS           //
// -------------------------------- // 


else 
{
  sphereSolid_ = new G4Box("Slab", 25*um, 1*mm, 1*mm);
  std::cout << "Slab." << std::endl;
}

G4Box* solidSiO2 =
new G4Box("SiO2",50*um,1*mm, 1*mm);

G4Box* solidAdhesive1 =
new G4Box("Adhesive1",12.5*um,1*mm, 1*mm);

G4Box* solidSi =
new G4Box("Silicon",90*um,1*mm, 1*mm);

G4Box* solidAdhesive2 =
new G4Box("Adhesive2",12.5*um,1*mm, 1*mm);

G4Box* solidAl =
new G4Box("Aluminium",250*um,1*mm, 1*mm);



// Create Electrons..............................................................................
if (!RootInput_.empty()) {
    std::istringstream iss(RootInput_);
    std::vector<std::string> file_list;
    std::string file_name;
    TFile* file;
    TTree* tree;

    while (iss >> file_name) {
        file_list.push_back(file_name);
    }

    for (const auto& fil : file_list) {
        std::string folder_name = "root";
        std::ostringstream oss;
        oss << folder_name << "/" << fil;
        std::string full_path = oss.str();

        file = TFile::Open(full_path.c_str(), "READ");
        tree = nullptr;

        if (file && file->IsOpen()) {
            file->GetObject("Hit Data", tree);
            if (tree) {
                std::cout << full_path << " successfully loaded!" << std::endl;
            } else {
                std::cout << "Tree not found in file: " << full_path << std::endl;
                continue;  
            }
        } else {
            std::cout << "Failed to open file: " << full_path << std::endl;
            continue;
        }

        // Branch variables
        int event_number;
        std::vector<double>* post_step_position = nullptr;
        std::vector<double>* pre_step_position = nullptr;
        Char_t volume_name_post[100];
        double kinetic_energy_post_mev;
        double parent_id;
        Char_t particle_type[50];
        Char_t process_name_pre[100];

        // Set branch addresses
        tree->SetBranchAddress("Event_Number", &event_number);
        tree->SetBranchAddress("Post_Step_Position_mm", &post_step_position);
        tree->SetBranchAddress("Pre_Step_Position_mm", &pre_step_position);
        tree->SetBranchAddress("Volume_Name_Post", &volume_name_post);
        tree->SetBranchAddress("Parent_ID", &parent_id);
        tree->SetBranchAddress("Kinetic_Energy_Post_MeV", &kinetic_energy_post_mev);
        tree->SetBranchAddress("Particle_Type", &particle_type);
        tree->SetBranchAddress("Process_Name_Pre", &process_name_pre);

        const std::string target_volume = "Kapton";

        // Sets to track photon stops and unique holes
        std::set<int> photonStops;
        std::set<int> holeRecordedEvents;

        Long64_t nEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nEntries; i++) {
            tree->GetEntry(i);
            if (!post_step_position || post_step_position->size() < 3) continue;
            if (!pre_step_position || pre_step_position->size() < 3) continue;


            std::string ptype = particle_type;

            // // Track photon stops (photon reaches zero kinetic energy)
            // if (ptype == "gamma" && kinetic_energy_post_mev == 0.0 && std::string(volume_name_post) == target_volume) {
            //     photonStops.insert(event_number);
            // }

            // Stopped electrons
            if (ptype == "e-" && kinetic_energy_post_mev == 0.0 && std::string(volume_name_post) == target_volume) {
                G4ThreeVector pos((*post_step_position)[0] * mm,
                                  (*post_step_position)[1] * mm,
                                  (*post_step_position)[2] * mm);
                fElectronPositions.push_back(pos);
            }

            // Stopped protons
            if (ptype == "proton" && kinetic_energy_post_mev == 0.0 && std::string(volume_name_post) == target_volume) {
                G4ThreeVector pos((*post_step_position)[0] * mm,
                                  (*post_step_position)[1] * mm,
                                  (*post_step_position)[2] * mm);
                fProtonPositions.push_back(pos);
            }

            if (ptype == "e-" && parent_id == 1 && std::string(process_name_pre) == "initStep") {
                G4ThreeVector pos((*pre_step_position)[0] * mm,
                                  (*pre_step_position)[1] * mm,
                                  (*pre_step_position)[2] * mm);
                fHolePositions.push_back(pos);
 
            }
        }

        // Print results per file
        G4cout << "File: " << full_path << G4endl;
        G4cout << "  Electrons: " << fElectronPositions.size() << G4endl;
        G4cout << "  Protons:   " << fProtonPositions.size() << G4endl;
        G4cout << "  Holes:     " << fHolePositions.size() << G4endl;

        file->Close();
    }
}




  // -------------------------------- //
 //         LOGICAL VOLUMES          //
// -------------------------------- // 


G4LogicalVolume*logicSphere = 
new G4LogicalVolume(sphereSolid_, Kapton , "Kapton");  

G4LogicalVolume* logicSiO2 =
new G4LogicalVolume(solidSiO2,SiO2,"SiO2");

G4LogicalVolume* logicSi =
new G4LogicalVolume(solidSi,Silicon,"Silicon");

G4LogicalVolume* logicAl =
new G4LogicalVolume(solidAl,Al,"Aluminium");

G4LogicalVolume* logicAdhesive1 =
new G4LogicalVolume(solidAdhesive1,Silicone,"Adhesive1");

G4LogicalVolume* logicAdhesive2 =
new G4LogicalVolume(solidAdhesive2,Silicone,"Adhesive2");



  // -------------------------------- //
 //          VISUALISATION           //
// -------------------------------- //

auto visGlass = new G4VisAttributes(G4Colour(0.2,0.8,1.0,0.4));      // Cyan
visGlass->SetForceSolid(true);
visGlass->SetForceAuxEdgeVisible(true);
logicSiO2->SetVisAttributes(visGlass);

auto visSi = new G4VisAttributes(G4Colour(0.0,0.8,0.0,0.4));         // Green
visSi->SetForceSolid(true);
visGlass->SetForceAuxEdgeVisible(true);
logicSi->SetVisAttributes(visSi);

auto visKapton = new G4VisAttributes(G4Colour(1.0,0.5,0.0,0.4));     // Orange
visKapton->SetForceSolid(true);
visGlass->SetForceAuxEdgeVisible(true);
logicSphere->SetVisAttributes(visKapton);

auto visAdhesive = new G4VisAttributes(G4Colour(1.0,1.0,0.0,0.4));   // Yellow
visAdhesive->SetForceSolid(true);
visGlass->SetForceAuxEdgeVisible(true);
logicAdhesive1->SetVisAttributes(visAdhesive);
logicAdhesive2->SetVisAttributes(visAdhesive);

auto visAl = new G4VisAttributes(G4Colour(0.75,0.75,0.75,0.4));      // Grey
visAl->SetForceSolid(true);
visGlass->SetForceAuxEdgeVisible(true);
logicAl->SetVisAttributes(visAl);




  // -------------------------------- //
 //            PLACEMENT             //
// -------------------------------- //


//new G4PVPlacement(0,                   
//              G4ThreeVector(0,0,0),   
//              logicSphere,                   
//              Kapton->GetName(),            
//             logicWorld_,            
//              false,                             
//              0); 
                            
new G4PVPlacement(0,G4ThreeVector(50*um,0,0),logicSiO2, "SiO2",logicWorld_,false,0);
new G4PVPlacement(0,G4ThreeVector(112.5*um,0,0),logicAdhesive1, "Adhesive1",logicWorld_,false,0);
new G4PVPlacement(0,G4ThreeVector(215*um,0,0),logicSi, "Silicon",logicWorld_,false,0);
new G4PVPlacement(0,G4ThreeVector(317.5*um,0,0),logicAdhesive2, "Adhesive2",logicWorld_,false,0);
new G4PVPlacement(0,G4ThreeVector(355*um,0,0),logicSphere, "Kapton",logicWorld_,false,0);
new G4PVPlacement(0,G4ThreeVector(630*um,0,0),logicAl, "Aluminium",logicWorld_,false,0);



return physWorld;
}

void DetectorConstruction::ConstructSDandField() {
  static auto sdManager = SDManager::GetInstance();
  sdManager->CreateSD();
  auto sd = sdManager->GetSD();
  G4SDManager::GetSDMpointer()->AddNewDetector(sd);

  std::vector<G4ThreeVector> allPositions;
  std::vector<G4double> allCharges;

  G4double eCharge = -1.602e-19 * CLHEP::coulomb;
  for (const auto& pos : fElectronPositions) {
    allPositions.push_back(pos);
    allCharges.push_back(eCharge);
  }
  G4double pCharge = +1.602e-19 * CLHEP::coulomb;
  for (const auto& pos : fProtonPositions) {
    allPositions.push_back(pos);
    allCharges.push_back(pCharge);
  }
  G4double hCharge = +1.602e-19 * CLHEP::coulomb;
  for (const auto& pos : fHolePositions) {
    allPositions.push_back(pos);
    allCharges.push_back(hCharge);
  } 

  G4ThreeVector min(-worldX_/2, -worldY_/2, -worldZ_/2); 
  G4ThreeVector max(worldX_/2, worldY_/2, worldZ_/2); 
  G4ThreeVector step(10*um, 10*um, 10*um);

  if (!allPositions.empty() && !allCharges.empty()) {

    const G4double time_step_dt = equivalentIterationTime_ / second;
    G4cout << "Equivalent iteration time for charge leakage: "
       << G4BestUnit(equivalentIterationTime_, "Time") << G4endl;

    G4cout << "Starting Adaptive Field Map Precomputation" << G4endl;

    G4cout << "   Final Octree Depth: " << octreeDepth_ << G4endl;
    G4cout << "   Minimum Step: " << G4BestUnit(fieldMinimumStep_,"Length") << G4endl;

    auto start = std::chrono::high_resolution_clock::now();

    const G4double material_temperature = materialTemperature_ / kelvin;
    auto adaptiveFieldMap = new AdaptiveSumRadialFieldMap(
        allPositions, allCharges, 
        fieldGradThreshold_,
        sphereSolid_,
        Epsilon_,  
        fieldMinimumStep_,
        time_step_dt, 
        material_temperature,     
        charges_filename_,
        filename_,
        min, max,
        octreeDepth_,
        initial_depth_,
        boolDissipationModel_,
        AdaptiveSumRadialFieldMap::StorageType::Double
    );

    // End timer
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    G4cout << "Ending Adaptive Field Map Precomputation" << G4endl;

    double duration_in_minutes = duration.count() / 60.0;
    G4cout << "Precomputation took " << duration_in_minutes << " minutes." << G4endl;

    auto worldFM = new G4FieldManager();
    worldFM->SetDetectorField(adaptiveFieldMap); 
    worldFM->SetMinimumEpsilonStep(1.0e-7);
    worldFM->SetMaximumEpsilonStep(1.0e-4);
    worldFM->SetDeltaOneStep(0.1*um);

    auto equation = new G4EqMagElectricField(adaptiveFieldMap);
    const G4int nvar = 8;
    auto stepper = new G4DormandPrince745(equation, nvar);
    auto driver  = new G4IntegrationDriver<G4DormandPrince745>(0.1*um, stepper, nvar);
    auto chord   = new G4ChordFinder(driver);
    worldFM->SetChordFinder(chord);

    logicWorld_->SetFieldManager(worldFM, true);
  }
  //Set all the Daughters & World as sensitive Detectors
  G4int nD = logicWorld_->GetNoDaughters();
  for (G4int i = 0; i < nD; ++i) {
    auto lv = logicWorld_->GetDaughter(i)->GetLogicalVolume();
    if (sd) {
      lv->SetSensitiveDetector(sd);
    }
  }

  // set sensitive detector to the ENTIRE WORLD
  // only for creating figures of deflection and scattering of particles outside geometry
  // otherwise uncomment this (makes ROOT files unnecessarily large)
  logicWorld_->SetSensitiveDetector(sd);
}


void DetectorConstruction::SetPBC(G4bool value)
{
  boolPBC_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetWorldX(G4double value)
{
  worldX_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetWorldY(G4double value)
{
  worldY_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetWorldZ(G4double value)
{
  worldZ_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetEpsilon(G4double value)
{
  Epsilon_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetMaterialDensity(G4double value)
{
  density_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}
 

void DetectorConstruction::SetRootInput(G4String value)
{
  RootInput_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}
 

void DetectorConstruction::SetChargesFile(G4String value)
{
  charges_filename_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetCADFile(G4String value)
{
  CADFile_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}
 
void DetectorConstruction::SetCADScale(G4double value)
{
  Scale_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetFieldFile(G4String value)
{
  filename_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetFieldMinimumStep(G4double value)
{
  fieldMinimumStep_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetFieldGradThreshold(G4double value)
{
  fieldGradThreshold_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetOctreeMaxDepth(G4double value)
{
  octreeDepth_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetMaterialTemperature(G4double value)
{
  materialTemperature_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetEquivalentIterationTime(G4double value)
{
  equivalentIterationTime_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetInitialDepth(G4double value)
{
  initial_depth_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetChargeDissipationModel(G4bool value)
{
  boolDissipationModel_ = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}
