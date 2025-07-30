cmake_minimum_required(VERSION 3.16)

project(FluidSwirlPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# OpenFX SDK path - adjust this to your OpenFX SDK location
set(OFX_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/openfx" CACHE PATH "Path to OpenFX SDK")

# Include OpenFX headers
include_directories(
    ${OFX_SDK_ROOT}/include
    ${OFX_SDK_ROOT}/Support/include
    ${OFX_SDK_ROOT}/Support/Plugins/include
)

# Source files
set(SOURCES
    src/FluidSwirlPlugin.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsCore.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsImageEffect.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsInteract.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsLog.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsMultiThread.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsParams.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsProperty.cpp
    ${OFX_SDK_ROOT}/Support/Library/ofxsPropertyValidation.cpp
)

# Platform-specific settings
if(WIN32)
    set(PLUGIN_SUFFIX ".ofx")
    set(PLUGIN_ARCH "Win64")
    add_definitions(-DWINDOWS -D_WINDOWS)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
elseif(APPLE)
    set(PLUGIN_SUFFIX ".ofx")
    set(PLUGIN_ARCH "MacOS")
    add_definitions(-DMAC_OS_X_VERSION_MIN_REQUIRED=1070)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
else()
    set(PLUGIN_SUFFIX ".ofx")
    set(PLUGIN_ARCH "Linux-x86-64")
    add_definitions(-DLINUX)
endif()

# Create the plugin
add_library(FluidSwirl SHARED ${SOURCES})

# Set output name and properties
set_target_properties(FluidSwirl PROPERTIES
    OUTPUT_NAME "FluidSwirl${PLUGIN_SUFFIX}"
    PREFIX ""
    SUFFIX ""
)

# Link libraries
target_link_libraries(FluidSwirl ${OPENGL_LIBRARIES})

if(WIN32)
    target_link_libraries(FluidSwirl opengl32 glu32)
elseif(APPLE)
    target_link_libraries(FluidSwirl "-framework OpenGL" "-framework CoreFoundation")
else()
    target_link_libraries(FluidSwirl GL GLU)
endif()

# Create bundle structure
set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/FluidSwirl.ofx.bundle")
set(CONTENTS_DIR "${BUNDLE_DIR}/Contents")
set(PLUGIN_DIR "${CONTENTS_DIR}/${PLUGIN_ARCH}")

# Create directories
add_custom_command(TARGET FluidSwirl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONTENTS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_DIR}
)

# Copy plugin binary to bundle
add_custom_command(TARGET FluidSwirl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:FluidSwirl> ${PLUGIN_DIR}/
)

# Copy Info.plist
add_custom_command(TARGET FluidSwirl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist ${CONTENTS_DIR}/
)

# Installation
install(DIRECTORY ${BUNDLE_DIR} 
    DESTINATION "$ENV{PROGRAMFILES}/Common Files/OFX/Plugins"
    COMPONENT plugin
)