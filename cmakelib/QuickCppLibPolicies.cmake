# cmake_minimum_required() resets policies, so this script
# resets those policies back to what BoostLite needs again

# Apply visibility to all objects
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()
# respect CMAKE_MSVC_RUNTIME_LIBRARY if available
if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
