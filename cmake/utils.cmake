function(detect_compiler)
  # if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(\\.bat)?$")
    message(STATUS "C++ compiler is Emscripten")
    set(CMAKE_CXX_COMPILER_ID
        "Emscripten"
        CACHE STRING "Emscripten C++ compiler detected.")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Selecting compiler: MSVC")
    set(IS_MSVC
        ON
        CACHE BOOL "MSVC compiler detected")
    set(IS_CLANG
        OFF
        CACHE BOOL "Clang compiler not detected")
    set(IS_EMSCRIPTEN
        OFF
        CACHE BOOL "Emscripten compiler not detected")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
    message(STATUS "Selecting compiler: Emscripten")
    set(IS_MSVC
        OFF
        CACHE BOOL "MSVC compiler not detected")
    set(IS_CLANG
        OFF
        CACHE BOOL "Clang compiler not detected")
    set(IS_EMSCRIPTEN
        ON
        CACHE BOOL "Emscripten compiler detected")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Selecting compiler: Clang")
    set(IS_MSVC
        OFF
        CACHE BOOL "MSVC compiler not detected")
    set(IS_CLANG
        ON
        CACHE BOOL "Clang compiler detected")
    set(IS_EMSCRIPTEN
        OFF
        CACHE BOOL "Emscripten compiler not detected")
  else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()

macro(setup_compiler_flags)
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
    # CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}
    # -Wl,--no-undefined")

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
    message(FATAL_ERROR "Unsupported compiler.")
  endif()
endmacro()
