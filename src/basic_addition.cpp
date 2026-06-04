// This generates the Metal implementation.
// This should only be defined once per executable.
// See also https://developer.apple.com/metal/cpp/
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

// Foundation.hpp wraps Apple's Foundation framework (NSArray, NSString, etc.)
// into the NS:: C++ namespace.
#include <Foundation/Foundation.hpp>

// Metal.hpp wraps Apple's Metal framework (MTLDevice, MTLCommandQueue, etc.)
// into the MTL:: C++ namespace.
#include <Metal/Metal.hpp>

// For loading the metallib embedded in the Mach-O binary.
#include <dispatch/dispatch.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>

#include <cstdio>

constexpr unsigned int kArrayLength = 32;
constexpr unsigned int kBufferSize  = kArrayLength * sizeof(float);

int main() {
    NS::Error* error = nullptr;

    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

    auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!device) {
        printf("Metal is not supported on this device.\n");
        return -1;
    }

    printf("Using device: %s\n", device->name()->utf8String());

    // Load the Metal library from the embedded Mach-O section ──
    //
    //  The metallib is embedded at link time with:
    //    -sectcreate __TEXT __metallib default.metallib
    //
    // At runtime we read it back with getsectiondata() and hand the
    // raw bytes to MTL::Device::newLibrary(dispatch_data_t, …).
    unsigned long  metallib_size = 0;
    const uint8_t* metallib_data = getsectiondata(
        &_mh_execute_header, "__TEXT", "__metallib", &metallib_size);
 
    if (!metallib_data || metallib_size == 0) {
        printf("Failed to find embedded metallib section.\n");
        return -1;
    }
 
    dispatch_data_t lib_data = dispatch_data_create(
        metallib_data, metallib_size,
        nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
 
    auto library = NS::TransferPtr(device->newLibrary(lib_data, &error));
    dispatch_release(lib_data);
    if (!library) {
        printf("Failed to create library from embedded metallib: %s\n",
               error->localizedDescription()->utf8String());
        return -1;
    }

    // Look up the kernel function by name
    auto fnName = NS::String::string("add_arrays", NS::UTF8StringEncoding);
    auto addFunction = NS::TransferPtr(library->newFunction(fnName));
    // At this point, the library is no longer needed
    if (!addFunction) {
        printf("Failed to find the adder function.\n");
        return -1;
    }

    // Create a compute pipeline state from the function
    // This compiles the MSL function for the specific GPU.
    auto addFunctionPSO = device->newComputePipelineState(addFunction.get(), &error);
    // At this point, addFunction is no longer needed
    if (!addFunctionPSO) {
        printf("Failed to create pipeline state object: %s\n",
               error->localizedDescription()->utf8String());
        return -1;
    }

    // A command queue is used to schedule work on a GPU.
    auto commandQueue = device->newCommandQueue();
    if (!commandQueue) {
        printf("Failed to create command queue.\n");
        return -1;
    }

    // Allocate (on-)devive buffers
    // StorageModeShared means both the CPU and GPU can access the data.
    auto bufferA = NS::TransferPtr(device->newBuffer(kBufferSize, MTL::ResourceStorageModeShared));
    auto bufferB = NS::TransferPtr(device->newBuffer(kBufferSize, MTL::ResourceStorageModeShared));
    auto bufferResult = NS::TransferPtr(device->newBuffer(kBufferSize, MTL::ResourceStorageModeShared));

    // Fill with data so that A + B = kArrayLength
    auto* dataPtrA = static_cast<float*>(bufferA->contents());
    auto* dataPtrB = static_cast<float*>(bufferB->contents());
    for (unsigned long i = 0; i < kArrayLength; ++i) {
        dataPtrA[i] = static_cast<float>(i);
	dataPtrB[i] = static_cast<float>(kArrayLength - i);
    }

    // Buffer to encode commands and submit them in batches to the command queue.
    // Note that we don't need to call TransferPtr here as this is an auto-released object.
    auto commandBuffer = commandQueue->commandBuffer();
    // Encoder to write compute-specific commands into the command buffer.
    // These are single use; once you have written a command to the buffer,
    // you need to create a new instance.
    auto computeEncoder = commandBuffer->computeCommandEncoder();

    // Set what to run (which shader) and bind the arguments of the function.
    computeEncoder->setComputePipelineState(addFunctionPSO);
    // The second argument is the offset of the used buffer.
    // As we use three separate buffers, we use them all from the start.
    computeEncoder->setBuffer(bufferA.get(), 0, 0);
    computeEncoder->setBuffer(bufferB.get(), 0, 1);
    computeEncoder->setBuffer(bufferResult.get(), 0, 2);

    // Determine grid and threadgroup sizes
    auto gridSize = MTL::Size::Make(kArrayLength, 1, 1);
 
    // Use the largest threadgroup the pipeline supports, clamped to
    // the array length.
    NS::UInteger threadGroupSize = addFunctionPSO->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > kArrayLength) {
        threadGroupSize = kArrayLength;
    }
    MTL::Size threadgroupSize = MTL::Size::Make(threadGroupSize, 1, 1);
    computeEncoder->dispatchThreads(gridSize, threadgroupSize);

    // We're done encoding the compute pass.
    computeEncoder->endEncoding();

    // Execute it and wait until completed.
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();

    // Print the result
    auto* dataPtrResult = static_cast<float*>(bufferResult->contents());
    printf("Result: ");
    for (unsigned long i = 0; i < kArrayLength; ++i) {
        printf("%.2f ", dataPtrResult[i]);
    }
    printf("\n");
}

