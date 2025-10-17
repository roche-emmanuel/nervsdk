set(TARGET_NAME "nervsdk")

include_directories(.)

file(GLOB_RECURSE PUBLIC_HEADERS "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_library(${TARGET_NAME} ${PUBLIC_HEADERS} ${SOURCE_FILES})

target_precompile_headers(${TARGET_NAME} PRIVATE "nvk_precomp.h")

# Set the installation prefix to dist directory only if not already set
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_CURRENT_SOURCE_DIR}/dist"
      CACHE PATH "Installation directory" FORCE)
endif()

# Install the library
install(
  TARGETS ${TARGET_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

# Install headers preserving directory structure
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "dist" EXCLUDE
  PATTERN "build" EXCLUDE)
