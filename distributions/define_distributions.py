import numpy as np
import pandas as pd
from scipy.interpolate import interp1d

def maxwellian(v, vth):
    """
    Maxwellian distribution function.
    
    Parameters:
    -----------
    v : array-like
        Velocity values
    vth : float
        Thermal velocity
    
    Returns:
    --------
    array-like
        Distribution values at given velocities
    """

    np.sqrt(E)
    return (v**2 / (vth * np.sqrt(np.pi))) * np.exp(-v**2 / vth**2)

def generate_maxwellian_distribution(vmin, vmax, nPoints, vth, scale_factor=1e-6, output_file=None):
    """
    Generate a Maxwellian velocity distribution and optionally save to file.
    
    Parameters:
    -----------
    vmin : float
        Minimum velocity
    vmax : float
        Maximum velocity
    nPoints : int
        Number of discretization points
    vth : float
        Thermal velocity
    scale_factor : float, optional
        Scaling factor for velocities (default: 1e-6 for µm/ns to m/s)
    output_file : str, optional
        Output filename. If None, no file is written.
    
    Returns:
    --------
    tuple
        (velocities_scaled, weights) arrays
    """
    # Discretize velocity range
    velocities = np.linspace(vmin, vmax, nPoints + 1)
    
    # Compute weights and normalize
    weights_raw = maxwellian(velocities, vth)
    weights = weights_raw / np.sum(weights_raw)
    
    # Scale velocities (e.g., µm/ns → m/s: 1 µm/ns = 10^6 m/s)
    velocities_scaled = velocities * scale_factor
    
    # Write to file if requested
    if output_file:
        lines = [f"{v:.6e} {w:.6e}" for v, w in zip(velocities_scaled, weights)]
        with open(output_file, "w") as f:
            f.write("\n".join(lines))
        print(f"Exported {len(lines)} lines to {output_file}")
    
    return velocities_scaled, weights

def interpolate_and_export_spectrum(csv_file, x_col, y_col, vmin, vmax, n_points, 
                                     x_transform=None, scale_factor=1e-6, output_file=None,
                                     log_interpolation=False):
    """
    Load spectrum data from CSV, interpolate over a new energy grid, and export.
    
    Parameters:
    -----------
    log_interpolation : bool, optional
        If True, use logarithmic spacing for interpolation grid AND 
        log-log interpolation for y-values (default: False)
    """
    # Load data
    data = pd.read_csv(csv_file)
    x_data = np.array(data[x_col])
    y_data = np.array(data[y_col])
    
    # Apply transformation if provided
    if x_transform:
        x_data = x_transform(x_data)
    
    # Normalize y-data
    y_data = y_data / np.sum(y_data)
    
    # Sort by x-values
    sorted_indices = np.argsort(x_data)
    x_data = x_data[sorted_indices]
    y_data = y_data[sorted_indices]
    
    # Remove zero or negative values for log interpolation
    if log_interpolation:
        mask = (y_data > 0) & (x_data > 0)
        x_data = x_data[mask]
        y_data = y_data[mask]
        
        if len(x_data) == 0:
            raise ValueError("No positive values remaining after filtering for log interpolation")
    
    print(f"Data range: {min(x_data):.2f} to {max(x_data):.2f} eV")
    
    # Create interpolation grid
    if log_interpolation:
        if vmin <= 0:
            raise ValueError("vmin must be > 0 for logarithmic interpolation")
        x_interp = np.logspace(np.log10(vmin), np.log10(vmax), n_points)
        
        # Log-log interpolation
        interp_func = interp1d(np.log10(x_data), np.log10(y_data), 
                              kind='linear', bounds_error=False, fill_value='extrapolate')
        y_interp = 10**interp_func(np.log10(x_interp))
        print(f"Using log-log interpolation")
    else:
        x_interp = np.linspace(vmin, vmax, n_points)
        interp_func = interp1d(x_data, y_data, kind='linear', 
                              bounds_error=False, fill_value='extrapolate')
        y_interp = interp_func(x_interp)
        print(f"Using linear interpolation")
    
    # Ensure all interpolated values are positive (clamp to small positive value)
    y_interp = np.maximum(y_interp, 1e-30)
    
    # Re-normalize after interpolation
    y_interp = y_interp / np.sum(y_interp)
    
    # Scale energies
    energies_scaled = x_interp * scale_factor
    
    # Write to file if requested
    if output_file:
        lines = [f"{energy:.6e} {weight:.6e}" 
                for energy, weight in zip(energies_scaled, y_interp)]
        with open(output_file, "w") as f:
            f.write("\n".join(lines))
        print(f"Exported {len(lines)} lines to {output_file}")
    
    return energies_scaled, y_interp

# =============================================================================
# Generate Electron Distribution (Maxwellian)
# =============================================================================
# print("Generating electron distribution...")
# electron_velocities, electron_weights = generate_maxwellian_distribution(
#     vmin=0.0,
#     vmax=100.0,
#     nPoints=80,
#     vth=10.0,  # Thermal velocity
#     scale_factor=1e-6,  # µm/ns to m/s
#     output_file="electronMaxwellian_distribution.txt"
# )

# =============================================================================
# Generate Solar Wind Electrons and Ions (from digitized data)
# =============================================================================

print("\nGenerating solar wind electrons distribution (Li 2023)...")
electron_energies, electron_weights = interpolate_and_export_spectrum(
    csv_file="Fig4-Li2023-SWelectrons.csv",
    x_col="x",
    y_col=" y",
    vmin=1,
    vmax=10000,
    n_points=500,
    log_interpolation=True,
    x_transform=None,  # Convert wavelength (nm) to energy (eV)
    scale_factor=1e-6,  # eV to MeV
    output_file="electronSolarWind_distribution.txt"
)

print("\nGenerating solar wind ions distribution (Li 2023)...")
ions_energies, ions_weights = interpolate_and_export_spectrum(
    csv_file="Fig4-Li2023-SWions.csv",
    x_col="x",
    y_col=" y",
    vmin=500,
    vmax=5000,
    n_points=500,
    log_interpolation=False,
    x_transform=None,  # Convert wavelength (nm) to energy (eV)
    scale_factor=1e-6,  # eV to MeV
    output_file="ionSolarWind_distribution.txt"
)

# =============================================================================
# Generate Solar Spectrum Distribution (Farrell Solar Minimum)
# =============================================================================
print("\nGenerating solar spectrum distribution (Farrell 2023)...")
solar_energies, solar_weights = interpolate_and_export_spectrum(
    csv_file="Fig2-FarrellSolarMinimum.csv",
    x_col="x",
    y_col=" y",
    vmin=8.1,
    vmax=330.0,
    n_points=1000,
    x_transform=lambda wavelength_nm: 1240 / wavelength_nm,  # Convert wavelength (nm) to energy (eV)
    scale_factor=1e-6,  # eV to MeV
    output_file="photonSolar_distribution.txt"
)

print("\nAll distributions generated successfully!")