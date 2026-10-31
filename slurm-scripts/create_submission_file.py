import os
import re
import glob
import shutil
import numpy as np
import sys
from collections import defaultdict

# Default configuration
DEFAULT_JOB_TYPES = "photo+solar"

# Get command line arguments
if len(sys.argv) < 3:
    print("Usage: python create_submission_file.py <start_iteration> <end_iteration> [job_types]")
    print("Example: python create_submission_file.py 5 10")
    print("Example: python create_submission_file.py 5 10 photoemission")
    print("Example: python create_submission_file.py 5 10 solarwind") 
    print("Example: python create_submission_file.py 5 10 allparticles")
    print("Example: python create_submission_file.py 5 10 onlyUV")
    print("Example: python create_submission_file.py 5 10 photo+solar")
    print("Example: python create_submission_file.py 5 10 allcases")
    sys.exit(1)

try:
    start_iteration = int(sys.argv[1])
    end_iteration   = int(sys.argv[2])
except ValueError:
    print("Error: Start and end iterations must be integers")
    sys.exit(1)

# Get job types from command line or use default
if len(sys.argv) >= 4:
    job_types = sys.argv[3].lower()
    if job_types not in ['photoemission','solarwind','allparticles','onlyuv','photo+solar','allcases']:
        print(f"Error: Invalid job type '{job_types}'.")
        sys.exit(1)
else:
    job_types = DEFAULT_JOB_TYPES

# Directories
batch_dir = "batchscripts"
output_script = "submit_iterations.sh"

# Get and sort all batch script files
batch_scripts = glob.glob(os.path.join(batch_dir, "*.sh"))
batch_scripts.sort()

# Storage
photoemission_scripts = []
solarwind_scripts = []
allparticles_scripts = []
onlyUV_scripts = []

# Regex supports:
#   iteration number
#   _onlyUV-energy5.3eV_
pattern = r"iteration(\d+)_(onlyphotoemission|onlysolarwind|allparticles|onlyUV)(?:-energy([0-9.]+)eV)?"

for script_path in batch_scripts:
    match = re.search(pattern, script_path)
    if match:
        iteration = int(match.group(1))
        script_type = match.group(2)
        uv_energy = match.group(3)  # May be None

        if iteration < start_iteration or iteration > end_iteration:
            continue

        if script_type == "onlyphotoemission":
            photoemission_scripts.append((iteration, script_path))
        elif script_type == "onlysolarwind":
            solarwind_scripts.append((iteration, script_path))
        elif script_type == "allparticles":
            allparticles_scripts.append((iteration, script_path))
        elif script_type == "onlyUV":
            # Save UV energy info as well
            onlyUV_scripts.append((iteration, uv_energy, script_path))

# Sort lists
photoemission_scripts.sort()
solarwind_scripts.sort()
allparticles_scripts.sort()
onlyUV_scripts.sort()

# Which job types to include
include_photoemission = job_types in ['photoemission','photo+solar','allcases']
include_solarwind     = job_types in ['solarwind','photo+solar','allcases']
include_allparticles  = job_types in ['allparticles','allcases']
include_onlyUV        = job_types in ['onlyuv','allcases']

# Write master script
with open(output_script, "w") as f:

    f.write("#!/bin/bash\n\n")
    f.write(f"# Submission script for iterations {start_iteration} to {end_iteration}\n")
    f.write(f"# Job types: {job_types}\n\n")

    job_count = 0

    # PHOTOEMISSION JOBS
    if include_photoemission and photoemission_scripts:
        f.write("# Photoemission jobs\n")
        previous = ""
        for i,(iteration,script_path) in enumerate(photoemission_scripts):
            dep = "" if i == 0 else f"--dependency=afterok:{previous}"
            f.write(f"jid{job_count}=$(sbatch --parsable {dep} {script_path})\n")
            f.write(f"echo \"Submitted photoemission iteration {iteration}: $jid{job_count}\"\n\n")
            previous = f"$jid{job_count}"
            job_count += 1
    elif include_photoemission:
        f.write("# No photoemission jobs found\n\n")

    # SOLARWIND JOBS
    if include_solarwind and solarwind_scripts:
        f.write("# Solarwind jobs\n")
        previous = ""
        for i,(iteration,script_path) in enumerate(solarwind_scripts):
            dep = "" if i == 0 else f"--dependency=afterok:{previous}"
            f.write(f"jid{job_count}=$(sbatch --parsable {dep} {script_path})\n")
            f.write(f"echo \"Submitted solarwind iteration {iteration}: $jid{job_count}\"\n\n")
            previous = f"$jid{job_count}"
            job_count += 1
    elif include_solarwind:
        f.write("# No solarwind jobs found\n\n")

    # ALLPARTICLES JOBS
    if include_allparticles and allparticles_scripts:
        f.write("# Allparticles jobs\n")
        previous = ""
        for i,(iteration,script_path) in enumerate(allparticles_scripts):
            dep = "" if i == 0 else f"--dependency=afterok:{previous}"
            f.write(f"jid{job_count}=$(sbatch --parsable {dep} {script_path})\n")
            f.write(f"echo \"Submitted allparticles iteration {iteration}: $jid{job_count}\"\n\n")
            previous = f"$jid{job_count}"
            job_count += 1
    elif include_allparticles:
        f.write("# No allparticles jobs found\n\n")

    # 🆕 ONLY UV JOBS (energy-specific, iteration 0 has no dependency)
    if include_onlyUV and onlyUV_scripts:
        f.write("# Only-UV jobs\n")
        previous = ""

        for i, (iteration, uv_energy, script_path) in enumerate(onlyUV_scripts):

            # REMOVE DEPENDENCY FOR ITERATION 0
            if iteration == 0:
                dep = ""
            else:
                dep = f"--dependency=afterok:{previous}" if previous else ""

            label = f"{uv_energy}eV" if uv_energy else "unknown_energy"

            f.write(f"jid{job_count}=$(sbatch --parsable {dep} {script_path})\n")
            f.write(f"echo \"Submitted onlyUV iteration {iteration}, energy {label}: $jid{job_count}\"\n\n")

            previous = f"$jid{job_count}"
            job_count += 1

    elif include_onlyUV:
        f.write("# No onlyUV jobs found\n\n")

    if job_count == 0:
        f.write(f"echo \"No scripts found in range {start_iteration}–{end_iteration}\"")

# Make executable
os.chmod(output_script, 0o755)

print(f"Created submission script with {job_count} jobs.")
