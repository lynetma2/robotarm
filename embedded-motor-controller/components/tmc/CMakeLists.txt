# This file tells the ESP-IDF build system how to compile the TMC-API

# Find all the .c source files in the library's 'tmc' directory
# (Use GLOB_RECURSE, not GLOB_RECURSIVE)
file(GLOB_RECURSE LIB_SRCS "TMC-API/tmc/*.c")

# The library's config files (like TMC_default_Config.h)
# include IC headers with paths like "TMC2209.h".
# This means we must add all the IC-specific directories
# to the include path.
# This finds all subdirectories under 'tmc/ic'
file(GLOB IC_DIRS "TMC-API/tmc/ic/*")

# Create a list of include paths
set(LIB_INCLUDE_DIRS "." "TMC-API/tmc/helpers")
foreach(IC_DIR ${IC_DIRS})
    if(IS_DIRECTORY "${IC_DIR}")
        list(APPEND LIB_INCLUDE_DIRS "${IC_DIR}")
    endif()
endforeach()

# Register this directory as a component
idf_component_register(
        # Compile the source files we found
        SRCS "${LIB_SRCS}"

        # Add the root directory ('.') AND all the IC-specific
        # subdirectories (e.g., "tmc/ic/TMC2209") to the include path.
        INCLUDE_DIRS ${LIB_INCLUDE_DIRS}
)