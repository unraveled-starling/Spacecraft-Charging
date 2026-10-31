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
/// \file PrimaryGeneratorAction.hh
/// \brief Definition of the PrimaryGeneratorAction class
//
//
//
// 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef SensitiveDetector_h
#define SensitiveDetector_h 1

#include "SensitiveDetectorHit.hh"

#include "G4THitsCollection.hh" // Necessary for the collections of the hit class
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4VSensitiveDetector.hh"
#include "G4RootAnalysisManager.hh"
#include "G4Event.hh"

#include <vector>
#include <array>

/// Forward declaration of the hit class we will declare below
class SensitiveDetectorHit;

/// This is an example class for a sensitive detector.
/* First a call to Initialize is performed to create a sensitive detector hit
 * collection and register it with the sensitive Detector manager. Then
 * CreateTrees is called from RunAction::BeginOfRunAction to construct the
 * output data structure. ProcessHits is then called for every interaction
 * within the sensitive detector volume. Finally at the end of each event
 * RecordTrees is called to fill the output data structure with the data
 * accumulated by the SD hit collection during the event.
 */

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class SensitiveDetector : public G4VSensitiveDetector
// G4VSensitiveDetector: abstract base class (V)
// has certain number of methods that must be defined 
// =0: want class to have this function but whoever makes the derived class define what the function does, tell compiler that it is an abstract class 
{
  public:

    SensitiveDetector(std::string sdName); 
    ~SensitiveDetector();
    
    /// Initialize the SD hit collection.
    virtual void Initialize(G4HCofThisEvent* HCE);

    /// Process a hit and store information relevant for the sensitive detector.
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory*);

    /// Create tree for storing the sensitive detector information.
    virtual void CreateTrees();

    /// Record sensitive detector event information in output structure.
    virtual void RecordTrees(); 

  private:

    /// The collection of hit information accumulated during an event.
    G4THitsCollection< SensitiveDetectorHit >* hits_;

    // collection ID and event_
    int collectionID_;
    G4Event* event_;
    
    // Hits and Analysis Manager
    G4RootAnalysisManager* rootManager_; 
 
    // tree ID
    int treeID_;

    // branch IDs for particle tree
    int evtBranchID_; 
    int processPreID_;
    int processPostID_;
    int particleTypeID_;
    int volumePreID_;
    int volumePostID_;

    // branch IDs for energy tree
    int edepBranchID_;
    int nonIedepBranchID_;
    int kineticEnergyPreID_;
    int kineticEnergyPostID_;
    int parentID_;
    int trackID_;
    //int chargePreID_;
    //int chargePostID_;

    // vectors for position information
    std::vector<double> vectorPreValues_;    
    std::vector<double> vectorPostValues_;

};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

typedef G4THitsCollection<SensitiveDetectorHit> SDHitsCollection;

#endif


