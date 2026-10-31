"""
Geant4 charging simulation script for Zimmerman 2016 parameters.
Refactored to use shared utility functions.
"""

import os
import numpy as np
from shared_utils import (
    reset_folder, get_particle_counts_by_type,
    write_macro_header, write_physics_settings, write_geometry_settings,
    write_cad_settings, write_particle_source_solarwind,
    write_particle_source_photoemission,
    write_run_commands, create_batch_script
)

#############################################################
###################### USER PARAMETERS ######################
#############################################################

# Account and user settings
account, username = "pf17", "avira7"

# Number of particles for each iteration
eventnumbers_onlysolarwind = 80000 # 100000 # for r=100 um spheres
eventnumbers_onlyphotoemission = 1000000 # 5000000 # for r=100 um spheres
iterationNUM = 200

# Material properties
temperature = 425  # Kelvin
density = 2.2  # g/cm3 (SiO2)
dielectric = 3.9 # set dielectric constant 

# Optional: set random seed for debugging
# seedIN = [10008859, 10005380]
seedIN = None

# Configuration list
config_list = ["onlysolarwind", "onlyphotoemission"] 

# Mesh and simulation parameters
minStepList = [0.01, 0.01]  # Minimum step for Octree mesh (µm)
initialOctreeDepth = 8
gradPercent = 0.8
finalOctreeDepth = 11
chargeDissipation = "true"

# Geometry and CAD settings
CAD_filename = "regularSpheres_radius100um_fromPython.stl" #"regularSpheres_radius100um_fromPython.stl"
CAD_dimensions = (400, 300, 373.2) #(100, 75, 123.3) # for r=25 um spheres 
#(400, 300, 373.2) # for r=100 um spheres 
particle_position = 40  # µm above geometry
bufferXYworld = 0  # µm

# World dimensions
worldX = CAD_dimensions[0] + 2*bufferXYworld  # µm
worldY = CAD_dimensions[1] + 2*bufferXYworld  # µm
worldZ = CAD_dimensions[2] + 2*particle_position  # µm

# Calculate offset for SW ions at 45 degrees
Z_position = CAD_dimensions[2]/2 + particle_position
XY_offset = Z_position - CAD_dimensions[2]/2 + bufferXYworld
bufferX_direction = 0#XY_offset/2  # µm
rotation = np.sin(45*np.pi/180)

# Flux values from Zimmerman 2016 manuscript (A/m² -> e/m²/s)
PEflux, SWelectrons, SWions = np.array([4e-6, 1.5e-6, 3e-7]) * 6.241509e18
planeArea = CAD_dimensions[0]*CAD_dimensions[1]/ (1e6**2)  # m²
electron_intensity, photon_intensity = SWelectrons / SWions, PEflux / SWions
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
    Write complete macro file.
    
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
    if "onlysolarwind" in increment_filename:
        write_particle_source_solarwind(f, CAD_dimensions, XY_offset, Z_position, 
                                       rotation, buffer_plane=bufferX_direction, 
                                       electron_dist_file="distributions/electronSolarWind_distribution.txt", 
                                       electron_intensity=electron_intensity, 
                                       ion_dist_file = "distributions/ionSolarWind_distribution.txt")
    elif "onlyphotoemission" in increment_filename:
        write_particle_source_photoemission(f, CAD_dimensions, XY_offset, Z_position, 
                                           rotation, buffer_plane=bufferX_direction, 
                                           photon_dist_file="distributions/photonSolar_distribution.txt")
    else:
        raise Warning("Configuration must be one of: onlysolarwind, onlyphotoemission")
    
    # Write run commands
    write_run_commands(f, event_num)

#############################################################
############### CREATE ITERATIONS ###########################
#############################################################

# Reset output folders
reset_folder("macros")
reset_folder("batchscripts")

output_files = []

for optionIN, minStepIN in zip(config_list, minStepList):
    
    select_num = vars()['eventnumbers_' + optionIN]
    i = 0  # Reset iteration counter for each config
    
    while i < iterationNUM:
        
        if i == 0:
            # Create macro for iteration 0
            increment_filename = f"{i:03d}_iteration0_{optionIN}_num{select_num}"
            macro_path = f"macros/{increment_filename}.mac"
            
            with open(macro_path, 'w') as f:
                write_macro(f, increment_filename, select_num, 0, minStep=minStepIN)
            
            output_files.append((increment_filename + ".root", select_num, i, optionIN))
            
            # Create batch script
            batch_path = f"batchscripts/{i:03d}_submit_iteration0_{optionIN}.sh"
            create_batch_script(batch_path, i, optionIN, macro_path, account, username)
            
            i += 1
        
        elif i == 1:
            # Wait for and read ROOT file from iteration 0
            previous_root_file = f"root/{output_files[-1][0]}"
            
            if not os.path.exists(previous_root_file):
                print(f"[{optionIN}] Requires: {previous_root_file}")
                break  # STOP iteration for this config until ROOT exists
            
            particlesforIteration = get_particle_counts_by_type(previous_root_file)
            
            if not particlesforIteration:
                print(f"[{optionIN}] No particles found in {previous_root_file}")
                break  # STOP and wait for valid ROOT file
            
            # Calculate iteration time
            if optionIN == "onlyphotoemission":
                iterationTime = (particlesforIteration.get("gamma", 0) / planeArea) / PEflux
            elif optionIN == "onlysolarwind":
                iterationTime = (particlesforIteration.get("proton", 0) / planeArea) / SWions
                iterationTime2 = (particlesforIteration.get("e-", 0) / planeArea) / SWelectrons
                print("ion time: ", iterationTime*1000, "e- time: ", iterationTime2*1000)
            else:
                iterationTime = 0
            
            print(f"[{optionIN}] Particles: {particlesforIteration}, Time: {iterationTime*1000:.2f} ms")
            
            increment_filename = f"{i:03d}_iteration{i}_{optionIN}_num{select_num}"
            macro_path = f"macros/{increment_filename}.mac"
            
            with open(macro_path, 'w') as f:
                write_macro(f, increment_filename, select_num, iterationTime,
                          input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
            
            output_files.append((increment_filename + ".root", select_num, i, optionIN))
            
            # Create batch script
            batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{optionIN}.sh"
            create_batch_script(batch_path, i, optionIN, macro_path, account, username)
            
            i += 1
        
        else:  # i > 1
            previous_root_file = f"root/{output_files[-1][0]}"
            
            increment_filename = f"{i:03d}_iteration{i}_{optionIN}_num{select_num}"
            macro_path = f"macros/{increment_filename}.mac"
            
            with open(macro_path, 'w') as f:
                write_macro(f, increment_filename, select_num, iterationTime,
                          input_files=[f"{output_files[-1][0]}"], minStep=minStepIN)
            
            output_files.append((increment_filename + ".root", select_num, i, optionIN))
            
            # Create batch script
            batch_path = f"batchscripts/{i:03d}_submit_iteration{i}_{optionIN}.sh"
            create_batch_script(batch_path, i, optionIN, macro_path, account, username)
            
            i += 1

print(72*"-")
print(f"Created {len(output_files)} macro files and batch scripts")
print(f"Maximum iterations reached: {i-1}")