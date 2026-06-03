// This generates the Metal implementation.
// This should only be defined once per executable.
// See also https://developer.apple.com/metal/cpp/
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

// Foundation.hpp wraps Apple's Foundation framework (NSArray, NSString, etc.)
// into the NS:: C++ namespace.
#include <Foundation/Foundation.hpp>

// Metal.hpp wraps Apple's Metal framework (MTLDevice, MTLCommandQueue, etc.)
// into the MTL:: C++ namespace.
#include <Metal/Metal.hpp>

#include <cstdio>

int main() {
    // Create an autorelease pool. Metal-cpp objects follow Objective-C memory
    // management conventions under the hood. The pool collects autoreleased
    // objects and frees them when the pool itself is released, preventing leaks.
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // MTL::CopyAllDevices() returns an NS::Array* containing every Metal-capable
    // GPU in the system. On Apple Silicon Macs this is typically one device;
    // on older Intel Macs with both integrated and discrete GPUs you may see two.
    // The "Copy" in the name means we own the returned array and must release it.
    NS::Array* devices = MTL::CopyAllDevices();

    // Guard against the (unlikely) case where no Metal devices are available.
    if (!devices || devices->count() == 0) {
        printf("No Metal devices found.\n");
        pool->release(); // Always clean up the autorelease pool before exiting.
        return 1;
    }
    printf("Found %lu Metal device(s):\n\n", devices->count());

    for (NS::UInteger i = 0; i < devices->count(); i++) {

        // Retrieve the device at index i. object<MTL::Device>() is a templated
        // accessor that casts the raw NS::Object* to the type we expect.
        auto* device = devices->object<MTL::Device>(i);

        // ----- Basic identity -----

        // name() returns an NS::String* with the GPU's marketing name,
        // e.g. "Apple M1 Pro" or "AMD Radeon Pro 5500M".
        // utf8String() converts it to a plain C string for printf.
        printf("Device %lu: %s\n", i, device->name()->utf8String());

        // registryID() is a system-wide unique 64-bit identifier assigned by
        // the IOKit registry. Useful for matching a Metal device to its
        // IOService entry or to an OpenGL/OpenCL device.
        printf("  Registry ID:          %llu\n", device->registryID());

        // ----- Power and topology flags -----

        // lowPower() is true for integrated GPUs that share memory with the CPU
        // and draw less power. On Intel Macs the integrated Intel GPU returns
        // true here; on Apple Silicon this is false (there's only one GPU).
        printf("  Low Power:            %s\n",
               device->isLowPower() ? "Yes" : "No");

        // removable() is true for GPUs connected via an external enclosure
        // (eGPU) over Thunderbolt. Your app should handle surprise removal
        // by listening for device notifications.
	//
	// The support for removable devices was removed with Apple Silicon,
	// so you can expect this to be false for all modern Apple devices.
        printf("  Removable:            %s\n",
               device->isRemovable() ? "Yes" : "No");

        // headless() is true if the GPU has no display outputs attached.
        // A headless GPU can still do compute work but cannot present frames.
        printf("  Headless:             %s\n",
               device->isHeadless() ? "Yes" : "No");

        // ----- Memory architecture -----

        // hasUnifiedMemory() is true on Apple Silicon where the CPU and GPU
        // share the same physical RAM. This means MTL::Buffer contents are
        // visible to both processors without an explicit copy, which is a
        // major performance advantage.
        printf("  Has Unified Memory:   %s\n",
               device->hasUnifiedMemory() ? "Yes" : "No");

        // ----- Compute limits -----

        // maxThreadsPerThreadgroup() returns an MTL::Size (width, height, depth)
        // describing the largest threadgroup you can dispatch in a compute
        // shader. Typical value on Apple Silicon: 1024 x 1024 x 1024
        // (but the product width*height*depth must also be <= 1024).
        MTL::Size maxThreads = device->maxThreadsPerThreadgroup();
        printf("  Max Threads/Group:    %lu x %lu x %lu\n",
               maxThreads.width,  // Maximum in the X dimension
               maxThreads.height, // Maximum in the Y dimension
               maxThreads.depth); // Maximum in the Z dimension

        // ----- Buffer limits -----

        // maxBufferLength() is the largest single MTL::Buffer you can allocate,
        // in bytes. On Apple Silicon with 16 GB RAM this is typically ~12 GB.
        printf("  Max Buffer Length:    %lu bytes (%.1f GB)\n",
               device->maxBufferLength(),
               device->maxBufferLength() / (1024.0 * 1024.0 * 1024.0));

        // recommendedMaxWorkingSetSize() is Apple's recommendation for the total
        // amount of GPU memory your app should try to stay under. Exceeding it
        // won't crash, but may cause the system to page and hurt performance.
        printf("  Recommended Max RAM:  %llu bytes (%.1f GB)\n",
               device->recommendedMaxWorkingSetSize(),
               device->recommendedMaxWorkingSetSize()
                   / (1024.0 * 1024.0 * 1024.0));

        // ----- Feature queries -----

        // supportsRaytracing() indicates hardware-accelerated ray tracing
        // support (available on Apple Silicon M1 and later).
        printf("  Supports Ray Tracing: %s\n",
               device->supportsRaytracing() ? "Yes" : "No");

        // ----- GPU family support -----

        // Metal groups GPU capabilities into "families". Each family guarantees
        // a baseline set of features (texture formats, argument buffers, etc.).
        // supportsFamily() lets you query at runtime which families this device
        // belongs to, so you can enable or disable features accordingly.
        struct FamilyEntry {
            MTL::GPUFamily family; // The enum value to query
            const char*    name;   // A human-readable label for printing
        };

        FamilyEntry families[] = {
	    // M1 series
            { MTL::GPUFamilyApple7,  "Apple7"  },
	    // M2 series
            { MTL::GPUFamilyApple8,  "Apple8"  },
	    // M3 & M4 series
            { MTL::GPUFamilyApple9,  "Apple9"  },
	    // M5 series
            { MTL::GPUFamilyApple10, "Apple10"  },
            // Mac1: baseline family for all macOS Metal devices.
            { MTL::GPUFamilyMac1,    "Mac1"    },
            // Mac2: adds features like 32-bit MSAA, query texture LOD, etc.
            { MTL::GPUFamilyMac2,    "Mac2"    },
            // Metal3: adds mesh shaders, offline compilation,
	    // MetalFX upscaling support, etc.
            { MTL::GPUFamilyMetal3,  "Metal3"  },
	    // Metal4: adds the Tensor API
            { MTL::GPUFamilyMetal4,  "Metal4"  },
        };

        printf("  GPU Families:\n");

        // Loop through each family and print it only if the device supports it.
        for (auto& f : families) {
            if (device->supportsFamily(f.family))
                printf("    - %s\n", f.name);
        }

        printf("\n");
    }

    // Release the devices array. We own it because the function name contains
    // "Copy" (Objective-C naming convention: Copy/Create = caller owns).
    devices->release();

    // Release the autorelease pool, which frees any autoreleased temporaries
    // (like the NS::String objects returned by name()).
    pool->release();

    return 0;
}
