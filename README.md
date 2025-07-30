# Fluid Swirl Plugin for DaVinci Resolve

A professional OpenFX plugin that creates fluid swirl distortion effects, making your video appear as if shot through water or other fluid mediums.

## Features

- **Real-time GPU-accelerated processing** - Optimized for smooth playback
- **Intuitive controls** - Easy-to-use parameters for creative flexibility
- **Cross-platform compatibility** - Works on Windows, macOS, and Linux
- **Professional quality** - Suitable for broadcast and cinema work
- **DaVinci Resolve integration** - Available across all Resolve pages (Cut, Edit, Fusion, Color)

## Parameters

### Flow Mode
Choose the type of fluid distortion:
- **Radial Swirl** - Classic circular swirl from center point
- **Directional Flow** - Unidirectional fluid current effect  
- **Boat Wake** - Realistic boat wake with alternating vortices

### Swirl Intensity (-10.0 to 10.0)
Controls the strength of the swirl effect. Positive values create clockwise swirls, negative values create counter-clockwise swirls.

### Center (X, Y coordinates)
Sets the center point or starting position of the effect. For boat wake mode, this is where the boat path begins.

### Radius (1.0 to 1000.0)
Defines the size of the swirl effect area. Larger values affect more of the image.

### Decay (1.0 to 500.0)
Controls how quickly the swirl effect fades from the center. Lower values create more concentrated swirls.

### Flow Direction (-180° to 180°)
**For Directional Flow and Boat Wake modes:**
Sets the direction of fluid flow in degrees (0° = right, 90° = up, 180° = left, -90° = down).

### Flow Strength (0.0 to 10.0)
**For Directional Flow and Boat Wake modes:**
Controls the intensity of the directional flow displacement.

### Wake Width (5.0 to 200.0)
**For Directional Flow and Boat Wake modes:**
Defines the width of the wake or flow disturbance area.

### Vortex Spacing (10.0 to 300.0)
**For Boat Wake mode only:**
Controls the distance between alternating vortices in the wake pattern. Smaller values create more frequent vortices.

## Installation

### Prerequisites
1. **OpenFX SDK** - Download from [OpenFX.org](http://openeffects.org/)
2. **CMake 3.16+** - [Download here](https://cmake.org/download/)
3. **Visual Studio 2019+** (Windows) or Xcode (macOS) or GCC (Linux)

### Build Instructions

#### Windows
1. Clone or download this repository
2. Download and extract the OpenFX SDK to `FluidSwirlPlugin/openfx/`
3. Run `build.bat` or build manually:
   ```batch
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019" -A x64
   cmake --build . --config Release
   ```

#### macOS/Linux
```bash
mkdir build && cd build
cmake ..
make -j4
```

### Plugin Installation
1. Copy the generated `FluidSwirl.ofx.bundle` folder to your OFX plugins directory:
   - **Windows**: `C:\Program Files\Common Files\OFX\Plugins\`
   - **macOS**: `/Library/OFX/Plugins/`
   - **Linux**: `/usr/OFX/Plugins/`
2. Restart DaVinci Resolve
3. Find "Fluid Swirl" in Effects Library > Filter category

## Usage

### Basic Setup
1. **Add the effect** to your clip in DaVinci Resolve
2. **Choose Flow Mode** based on your desired effect:
   - Radial Swirl for whirlpools and circular distortion
   - Directional Flow for current and wind effects
   - Boat Wake for realistic wake patterns

### Radial Swirl Mode
3. **Set the center point** by adjusting the Center X/Y parameters
4. **Adjust Swirl Intensity** to control rotation strength
5. **Fine-tune Radius and Decay** for the desired falloff

### Directional Flow Mode
3. **Set Flow Direction** (0° = right, 90° = up, etc.)
4. **Adjust Flow Strength** to control displacement intensity
5. **Set Wake Width** to define the affected area
6. **Position Center** as the starting point of the flow

### Boat Wake Mode
3. **Set Flow Direction** for the boat's travel direction
4. **Adjust Swirl Intensity** for vortex rotation strength
5. **Set Flow Strength** for the wake's forward displacement
6. **Fine-tune Wake Width** for the disturbance area
7. **Adjust Vortex Spacing** for the distance between swirls
8. **Position Center** where the boat path begins

### Animation Tips
- **Animate Flow Direction** to simulate changing wind or current
- **Keyframe Vortex Spacing** for varying turbulence intensity
- **Animate Center position** to follow moving objects (boats, swimmers)
- **Combine multiple instances** for complex fluid interactions

## Technical Specifications

- **Supported formats**: All DaVinci Resolve supported formats
- **Bit depths**: 8-bit, 16-bit, 32-bit float
- **Color spaces**: RGB, RGBA, Alpha
- **Processing**: GPU-accelerated with CPU fallback
- **Threading**: Multi-threaded for optimal performance

## Algorithm Details

The plugin implements three different fluid distortion algorithms:

### Radial Swirl Mode
Classic polar coordinate transformation:
```
1. Convert pixel coordinates to polar (distance, angle) from swirl center
2. Apply swirl: new_angle = angle + intensity * exp(-distance / decay)
3. Convert back to cartesian coordinates
4. Sample source image with bilinear interpolation
```

### Directional Flow Mode
Unidirectional flow displacement:
```
1. Calculate perpendicular distance from flow line
2. Apply flow displacement: displacement = strength * exp(-perpDist / wakeWidth)
3. Offset pixel sampling in flow direction
4. Sample with bilinear interpolation
```

### Boat Wake Mode
von Kármán vortex street with alternating vortices:
```
1. Transform coordinates to flow-aligned system
2. Calculate vortex positions with alternating offsets
3. Apply vortex rotation: angle += intensity * exp(-vortexDist / decay)
4. Combine with directional flow component
5. Transform back and sample with interpolation
```

These algorithms create realistic fluid dynamics effects suitable for professional video production.

## Creative Applications

### Radial Swirl Mode
- **Underwater shots** - Simulate water refraction and whirlpools
- **Dream sequences** - Create surreal, fluid transitions
- **Abstract backgrounds** - Generate organic motion graphics
- **Psychedelic effects** - Intense swirls for music videos
- **Portal effects** - Dimensional gateways and vortices

### Directional Flow Mode
- **River currents** - Simulate flowing water movement
- **Wind effects** - Atmospheric distortion from strong winds
- **Heat shimmer** - Rising heat distortion effects
- **Fabric simulation** - Flowing cloth or silk movement
- **Smoke trails** - Directional smoke and vapor effects

### Boat Wake Mode
- **Marine footage** - Realistic boat wake distortion
- **Swimming scenes** - Water displacement from movement
- **Propeller wash** - Aircraft or boat propeller effects  
- **Object trails** - Fast-moving objects through fluid
- **Turbulence patterns** - von Kármán vortex streets in nature

## Performance Tips

- Use lower radius values for better performance
- GPU acceleration provides significant speedup
- Consider rendering effects for final output
- Animate parameters smoothly for best visual results

## Troubleshooting

### Plugin doesn't appear in DaVinci Resolve
- Ensure bundle is in correct OFX plugins folder
- Check file permissions (plugin should be executable)
- Restart DaVinci Resolve completely
- Check Resolve console for error messages

### Performance issues
- Lower the radius parameter
- Ensure GPU drivers are up to date
- Close other GPU-intensive applications

### Build errors
- Verify OpenFX SDK path in CMake configuration
- Check that all dependencies are installed
- Ensure compatible compiler version

## Development

### Project Structure
```
FluidSwirlPlugin/
├── src/
│   └── FluidSwirlPlugin.cpp    # Main plugin implementation
├── CMakeLists.txt              # Build configuration
├── Info.plist                  # Plugin metadata
├── build.bat                   # Windows build script
└── README.md                   # This file
```

### Contributing
Feel free to submit issues, feature requests, or pull requests. This project aims to provide a professional-quality fluid swirl effect for video professionals.

## License

This project is provided as-is for educational and professional use. OpenFX components remain under their respective licenses.

## Version History

- **v1.0.0** - Initial release with core swirl functionality
  - Real-time GPU processing
  - Full parameter control
  - Cross-platform support