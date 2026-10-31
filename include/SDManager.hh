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
//
/// \file SDManager.hh
/// \brief Definition of the SDManager class

#ifndef SDManager_h
#define SDManager_h 1

/// Action initialization class.
///

#include "SensitiveDetector.hh"


class SDManager
{

  public:

    /// Get instance of SD Manager
    static SDManager* GetInstance();
    /// Initalize SD manager, return the SD Manager
    static void Initalize() {new SDManager;};

    /// Record Trees to save the relevant data
    static void RecordTrees() {sd_ -> RecordTrees();};
    /// Create Trees for a place to save the relevant data
    static void CreateTrees() {sd_ -> CreateTrees();};

    /// Get sensitive detector from sensitive detector class
    SensitiveDetector* GetSD() {return sd_;};
    /// Create sensitive detector called "poly_sensitive"
    void CreateSD() {sd_= new SensitiveDetector("sensitive_detector_name");};

  private:

    SDManager();
    virtual ~SDManager() {};     

    static SDManager* singletonInstance_;
    static SensitiveDetector* sd_; //AV: G4ThreadLocal 
    // make pointer to sensitive detector, static since there is only one for SD managers, thread local so only get one pre set of thread
    
};

#endif

    
