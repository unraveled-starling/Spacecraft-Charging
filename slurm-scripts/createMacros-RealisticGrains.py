"""
Geant4 charging simulation script for Wang 2016 parameters.
Refactored to use shared utility functions.
"""

import os
import re
import numpy as np
from shared_utils import (
    reset_folder, get_particle_counts_by_type,
    write_macro_header, write_physics_settings, write_geometry_settings,
    write_cad_settings, write_particle_source_electrons, 
    write_particle_source_uv_single, write_particle_source_solarwind,
    write_particle_source_photoemission, write_run_commands,
    create_batch_script
)

#############################################################
###################### USER PARAMETERS ######################
#############################################################

# Account and user settings
account, username = "pf17", "avira7"

# Material properties
density = 1.9  # g/cm3 (from Wang manuscript)
temperature = 425  # K (293 K for room temp in Wang experiments)
dielectric = 3.9 # set dielectric constant 

# Iteration settings
iterationNUM = 100

# Optional: set random seed for debugging
# seedIN = [10008859, 10005380]
seedIN = None

# Experimental parameters from Wang 2016 manuscript
electron_energy = 120  # eV
photonSource_wavelength = 172  # nm
photonSource_FWHM = 14  # nm
photonSource_center = 1239.8 / photonSource_wavelength  # eV
photonSource_sigma = abs((1239.8/(photonSource_wavelength + photonSource_FWHM/2) - 
                         1239.8/(photonSource_wavelength - photonSource_FWHM/2)) / 2.3548)  # eV

# UV photon energies to simulate
photonSource_energy = [5.3]  # , 5.4, 5.5, 5.6, 6, 8, 10, 12, 13]  # eV

# Configuration list
config_list = ["onlysolarwind", "onlyphotoemission"] 

# Flux parameters from Wang et al. 2016 (experimental setup)
UVflux, eflux = np.array([5.05e-6, 2*3e-7]) * 6.241509e18  # A/m2 -> e/m2/s

# Flux parameters from Zimmerman et al. 2016 (lunar environment)
PEflux, SWelectrons, SWions = np.array([4e-6, 1.5e-6, 3e-7]) * 6.241509e18  # A/m2 -> e/m2/s
electron_intensity, photon_intensity = SWelectrons / SWions, PEflux / SWions

# Mesh and simulation parameters
minStepList = [0.01, 0.01]  # Minimum step for Octree mesh (µm)
#eventnumbersList = [1000000, 1000000] # for illumination plots
eventnumbersList = [50000, 50000]  # for iterations
initialOctreeDepth = 8
gradPercent = 0.7
finalOctreeDepth = 11
chargeDissipation = "true"

# Geometry and CAD settings
CAD_filename = "realisticGrains_fromPython.stl"  # STL file to import
CAD_dimensions = (107.00044, 120.77438, 100.29045)  # µm
particle_position = 40  # µm above geometry
bufferXYworld = 0  # µm
worldX = CAD_dimensions[0] + 2*bufferXYworld  # µm
worldY = CAD_dimensions[1] + 2*bufferXYworld  # µm
worldZ = CAD_dimensions[2] + 2*particle_position  # µm
planeArea = CAD_dimensions[0]*CAD_dimensions[1]/ (1e6**2)  # m²

# Calculate offset for SW ions at 45 degrees
Z_position = CAD_dimensions[2]/2 + particle_position
XY_offset = Z_position - CAD_dimensions[2]/2 + bufferXYworld
rotation = np.sin(45 * np.pi / 180)

# print out the lunar environment parameters
print(f"PE flux: {PEflux:.2e} e/m²/s, SW e- flux: {SWelectrons:.2e} e/m²/s, SW ions flux: {SWions:.2e} e/m²/s")
print(f"e- intensity wrt SW ions: {electron_intensity:.2f}, photon intensity wrt SW ions: {photon_intensity:.2f}")
print(72*"-")
print(f"Configurations w/ plane area of {planeArea:.2e} m²")

#############################################################
###################### HELPER FUNCTIONS #####################
#############################################################

def write_macro(f, increment_filename, event_num, iterationTime, 
                input_files=None, minStep=1):
    """
    Write complete macro file for Wang simulations.
    
    Parameters:
    -----------
    f : file object
        Open file to write to
    increment_filename : str
        Name for this macro/output
    event_num : int
        Number of events to simulate
    iterationTime : float
        Physical time for this iteration (seconds)
    input_files : list, optional
        Previous ROOT files to read
    minStep : float
        Minimum step size for octree mesh (µm)
    """
    # Write header
    write_macro_header(f, increment_filename, seedIN)
    
    # Write physics settings
    write_physics_settings(f)
    
    # Write geometry settings
    world_dims = {
        'worldX': worldX,
        'worldY': worldY,
        'worldZ': worldZ
    }
    material_props = {
        'temperature': temperature,
        'density': density,
        'dielectric': dielectric,
        'iterationTime': iterationTime,
        'chargeDissipation': chargeDissipation
    }
    mesh_params = {
        'initialOctreeDepth': initialOctreeDepth,
        'minStep': minStep,
        'gradPercent': gradPercent,
        'finalOctreeDepth': finalOctreeDepth
    }
    
    write_geometry_settings(f, increment_filename, world_dims, 
                          material_props, mesh_params, input_files)
    
    # Write CAD settings
    write_cad_settings(f, CAD_filename, epsilon=4, pbc=True)
    
    # Select particle source based on configuration
    if "onlyelectrons" in increment_filename:
        write_particle_source_electrons(f, electron_energy, CAD_dimensions, Z_position)
    elif "onlyUV" in increment_filename:
        # Check if this is a single-energy UV simulation
        energy_match = re.search(r'energy([\d.]+)eV', increment_filename)
        if energy_match:
            energy = float(energy_match.group(1))
            write_particle_source_uv_single(f, energy, CAD_dimensions, Z_position)
        else:
            # Multiple energies (not commonly used, but keeping for compatibility)
            for energy in photonSource_energy:
                write_particle_source_uv_single(f, energy, CAD_dimensions, Z_position)
    elif "onlysolarwind" in increment_filename:
        write_particle_source_solarwind(f, CAD_dimensions, XY_offset, Z_position, 
                                       rotation, buffer_plane=0, 
                                       electron_dist_file="distributions/electronSolarWind_distribution.txt", 
                                       electron_intensity=electron_intensity, 
                                       ion_dist_file = "distributions/ionSolarWind_distribution.txt")
    elif "onlyphotoemission" in increment_filename:
        write_particle_source_photoemission(f, CAD_dimensions, XY_offset, Z_position, 
                                           rotation, buffer_plane=0, 
                                           photon_dist_file="distributions/photonSolar_distribution.txt")
    else:
        raise Warning("Configuration must be one of: onlyUV, onlyelectrons, onlysolarwind, onlyphotoemission")
    
    # Write run commands
    write_run_commands(f, event_num)


#############################################################
############### CREATE ITERATIONS ###########################
#############################################################

# Reset output folders
reset_folder("macros")
reset_folder("batchscripts")

output_files = []

for optionIN, minStepIN, select_num in zip(config_list, minStepList, eventnumbersList):

    # Standard processing for other configurations
    i = 0
    
    # For onlyUV, create separate macros for each energy
    if optionIN == "onlyUV":
        for energy in photonSource_energy:
            config_name = f"{optionIN}-energy{energy}eV"
            i = 0  # Reset iteration counter
            
            while i < iterationNUM:
                
                if i == 0:
                    # Create macro for iteration 0
                    increment_filename = f"{i:03d}_iteration0_{config_name}_num{select_num}"
                    macro_path = f"macros/{increment_filename}.mac"
                    
                    with open(macro_path, 'w') as f:
                        write_macro(f, increment_filename, select_num, 0, minStep=minStepIN)
                    
                    output_files.append((increment_filename + ".root", select_num, i, config_name))
                    
                    # Create batch script
                    batch_path = f"batchscripts/{i:03d}_submit_iteration0_{config_name}.sh"
                    create_batch_script(batch_path, i, config_name, macro_path, account, username)
                    
                    i += 1
                
                elif i == 1:
                    # Read ROOT file from iteration 0
                    previous_root_file = f"root/{output_files[-1][0]}"
                    
                    if not os.path.exists(previous_root_file):
                        print(f"[{config_name}] Requires: {previous_root_file}")
                        break
                    
                    particlesforIteration = get_particle_counts_by_type(previous_root_file)
                    
                    if not particlesforIteration:
                        print(f"[{config_name}] No particles found in {previous_root_file}")
                        break
                    
                    # Calculate iteration time
                    iterationTime = (particlesforIteration.get("gamma", 0) / planeArea) / UVflux
                    
                    print(f"[{config_name}] Particles: {particlesforIteration}, Time: {iterationTime:.2e} s")
                    
                    increment_filename = f"{i:03d}_iteration{i}_{config_name}_num{select_num}"
                    macro_path = f"macros/{increment_filename}.mac"
                    
                    with open(macro_path, 'w') as f:
                        write_macro(f, increment_filename, select_num, iterationTime,
                                  input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
                    
                    output_files.append((increment_filename + ".root", select_num, i, config_name))
                    
                    # Create batch script
                    batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{config_name}.sh"
                    create_batch_script(batch_path, i, config_name, macro_path, account, username)
                    
                    i += 1
                
                else:  # i > 1
                    increment_filename = f"{i:03d}_iteration{i}_{config_name}_num{select_num}"
                    macro_path = f"macros/{increment_filename}.mac"
                    
                    with open(macro_path, 'w') as f:
                        write_macro(f, increment_filename, select_num, iterationTime,
                                  input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
                    
                    output_files.append((increment_filename + ".root", select_num, i, config_name))
                    
                    # Create batch script
                    batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{config_name}.sh"
                    create_batch_script(batch_path, i, config_name, macro_path, account, username)
                    
                    i += 1
    
    else:
        
        while i < iterationNUM:
            
            if i == 0:
                increment_filename = f"{i:03d}_iteration0_{optionIN}_num{select_num}"
                macro_path = f"macros/{increment_filename}.mac"
                
                with open(macro_path, 'w') as f:
                    write_macro(f, increment_filename, select_num, 0, minStep=minStepIN)
                
                output_files.append((increment_filename + ".root", select_num, i, optionIN))
                
                batch_path = f"batchscripts/{i:03d}_submit_iteration0_{optionIN}.sh"
                create_batch_script(batch_path, i, optionIN, macro_path, account, username)
                
                i += 1
            
            elif i == 1:

                previous_root_file = f"root/{output_files[-1][0]}"
                
                if not os.path.exists(previous_root_file):
                    print(f"[{optionIN}] Requires: {previous_root_file}")
                    break
                
                particlesforIteration = get_particle_counts_by_type(previous_root_file)
                
                if not particlesforIteration:
                    print(f"[{optionIN}] No particles found in {previous_root_file}")
                    break
                
                # Calculate iteration time based on configuration
                if optionIN == "onlyphotoemission":
                    iterationTime = (particlesforIteration.get("gamma", 0) / planeArea) / PEflux
                elif optionIN == "onlysolarwind":
                    iterationTime = (particlesforIteration.get("proton", 0) / planeArea) / SWions
                    iterationTime2 = (particlesforIteration.get("e-", 0) / planeArea) / SWelectrons
                    print("ion time: ", iterationTime*1000, "e- time: ", iterationTime2*1000)
                elif optionIN == "onlyelectrons":
                    iterationTime = (particlesforIteration.get("e-", 0) / planeArea) / eflux
                else:
                    iterationTime = 0
                
                print(f"[{optionIN}] Particles: {particlesforIteration}, Time: {iterationTime*1000:.2f} ms")
                
                increment_filename = f"{i:03d}_iteration{i}_{optionIN}_num{select_num}"
                macro_path = f"macros/{increment_filename}.mac"
                
                with open(macro_path, 'w') as f:
                    write_macro(f, increment_filename, select_num, iterationTime,
                              input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
                
                output_files.append((increment_filename + ".root", select_num, i, optionIN))
                
                batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{optionIN}.sh"
                create_batch_script(batch_path, i, optionIN, macro_path, account, username)
                
                i += 1
            
            else:  # i > 1
                increment_filename = f"{i:03d}_iteration{i}_{optionIN}_num{select_num}"
                macro_path = f"macros/{increment_filename}.mac"
                
                with open(macro_path, 'w') as f:
                    write_macro(f, increment_filename, select_num, iterationTime,
                              input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
                
                output_files.append((increment_filename + ".root", select_num, i, optionIN))
                
                batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{optionIN}.sh"
                create_batch_script(batch_path, i, optionIN, macro_path, account, username)
                
                i += 1

print(72*"-")
print(f"Created {len(output_files)} macro files and batch scripts")
print(f"Maximum iterations reached: {i-1}")