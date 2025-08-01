# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FluidSwirlPlugin is a professional OpenFX plugin for DaVinci Resolve that creates fluid swirl distortion effects. The plugin simulates video shot through water or other fluid mediums with three distinct modes: Radial Swirl, Directional Flow, and Boat Wake effects.

## Build System and Commands

### Build Commands
- **Windows**: Run `build.bat` or manually use CMake:
  ```batch
  mkdir build && cd build
  cmake .. -G "Visual Studio 17 2022" -A x64
  cmake --build . --config Release
  ```
- **macOS/Linux**: 
  ```bash
  mkdir build && cd build
  cmake ..
  make -j4
  ```

### Installation
Copy the generated `FluidSwirl.ofx.bundle` to:
- **Windows**: `C:\Program Files\Common Files\OFX\Plugins\`
- **macOS**: `/Library/OFX/Plugins/`
- **Linux**: `/usr/OFX/Plugins/`

### Testing
Test the plugin in DaVinci Resolve after installation. Find "Fluid Swirl" in Effects Library > Filter category.

## Code Architecture

### Core Components

1. **FluidSwirlPlugin.cpp** (`src/`) - Main plugin implementation
   - `FluidSwirlPlugin` class: Main effect class inheriting from `OFX::ImageEffect`
   - `FluidSwirlProcessorBase` and `FluidSwirlProcessor<>`: Template-based pixel processing
   - `FluidSwirlPluginFactory`: Plugin registration and parameter setup

2. **FluidSwirlOptimized.hpp** (`src/`) - Performance optimization layer
   - `GPUAccelerator` interface with CUDA/OpenCL implementations
   - `OptimizedSwirlProcessor<>`: CPU-optimized processor with SIMD hints
   - GPU fallback system for maximum compatibility

3. **FluidSwirlGPU.cu** (`src/`) - CUDA GPU acceleration
   - `fluidSwirlKernel`: GPU kernel for parallel swirl processing
   - Memory management helpers and CUDA availability checking

### Plugin Parameters Architecture
The plugin uses a comprehensive parameter system:
- **Flow Mode**: Choice parameter (0=Radial, 1=Directional, 2=Boat Wake)
- **Core Parameters**: Swirl Intensity, Center (X,Y), Radius, Decay
- **Flow Parameters**: Flow Direction, Flow Strength, Wake Width, Vortex Spacing

### Processing Pipeline
1. Parameter extraction at render time
2. GPU availability check and processor selection
3. Multi-threaded processing with render window subdivision
4. Bilinear interpolation for smooth distortion
5. Three distinct algorithm modes with optimized mathematical transformations

### OpenFX Integration
- Uses OpenFX Support Library from `openfx/Support/`
- Implements standard OpenFX interfaces for DaVinci Resolve compatibility
- Template-based pixel format support (8-bit, 16-bit, 32-bit float)
- Multi-component support (RGB, RGBA, Alpha)

## Development Guidelines

### When Adding New Features
- Follow the template pattern established in `FluidSwirlProcessor<>`
- Add parameters through `FluidSwirlPluginFactory::describeInContext()`
- Update both CPU and GPU code paths for consistency
- Test across all supported bit depths and pixel formats

### Performance Considerations
- GPU acceleration is preferred for real-time playback
- CPU path includes SIMD optimizations and loop unrolling
- Use bilinear interpolation for quality, nearest-neighbor for speed tests
- Consider memory bandwidth when modifying algorithms

### Common Modifications
- **New flow modes**: Add to the `_flowMode` switch statement in processing loop
- **Parameter additions**: Add to both parameter setup and processing functions
- **Algorithm changes**: Update both `FluidSwirlProcessor` and CUDA kernel
- **Optimization**: Profile both CPU and GPU code paths

### Cross-Platform Development
- Windows uses Visual Studio 2019+ with CUDA toolkit
- macOS/Linux use GCC/Clang with OpenMP support
- CMake handles platform-specific linking (OpenGL, CUDA)
- Test plugin bundle structure on target platforms

## Algorithm Details

The plugin implements three mathematically distinct fluid simulation approaches:

1. **Radial Swirl**: Polar coordinate transformation with exponential decay
2. **Directional Flow**: Unidirectional displacement based on perpendicular distance
3. **Boat Wake**: von Kármán vortex street with alternating vortices and directional flow

Each mode uses optimized mathematical operations and GPU-accelerated parallel processing for real-time performance.