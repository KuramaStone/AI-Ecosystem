# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("EcoRT")

set(CMAKE_PREFIX_PATH "third-party\\libtorch\\share\\cmake\\Torch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ox /favor:INTEL64 /arch:AVX2 /fp:fast /openmp")


set(SFML_DIR "third-party\\SFML\\lib\\cmake\\SFML")
set(SFML_DLL_DIR "\\third-party\\SFML\\bin\\")
find_package(SFML 2.6 COMPONENTS system window graphics network audio REQUIRED)

set(CMAKE_PREFIX_PATH "third-party\\glew")
set(CMAKE_LIBRARY_PATH "third-party\\glew")
include_directories(${GLEW_INCLUDE_DIRS})
find_package(GLEW REQUIRED)

 
SET( EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}" )
SET( CMAKE_MODULE_PATH "$ENV{EIGEN3_DIR}" )
find_package(Eigen3 REQUIRED)
include_directories( ${EIGEN3_INCLUDE_DIR} )

find_package(OpenGL REQUIRED)

find_package(OpenMP REQUIRED)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(CMAKE_LIBRARY_PATH "third-party\\freetype-2.10.0\\build\\RelWithDebInfo\\")
set(CMAKE_PREFIX_PATH "third-party\\freetype-2.10.0")
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# Include sub-projects.
add_subdirectory ("third-party/box2d-release")
add_subdirectory ("EcoRT")

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
