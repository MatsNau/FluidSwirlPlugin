#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define kPluginName "FluidSwirl"
#define kPluginGrouping "Filter"
#define kPluginDescription "Creates fluid swirl distortion effects like video shot through water"
#define kPluginIdentifier "com.resolve.fluidswirl"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamSwirlIntensity "swirlIntensity"
#define kParamSwirlIntensityLabel "Swirl Intensity"
#define kParamSwirlIntensityHint "Controls the strength of the swirl effect"

#define kParamCenterX "centerX"
#define kParamCenterXLabel "Center X"
#define kParamCenterXHint "X coordinate of swirl center"

#define kParamCenterY "centerY"
#define kParamCenterYLabel "Center Y"
#define kParamCenterYHint "Y coordinate of swirl center"

#define kParamRadius "radius"
#define kParamRadiusLabel "Radius"
#define kParamRadiusHint "Radius of swirl effect"

#define kParamDecay "decay"
#define kParamDecayLabel "Decay"
#define kParamDecayHint "Rate of swirl decay from center"

#define kParamFlowDirection "flowDirection"
#define kParamFlowDirectionLabel "Flow Direction"
#define kParamFlowDirectionHint "Direction of fluid flow in degrees (0=right, 90=up)"

#define kParamFlowStrength "flowStrength"
#define kParamFlowStrengthLabel "Flow Strength"
#define kParamFlowStrengthHint "Strength of directional flow"

#define kParamWakeWidth "wakeWidth"
#define kParamWakeWidthLabel "Wake Width"
#define kParamWakeWidthHint "Width of the wake disturbance area"

#define kParamVortexSpacing "vortexSpacing"
#define kParamVortexSpacingLabel "Vortex Spacing"
#define kParamVortexSpacingHint "Distance between alternating vortices"

#define kParamFlowMode "flowMode"
#define kParamFlowModeLabel "Flow Mode"
#define kParamFlowModeHint "Type of flow pattern"

#define kParamProjectileStart "projectileStart"
#define kParamProjectileStartLabel "Projectile Start"
#define kParamProjectileStartHint "Starting position of the projectile"

#define kParamProjectileEnd "projectileEnd"
#define kParamProjectileEndLabel "Projectile End"
#define kParamProjectileEndHint "Ending position of the projectile"

#define kParamProjectileSpeed "projectileSpeed"
#define kParamProjectileSpeedLabel "Projectile Speed"
#define kParamProjectileSpeedHint "Speed of projectile movement (frames to cross screen)"

#define kParamProjectileRadius "projectileRadius"
#define kParamProjectileRadiusLabel "Impact Radius"
#define kParamProjectileRadiusHint "Radius of displacement around projectile"

#define kParamWakeDecay "wakeDecay"
#define kParamWakeDecayLabel "Wake Decay"
#define kParamWakeDecayHint "How quickly the wake trail fades behind projectile"

using namespace OFX;

// Forward declaration
class FluidSwirlProcessorBase;

class FluidSwirlPlugin : public OFX::ImageEffect
{
protected:
    OFX::Clip *_dstClip;
    OFX::Clip *_srcClip;
    
    OFX::DoubleParam *_swirlIntensity;
    OFX::Double2DParam *_center;
    OFX::DoubleParam *_radius;
    OFX::DoubleParam *_decay;
    OFX::DoubleParam *_flowDirection;
    OFX::DoubleParam *_flowStrength;
    OFX::DoubleParam *_wakeWidth;
    OFX::DoubleParam *_vortexSpacing;
    OFX::ChoiceParam *_flowMode;
    
    // Projectile parameters
    OFX::Double2DParam *_projectileStart;
    OFX::Double2DParam *_projectileEnd;
    OFX::DoubleParam *_projectileSpeed;
    OFX::DoubleParam *_projectileRadius;
    OFX::DoubleParam *_wakeDecay;

public:
    FluidSwirlPlugin(OfxImageEffectHandle handle) : ImageEffect(handle), _dstClip(0), _srcClip(0)
    {
        _dstClip = fetchClip(kOfxImageEffectOutputClipName);
        _srcClip = getContext() == OFX::eContextGenerator ? NULL : fetchClip(kOfxImageEffectSimpleSourceClipName);

        _swirlIntensity = fetchDoubleParam(kParamSwirlIntensity);
        _center = fetchDouble2DParam("center");
        _radius = fetchDoubleParam(kParamRadius);
        _decay = fetchDoubleParam(kParamDecay);
        _flowDirection = fetchDoubleParam(kParamFlowDirection);
        _flowStrength = fetchDoubleParam(kParamFlowStrength);
        _wakeWidth = fetchDoubleParam(kParamWakeWidth);
        _vortexSpacing = fetchDoubleParam(kParamVortexSpacing);
        _flowMode = fetchChoiceParam(kParamFlowMode);
        
        // Projectile parameters
        _projectileStart = fetchDouble2DParam(kParamProjectileStart);
        _projectileEnd = fetchDouble2DParam(kParamProjectileEnd);
        _projectileSpeed = fetchDoubleParam(kParamProjectileSpeed);
        _projectileRadius = fetchDoubleParam(kParamProjectileRadius);
        _wakeDecay = fetchDoubleParam(kParamWakeDecay);
        
        assert(_dstClip && _swirlIntensity && _center && _radius && _decay && 
               _flowDirection && _flowStrength && _wakeWidth && _vortexSpacing && _flowMode &&
               _projectileStart && _projectileEnd && _projectileSpeed && _projectileRadius && _wakeDecay);
    }

private:
    template <class PIX, int nComponents, int maxValue>
    void renderInternal(const OFX::RenderArguments &args,
                       OFX::BitDepthEnum bitDepth);

    void setupAndProcess(FluidSwirlProcessorBase &processor,
                        const OFX::RenderArguments &args);

public:
    virtual void render(const OFX::RenderArguments &args);
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);
    virtual void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences);
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);
};

class FluidSwirlProcessorBase
{
protected:
    double _swirlIntensity;
    double _centerX, _centerY;
    double _radius;
    double _decay;
    double _flowDirection;
    double _flowStrength;
    double _wakeWidth;
    double _vortexSpacing;
    int _flowMode;
    
    // Projectile parameters
    double _projectileStartX, _projectileStartY;
    double _projectileEndX, _projectileEndY;
    double _projectileSpeed;
    double _projectileRadius;
    double _wakeDecayParam;
    double _currentTime;
    
    OFX::ImageEffect &_effect;
    const OFX::Image *_srcImg;
    OFX::Image *_dstImg;
    OfxRectI _renderWindow;

public:
    FluidSwirlProcessorBase(OFX::ImageEffect &instance) : _effect(instance), _srcImg(0), _dstImg(0) {}
    
    void setDstImg(OFX::Image *v) { _dstImg = v; }
    void setSrcImg(const OFX::Image *v) { _srcImg = v; }
    void setRenderWindow(OfxRectI rect) { _renderWindow = rect; }
    void process() { multiThreadProcessImages(_renderWindow); }
    
    void setSwirlParams(double intensity, double centerX, double centerY, double radius, double decay,
                       double flowDirection, double flowStrength, double wakeWidth, double vortexSpacing, int flowMode,
                       double projStartX, double projStartY, double projEndX, double projEndY, 
                       double projSpeed, double projRadius, double wakeDecay, double currentTime)
    {
        _swirlIntensity = intensity;
        _centerX = centerX;
        _centerY = centerY;
        _radius = radius;
        _decay = decay;
        _flowDirection = flowDirection;
        _flowStrength = flowStrength;
        _wakeWidth = wakeWidth;
        _vortexSpacing = vortexSpacing;
        _flowMode = flowMode;
        
        // Projectile parameters
        _projectileStartX = projStartX;
        _projectileStartY = projStartY;
        _projectileEndX = projEndX;
        _projectileEndY = projEndY;
        _projectileSpeed = projSpeed;
        _projectileRadius = projRadius;
        _wakeDecayParam = wakeDecay;
        _currentTime = currentTime;
    }
    
protected:
    virtual void multiThreadProcessImages(OfxRectI procWindow) = 0;
    
    void* getDstPixelAddress(int x, int y) {
        return _dstImg->getPixelAddress(x, y);
    }
    
    const void* getSrcPixelAddress(int x, int y) {
        return _srcImg->getPixelAddress(x, y);
    }
};

template <class PIX, int nComponents, int maxValue>
class FluidSwirlProcessor : public FluidSwirlProcessorBase
{
public:
    FluidSwirlProcessor(OFX::ImageEffect &instance) : FluidSwirlProcessorBase(instance) {}

private:
    void multiThreadProcessImages(OfxRectI procWindow)
    {
        const int width = procWindow.x2 - procWindow.x1;
        const int height = procWindow.y2 - procWindow.y1;
        
        // Convert flow direction to radians
        const double flowDirRad = _flowDirection * M_PI / 180.0;
        const double flowCos = cos(flowDirRad);
        const double flowSin = sin(flowDirRad);
        
        for (int y = procWindow.y1; y < procWindow.y2; y++) {
            if (_effect.abort()) break;
            
            PIX *dstPix = (PIX *) getDstPixelAddress(procWindow.x1, y);
            
            for (int x = procWindow.x1; x < procWindow.x2; x++) {
                double srcX = x;
                double srcY = y;
                
                // Check if effect is strong enough to apply
                bool applyEffect = (fabs(_swirlIntensity) > 0.001 || fabs(_flowStrength) > 0.001);
                
                if (applyEffect && _flowMode == 0) {
                    // Original radial swirl
                    double dx = x - _centerX;
                    double dy = y - _centerY;
                    double distance = sqrt(dx * dx + dy * dy);
                    
                    double angle = atan2(dy, dx);
                    double swirlAngle = 0.0;
                    if (_decay > 0.001) {
                        swirlAngle = _swirlIntensity * exp(-distance / _decay);
                    }
                    angle += swirlAngle;
                    
                    srcX = _centerX + distance * cos(angle);
                    srcY = _centerY + distance * sin(angle);
                    
                } else if (applyEffect && _flowMode == 1) {
                    // Directional flow
                    double dx = x - _centerX;
                    double dy = y - _centerY;
                    
                    // Distance from flow line (perpendicular distance)
                    double perpDist = fabs(dx * flowSin - dy * flowCos);
                    double flowEffect = 0.0;
                    if (_wakeWidth > 0.001) {
                        flowEffect = _flowStrength * exp(-perpDist / _wakeWidth);
                    }
                    
                    // Apply flow displacement
                    srcX = x - flowEffect * flowCos;
                    srcY = y - flowEffect * flowSin;
                    
                } else if (applyEffect && _flowMode == 2) {
                    // Projectile Wake Effect - like a bullet flying through fluid with expanding waves
                    
                    // Calculate projectile position based on time
                    double progress = (_currentTime / _projectileSpeed);
                    double projectileX = _projectileStartX + progress * (_projectileEndX - _projectileStartX);
                    double projectileY = _projectileStartY + progress * (_projectileEndY - _projectileStartY);
                    
                    // Add expanding wave distortion from start point
                    double distFromStart = sqrt((x - _projectileStartX) * (x - _projectileStartX) + 
                                              (y - _projectileStartY) * (y - _projectileStartY));
                    
                    double waveRadius = progress * _projectileRadius * 4.0;
                    double maxWaveRadius = _projectileRadius * 8.0;
                    waveRadius = std::min(waveRadius, maxWaveRadius);
                    
                    // Apply expanding wave distortion
                    if (distFromStart < waveRadius && waveRadius > 1.0 && distFromStart > 0.1) {
                        double waveDirection = atan2(y - _projectileStartY, x - _projectileStartX);
                        double waveStrength = _swirlIntensity * 15.0; // Wave displacement strength
                        
                        // Wave front effect - stronger at the edges
                        double distanceRatio = distFromStart / waveRadius;
                        double waveFrontEffect = sin(distanceRatio * M_PI) * 2.0; // Peak at middle of wave
                        
                        // Time decay
                        double timeDecay = exp(-progress / (_wakeDecayParam * 2.0));
                        
                        double totalWaveDisplacement = waveStrength * waveFrontEffect * timeDecay;
                        
                        // Apply radial displacement (outward from start point)
                        srcX += cos(waveDirection) * totalWaveDisplacement;
                        srcY += sin(waveDirection) * totalWaveDisplacement;
                        
                        // Add some rotational component for more fluid-like motion
                        double rotationalComponent = totalWaveDisplacement * 0.3;
                        srcX += -sin(waveDirection) * rotationalComponent * sin(distFromStart * 0.1);
                        srcY += cos(waveDirection) * rotationalComponent * sin(distFromStart * 0.1);
                    }
                    
                    // Distance from current projectile position
                    double dx = x - projectileX;
                    double dy = y - projectileY;
                    double distanceFromProjectile = sqrt(dx * dx + dy * dy);
                    
                    // Calculate displacement field around current projectile position
                    if (distanceFromProjectile < _projectileRadius && distanceFromProjectile > 0.1) {
                        // Strong displacement field - pull pixels toward projectile trajectory
                        double projDirX = _projectileEndX - _projectileStartX;
                        double projDirY = _projectileEndY - _projectileStartY;
                        double projDirLength = sqrt(projDirX * projDirX + projDirY * projDirY);
                        if (projDirLength > 0.001) {
                            projDirX /= projDirLength;
                            projDirY /= projDirLength;
                        }
                        
                        // Calculate displacement strength (stronger closer to projectile)
                        double falloff = exp(-distanceFromProjectile / (_projectileRadius * 0.2));
                        double displacement = _swirlIntensity * 80.0 * falloff; // Stronger base displacement
                        
                        // Pull pixels in projectile direction with some perpendicular swirl
                        double perpX = -projDirY; // Perpendicular to projectile direction
                        double perpY = projDirX;
                        
                        // Add rotational component based on distance from trajectory line
                        double perpDist = fabs(dx * perpX + dy * perpY);
                        double swirlAmount = displacement * 0.5 * sin(perpDist * 0.1);
                        
                        srcX = x + projDirX * displacement + perpX * swirlAmount;
                        srcY = y + projDirY * displacement + perpY * swirlAmount;
                    }
                    
                    // Add wake trail effect - disturbance behind projectile
                    double wakeStartX = _projectileStartX;
                    double wakeStartY = _projectileStartY;
                    double wakeEndX = projectileX;
                    double wakeEndY = projectileY;
                    
                    // Distance to wake trail line
                    double wakeLength = sqrt((wakeEndX - wakeStartX) * (wakeEndX - wakeStartX) + 
                                           (wakeEndY - wakeStartY) * (wakeEndY - wakeStartY));
                    if (wakeLength > 0.001) {
                        double wakeDirX = (wakeEndX - wakeStartX) / wakeLength;
                        double wakeDirY = (wakeEndY - wakeStartY) / wakeLength;
                        
                        // Project point onto wake line
                        double projOntoWake = (x - wakeStartX) * wakeDirX + (y - wakeStartY) * wakeDirY;
                        
                        if (projOntoWake > 0 && projOntoWake < wakeLength) {
                            double closestX = wakeStartX + projOntoWake * wakeDirX;
                            double closestY = wakeStartY + projOntoWake * wakeDirY;
                            
                            double distToWake = sqrt((x - closestX) * (x - closestX) + (y - closestY) * (y - closestY));
                            
                            if (distToWake < _wakeWidth) {
                                // Wake trail effect - fluid diffusion and streaking
                                double wakeStrength = _flowStrength * exp(-distToWake / (_wakeWidth * 0.3));
                                double ageOfWake = 1.0 - (projOntoWake / wakeLength); // Newer wake is stronger
                                wakeStrength *= exp(-ageOfWake / _wakeDecayParam);
                                
                                // Strong longitudinal streaking (along projectile direction)
                                double streakDistance = wakeStrength * 20.0; // Much stronger streaking
                                srcX += wakeDirX * streakDistance * (1.0 + sin(distToWake * 0.1) * 0.3);
                                srcY += wakeDirY * streakDistance * (1.0 + cos(distToWake * 0.1) * 0.3);
                                
                                // Add perpendicular diffusion (spreading outward)
                                double perpX = -wakeDirY;
                                double perpY = wakeDirX;
                                double diffusion = wakeStrength * 5.0 * sin(projOntoWake * 0.05 + distToWake * 0.2);
                                srcX += perpX * diffusion;
                                srcY += perpY * diffusion;
                                
                                // Add some turbulent mixing
                                double turbulence = wakeStrength * 8.0;
                                srcX += sin(distToWake * 0.4 + projOntoWake * 0.08) * turbulence;
                                srcY += cos(distToWake * 0.35 + projOntoWake * 0.12) * turbulence;
                            }
                        }
                    }
                }
                
                // Determine if we're in a wake-affected area for special sampling
                bool inWakeArea = false;
                double wakeBlurAmount = 0.0;
                
                // Check if we're in the wake trail for fluid diffusion sampling
                if (applyEffect && _flowMode == 2) {
                    double progress = (_currentTime / _projectileSpeed);
                    double projectileX = _projectileStartX + progress * (_projectileEndX - _projectileStartX);
                    double projectileY = _projectileStartY + progress * (_projectileEndY - _projectileStartY);
                    
                    // Check for expanding wave diffusion from start point
                    double distFromStart = sqrt((x - _projectileStartX) * (x - _projectileStartX) + 
                                              (y - _projectileStartY) * (y - _projectileStartY));
                    
                    // Wave expansion: starts small and grows over time
                    double waveRadius = progress * _projectileRadius * 4.0; // Wave expands 4x projectile radius
                    double maxWaveRadius = _projectileRadius * 8.0; // Maximum expansion
                    waveRadius = std::min(waveRadius, maxWaveRadius);
                    
                    // Check if we're in the expanding wave field
                    if (distFromStart < waveRadius && waveRadius > 1.0) {
                        double waveStrength = _flowStrength * 0.5; // Base wave strength
                        
                        // Create ripple effect - stronger at wave fronts
                        double ripplePhase = (distFromStart / waveRadius) * 2.0 * M_PI;
                        double rippleEffect = (sin(ripplePhase * 3.0) + 1.0) * 0.5; // 0 to 1
                        
                        // Distance-based falloff
                        double waveFalloff = 1.0 - (distFromStart / waveRadius);
                        waveFalloff = waveFalloff * waveFalloff; // Quadratic falloff
                        
                        // Time-based decay
                        double timeDecay = exp(-progress / _wakeDecayParam);
                        
                        double totalWaveStrength = waveStrength * rippleEffect * waveFalloff * timeDecay;
                        
                        if (totalWaveStrength > wakeBlurAmount) {
                            inWakeArea = true;
                            wakeBlurAmount = totalWaveStrength;
                        }
                    }
                    
                    // Original wake trail (but with expanding width)
                    double wakeStartX = _projectileStartX;
                    double wakeStartY = _projectileStartY;
                    double wakeEndX = projectileX;
                    double wakeEndY = projectileY;
                    
                    double wakeLength = sqrt((wakeEndX - wakeStartX) * (wakeEndX - wakeStartX) + 
                                           (wakeEndY - wakeStartY) * (wakeEndY - wakeStartY));
                    
                    if (wakeLength > 0.001) {
                        double wakeDirX = (wakeEndX - wakeStartX) / wakeLength;
                        double wakeDirY = (wakeEndY - wakeStartY) / wakeLength;
                        double projOntoWake = (x - wakeStartX) * wakeDirX + (y - wakeStartY) * wakeDirY;
                        
                        if (projOntoWake > 0 && projOntoWake < wakeLength) {
                            double closestX = wakeStartX + projOntoWake * wakeDirX;
                            double closestY = wakeStartY + projOntoWake * wakeDirY;
                            double distToWake = sqrt((x - closestX) * (x - closestX) + (y - closestY) * (y - closestY));
                            
                            // Wake width expands over time/distance
                            double dynamicWakeWidth = _wakeWidth * (1.0 + progress * 2.0); // Expands 3x over time
                            
                            if (distToWake < dynamicWakeWidth) {
                                double trailBlurAmount = _flowStrength * exp(-distToWake / (dynamicWakeWidth * 0.4));
                                double ageOfWake = 1.0 - (projOntoWake / wakeLength);
                                trailBlurAmount *= exp(-ageOfWake / _wakeDecayParam);
                                
                                if (trailBlurAmount > wakeBlurAmount) {
                                    inWakeArea = true;
                                    wakeBlurAmount = trailBlurAmount;
                                }
                            }
                        }
                    }
                    
                    // Add concentric ripples around current projectile position
                    double distFromProjectile = sqrt((x - projectileX) * (x - projectileX) + 
                                                    (y - projectileY) * (y - projectileY));
                    
                    if (distFromProjectile < _projectileRadius * 2.0) {
                        double ripplePhase = (distFromProjectile / _projectileRadius) * M_PI;
                        double rippleStrength = _flowStrength * 0.3 * sin(ripplePhase);
                        
                        if (rippleStrength > 0 && rippleStrength > wakeBlurAmount * 0.5) {
                            inWakeArea = true;
                            wakeBlurAmount = std::max(wakeBlurAmount, rippleStrength);
                        }
                    }
                }
                
                // Sample with bilinear interpolation or fluid diffusion
                int srcXInt = (int)floor(srcX);
                int srcYInt = (int)floor(srcY);
                
                // Get source image bounds
                OfxRectI srcBounds = _srcImg->getBounds();
                
                // Check if we can do bilinear interpolation (need all 4 pixels)
                if (srcXInt >= srcBounds.x1 && srcXInt < srcBounds.x2-1 && 
                    srcYInt >= srcBounds.y1 && srcYInt < srcBounds.y2-1) {
                    
                    double fx = srcX - srcXInt;
                    double fy = srcY - srcYInt;
                    double fx1 = 1.0 - fx;
                    double fy1 = 1.0 - fy;
                    
                    // Get four surrounding pixels
                    PIX *p00 = (PIX *) getSrcPixelAddress(srcXInt, srcYInt);
                    PIX *p10 = (PIX *) getSrcPixelAddress(srcXInt + 1, srcYInt);
                    PIX *p01 = (PIX *) getSrcPixelAddress(srcXInt, srcYInt + 1);
                    PIX *p11 = (PIX *) getSrcPixelAddress(srcXInt + 1, srcYInt + 1);
                    
                    // Use fluid diffusion sampling in wake areas
                    if (inWakeArea && wakeBlurAmount > 0.01) {
                        // Multi-sample for fluid diffusion effect
                        double totalWeight = 0.0;
                        double sampledColor[4] = {0.0, 0.0, 0.0, 0.0}; // Max 4 components
                        
                        // Sample multiple points around the source location for blur/diffusion
                        int numSamples = 5; // Number of samples for diffusion
                        double blurRadius = wakeBlurAmount * 3.0;
                        
                        for (int s = 0; s < numSamples; s++) {
                            double angle = (s * 2.0 * M_PI) / numSamples;
                            double sampleX = srcX + cos(angle) * blurRadius * ((double)s / numSamples);
                            double sampleY = srcY + sin(angle) * blurRadius * ((double)s / numSamples);
                            
                            int sampleXInt = (int)floor(sampleX);
                            int sampleYInt = (int)floor(sampleY);
                            
                            if (sampleXInt >= srcBounds.x1 && sampleXInt < srcBounds.x2-1 && 
                                sampleYInt >= srcBounds.y1 && sampleYInt < srcBounds.y2-1) {
                                
                                double sfx = sampleX - sampleXInt;
                                double sfy = sampleY - sampleYInt;
                                double sfx1 = 1.0 - sfx;
                                double sfy1 = 1.0 - sfy;
                                
                                PIX *sp00 = (PIX *) getSrcPixelAddress(sampleXInt, sampleYInt);
                                PIX *sp10 = (PIX *) getSrcPixelAddress(sampleXInt + 1, sampleYInt);
                                PIX *sp01 = (PIX *) getSrcPixelAddress(sampleXInt, sampleYInt + 1);
                                PIX *sp11 = (PIX *) getSrcPixelAddress(sampleXInt + 1, sampleYInt + 1);
                                
                                double weight = 1.0; // Equal weight for now
                                totalWeight += weight;
                                
                                for (int c = 0; c < nComponents; c++) {
                                    double sampleValue = sp00[c] * sfx1 * sfy1 +
                                                       sp10[c] * sfx * sfy1 +
                                                       sp01[c] * sfx1 * sfy +
                                                       sp11[c] * sfx * sfy;
                                    sampledColor[c] += sampleValue * weight;
                                }
                            }
                        }
                        
                        // Normalize and apply
                        if (totalWeight > 0.001) {
                            for (int c = 0; c < nComponents; c++) {
                                dstPix[c] = (PIX)(sampledColor[c] / totalWeight);
                            }
                        } else {
                            // Fallback to regular bilinear
                            for (int c = 0; c < nComponents; c++) {
                                double interpolated = p00[c] * fx1 * fy1 +
                                                    p10[c] * fx * fy1 +
                                                    p01[c] * fx1 * fy +
                                                    p11[c] * fx * fy;
                                dstPix[c] = (PIX)interpolated;
                            }
                        }
                    } else {
                        // Regular bilinear interpolation
                        for (int c = 0; c < nComponents; c++) {
                            double interpolated = p00[c] * fx1 * fy1 +
                                                p10[c] * fx * fy1 +
                                                p01[c] * fx1 * fy +
                                                p11[c] * fx * fy;
                            dstPix[c] = (PIX)interpolated;
                        }
                    }
                } else if (srcXInt >= srcBounds.x1 && srcXInt < srcBounds.x2 && 
                          srcYInt >= srcBounds.y1 && srcYInt < srcBounds.y2) {
                    // Nearest neighbor for edge pixels
                    PIX *srcPix = (PIX *) getSrcPixelAddress(srcXInt, srcYInt);
                    for (int c = 0; c < nComponents; c++) {
                        dstPix[c] = srcPix[c];
                    }
                } else {
                    // For completely out-of-bounds pixels, use transparent black or edge clamping
                    if (x >= srcBounds.x1 && x < srcBounds.x2 && y >= srcBounds.y1 && y < srcBounds.y2) {
                        // If original position is valid, use it
                        PIX *srcPix = (PIX *) getSrcPixelAddress(x, y);
                        for (int c = 0; c < nComponents; c++) {
                            dstPix[c] = srcPix[c];
                        }
                    } else {
                        // Clamp to nearest edge pixel
                        int clampX = std::max(srcBounds.x1, std::min(srcBounds.x2-1, srcXInt));
                        int clampY = std::max(srcBounds.y1, std::min(srcBounds.y2-1, srcYInt));
                        PIX *srcPix = (PIX *) getSrcPixelAddress(clampX, clampY);
                        for (int c = 0; c < nComponents; c++) {
                            dstPix[c] = srcPix[c];
                        }
                    }
                }
                
                dstPix += nComponents;
            }
        }
    }
};

void FluidSwirlPlugin::render(const OFX::RenderArguments &args)
{
    OFX::BitDepthEnum srcBitDepth = _srcClip->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = _srcClip->getPixelComponents();

    assert(srcComponents == OFX::ePixelComponentRGBA || 
           srcComponents == OFX::ePixelComponentRGB ||
           srcComponents == OFX::ePixelComponentAlpha);

    // Handle RGBA formats
    if (srcBitDepth == OFX::eBitDepthUByte && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<unsigned char, 4, 255>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthUShort && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<unsigned short, 4, 65535>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthFloat && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<float, 4, 1>(args, srcBitDepth);
    }
    // Handle RGB formats
    else if (srcBitDepth == OFX::eBitDepthUByte && srcComponents == OFX::ePixelComponentRGB) {
        renderInternal<unsigned char, 3, 255>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthUShort && srcComponents == OFX::ePixelComponentRGB) {
        renderInternal<unsigned short, 3, 65535>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthFloat && srcComponents == OFX::ePixelComponentRGB) {
        renderInternal<float, 3, 1>(args, srcBitDepth);
    }
    // Handle Alpha formats
    else if (srcBitDepth == OFX::eBitDepthUByte && srcComponents == OFX::ePixelComponentAlpha) {
        renderInternal<unsigned char, 1, 255>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthUShort && srcComponents == OFX::ePixelComponentAlpha) {
        renderInternal<unsigned short, 1, 65535>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthFloat && srcComponents == OFX::ePixelComponentAlpha) {
        renderInternal<float, 1, 1>(args, srcBitDepth);
    } else {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

template <class PIX, int nComponents, int maxValue>
void FluidSwirlPlugin::renderInternal(const OFX::RenderArguments &args,
                                     OFX::BitDepthEnum bitDepth)
{
    FluidSwirlProcessor<PIX, nComponents, maxValue> processor(*this);
    setupAndProcess(processor, args);
}

void FluidSwirlPlugin::setupAndProcess(FluidSwirlProcessorBase &processor,
                                      const OFX::RenderArguments &args)
{
    // Check if clips are connected
    if (!_srcClip || !_srcClip->isConnected()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
    
    std::auto_ptr<OFX::Image> dst(_dstClip->fetchImage(args.time));
    std::auto_ptr<OFX::Image> src(_srcClip->fetchImage(args.time));

    if (!dst.get() || !src.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
    
    // Verify image bounds match
    OfxRectI dstBounds = dst->getBounds();
    OfxRectI srcBounds = src->getBounds();
    if (dstBounds.x1 != srcBounds.x1 || dstBounds.y1 != srcBounds.y1 || 
        dstBounds.x2 != srcBounds.x2 || dstBounds.y2 != srcBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Get parameter values
    double swirlIntensity = _swirlIntensity->getValueAtTime(args.time);
    double centerX, centerY;
    _center->getValueAtTime(args.time, centerX, centerY);
    
    // Convert normalized coordinates to pixel coordinates
    int imageWidth = srcBounds.x2 - srcBounds.x1;
    int imageHeight = srcBounds.y2 - srcBounds.y1;
    centerX = srcBounds.x1 + centerX * imageWidth;
    centerY = srcBounds.y1 + centerY * imageHeight;
    
    double radius = _radius->getValueAtTime(args.time);
    double decay = _decay->getValueAtTime(args.time);
    double flowDirection = _flowDirection->getValueAtTime(args.time);
    double flowStrength = _flowStrength->getValueAtTime(args.time);
    double wakeWidth = _wakeWidth->getValueAtTime(args.time);
    double vortexSpacing = _vortexSpacing->getValueAtTime(args.time);
    int flowMode = _flowMode->getValueAtTime(args.time);
    
    // Get projectile parameters
    double projectileStartX, projectileStartY;
    _projectileStart->getValueAtTime(args.time, projectileStartX, projectileStartY);
    double projectileEndX, projectileEndY;
    _projectileEnd->getValueAtTime(args.time, projectileEndX, projectileEndY);
    double projectileSpeed = _projectileSpeed->getValueAtTime(args.time);
    double projectileRadius = _projectileRadius->getValueAtTime(args.time);
    double wakeDecay = _wakeDecay->getValueAtTime(args.time);
    
    // Convert normalized projectile coordinates to pixel coordinates
    projectileStartX = srcBounds.x1 + projectileStartX * imageWidth;
    projectileStartY = srcBounds.y1 + projectileStartY * imageHeight;
    projectileEndX = srcBounds.x1 + projectileEndX * imageWidth;
    projectileEndY = srcBounds.y1 + projectileEndY * imageHeight;
    
    // Scale parameters to image size (assuming 1920x1080 reference)
    double scale = sqrt((double)imageWidth * imageWidth + (double)imageHeight * imageHeight) / sqrt(1920.0 * 1920.0 + 1080.0 * 1080.0);
    radius *= scale;
    decay *= scale;
    wakeWidth *= scale;
    vortexSpacing *= scale;
    projectileRadius *= scale;

    processor.setDstImg(dst.get());
    processor.setSrcImg(src.get());
    processor.setRenderWindow(args.renderWindow);
    processor.setSwirlParams(swirlIntensity, centerX, centerY, radius, decay,
                           flowDirection, flowStrength, wakeWidth, vortexSpacing, flowMode,
                           projectileStartX, projectileStartY, projectileEndX, projectileEndY,
                           projectileSpeed, projectileRadius, wakeDecay, args.time);
    
    processor.process();
}

bool FluidSwirlPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
    double swirlIntensity = _swirlIntensity->getValueAtTime(args.time);
    double flowStrength = _flowStrength->getValueAtTime(args.time);
    int flowMode = _flowMode->getValueAtTime(args.time);
    
    // Check if effect is essentially disabled
    bool isDisabled = false;
    
    if (flowMode == 0) {
        // Radial swirl mode - check swirl intensity
        isDisabled = (fabs(swirlIntensity) < 0.001);
    } else {
        // Directional flow or boat wake modes - check both parameters
        isDisabled = (fabs(swirlIntensity) < 0.001 && fabs(flowStrength) < 0.001);
    }
    
    if (isDisabled) {
        identityClip = _srcClip;
        identityTime = args.time;
        return true;
    }
    return false;
}

void FluidSwirlPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    // Handle parameter changes if needed
}

void FluidSwirlPlugin::getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences)
{
    // Set clip preferences if needed
}

bool FluidSwirlPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    // Use source region of definition
    if (_srcClip && _srcClip->isConnected()) {
        rod = _srcClip->getRegionOfDefinition(args.time);
        return true;
    }
    return false;
}

class FluidSwirlPluginFactory : public OFX::PluginFactoryHelper<FluidSwirlPluginFactory>
{
public:
    FluidSwirlPluginFactory() : OFX::PluginFactoryHelper<FluidSwirlPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor) {}
    
    virtual void describe(OFX::ImageEffectDescriptor &desc);
    virtual void describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context);
};

void FluidSwirlPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedBitDepth(OFX::eBitDepthUByte);
    desc.addSupportedBitDepth(OFX::eBitDepthUShort);
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    desc.setSupportsMultipleClipDepths(false);
    desc.setRenderThreadSafety(OFX::eRenderFullySafe);
}

void FluidSwirlPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
{
    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);
    srcClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);
    srcClip->setIsMask(false);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGB);
    dstClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    dstClip->setSupportsTiles(true);

    OFX::PageParamDescriptor *page = desc.definePageParam("Controls");

    // Swirl Intensity
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSwirlIntensity);
    param->setLabel(kParamSwirlIntensityLabel);
    param->setHint(kParamSwirlIntensityHint);
    param->setDefault(1.0);
    param->setRange(-10.0, 10.0);
    param->setDisplayRange(-5.0, 5.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Center Point
    OFX::Double2DParamDescriptor *centerParam = desc.defineDouble2DParam("center");
    centerParam->setLabel("Center");
    centerParam->setHint("Center point of the swirl effect");
    centerParam->setDefault(0.5, 0.5);
    centerParam->setDoubleType(OFX::eDoubleTypeNormalisedXYAbsolute);
    if (page) {
        page->addChild(*centerParam);
    }

    // Radius
    param = desc.defineDoubleParam(kParamRadius);
    param->setLabel(kParamRadiusLabel);
    param->setHint(kParamRadiusHint);
    param->setDefault(200.0);
    param->setRange(1.0, 1000.0);
    param->setDisplayRange(10.0, 500.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Decay
    param = desc.defineDoubleParam(kParamDecay);
    param->setLabel(kParamDecayLabel);
    param->setHint(kParamDecayHint);
    param->setDefault(100.0);
    param->setRange(1.0, 500.0);
    param->setDisplayRange(10.0, 200.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Flow Mode
    OFX::ChoiceParamDescriptor *choiceParam = desc.defineChoiceParam(kParamFlowMode);
    choiceParam->setLabel(kParamFlowModeLabel);
    choiceParam->setHint(kParamFlowModeHint);
    choiceParam->appendOption("Radial Swirl", "Classic radial swirl from center point");
    choiceParam->appendOption("Directional Flow", "Unidirectional fluid flow");
    choiceParam->appendOption("Projectile Wake", "Bullet-like projectile flying through fluid with wake trail");
    choiceParam->setDefault(0);
    if (page) {
        page->addChild(*choiceParam);
    }

    // Flow Direction
    param = desc.defineDoubleParam(kParamFlowDirection);
    param->setLabel(kParamFlowDirectionLabel);
    param->setHint(kParamFlowDirectionHint);
    param->setDefault(0.0);
    param->setRange(-360.0, 360.0);
    param->setDisplayRange(-180.0, 180.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Flow Strength
    param = desc.defineDoubleParam(kParamFlowStrength);
    param->setLabel(kParamFlowStrengthLabel);
    param->setHint(kParamFlowStrengthHint);
    param->setDefault(1.0);
    param->setRange(0.0, 10.0);
    param->setDisplayRange(0.0, 5.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Wake Width
    param = desc.defineDoubleParam(kParamWakeWidth);
    param->setLabel(kParamWakeWidthLabel);
    param->setHint(kParamWakeWidthHint);
    param->setDefault(50.0);
    param->setRange(5.0, 200.0);
    param->setDisplayRange(10.0, 100.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Vortex Spacing
    param = desc.defineDoubleParam(kParamVortexSpacing);
    param->setLabel(kParamVortexSpacingLabel);
    param->setHint(kParamVortexSpacingHint);
    param->setDefault(80.0);
    param->setRange(10.0, 300.0);
    param->setDisplayRange(20.0, 150.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Projectile Start Position
    OFX::Double2DParamDescriptor *projectileStartParam = desc.defineDouble2DParam(kParamProjectileStart);
    projectileStartParam->setLabel(kParamProjectileStartLabel);
    projectileStartParam->setHint(kParamProjectileStartHint);
    projectileStartParam->setDefault(0.1, 0.5);
    projectileStartParam->setDoubleType(OFX::eDoubleTypeNormalisedXYAbsolute);
    if (page) {
        page->addChild(*projectileStartParam);
    }

    // Projectile End Position
    OFX::Double2DParamDescriptor *projectileEndParam = desc.defineDouble2DParam(kParamProjectileEnd);
    projectileEndParam->setLabel(kParamProjectileEndLabel);
    projectileEndParam->setHint(kParamProjectileEndHint);
    projectileEndParam->setDefault(0.9, 0.5);
    projectileEndParam->setDoubleType(OFX::eDoubleTypeNormalisedXYAbsolute);
    if (page) {
        page->addChild(*projectileEndParam);
    }

    // Projectile Speed
    param = desc.defineDoubleParam(kParamProjectileSpeed);
    param->setLabel(kParamProjectileSpeedLabel);
    param->setHint(kParamProjectileSpeedHint);
    param->setDefault(30.0);
    param->setRange(5.0, 200.0);
    param->setDisplayRange(10.0, 100.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Projectile Impact Radius
    param = desc.defineDoubleParam(kParamProjectileRadius);
    param->setLabel(kParamProjectileRadiusLabel);
    param->setHint(kParamProjectileRadiusHint);
    param->setDefault(80.0);
    param->setRange(10.0, 300.0);
    param->setDisplayRange(20.0, 150.0);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }

    // Wake Decay
    param = desc.defineDoubleParam(kParamWakeDecay);
    param->setLabel(kParamWakeDecayLabel);
    param->setHint(kParamWakeDecayHint);
    param->setDefault(0.5);
    param->setRange(0.1, 2.0);
    param->setDisplayRange(0.2, 1.5);
    param->setDoubleType(OFX::eDoubleTypePlain);
    if (page) {
        page->addChild(*param);
    }
}

OFX::ImageEffect* FluidSwirlPluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
    return new FluidSwirlPlugin(handle);
}

static FluidSwirlPluginFactory p;
mRegisterPluginFactoryInstance(p)