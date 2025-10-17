cmake_minimum_required(VERSION 3.20)

project(NervSDK)
set(PROJECT_VERSION 22.7)

# Include the utils definitions:
include(cmake/utils.cmake)

# Convenient setup of the compiler type:
set_compiler_flags()

message(STATUS "Building ${CMAKE_BUILD_TYPE} version of NervSDK")

if(IS_MSVC)
  message(STATUS "Building with MSVC compiler")
  # Using MSVC compiler: To build with static runtime linkage:
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "/EHsc /MD")

    if(WITH_DEBUG_INFO)
      set(CMAKE_EXE_LINKER_FLAGS
          "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /MAP /MAPINFO:EXPORTS")
      set(CMAKE_SHARED_LINKER_FLAGS
          "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG /MAP /MAPINFO:EXPORTS")
      set(CMAKE_CXX_FLAGS "/EHsc /MD /Zi") # /Z7
    endif()
  else()
    set(CMAKE_CXX_FLAGS "/EHsc /MD")

    if(WITH_DEBUG_INFO)
      set(CMAKE_EXE_LINKER_FLAGS
          "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF,NOICF /MAP /MAPINFO:EXPORTS"
      )
      set(CMAKE_SHARED_LINKER_FLAGS
          "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG /OPT:REF,NOICF /MAP /MAPINFO:EXPORTS"
      )
      set(CMAKE_CXX_FLAGS "/EHsc /MD /Zi") # /Z7
    endif()
  endif()

  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")

  # cf.
  # https://stackoverflow.com/questions/44960715/how-to-enable-stdc17-in-vs2017-with-cmake
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /Zc:__cplusplus")

elseif(IS_CLANG)
  message(STATUS "Building with Clang compiler")

  # using regular Clang or AppleClang
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -std=c++20 -fpch-instantiate-templates -Wno-deprecated-declarations"
  ) # -fno-strict-aliasing -stdlib=libc++

  # Always use the release version of the runtime:
  # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

  # Setup optimization flags: if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG
      "-g3 -Xclang -gcodeview -O0 -fno-omit-frame-pointer -DDEBUG -Wall -Wno-unused-function"
  ) # -Wno-unused-parameter -fsanitize=address -fsanitize=undefined
  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

  set(CMAKE_LINKER_FLAGS_DEBUG
      "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer"
  )# -fsanitize=address -fsanitize=undefined

  # if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  # add_link_options(-fsanitize=address) endif()

  # -fsanitize=address -Wextra message(STATUS "Debug CXX flags:
  # ${CMAKE_CXX_FLAGS_DEBUG}")

  # Ensure we will not compile with undefined symbols: cf.
  # https://cmake.org/pipermail/cmake/2017-July/065819.html set(
  # CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

elseif(IS_EMSCRIPTEN)
  message(STATUS "Building with Emscripten compiler")

  # set(CMAKE_CXX_FLAGS "-std=c++20 -fpch-instantiate-templates")
  set(CMAKE_CXX_FLAGS "-std=c++20 -fpch-instantiate-templates")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g -O0")
  # set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -O0 -fsanitize=address
  # -fno-omit-frame-pointer -g -gsource-map")

  # => cf.  https://www.youtube.com/watch?v=1RxMPEVBMJA on ASan

  # set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O0 -g")

  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

  # set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O0 -g")

else()
  message(STATUS "Building with GCC compiler")
  # Using GCC compiler
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20") # -Wall -Wcomment
                                                       # -fno-strict-aliasing

  # Setup optimization flags:
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -s")

  if("${FLAVOR}" STREQUAL "WIN32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i686")
  endif()

  if("${FLAVOR}" STREQUAL "LINUX64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
endif()

# prepare the source folder:
set(SRC_DIR ${PROJECT_SOURCE_DIR}/sources)

# Add the sources folder
add_subdirectory(sources)
