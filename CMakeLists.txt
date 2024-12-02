
cmake_minimum_required(VERSION 3.15)

project(sorth)

# cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja



# Find lib ffi.
if(WIN32)
    find_package(unofficial-libffi CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFI REQUIRED libffi)

    message(STATUS "FFI: ${FFI_INCLUDE_DIRS}")
    include_directories(${FFI_INCLUDE_DIRS})
endif()



# Find and configure our llvm dependencies if available.
if(APPLE)
    if (EXISTS "/usr/local/opt/llvm/lib/cmake/llvm")
        # Intel Macs
        set(LLVM_DIR "/usr/local/opt/llvm/lib/cmake/llvm")
    elseif (EXISTS "/opt/homebrew/opt/llvm/lib/cmake/llvm")
        # Apple Silicon Macs
        set(LLVM_DIR "/opt/homebrew/opt/llvm/lib/cmake/llvm")
    endif()

    message(STATUS "Checking for LLVM in ${LLVM_DIR}...")
    find_package(LLVM CONFIG QUIET)
else()
    message(STATUS "Checking for LLVM...")
    find_package(LLVM CONFIG QUIET)
endif()

set(SORTH_JIT_DISABLED 1) # Default to disabled

if (LLVM_FOUND)
    message(STATUS "  Checking LLVM version...")

    if(LLVM_VERSION_MAJOR EQUAL 19 OR LLVM_VERSION_MAJOR EQUAL 20)
        message(STATUS "  LLVM version ${LLVM_VERSION_MAJOR} detected, enabling JIT functionality.")
        set(SORTH_JIT_DISABLED 0)
    else()
        message(STATUS "  LLVM version ${LLVM_VERSION_MAJOR} detected, not 19. JIT functionality will be disabled.")
    endif()
else()
    message(STATUS "  LLVM not found, JIT functionality will be disabled.")
endif()



# Make sure we're using C++ 20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Define source files and directories.
set(SRC_DIR "src")
file(GLOB SRCS "${SRC_DIR}/*.cpp")

set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist")



# Custom target to create the dist directory
add_custom_target(create_dist_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMENT "Creating dist directory.")



# Get the compiler name and version.  Also, remove the .exe extension if it exists.
get_filename_component(COMPILER_NAME ${CMAKE_CXX_COMPILER} NAME)
string(REGEX REPLACE "\\.exe$" "" COMPILER_NAME_SIMPLE "${COMPILER_NAME}")

set(CXXFLAGS "-DMESSAGE=\"${COMPILER_NAME_SIMPLE} ${CMAKE_CXX_COMPILER_VERSION}\"")



# Detect OS for setting compiler flags and other OS-specific settings
if(WIN32)
    #
elseif(APPLE)
    set(LINKFLAGS "-fuse-ld=lld -ldl -lffi -lm")

    if(CXXTARGET STREQUAL "x86_64-unknown-linux-gnu")
        set(LINKFLAGS "${LINKFLAGS} -static -stdlib=libc++")
    elseif(CXXTARGET STREQUAL "arm-unknown-linux-gnu")
        set(LINKFLAGS "${LINKFLAGS} -latomic -mfloat-abi=soft")
    endif()
else()
    #
endif()



# Include the source directory for the .h files.
include_directories(${SRC_DIR})

# Include the llvm directories and definitions if we found llvm.
if (NOT SORTH_JIT_DISABLED)
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
    set(LLVM_COMPONENTS
        Target
        TargetParser
        X86TargetMCA
        X86CodeGen
        X86AsmParser
        OrcJIT
        ExecutionEngine
        Support
        Passes
        Core
        X86
        X86AsmParser
        X86CodeGen
        Analysis
        MC
        MCParser)

    llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})
else()
    add_definitions(-DSORTH_JIT_DISABLED=1)
endif()



# Define target.
add_executable(${PROJECT_NAME} ${SRCS})



# Set compiler flags.
target_compile_options(${PROJECT_NAME} PRIVATE ${CXXFLAGS})


# Add lib ffi and llvm to the target.
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::libffi::libffi)
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFI_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FFI_LIBRARIES})
endif()

# Add the llvm libraries if we found llvm.
if (NOT SORTH_JIT_DISABLED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})
endif()


# Output directory.
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})



# Build the library that the tests depend on.
add_library(TestLib SHARED
    ${CMAKE_SOURCE_DIR}/tests/test_ffi_lib.cpp)

set_target_properties(TestLib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${DIST_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${DIST_DIR})



# Git versioning.
execute_process(COMMAND git rev-list --abbrev-commit --tags --max-count=1
                OUTPUT_VARIABLE TAG_COMMIT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND git log -1 --format=%cd --date=format:%Y.%m.%d
                OUTPUT_VARIABLE DATE_RAW
                OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "^\"|\"$" "" DATE "${DATE_RAW}")

execute_process(COMMAND git describe --abbrev=0 --tags ${TAG_COMMIT}
                OUTPUT_VARIABLE TAG
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)

if(TAG STREQUAL "")
    set(TAG "dev")
endif()



execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE COMMIT)
string(SUBSTRING ${COMMIT} 0 8 COMMIT)
set(VERSION "${TAG}-${COMMIT}-${DATE}")



# Define preprocessor macro for version
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION="${VERSION}")



# Custom target to copy the standard library
add_custom_target(copy_stdlib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/std.f ${DIST_DIR}/std.f
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/std ${DIST_DIR}/std
    COMMENT "Copying standard library files to the dist directory.")



# Ensure the dist dir exists and copy_stdlib runs before the main target.
add_dependencies(copy_stdlib create_dist_dir)
add_dependencies(${PROJECT_NAME} copy_stdlib TestLib)

