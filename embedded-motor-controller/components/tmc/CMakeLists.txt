# ==============================
# ESP-IDF Component for TMC-API
# ==============================

# === Step 1: Define which ICs you want to include ===
# (Add or remove ICs here as needed)
set(USED_TMC_ICS
        # MAX22200
        TMC2209
        # TMC4671
)

# === Step 2: Collect source files ===

# Start with an empty source list
set(LIB_SRCS "")

# Add helper .c files automatically
file(GLOB HELPER_SRCS "TMC-API/tmc/helpers/*.c")
list(APPEND LIB_SRCS ${HELPER_SRCS})

# Add selected IC driver sources
foreach(ic ${USED_TMC_ICS})
    list(APPEND LIB_SRCS "TMC-API/tmc/ic/${ic}/${ic}.c")
endforeach()

# === Step 3: Include directories ===

# Gather all IC subdirectories (for includes)
file(GLOB IC_DIRS "TMC-API/tmc/ic/*")

# Core include directories
set(LIB_INCLUDE_DIRS
        "."                     # local dir
        "TMC-API"               # for includes like <tmc/ic/...>
        "TMC-API/tmc/helpers"   # helper headers
)

# Add all IC subdirs (so each IC header path works)
foreach(IC_DIR ${IC_DIRS})
    if(IS_DIRECTORY "${IC_DIR}")
        list(APPEND LIB_INCLUDE_DIRS "${IC_DIR}")
    endif()
endforeach()

# === Step 4: Register the component with ESP-IDF ===
idf_component_register(
        SRCS ${LIB_SRCS}
        INCLUDE_DIRS ${LIB_INCLUDE_DIRS}
)

# === Optional: Print info for debugging ===
message(STATUS "TMC-API component configured with ICs: ${USED_TMC_ICS}")
message(STATUS "Included helper sources: ${HELPER_SRCS}")
