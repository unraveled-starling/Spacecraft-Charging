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
/// \file DetectorMessenger.cc
/// \brief Implementation of the DetectorMessenger class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorMessenger.hh"

#include "DetectorConstruction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::DetectorMessenger(DetectorConstruction * Det)
:G4UImessenger(), 
 detector_(Det), rootManager_(G4RootAnalysisManager::Instance()), 
 fileNameCmd_(0), PBCCmd_(0), EpsilonCmd_(0), WorldXCmd_(0),WorldYCmd_(0), WorldZCmd_(0), 
 FieldMinimumStepCmd_(0), FieldGradThresholdCmd_(0), RootInputCmd_(nullptr), CADFileCmd_(nullptr), ScaleCmd_(0), 
 FieldFileCmd_(nullptr),ChargesFileCmd_(nullptr), EquivalentIterationTimeCmd_(0), MaterialTemperatureCmd_(0), MaterialDensityCmd_(0),
 InitialDepthCmd_(0)
 
{ 

  fileNameCmd_ = new G4UIcmdWithAString("/geometry/rootoutput/file",this);
  fileNameCmd_->SetGuidance("Define the filename.");
  fileNameCmd_->SetParameterName("choice",false);
  fileNameCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  PBCCmd_ = new G4UIcmdWithABool("/geometry/PBC",this);
  PBCCmd_->SetGuidance("PBC conditions on or off.");
  PBCCmd_->SetParameterName("choice",false);
  PBCCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  RootInputCmd_ = new G4UIcmdWithAString("/geometry/rootinput/file",this);
  RootInputCmd_->SetGuidance("Root Input Files (space-separated)");
  RootInputCmd_->SetParameterName("choice",false);
  RootInputCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  CADFileCmd_ = new G4UIcmdWithAString("/geometry/cadinput/file",this);
  CADFileCmd_->SetGuidance("CAD Input File.");
  CADFileCmd_->SetParameterName("choice",false);
  CADFileCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  EpsilonCmd_ = new G4UIcmdWithADouble("/geometry/epsilon", this);
  EpsilonCmd_->SetGuidance("Epsilon Value.");
  EpsilonCmd_->SetParameterName("choice",false);
  EpsilonCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  ScaleCmd_ = new G4UIcmdWithADouble("/geometry/cadinput/scale", this);
  ScaleCmd_->SetGuidance("Scale CAD Value.");
  ScaleCmd_->SetParameterName("choice",false);
  ScaleCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  WorldXCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/worldX", this);
  WorldXCmd_->SetGuidance("Set XY Scale of the World.");
  WorldXCmd_->SetParameterName("choice",false);
  WorldXCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  WorldYCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/worldY", this);
  WorldYCmd_->SetGuidance("Set XY Scale of the World.");
  WorldYCmd_->SetParameterName("choice",false);
  WorldYCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  WorldZCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/worldZ", this);
  WorldZCmd_->SetGuidance("Set XY Scale of the World.");
  WorldZCmd_->SetParameterName("choice",false);
  WorldZCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  MaterialTemperatureCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/MaterialTemperature", this);
  MaterialTemperatureCmd_->SetGuidance("Set temperature of material (units of Kelvin).");
  MaterialTemperatureCmd_->SetParameterName("choice",false);
  MaterialTemperatureCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  MaterialDensityCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/MaterialDensity", this);
  MaterialDensityCmd_->SetGuidance("Set temperature of material (units of Kelvin).");
  MaterialDensityCmd_->SetParameterName("choice",false);
  MaterialDensityCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  EquivalentIterationTimeCmd_ = new G4UIcmdWithADoubleAndUnit("/geometry/IterationTime", this);
  EquivalentIterationTimeCmd_->SetGuidance("Set equivalent iteration time in lunar equivalent photoemission flux (units of seconds).");
  EquivalentIterationTimeCmd_->SetParameterName("choice",false);
  EquivalentIterationTimeCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  ChargeDissipationModelCmd_ = new G4UIcmdWithABool("/geometry/ApplyChargeDissipation",this);
  ChargeDissipationModelCmd_->SetGuidance("Turn on or off the charge dissipation model.");
  ChargeDissipationModelCmd_->SetParameterName("choice",false);
  ChargeDissipationModelCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  FieldMinimumStepCmd_ = new G4UIcmdWithADoubleAndUnit("/field/MinimumStep", this);
  FieldMinimumStepCmd_->SetGuidance("Set minimum step size for adapative field map.");
  FieldMinimumStepCmd_->SetParameterName("choice",false);
  FieldMinimumStepCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  FieldGradThresholdCmd_ = new G4UIcmdWithADouble("/field/PercentGradThreshold", this);
  FieldGradThresholdCmd_->SetGuidance("Set percentage of maximum field for the gradient threshold in adapative field map.");
  FieldGradThresholdCmd_->SetParameterName("choice",false);
  FieldGradThresholdCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  FieldOctreeDepthCmd_ = new G4UIcmdWithADouble("/field/OctreeDepth", this);
  FieldOctreeDepthCmd_->SetGuidance("Set maximum depth for Octree adaptive mesh.");
  FieldOctreeDepthCmd_->SetParameterName("choice",false);
  FieldOctreeDepthCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  InitialDepthCmd_ = new G4UIcmdWithADouble("/field/InitialDepth", this);
  InitialDepthCmd_->SetGuidance("Set maximum depth for Initial Course Grid");
  InitialDepthCmd_->SetParameterName("choice",false);
  InitialDepthCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  FieldFileCmd_ = new G4UIcmdWithAString("/field/file",this);
  FieldFileCmd_->SetGuidance("Field Map Save File");
  FieldFileCmd_->SetParameterName("choice",false);
  FieldFileCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

  ChargesFileCmd_ = new G4UIcmdWithAString("/charges/file",this);
  ChargesFileCmd_->SetGuidance("Name of file to save list of charges for leakage.");
  ChargesFileCmd_->SetParameterName("choice",false);
  ChargesFileCmd_->AvailableForStates(G4State_PreInit,G4State_Idle);

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::~DetectorMessenger()
{
  delete fileNameCmd_;
  delete PBCCmd_;
  delete EpsilonCmd_;
  delete ScaleCmd_;
  delete RootInputCmd_;
  delete WorldXCmd_;
  delete WorldYCmd_;
  delete WorldZCmd_;
  delete FieldMinimumStepCmd_;
  delete FieldGradThresholdCmd_;
  delete FieldFileCmd_;
  delete FieldOctreeDepthCmd_;
  delete MaterialTemperatureCmd_;
  delete EquivalentIterationTimeCmd_;
  delete ChargesFileCmd_;
  delete InitialDepthCmd_;
  delete MaterialDensityCmd_;
  delete ChargeDissipationModelCmd_;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorMessenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 

  if( command == fileNameCmd_ )
  {G4cout << "Opening ROOT file: " << newValue << G4endl;
    rootManager_->OpenFile(newValue);}

  if( command == PBCCmd_ )
  { detector_->SetPBC(PBCCmd_->GetNewBoolValue(newValue));}

  if( command == ChargeDissipationModelCmd_ )
  { detector_->SetChargeDissipationModel(ChargeDissipationModelCmd_->GetNewBoolValue(newValue));}

  if( command == RootInputCmd_ )
  { detector_->SetRootInput(newValue);}

  if( command == CADFileCmd_ )
  { detector_->SetCADFile(newValue);}

  if( command == ChargesFileCmd_ )
  { detector_->SetChargesFile(newValue);}

  if( command == MaterialDensityCmd_ )
  { detector_->SetMaterialDensity(MaterialDensityCmd_->GetNewDoubleValue(newValue));}

  if( command == EpsilonCmd_ )
  { detector_->SetEpsilon(EpsilonCmd_->GetNewDoubleValue(newValue));}

  if( command == ScaleCmd_ )
  { detector_->SetCADScale(ScaleCmd_->GetNewDoubleValue(newValue));}
  
  if( command == WorldXCmd_ )
  { detector_->SetWorldX(WorldXCmd_->GetNewDoubleValue(newValue));}

  if( command == WorldYCmd_ )
  { detector_->SetWorldY(WorldYCmd_->GetNewDoubleValue(newValue));}

  if( command == WorldZCmd_ )
  { detector_->SetWorldZ(WorldZCmd_->GetNewDoubleValue(newValue));}

  if( command == FieldMinimumStepCmd_ )
  { detector_->SetFieldMinimumStep(FieldMinimumStepCmd_->GetNewDoubleValue(newValue));}

  if( command == FieldGradThresholdCmd_ )
  { detector_->SetFieldGradThreshold(FieldGradThresholdCmd_->GetNewDoubleValue(newValue));}

  if( command == FieldOctreeDepthCmd_ )
  { detector_->SetOctreeMaxDepth(FieldOctreeDepthCmd_->GetNewDoubleValue(newValue));}

  if( command == InitialDepthCmd_ )
  { detector_->SetInitialDepth(InitialDepthCmd_->GetNewDoubleValue(newValue));}

  if( command == MaterialTemperatureCmd_ )
  { detector_->SetMaterialTemperature(MaterialTemperatureCmd_->GetNewDoubleValue(newValue));}

  if( command == EquivalentIterationTimeCmd_ )
  { detector_->SetEquivalentIterationTime(EquivalentIterationTimeCmd_->GetNewDoubleValue(newValue));}

  if( command == FieldFileCmd_ )
  { detector_->SetFieldFile(newValue);}


}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


