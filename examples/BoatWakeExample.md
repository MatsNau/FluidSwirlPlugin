# Boat Wake Effect Example

This example demonstrates how to create a realistic boat wake effect using the Fluid Swirl plugin's Boat Wake mode.

## Setup

1. **Add the Fluid Swirl effect** to your boat footage in DaVinci Resolve
2. **Set Flow Mode** to "Boat Wake"

## Parameter Settings for Realistic Boat Wake

### Basic Wake Parameters
- **Flow Direction**: 0° (boat moving right) or adjust to match boat direction
- **Center**: Position at the boat's stern (back of the boat)
- **Flow Strength**: 2.0-4.0 for noticeable wake displacement
- **Wake Width**: 40-80 pixels depending on boat size and video resolution

### Vortex Configuration  
- **Swirl Intensity**: 1.5-3.0 for realistic vortex rotation
- **Vortex Spacing**: 60-100 pixels for natural wake pattern
- **Decay**: 80-120 for realistic fade-out distance

## Animation Keyframes

### Following a Moving Boat
1. **Track the Center parameter** to follow the boat's stern position
2. **Animate Flow Direction** if the boat changes course
3. **Vary Flow Strength** based on boat speed (faster = stronger wake)

### Dynamic Wake Intensity
```
Frame 1:   Flow Strength = 0.0    (boat starting)
Frame 30:  Flow Strength = 3.0    (boat at cruising speed)  
Frame 60:  Flow Strength = 5.0    (boat accelerating)
Frame 90:  Flow Strength = 2.0    (boat slowing down)
```

### Realistic Vortex Variation
```
Frame 1:   Vortex Spacing = 100   (slow, regular pattern)
Frame 30:  Vortex Spacing = 70    (faster, tighter vortices)
Frame 60:  Vortex Spacing = 50    (high speed, rapid vortices)
```

## Advanced Techniques

### Multiple Wake Layers
Create multiple instances for complex wakes:
1. **Primary wake** - Strong flow displacement with tight vortices
2. **Secondary wake** - Wider, gentler disturbance with larger spacing
3. **Outer wash** - Very wide, subtle directional flow

### Wake Interaction with Shoreline
- Reduce **Flow Strength** as wake approaches shallow water
- Increase **Decay** parameter for faster fade-out near shore
- Use **Directional Flow mode** for reflected wake patterns

### Weather Conditions
**Calm conditions:**
- Flow Strength: 1.5-2.5
- Vortex Spacing: 80-120
- Decay: 100-150

**Rough seas:**
- Flow Strength: 3.0-5.0  
- Vortex Spacing: 40-70
- Decay: 60-90

## Tips for Best Results

1. **Match the boat's motion** - Wake should follow the boat's actual path
2. **Scale with distance** - Reduce intensity for distant boats
3. **Consider perspective** - Adjust wake width based on camera angle
4. **Blend with original** - Use lower opacity for subtle, realistic effects
5. **Combine with other effects** - Add foam, spray, or color correction

## Common Mistakes to Avoid

- Don't make the wake too strong - subtle is more realistic
- Don't use perfectly regular vortex spacing - vary it slightly
- Don't forget to animate the center point with the boat
- Don't apply the same settings to all boat sizes - scale appropriately

This example produces professional-quality boat wake effects suitable for film, television, and documentary production.