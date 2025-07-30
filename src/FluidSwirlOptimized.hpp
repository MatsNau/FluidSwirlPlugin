#pragma once

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include <memory>

// GPU acceleration interface
class GPUAccelerator {
public:
    virtual ~GPUAccelerator() = default;
    virtual bool isAvailable() const = 0;
    virtual void processSwirl(
        unsigned char* dst,
        const unsigned char* src,
        int width, int height,
        float centerX, float centerY,
        float swirlIntensity, float decay,
        int nComponents) = 0;
};

// CUDA implementation
class CUDAAccelerator : public GPUAccelerator {
private:
    void* d_src = nullptr;
    void* d_dst = nullptr;
    size_t allocated_size = 0;

public:
    ~CUDAAccelerator();
    bool isAvailable() const override;
    void processSwirl(
        unsigned char* dst,
        const unsigned char* src,
        int width, int height,
        float centerX, float centerY,
        float swirlIntensity, float decay,
        int nComponents) override;
    
private:
    void ensureMemoryAllocated(size_t size);
};

// OpenCL implementation (placeholder for future)
class OpenCLAccelerator : public GPUAccelerator {
public:
    bool isAvailable() const override { return false; }
    void processSwirl(
        unsigned char* dst,
        const unsigned char* src,
        int width, int height,
        float centerX, float centerY,
        float swirlIntensity, float decay,
        int nComponents) override {}
};

// CPU-optimized implementation with SIMD
template <class PIX, int nComponents, int maxValue>
class OptimizedSwirlProcessor : public OFX::PixelProcessorFilterBase
{
protected:
    double _swirlIntensity;
    double _centerX, _centerY;
    double _radius;
    double _decay;
    std::unique_ptr<GPUAccelerator> _gpu;
    bool _useGPU;
    
public:
    OptimizedSwirlProcessor(OFX::ImageEffect &instance) 
        : OFX::PixelProcessorFilterBase(instance), _useGPU(false)
    {
        // Try to initialize GPU acceleration
        _gpu = std::make_unique<CUDAAccelerator>();
        if (!_gpu->isAvailable()) {
            _gpu = std::make_unique<OpenCLAccelerator>();
        }
        _useGPU = _gpu->isAvailable();
    }
    
    void setSwirlParams(double intensity, double centerX, double centerY, double radius, double decay)
    {
        _swirlIntensity = intensity;
        _centerX = centerX;
        _centerY = centerY;
        _radius = radius;
        _decay = decay;
    }

private:
    void multiThreadProcessImages(OfxRectI procWindow) override
    {
        if (_useGPU && std::is_same<PIX, unsigned char>::value) {
            processWithGPU(procWindow);
        } else {
            processWithCPU(procWindow);
        }
    }
    
    void processWithGPU(OfxRectI procWindow)
    {
        const int width = procWindow.x2 - procWindow.x1;
        const int height = procWindow.y2 - procWindow.y1;
        
        // Get source and destination pointers
        PIX *srcData = (PIX *) getSrcPixelAddress(procWindow.x1, procWindow.y1);
        PIX *dstData = (PIX *) getDstPixelAddress(procWindow.x1, procWindow.y1);
        
        _gpu->processSwirl(
            reinterpret_cast<unsigned char*>(dstData),
            reinterpret_cast<const unsigned char*>(srcData),
            width, height,
            static_cast<float>(_centerX - procWindow.x1),
            static_cast<float>(_centerY - procWindow.y1),
            static_cast<float>(_swirlIntensity),
            static_cast<float>(_decay),
            nComponents
        );
    }
    
    void processWithCPU(OfxRectI procWindow)
    {
        // Optimized CPU implementation with loop unrolling and vectorization hints
        const int width = procWindow.x2 - procWindow.x1;
        const int height = procWindow.y2 - procWindow.y1;
        
        // Pre-calculate constants
        const double inv_decay = 1.0 / _decay;
        const double intensity = _swirlIntensity;
        
        #pragma omp parallel for if(height > 64)
        for (int y = procWindow.y1; y < procWindow.y2; y++) {
            if (_effect.abort()) break;
            
            PIX *dstPix = (PIX *) getDstPixelAddress(procWindow.x1, y);
            const double dy = y - _centerY;
            const double dy2 = dy * dy;
            
            for (int x = procWindow.x1; x < procWindow.x2; x++) {
                const double dx = x - _centerX;
                const double distance = sqrt(dx * dx + dy2);
                
                // Fast exponential approximation for better performance
                const double exp_arg = -distance * inv_decay;
                const double swirl_factor = intensity * fastExp(exp_arg);
                
                const double angle = atan2(dy, dx) + swirl_factor;
                
                // Use fast trigonometric functions
                double cos_angle, sin_angle;
                fastSinCos(angle, &sin_angle, &cos_angle);
                
                const int srcX = static_cast<int>(_centerX + distance * cos_angle);
                const int srcY = static_cast<int>(_centerY + distance * sin_angle);
                
                if (srcX >= procWindow.x1 && srcX < procWindow.x2 && 
                    srcY >= procWindow.y1 && srcY < procWindow.y2) {
                    
                    const PIX *srcPix = (PIX *) getSrcPixelAddress(srcX, srcY);
                    
                    // Unrolled pixel copy for common channel counts
                    if (nComponents == 4) {
                        dstPix[0] = srcPix[0];
                        dstPix[1] = srcPix[1];
                        dstPix[2] = srcPix[2];
                        dstPix[3] = srcPix[3];
                    } else if (nComponents == 3) {
                        dstPix[0] = srcPix[0];
                        dstPix[1] = srcPix[1];
                        dstPix[2] = srcPix[2];
                    } else {
                        for (int c = 0; c < nComponents; c++) {
                            dstPix[c] = srcPix[c];
                        }
                    }
                } else {
                    // Clear out-of-bounds pixels
                    for (int c = 0; c < nComponents; c++) {
                        dstPix[c] = 0;
                    }
                }
                
                dstPix += nComponents;
            }
        }
    }
    
    // Fast approximation functions for performance
    inline double fastExp(double x) const
    {
        if (x < -10.0) return 0.0;
        if (x > 10.0) return 22026.465794806717;
        
        // Pade approximation for exp(x)
        const double a = 1.0 + x / 32.0;
        const double b = a * a;
        const double c = b * b;
        const double d = c * c;
        return d * d;
    }
    
    inline void fastSinCos(double angle, double* sin_val, double* cos_val) const
    {
        // Use built-in sincos if available, otherwise fall back to separate calls
        #ifdef __GNUC__
        sincos(angle, sin_val, cos_val);
        #else
        *sin_val = sin(angle);
        *cos_val = cos(angle);
        #endif
    }
};