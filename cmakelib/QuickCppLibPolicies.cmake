# cmake_minimum_required() resets policies, so this script
# resets those policies back to what BoostLite needs again

# Enable IN_LIST always
if(POLICY CMP0057)
  cmake_policy(SET CMP0057 NEW)
endif()

# Apply visibility to all objects
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()
