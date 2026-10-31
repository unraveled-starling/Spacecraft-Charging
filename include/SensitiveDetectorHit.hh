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
/// \file SensitiveDetectorHit.hh
/// \brief Definition of the SensitiveDetectorHit header
//
//
//
// 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef SensitiveDetectorHit_h
#define SensitiveDetectorHit_h 1

#include "G4VHit.hh"
#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4Step.hh"

//#include "G4ThreeVector.hh"

/// An example Sensitive Detector hit class.
/** An instance of this class is generated for each hit within a volume
 * assigned to the sensitive detector for which this hit class is used.  The
 * class has private members that store the hit values as well as setter and
 * getter method to access those values. The setters are typically used in
 * SensitiveDetector::ProcessHits to process the information form a hit and the
 * getters are typically used in SensitiveDetector::RecordTrees to store hit /
 * event information in the output.
 */

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class SensitiveDetectorHit : public G4VHit {

public:

    /// Constructor
    SensitiveDetectorHit(const G4Step* step);
    /// Destructor
    ~SensitiveDetectorHit();

public:
    //! \name The new and delete operators are overloaded for performances reasons:
    /*! -- Tricky business here... :-(, but provided for you below */
    //@{
    inline void *operator    new(size_t);
    inline void  operator delete(void *hits_);
    //@}
    
    // methods for particle tree
    G4String GetPreProcessName() {return processPre_;};
    G4String GetPostProcessName() {return processPost_;};
    G4String GetParticleType() {return particleType_;};
    G4String GetPreVolumeName() {return volumePre_;};
    G4String GetPostVolumeName() {return volumePost_;};

    // methods for energy tree
    G4double GetDepositedEnergy() {return eDep_;};
    G4double GetNonIonizingDepositedEnergy() {return nonIeDep_;};
    G4double GetPreKineticEnergy() {return kineticEnergyPre_;};
    G4double GetPostKineticEnergy() {return kineticEnergyPost_;};
    G4double GetParentID() {return parentID_;};
    G4double GetTrackID() {return trackID_;};
    //G4double GetPreCharge() {return chargePre_;};
    //G4double GetPostCharge() {return chargePost_;};

    // methods for position tree

    G4double GetPrePositionX() {return postPreX_;};
    G4double GetPrePositionY() {return postPreY_;};
    G4double GetPrePositionZ() {return postPreZ_;};

    G4double GetPostPositionX() {return postPostX_;};
    G4double GetPostPositionY() {return postPostY_;};
    G4double GetPostPositionZ() {return postPostZ_;};

private:

    G4String particleType_;
    G4String processPre_;
    G4String processPost_;
    G4String volumePre_;
    G4String volumePost_;

    G4double eDep_;
    G4double nonIeDep_;
    G4double kineticEnergyPre_;
    G4double kineticEnergyPost_;
    G4double parentID_;
    G4double trackID_;
    //G4double chargePre_;
    //G4double chargePost_;

    G4double postPreX_; 
    G4double postPreY_; 
    G4double postPreZ_; 

    G4double postPostX_; 
    G4double postPostY_; 
    G4double postPostZ_; 
};
      
// Define the "hit collection" using the template class G4THitsCollection:
typedef G4THitsCollection<SensitiveDetectorHit> SDHitCollection;
      
// -- new and delete overloaded operators:
 
extern G4Allocator<SensitiveDetectorHit> SDHitAllocator;

inline void* SensitiveDetectorHit::operator new(size_t)
{
   void *hits_;
   hits_ = (void *) SDHitAllocator.MallocSingle();
   return hits_;
}
inline void SensitiveDetectorHit::operator delete(void* obj)
{
   SDHitAllocator.FreeSingle((SensitiveDetectorHit*) obj);
}

#endif

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......



