# CMakeList.txt : CMake project for EcoRT, include source and define
# project specific logic here.  
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
file(GLOB MY_SOURCE_FILES ".hpp" "*.cpp" "*.h" "imgui/*.cpp" "imgui/*.h")
add_executable (ecort ${MY_SOURCE_FILES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ecort PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(ecort 
    ${TORCH_LIBRARIES}
    box2d
    sfml-graphics
    GLEW::GLEW
    glfw
    OpenGL::GL
    ${ZLIB_LIBRARIES}
    ${FREETYPE_LIBRARIES})

# add shader file
target_include_directories(ecort PRIVATE "shaders")

target_compile_features(ecort PRIVATE cxx_std_17)
set_property(TARGET ecort PROPERTY CXX_STANDARD 17)

# The following code block is suggested to be used on Windows.
# According to https://github.com/pytorch/pytorch/issues/25457,
# the DLLs need to be copied to avoid memory errors.
if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET ecort
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:ecort>)
endif (MSVC)


#file(COPY "${CMAKE_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}/EcoRT")
file(COPY "${CMAKE_SOURCE_DIR}/LatinNames.txt" DESTINATION "${CMAKE_BINARY_DIR}")

install(TARGETS ecort)