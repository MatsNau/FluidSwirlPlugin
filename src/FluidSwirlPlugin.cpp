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
        
        assert(_dstClip && _swirlIntensity && _center && _radius && _decay && 
               _flowDirection && _flowStrength && _wakeWidth && _vortexSpacing && _flowMode);
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
                       double flowDirection, double flowStrength, double wakeWidth, double vortexSpacing, int flowMode)
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
                    double swirlAngle = _swirlIntensity * exp(-distance / _decay);
                    angle += swirlAngle;
                    
                    srcX = _centerX + distance * cos(angle);
                    srcY = _centerY + distance * sin(angle);
                    
                } else if (applyEffect && _flowMode == 1) {
                    // Directional flow
                    double dx = x - _centerX;
                    double dy = y - _centerY;
                    
                    // Distance from flow line (perpendicular distance)
                    double perpDist = fabs(dx * flowSin - dy * flowCos);
                    double flowEffect = _flowStrength * exp(-perpDist / _wakeWidth);
                    
                    // Apply flow displacement
                    srcX = x - flowEffect * flowCos;
                    srcY = y - flowEffect * flowSin;
                    
                } else if (applyEffect && _flowMode == 2) {
                    // Boat wake with alternating vortices
                    double dx = x - _centerX;
                    double dy = y - _centerY;
                    
                    // Distance along flow direction
                    double alongFlow = dx * flowCos + dy * flowSin;
                    // Distance perpendicular to flow
                    double perpDist = dx * flowSin - dy * flowCos;
                    
                    // Create alternating vortices (von Kármán vortex street)
                    double vortexPhase = alongFlow / _vortexSpacing;
                    int vortexIndex = (int)floor(vortexPhase);
                    double vortexOffset = (vortexIndex % 2 == 0) ? _wakeWidth * 0.3 : -_wakeWidth * 0.3;
                    
                    // Distance from vortex center
                    double vortexDx = perpDist - vortexOffset;
                    double vortexDy = (vortexPhase - vortexIndex) * _vortexSpacing - _vortexSpacing * 0.5;
                    double vortexDist = sqrt(vortexDx * vortexDx + vortexDy * vortexDy);
                    
                    // Vortex rotation (alternating direction)
                    double vortexAngle = atan2(vortexDy, vortexDx);
                    double rotationStrength = _swirlIntensity * exp(-vortexDist / (_decay * 0.5));
                    if (vortexIndex % 2 == 1) rotationStrength = -rotationStrength;
                    vortexAngle += rotationStrength;
                    
                    // Apply vortex displacement
                    double vortexDispX = vortexDist * cos(vortexAngle) - vortexDx;
                    double vortexDispY = vortexDist * sin(vortexAngle) - vortexDy;
                    
                    // Transform back to image coordinates
                    srcX = x + vortexDispX * flowSin + vortexDispY * flowCos;
                    srcY = y - vortexDispX * flowCos + vortexDispY * flowSin;
                    
                    // Add directional flow component
                    double flowEffect = _flowStrength * exp(-fabs(perpDist) / _wakeWidth);
                    srcX -= flowEffect * flowCos;
                    srcY -= flowEffect * flowSin;
                }
                
                // Sample with bilinear interpolation
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
                    
                    // Bilinear interpolation
                    for (int c = 0; c < nComponents; c++) {
                        double interpolated = p00[c] * fx1 * fy1 +
                                            p10[c] * fx * fy1 +
                                            p01[c] * fx1 * fy +
                                            p11[c] * fx * fy;
                        dstPix[c] = (PIX)interpolated;
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

    if (srcBitDepth == OFX::eBitDepthUByte && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<unsigned char, 4, 255>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthUShort && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<unsigned short, 4, 65535>(args, srcBitDepth);
    } else if (srcBitDepth == OFX::eBitDepthFloat && srcComponents == OFX::ePixelComponentRGBA) {
        renderInternal<float, 4, 1>(args, srcBitDepth);
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
    double radius = _radius->getValueAtTime(args.time);
    double decay = _decay->getValueAtTime(args.time);
    double flowDirection = _flowDirection->getValueAtTime(args.time);
    double flowStrength = _flowStrength->getValueAtTime(args.time);
    double wakeWidth = _wakeWidth->getValueAtTime(args.time);
    double vortexSpacing = _vortexSpacing->getValueAtTime(args.time);
    int flowMode = _flowMode->getValueAtTime(args.time);

    processor.setDstImg(dst.get());
    processor.setSrcImg(src.get());
    processor.setRenderWindow(args.renderWindow);
    processor.setSwirlParams(swirlIntensity, centerX, centerY, radius, decay,
                           flowDirection, flowStrength, wakeWidth, vortexSpacing, flowMode);
    
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
    choiceParam->appendOption("Boat Wake", "Boat wake with alternating vortices");
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
}

OFX::ImageEffect* FluidSwirlPluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
    return new FluidSwirlPlugin(handle);
}

static FluidSwirlPluginFactory p;
mRegisterPluginFactoryInstance(p)