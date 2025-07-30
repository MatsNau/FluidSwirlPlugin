#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>

// CUDA kernel for GPU-accelerated swirl processing
__global__ void fluidSwirlKernel(
    unsigned char* dst, 
    const unsigned char* src,
    int width, 
    int height,
    float centerX, 
    float centerY,
    float swirlIntensity,
    float decay,
    int nComponents)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x >= width || y >= height) return;
    
    // Calculate distance from swirl center
    float dx = x - centerX;
    float dy = y - centerY;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // Apply swirl transformation
    float angle = atan2f(dy, dx);
    float swirlAngle = swirlIntensity * expf(-distance / decay);
    angle += swirlAngle;
    
    // Calculate source coordinates with subpixel precision
    float srcX = centerX + distance * cosf(angle);
    float srcY = centerY + distance * sinf(angle);
    
    int dstIdx = (y * width + x) * nComponents;
    
    // Bilinear interpolation for smooth results
    if (srcX >= 0 && srcX < width-1 && srcY >= 0 && srcY < height-1) {
        int x0 = (int)floorf(srcX);
        int y0 = (int)floorf(srcY);
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        
        float fx = srcX - x0;
        float fy = srcY - y0;
        float fx1 = 1.0f - fx;
        float fy1 = 1.0f - fy;
        
        // Interpolation weights
        float w00 = fx1 * fy1;
        float w10 = fx * fy1;
        float w01 = fx1 * fy;
        float w11 = fx * fy;
        
        for (int c = 0; c < nComponents; c++) {
            int src00 = (y0 * width + x0) * nComponents + c;
            int src10 = (y0 * width + x1) * nComponents + c;
            int src01 = (y1 * width + x0) * nComponents + c;
            int src11 = (y1 * width + x1) * nComponents + c;
            
            float result = w00 * src[src00] + 
                          w10 * src[src10] + 
                          w01 * src[src01] + 
                          w11 * src[src11];
            
            dst[dstIdx + c] = (unsigned char)fminf(255.0f, fmaxf(0.0f, result));
        }
    } else {
        // Black for out-of-bounds pixels
        for (int c = 0; c < nComponents; c++) {
            dst[dstIdx + c] = 0;
        }
    }
}

// Host function to launch CUDA kernel
extern "C" void launchSwirlKernel(
    unsigned char* d_dst,
    const unsigned char* d_src,
    int width,
    int height,
    float centerX,
    float centerY,
    float swirlIntensity,
    float decay,
    int nComponents)
{
    // Configure CUDA grid and block dimensions
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);
    
    // Launch kernel
    fluidSwirlKernel<<<gridSize, blockSize>>>(
        d_dst, d_src, width, height,
        centerX, centerY, swirlIntensity, decay, nComponents);
    
    // Wait for kernel to complete
    cudaDeviceSynchronize();
}

// Memory management helpers
extern "C" void* allocateGPUMemory(size_t size)
{
    void* ptr;
    cudaMalloc(&ptr, size);
    return ptr;
}

extern "C" void freeGPUMemory(void* ptr)
{
    cudaFree(ptr);
}

extern "C" void copyToGPU(void* dst, const void* src, size_t size)
{
    cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice);
}

extern "C" void copyFromGPU(void* dst, const void* src, size_t size)
{
    cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost);
}

// Check if CUDA is available
extern "C" bool isCudaAvailable()
{
    int deviceCount;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    return (error == cudaSuccess && deviceCount > 0);
}