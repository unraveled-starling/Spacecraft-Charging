"""
Shared utility functions for Geant4 charging simulation scripts.
"""

import os
import shutil
import numpy as np
import pandas as pd
import uproot


def reset_folder(folder_path):
    """
    Remove and recreate a folder.
    
    Parameters:
    -----------
    folder_path : str
        Path to the folder to reset
    """
    if os.path.exists(folder_path):
        shutil.rmtree(folder_path)
    os.makedirs(folder_path, exist_ok=True)


def read_rootfile(file, directory_path=None):
    """
    Read a ROOT file and return a pandas DataFrame.
    
    Parameters:
    -----------
    file : str
        ROOT filename
    directory_path : str, optional
        Directory containing the ROOT file
    
    Returns:
    --------
    pd.DataFrame
        Data from the ROOT file
    """
    file_path = f"{directory_path}/{file}" if directory_path else file
    tree_name = "Hit Data"

    with uproot.open(file_path) as root_file:
        tree = root_file[tree_name]
        branch_names = tree.keys()
        branch_vars = {name: tree[name].array(library="np") for name in branch_names}

    return pd.DataFrame(branch_vars)


def get_particle_counts_by_type(root_file, directory_path=None):
    """
    Extract particle counts by type from a ROOT file.
    
    Parameters:
    -----------
    root_file : str
        ROOT filename
    directory_path : str, optional
        Directory containing the ROOT file
    
    Returns:
    --------
    dict
        Particle type -> count mapping
    """
    try:
        df = read_rootfile(root_file, directory_path)
        counts = {}
        unique_particles = df["Particle_Type"].unique()
        
        for particleIN in unique_particles:
            if particleIN == "gamma":
                # # Get dataframe of the last e- event within the sensitive detector
                # last_e_event = df[
                #     (df["Particle_Type"] == "e-") & 
                #     (df["Parent_ID"] > 0.0)
                # ].drop_duplicates(subset="Event_Number", keep="last")
                
                # # Get all the initial gamma energy that leads to the creation of e-
                # matching_event_numbers = np.intersect1d(
                #     df["Event_Number"], 
                #     last_e_event["Event_Number"]
                # )
                # gamma_initial_leading_e_creation = df[
                #     df["Event_Number"].isin(matching_event_numbers)
                # ].drop_duplicates(subset="Event_Number", keep="first")
                
                # counts[particleIN] = gamma_initial_leading_e_creation.shape[0]


                # # get dataframe of the last e- event within the sensitive detector
                # last_e_event = df[(df["Particle_Type"] == "e-")].drop_duplicates(subset="Event_Number", keep="last")
                # # get new dataframe of all e- that left the world
                # world_e_energy=last_e_event[(last_e_event["Volume_Name_Post"]=="physical_cyclic") | (last_e_event["Volume_Name_Pre"]=="physical_cyclic")]
                # # get all the initial gamma energy that leads to an e- escaping
                # matching_event_numbers = np.intersect1d(df["Event_Number"], world_e_energy["Event_Number"])

                # #  get all the initial gamma energy that leds to the creation of e-
                # matching_event_numbers = np.intersect1d(df["Event_Number"], world_e_energy["Event_Number"])
                # gamma_initial_leading_e_creation = df[df["Event_Number"].isin(matching_event_numbers)].drop_duplicates(subset="Event_Number", keep="first")
                # counts[particleIN] = gamma_initial_leading_e_creation.shape[0]

                electron_dataframe = df[df["Particle_Type"]=="e-"]
                last_electron = electron_dataframe.drop_duplicates(subset="Event_Number", keep="last")
                escaping_electrons = last_electron[(last_electron["Volume_Name_Post"]=="physical_cyclic") | (last_electron["Volume_Name_Pre"]=="physical_cyclic")]
                counts[particleIN] = escaping_electrons.shape[0]


            else:
                # if particleIN == "proton":
                counts[particleIN] = df[
                    (df["Particle_Type"] == particleIN) & (df["Parent_ID"] == 0.0) & (df["Volume_Name_Post"]=="SiO2")
                ].drop_duplicates(subset="Event_Number", keep="first").shape[0]
                # else:
                #     counts[particleIN] = df[
                #         (df["Particle_Type"] == particleIN) #&
                #         #(df["Parent_ID"] == 0.0) #(df["Process_Name_Pre"]=="initStep")
                #     ].drop_duplicates(subset="Event_Number", keep="first").shape[0]
        
        return counts

    except Exception as e:
        print(f"Error reading {root_file}: {e}")
        return {}


def write_macro_header(f, increment_filename, seedIN=None):
    """
    Write common macro file header.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    increment_filename : str
        Name for this macro/output
    seedIN : list, optional
        Random seed [seed1, seed2]
    """
    f.write(f'# Macro file for {increment_filename}\n')
    f.write('#\n')
    f.write('/control/verbose 0\n')
    f.write('/run/verbose 0\n')
    f.write('/event/verbose 0\n')
    f.write('/tracking/verbose 0\n')
    f.write('/process/verbose 0\n')
    f.write('/process/had/verbose 0\n')
    f.write('/process/em/verbose 0\n')
    f.write('#\n')
    
    # Optional random seed
    if seedIN is not None:
        f.write(f'/random/setSeeds {seedIN[0]} {seedIN[1]}\n')
        f.write('/random/setSavingFlag 1\n')
    f.write('#\n')


def write_physics_settings(f):
    """Write physics process settings."""
    f.write('/process/em/fluo true\n')
    f.write('/process/em/auger true\n')
    f.write('/process/em/augerCascade true\n')
    f.write('/process/em/deexcitationIgnoreCut true\n')
    f.write('/process/em/Polarisation true\n')
    f.write('/process/em/PhotoeffectBelowKShell true\n')
    f.write('/process/em/pixe false\n')
    f.write('#\n')
    f.write('/process/em/lowestElectronEnergy 0 eV\n')
    f.write('/process/em/lowestMuHadEnergy 10 eV\n')
    f.write('/process/em/enableSamplingTable 1\n')
    f.write('#\n')
    f.write('/process/eLoss/CSDARange 1\n')
    f.write('/process/eLoss/UseICRU90 1\n')
    f.write('/process/eLoss/UseAngularGenerator true\n')
    f.write('#\n')
    f.write('/process/msc/RangeFactorMuHad 0.02\n')
    f.write('/process/msc/StepLimit UseSafetyPlus\n')
    f.write('/process/eLoss/StepFunctionMuHad 0.05 0.01 um\n')
    f.write('#\n')


def write_geometry_settings(f, increment_filename, world_dims, material_props, 
                           mesh_params, input_files=None):
    """
    Write geometry and field settings.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    increment_filename : str
        Name for this macro/output
    world_dims : dict
        Dictionary with keys 'worldX', 'worldY', 'worldZ'
    material_props : dict
        Dictionary with keys 'temperature', 'density', 'iterationTime', 'chargeDissipation', 'dielectric'
    mesh_params : dict
        Dictionary with keys 'initialOctreeDepth', 'minStep', 'gradPercent', 'finalOctreeDepth'
    input_files : list, optional
        Previous ROOT files to read
    """
    f.write(f'/geometry/worldX {world_dims["worldX"]} um\n')
    f.write(f'/geometry/worldY {world_dims["worldY"]} um\n')
    f.write(f'/geometry/worldZ {world_dims["worldZ"]} um\n')
    f.write(f'/geometry/epsilon {material_props["dielectric"]}\n')
    f.write(f'/geometry/MaterialTemperature {material_props["temperature"]} K\n')
    f.write(f'/geometry/MaterialDensity {material_props["density"]} g/cm3\n')
    f.write(f'/geometry/IterationTime {material_props["iterationTime"]} s\n')
    f.write(f'/geometry/ApplyChargeDissipation {material_props["chargeDissipation"]}\n')
    f.write(f'/field/InitialDepth {mesh_params["initialOctreeDepth"]}\n')
    f.write(f'/field/MinimumStep {mesh_params["minStep"]} um\n')
    f.write(f'/field/PercentGradThreshold {mesh_params["gradPercent"]}\n')
    f.write(f'/field/OctreeDepth {mesh_params["finalOctreeDepth"]}\n')
    
    # Extract base name for field map file
    base_name = f"{increment_filename.split('_')[0]}-{increment_filename.split('_')[2]}"
    f.write(f'/field/file fieldmaps/{base_name}-fieldmap.txt\n')
    f.write(f'/charges/file charges-{increment_filename.split("_")[2]}.txt\n')
    f.write('#\n')
    
    # Input files
    if input_files:
        f.write('/geometry/rootinput/file ' + ' '.join(input_files) + '\n')
    
    f.write(f'/geometry/rootoutput/file root/{increment_filename}.root\n')


def write_cad_settings(f, cad_filename, epsilon, pbc=True):
    """
    Write CAD input settings.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    cad_filename : str
        Name of STL file
    epsilon : float
        Dielectric constant
    pbc : bool
        Whether to use periodic boundary conditions
    """
    f.write(f'/geometry/cadinput/file {cad_filename}\n')
    f.write(f'/geometry/epsilon {epsilon}\n')
    f.write(f'/geometry/PBC {str(pbc).lower()}\n')
    f.write('/run/initialize\n')
    f.write('#\n')


def write_particle_source_electrons(f, electron_energy, cad_dims, z_position):
    """
    Write GPS commands for electron source.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    electron_energy : float
        Electron energy in eV
    cad_dims : tuple
        CAD dimensions (x, y, z) in µm
    z_position : float
        Z position for particle source in µm
    """
    f.write("/gps/particle e-\n")
    f.write('/gps/ene/type Mono\n')
    f.write(f'/gps/energy {electron_energy} eV\n')
    f.write('/gps/pos/type Plane\n')
    f.write('/gps/pos/shape Square\n')
    f.write(f'/gps/pos/halfx {cad_dims[0]/2} um\n')
    f.write(f'/gps/pos/halfy {cad_dims[1]/2} um\n')
    f.write(f"/gps/pos/centre 0 0 {z_position} um\n")
    f.write("/gps/ang/type iso\n")
    f.write('#\n')


def write_particle_source_uv_single(f, energy, cad_dims, z_position):
    """
    Write GPS commands for single UV energy.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    energy : float
        Photon energy in eV
    cad_dims : tuple
        CAD dimensions (x, y, z) in µm
    z_position : float
        Z position for particle source in µm
    """
    f.write('/gps/particle gamma\n')
    f.write(f'/gps/ene/mono {energy} eV\n')
    f.write("/gps/pos/type Plane\n")
    f.write("/gps/pos/shape Square\n")
    f.write(f'/gps/pos/halfx {cad_dims[0]/2} um\n')
    f.write(f'/gps/pos/halfy {cad_dims[1]/2} um\n')
    f.write(f"/gps/pos/centre 0 0 {z_position} um\n")
    f.write('/gps/direction 0 0 -1\n')
    f.write("/gps/ang/type cos\n")


def write_particle_source_solarwind(f, cad_dims, xy_offset, z_position, rotation, buffer_plane = 0, 
                                    electron_dist_file="electronSolarWind_distribution.txt", electron_intensity=5, 
                                    ion_dist_file=None):
    """
    Write GPS commands for solar wind (protons + electrons).
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    cad_dims : tuple
        CAD dimensions (x, y, z) in µm
    xy_offset : float
        XY offset for angled beam in µm
    z_position : float
        Z position for particle source in µm
    rotation : float
        Rotation factor (typically sin(45°))
    electron_dist_file : str
        Filename for electron energy distribution
    ion_dist_file : None [default] 
        Default: protons are monoenergetic 1 keV
        Otherwise: filename for ion energy distribution       
    """

    # Electrons (Maxwellian distribution)
    f.write("/gps/particle e-\n")
    f.write("/gps/ene/type Arb\n")
    f.write("/gps/hist/type arb\n")
    f.write("/gps/ene/diffspec true\n")
    f.write(f"/gps/hist/file {electron_dist_file}\n")
    f.write("/gps/hist/inter Log\n")
    f.write("/gps/pos/type Plane\n")
    f.write("/gps/pos/shape Square\n")
    f.write(f'/gps/pos/halfx {cad_dims[0]/2} um\n')
    f.write(f'/gps/pos/halfy {cad_dims[1]/2} um\n')
    f.write(f"/gps/pos/centre 0 0 {z_position} um\n")
    f.write("/gps/ang/type iso\n")
    f.write("/gps/ang/maxtheta 90 deg\n")
    f.write('#\n')

    # Protons at angle
    f.write('/gps/source/add 1\n')
    f.write(f"/gps/source/intensity {1/electron_intensity}\n")
    f.write('/gps/particle proton\n')

    if ion_dist_file == None:
        f.write('/gps/ene/type Mono\n')
        f.write('/gps/energy 1 keV\n')  
    else:
        f.write("/gps/ene/type Arb\n")
        f.write("/gps/hist/type arb\n")
        f.write("/gps/ene/diffspec true\n")
        f.write(f"/gps/hist/file {ion_dist_file}\n")
        f.write("/gps/hist/inter Lin\n")

    f.write('/gps/pos/type Plane\n')
    f.write('/gps/pos/shape Square\n')
    f.write(f'/gps/pos/halfx {cad_dims[0]/2 + buffer_plane} um\n')
    f.write(f'/gps/pos/halfy {cad_dims[1]/2} um\n')
    f.write(f'/gps/pos/centre {xy_offset} 0 {z_position} um\n')
    f.write(f'/gps/direction -{rotation} 0 -{rotation}\n')
    

def write_particle_source_photoemission(f, cad_dims, xy_offset, z_position, rotation,buffer_plane=0,
                                       photon_dist_file="photonSolar_distribution.txt"):
    """
    Write GPS commands for photoemission (solar spectrum).
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    cad_dims : tuple
        CAD dimensions (x, y, z) in µm
    xy_offset : float
        XY offset for angled beam in µm
    z_position : float
        Z position for particle source in µm
    rotation : float
        Rotation factor (typically sin(45°))
    photon_dist_file : str
        Filename for photon energy distribution
    """
    f.write('/gps/particle gamma\n')
    f.write('/gps/pos/type Plane\n')
    f.write('/gps/pos/shape Square\n')
    f.write(f'/gps/pos/halfx {cad_dims[0]/2 + buffer_plane} um\n')
    f.write(f'/gps/pos/halfy {cad_dims[1]/2} um\n')
    f.write(f'/gps/pos/centre {xy_offset} 0 {z_position} um\n')
    f.write(f'/gps/direction -{rotation} 0 -{rotation}\n')
    f.write("/gps/ene/type Arb\n")
    f.write("/gps/hist/type arb\n")
    f.write("/gps/ene/diffspec true\n")
    f.write(f"/gps/hist/file {photon_dist_file}\n")
    f.write("/gps/hist/inter Lin\n")

def write_run_commands(f, event_num, print_progress=5000):
    """
    Write run commands at end of macro.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    event_num : int
        Number of events to simulate
    print_progress : int
        How often to print progress
    """
    f.write('#\n')
    f.write(f'/run/printProgress {print_progress}\n')
    f.write(f'/run/beamOn {event_num}\n')


# SLURM batch template
BATCH_TEMPLATE = """#!/bin/bash
#SBATCH --job-name=Iteration{iter}_Configuration{config}
#SBATCH --account=gts-{account}
#SBATCH --mail-user={username}@gatech.edu
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=24
#SBATCH --mem-per-cpu=8gb
#SBATCH --time=05:00:00
#SBATCH --output=outputlogs/%A_iteration{iter}_{config}

echo "Starting iteration{iter} for {config} configuration"
{run_line}
date
"""


def create_batch_script(batch_path, iteration, config_name, macro_path, account, username):
    """
    Create a SLURM batch script.
    
    Parameters:
    -----------
    batch_path : str
        Path for the batch script
    iteration : int
        Iteration number
    config_name : str
        Configuration name
    macro_path : str
        Path to the macro file
    account : str
        SLURM account name
    username : str
        Username for email notifications
    """
    cmd = f"./g4chargeit {macro_path}"
    
    with open(batch_path, "w") as f:
        batch_script = BATCH_TEMPLATE.format(
            iter=iteration,
            config=config_name,
            run_line=cmd,
            account=account,
            username=username
        )
        f.write(batch_script)