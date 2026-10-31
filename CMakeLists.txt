#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.12)
project(g4chargeit)

#----------------------------------------------------------------------------
# Find Required Packages
#
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

find_package(g4pbc REQUIRED NO_MODULE)
find_package(ROOT REQUIRED)
find_package(OpenMP REQUIRED)

#----------------------------------------------------------------------------
# Setup include directories and compile definitions
#
include(${Geant4_USE_FILE})
include_directories(${g4pbc_INCLUDE_DIRS})
include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/outputlogs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/distributions)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/geometry)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/root)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/fieldmaps)

#----------------------------------------------------------------------------
# Add the executable
#
add_executable(g4chargeit g4chargeit.cc ${sources} ${headers})

# --- Add compile option for AVX/native instructions ---
target_compile_options(g4chargeit PRIVATE -march=native)


# --- MODIFIED: Consolidate all linking into a single, modern command ---
target_link_libraries(g4chargeit
  PRIVATE
    ${Geant4_LIBRARIES}
    g4pbc::g4pbc
    ${ROOT_LIBRARIES}
    OpenMP::OpenMP_CXX   # Link OpenMP for multithreading
  )


file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test-macros)
file(GLOB MACRO_FILES ${CMAKE_SOURCE_DIR}/test-macros/*)

foreach(_script ${MACRO_FILES})
  get_filename_component(_name ${_script} NAME)
  configure_file(
    ${_script}
    ${PROJECT_BINARY_DIR}/test-macros/${_name}
    COPYONLY
  )
endforeach()


file(GLOB SLURM_SCRIPTS ${CMAKE_SOURCE_DIR}/slurm-scripts/*)
file(COPY ${SLURM_SCRIPTS} DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB DISTRIBUTIONS ${CMAKE_SOURCE_DIR}/distributions/*.txt)
file(COPY ${DISTRIBUTIONS} DESTINATION ${CMAKE_BINARY_DIR}/distributions/)

file(GLOB GEOMETRY ${CMAKE_SOURCE_DIR}/geometry/*.stl)
file(COPY ${GEOMETRY} DESTINATION ${CMAKE_BINARY_DIR}/geometry/)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS g4chargeit DESTINATION bin)

