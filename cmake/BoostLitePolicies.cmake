# cmake_minimum_required() resets policies, so this script
# resets those policies back to what BoostLite needs again

# Apply visibility to all objects
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()
