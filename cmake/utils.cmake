function(set_compiler_flags)
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
