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
/// \file SensitiveDetector.cc
/// \brief Implementation of the SensitiveDetector class
//

#include "SensitiveDetector.hh"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4RootAnalysisManager.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"

//#include "<vector>"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SensitiveDetector::SensitiveDetector(std::string sdName) 

: G4VSensitiveDetector(sdName),
   hits_(nullptr), collectionID_(-1), event_(nullptr),
   rootManager_(G4RootAnalysisManager::Instance()) 
{  
   // print the SD name 
   G4cout<<"Creating SD with name:"<<sdName<<G4endl;

   // insert the name of the collectionName vector
   collectionName.insert("SDHitsCollection");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SensitiveDetector::~SensitiveDetector()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SensitiveDetector::Initialize(G4HCofThisEvent* HCE)
{

  hits_ = new SDHitsCollection(GetName(), collectionName[0]);

  if (collectionID_ <0) {
    collectionID_ = GetCollectionID(0); 
  } 

  HCE->AddHitsCollection(collectionID_, hits_);

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....



G4bool SensitiveDetector::ProcessHits(G4Step* step, 
                                      G4TouchableHistory*)
{  
      
  hits_->insert( new SensitiveDetectorHit(step) );


  return true;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......



void SensitiveDetector::CreateTrees()
{
   // create an ROOT Tree (n-tuple) and get the tree pointer.
   treeID_ = rootManager_-> CreateNtuple("Hit Data",
                                        "Particle, Energy, and Position Information");

   // create branches for particle tree
   evtBranchID_ = rootManager_ -> CreateNtupleIColumn(treeID_, "Event_Number");
   processPreID_ = rootManager_ -> CreateNtupleSColumn(treeID_, "Process_Name_Pre");
   processPostID_ = rootManager_ -> CreateNtupleSColumn(treeID_, "Process_Name_Post");
   particleTypeID_ = rootManager_ -> CreateNtupleSColumn(treeID_, "Particle_Type");
   volumePreID_ = rootManager_ -> CreateNtupleSColumn(treeID_, "Volume_Name_Pre");
   volumePostID_ = rootManager_ -> CreateNtupleSColumn(treeID_, "Volume_Name_Post");

   // create branches for energy Tree

   kineticEnergyPreID_ = rootManager_ -> CreateNtupleDColumn(treeID_, "Kinetic_Energy_Pre_keV");
   kineticEnergyPostID_ = rootManager_ -> CreateNtupleDColumn(treeID_, "Kinetic_Energy_Post_keV");
   parentID_ = rootManager_ -> CreateNtupleDColumn(treeID_, "Parent_ID");
   trackID_ = rootManager_ -> CreateNtupleDColumn(treeID_, "Track_ID");
   edepBranchID_ = rootManager_->CreateNtupleDColumn(treeID_, "Energy_Deposit_keV");
   nonIedepBranchID_ = rootManager_->CreateNtupleDColumn(treeID_, "NonIonizing_Energy_Deposit_keV");
   
   // create branches for position tree
   rootManager_ -> CreateNtupleDColumn(treeID_, "Pre_Step_Position_mm",vectorPreValues_);   
   rootManager_ -> CreateNtupleDColumn(treeID_, "Post_Step_Position_mm",vectorPostValues_);

   rootManager_-> FinishNtuple(treeID_);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


void SensitiveDetector::RecordTrees() 
{

   int eventNum = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

   for ( auto sdHit : *(hits_->GetVector()) ) {
  
     // save particle information
     rootManager_ -> FillNtupleIColumn(treeID_, evtBranchID_, eventNum); 
     rootManager_ -> FillNtupleSColumn(treeID_, processPreID_, sdHit -> GetPreProcessName());
     rootManager_ -> FillNtupleSColumn(treeID_, processPostID_, sdHit -> GetPostProcessName());
     rootManager_ -> FillNtupleSColumn(treeID_, particleTypeID_, sdHit ->GetParticleType());
     rootManager_ -> FillNtupleSColumn(treeID_, volumePreID_, sdHit ->GetPreVolumeName());
     rootManager_ -> FillNtupleSColumn(treeID_, volumePostID_, sdHit ->GetPostVolumeName());

     // save energy information
     rootManager_ -> FillNtupleDColumn(treeID_, kineticEnergyPreID_, sdHit ->GetPreKineticEnergy() / keV);
     rootManager_ -> FillNtupleDColumn(treeID_, kineticEnergyPostID_, sdHit ->GetPostKineticEnergy() / keV);
     rootManager_ -> FillNtupleDColumn(treeID_, parentID_, sdHit ->GetParentID());
     rootManager_ -> FillNtupleDColumn(treeID_, trackID_, sdHit ->GetTrackID());
     rootManager_->FillNtupleDColumn(treeID_, edepBranchID_, sdHit->GetDepositedEnergy()/keV);
     rootManager_->FillNtupleDColumn(treeID_, nonIedepBranchID_, sdHit->GetNonIonizingDepositedEnergy()/keV);

     // save post step position 
     vectorPreValues_.clear();    
     vectorPreValues_.push_back(sdHit ->GetPrePositionX() / mm);
     vectorPreValues_.push_back(sdHit ->GetPrePositionY() / mm);
     vectorPreValues_.push_back(sdHit ->GetPrePositionZ() / mm); 

     // save post step position 
     vectorPostValues_.clear();   
     vectorPostValues_.push_back(sdHit ->GetPostPositionX() / mm);
     vectorPostValues_.push_back(sdHit ->GetPostPositionY() / mm);
     vectorPostValues_.push_back(sdHit ->GetPostPositionZ() / mm); 

     // add position information to branch
     
     
     if (sdHit->GetTrackID() == 1 &&
    sdHit->GetParentID() == 0 &&
    sdHit->GetPostKineticEnergy() == 0) {

    G4cout
        << "STOPPED PRIMARY  Event "
        << eventNum
        << "  Z = "
        << sdHit->GetPostPositionZ()/um
        << " um"
        << G4endl;
        }
     
     rootManager_ -> AddNtupleRow(treeID_);
    
   }

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
