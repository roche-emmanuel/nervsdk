#include <nvk/dx/DX12Engine.h>
// #include <dxgi1_6.h>
// #include <fstream>

// #include <D3Dcompiler.h>
// #include <FastNoise/FastNoise.h>
// #include <chrono>
// #include <filesystem>
// #include <random>
// #include <regex>

#include <external/stb/stb_image.h>
#include <external/stb/stb_image_write.h>

using namespace DirectX;

#define DX12_USE_INCLUDE_HANDLER 1

namespace nv {

#if DX12_USE_INCLUDE_HANDLER
class ShaderIncludeHandler : public ID3DInclude {
  public:
    virtual ~ShaderIncludeHandler() = default;
    ShaderIncludeHandler(const std::string& includeDir)
        : _includeDir(includeDir) {}

    __declspec(nothrow) HRESULT __stdcall Open(D3D_INCLUDE_TYPE /*includeType*/,
                                               LPCSTR fileName,
                                               LPCVOID /*parentData*/,
                                               LPCVOID* data,
                                               UINT* bytes) override {
        std::string fullPath = _includeDir + "/" + fileName;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            return E_FAIL;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        char* buffer = new char[fileSize];
        file.read(buffer, fileSize);

        *data = buffer;
        *bytes = static_cast<UINT>(fileSize);
        return S_OK;
    }

    __declspec(nothrow) HRESULT __stdcall Close(LPCVOID data) override {
        delete[] static_cast<const char*>(data);
        return S_OK;
    }

  private:
    std::string _includeDir;
};

#endif

auto DX12Engine::instance(ID3D12Device* device) -> DX12Engine& {
    static std::unique_ptr<DX12Engine> singleton;
    if (singleton == nullptr) {
        logDEBUG("Creating DX12Engine.");
        singleton.reset(new DX12Engine(device));
    }
    return *singleton;
}

DX12Engine::DX12Engine(ID3D12Device* device) {
    if (device == nullptr) {
        logDEBUG("DX12Engine: allocating dedicated DX12 device.");
        createDevice();
    } else {
        logDEBUG("DX12Engine: using provided device.");
        _device = device;
    }

    NVCHK(_device != nullptr, "Cannot create DX12 device.");

    // Create command objects
    createCommandObjects();
    createSyncObjects();

    NVCHK(_cmdQueue != nullptr, "Cannot create DX12 command queue.");
    NVCHK(_cmdAllocator != nullptr, "Cannot create DX12 command allocator.");
    NVCHK(_cmdList != nullptr, "Cannot create DX12 command list.");
    NVCHK(_fence != nullptr, "Cannot create DX12 fence.");
}

void DX12Engine::createDevice() {
    // Enable debug layer in debug builds
#if defined(_DEBUG)
    logDEBUG("DX12Engine: Trying to enable debug controller...");
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        logDEBUG("DX12Engine: Debug controller enabled.");
        debugController->EnableDebugLayer();
    }
#endif

    // Create DXGI factory
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    NVCHK(SUCCEEDED(hr), "Failed to create DXGI factory.");

    // Try to create hardware device first
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT adapterIndex = 0;
         SUCCEEDED(factory->EnumAdapters1(adapterIndex, &hardwareAdapter));
         ++adapterIndex) {

        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // Try to create device with this adapter
        if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(),
                                        D3D_FEATURE_LEVEL_12_0,
                                        IID_PPV_ARGS(&_device)))) {
            break;
        }
    }

    // Fallback to WARP device if hardware creation failed
    if (_device == nullptr) {
        ComPtr<IDXGIAdapter> warpAdapter;
        hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        NVCHK(SUCCEEDED(hr), "Failed to get WARP adapter.");

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
                               IID_PPV_ARGS(&_device));
        NVCHK(SUCCEEDED(hr), "Failed to create DX12 device with WARP.");
    }
}

void DX12Engine::createCommandObjects() {
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr =
        _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue));
    NVCHK(SUCCEEDED(hr), "Failed to create command queue.");

    // Create command allocator
    hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         IID_PPV_ARGS(&_cmdAllocator));
    NVCHK(SUCCEEDED(hr), "Failed to create command allocator.");

    // Create command list
    hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    _cmdAllocator.Get(), nullptr,
                                    IID_PPV_ARGS(&_cmdList));
    NVCHK(SUCCEEDED(hr), "Failed to create command list.");

    // Command lists are created in recording state, close it for now
    _cmdList->Close();
}

void DX12Engine::createSyncObjects() {
    HRESULT hr =
        _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    NVCHK(SUCCEEDED(hr), "Failed to create fence.");

    _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    NVCHK(_fenceEvent != nullptr, "Failed to create fence event.");
}

// Command list management
void DX12Engine::clearCommands() {
    HRESULT hr = _cmdAllocator->Reset();
    NVCHK(SUCCEEDED(hr), "Failed to reset command allocator.");

    hr = _cmdList->Reset(_cmdAllocator.Get(), nullptr);
    NVCHK(SUCCEEDED(hr), "Failed to reset command list.");
}

void DX12Engine::executeCommands() {
    HRESULT hr = _cmdList->Close();
    NVCHK(SUCCEEDED(hr), "Failed to close command list.");

    ID3D12CommandList* commandLists[] = {_cmdList.Get()};
    _cmdQueue->ExecuteCommandLists(1, commandLists);
}

void DX12Engine::waitForGpu() {
    _fenceValue++;
    HRESULT hr = _cmdQueue->Signal(_fence.Get(), _fenceValue);
    NVCHK(SUCCEEDED(hr), "Failed to signal fence.");

    if (_fence->GetCompletedValue() < _fenceValue) {
        hr = _fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
        NVCHK(SUCCEEDED(hr), "Failed to set fence event.");
        WaitForSingleObject(_fenceEvent, INFINITE);
    }
}

auto DX12Engine::createVertexBuffer(const void* data, U32 size)
    -> ComPtr<ID3D12Resource> {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> vertexBuffer;

    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));
    NVCHK(SUCCEEDED(hr), "Failed to create vertex buffer.");

    // Upload data
    uploadToResource(vertexBuffer.Get(), data, size);

    // Transition to vertex buffer state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = vertexBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    clearCommands();
    _cmdList->ResourceBarrier(1, &barrier);
    executeCommands();

    return vertexBuffer;
}

void DX12Engine::createUploadBuffer(U32 size) {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    logDEBUG("Allocating upload buffer of size {}", size);

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&_uploadBuffer));
    NVCHK(SUCCEEDED(hr), "Failed to create upload buffer.");

    _uploadBufferSize = size;
    _uploadBufferOffset = 0;
}

void DX12Engine::uploadToResource(ID3D12Resource* resource, const void* data,
                                  U32 size) {
    if (_uploadBufferSize < size) {
        createUploadBuffer(size * 3);
    }

    // Map upload buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, 0};
    HRESULT hr = _uploadBuffer->Map(0, &readRange, &mappedData);
    NVCHK(SUCCEEDED(hr), "Failed to map upload buffer.");

    // Copy data
    memcpy(static_cast<char*>(mappedData) + _uploadBufferOffset, data, size);
    _uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to resource
    clearCommands();
    _cmdList->CopyBufferRegion(resource, 0, _uploadBuffer.Get(),
                               _uploadBufferOffset, size);
    executeCommands();

    // Update offset (simple linear allocator)
    _uploadBufferOffset += (size + 255) & ~255; // Align to 256 bytes
    if (_uploadBufferOffset + size > _uploadBufferSize) {
        _uploadBufferOffset = 0; // Reset (simple wraparound)
    }
}

void DX12Engine::printGPUInfos() {
    ComPtr<IDXGIFactory4> factory;
    ComPtr<IDXGIAdapter1> adapter;

    // Get the DXGI factory
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

    // Get the adapter associated with the device
    LUID adapterLuid = _device->GetAdapterLuid();

    factory->EnumAdapterByLuid(adapterLuid, IID_PPV_ARGS(&adapter));

    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    // Convert wide string to regular string
    std::wstring wDeviceName(desc.Description);
    std::string deviceName = WStringToString(wDeviceName);
    logDEBUG("DX12Engine GPU infos:");
    logDEBUG(" - GPU Name: {}", deviceName);

    logDEBUG(" - Dedicated Video Memory: {:.2f} GB",
             desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0));
    logDEBUG(" - Dedicated System Memory: {:.2f} MB",
             desc.DedicatedSystemMemory / (1024.0 * 1024.0));
    logDEBUG(" - Shared System Memory: {:.2f} MB",
             desc.SharedSystemMemory / (1024.0 * 1024.0));
    logDEBUG(" - Vendor ID: 0x{:X}", desc.VendorId);
    logDEBUG(" - Device ID: 0x{:X}", desc.DeviceId);
}

void DX12Engine::addTransition(ID3D12Resource* res,
                               D3D12_RESOURCE_STATES sBefore,
                               D3D12_RESOURCE_STATES sAfter) {
    if (sBefore == sAfter) {
        // No transition to perform.
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = res,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = sBefore,
            .StateAfter = sAfter,
        }};

    _cmdList->ResourceBarrier(1, &barrier);
}

void DX12Engine::addTransition(ID3D12Resource* res,
                               D3D12_RESOURCE_STATES sAfter) {
    auto sBefore = getCurrentState(res);
    addTransition(res, sBefore, sAfter);
    setCurrentState(res, sAfter);
}

auto DX12Engine::getCurrentState(ID3D12Resource* res,
                                 D3D12_RESOURCE_STATES defval)
    -> D3D12_RESOURCE_STATES {
    if (auto it = _stateMap.find(res); it != _stateMap.end()) {
        return it->second;
    }
    return defval;
}

void DX12Engine::setCurrentState(ID3D12Resource* res,
                                 D3D12_RESOURCE_STATES state) {
    _stateMap[res] = state;
}

void DX12Engine::addCopyDstTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COPY_DEST);
};
void DX12Engine::addCopySrcTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COPY_SOURCE);
};
void DX12Engine::addCommonTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COMMON);
};
void DX12Engine::addRenderTgtTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_RENDER_TARGET);
};
void DX12Engine::setCopyDstState(ID3D12Resource* res) {
    setCurrentState(res, D3D12_RESOURCE_STATE_COPY_DEST);
}
void DX12Engine::setCopySrcState(ID3D12Resource* res) {
    setCurrentState(res, D3D12_RESOURCE_STATE_COPY_SOURCE);
}
void DX12Engine::setCommonState(ID3D12Resource* res) {
    setCurrentState(res, D3D12_RESOURCE_STATE_COMMON);
}
void DX12Engine::setRenderTgtState(ID3D12Resource* res) {
    setCurrentState(res, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DX12Engine::addCopyFullTextureToTexture(ID3D12Resource* src,
                                             ID3D12Resource* dst) {
    NVCHK(src != nullptr, "Invalid source resource.");
    NVCHK(dst != nullptr, "Invalid dest resource.");

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dst;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    _cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void DX12Engine::addCopyFullTextureToBuffer(ID3D12Resource* src,
                                            ID3D12Resource* dst) {
    NVCHK(src != nullptr, "Invalid source resource.");
    NVCHK(dst != nullptr, "Invalid dest resource.");

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dst;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    D3D12_RESOURCE_DESC sourceDesc = src->GetDesc();

    _device->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &footprint, &numRows,
                                   &rowSizeInBytes, nullptr);
    dstLocation.PlacedFootprint = footprint;

    _cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void DX12Engine::saveTextureToFile(ID3D12Resource* tex, const char* filename) {
    NVCHK(tex != nullptr, "DX11 saveTextureToFile: Invalid texture.");

    auto reqSize = getRequiredReadBufferSize(tex);
    NVCHK(reqSize > 0, "Invalid required buffer size.");

    if (_readbackBufferSize < reqSize) {
        logDEBUG("Reallocating readback buffer with size {}", reqSize);
        createReadbackBuffer(reqSize);
    }

    clearCommands();
    addCopyDstTransition(_readbackBuffer.Get());
    addCopySrcTransition(tex);
    addCopyFullTextureToBuffer(tex, _readbackBuffer.Get());
    executeCommands();

    // Wait for GPU to complete
    waitForGpu();

    // Map the readback buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, _readbackBufferSize};
    HRESULT hr = _readbackBuffer->Map(0, &readRange, &mappedData);

    if (FAILED(hr) || !mappedData) {
        logERROR("Failed to map readback buffer");
        return;
    }

    // Get the footprint info
    D3D12_RESOURCE_DESC textureDesc = tex->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    _device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows,
                                   &rowSizeInBytes, nullptr);

    // Convert the data to RGBA format for stb_image
    UINT width = static_cast<UINT>(textureDesc.Width);
    UINT height = textureDesc.Height;
    std::vector<unsigned char> imageData(width * height * 4); // RGBA

    logDEBUG("Dest texture format is {}", (int)textureDesc.Format);

#if 0
  // Copy and convert pixel data (assuming BGRA format from render target)
  const unsigned char *srcData = static_cast<const unsigned char *>(mappedData);
  for (UINT y = 0; y < height; ++y) {
    for (UINT x = 0; x < width; ++x) {
      UINT srcIndex = y * footprint.Footprint.RowPitch + x * 4;
      UINT dstIndex = (y * width + x) * 4;

      // Convert BGRA to RGBA
      // imageData[dstIndex + 0] = srcData[srcIndex + 2]; // R
      // imageData[dstIndex + 1] = srcData[srcIndex + 1]; // G
      // imageData[dstIndex + 2] = srcData[srcIndex + 0]; // B
      // imageData[dstIndex + 3] = srcData[srcIndex + 3]; // A
      imageData[dstIndex + 0] = srcData[srcIndex + 1]; // R
      imageData[dstIndex + 1] = srcData[srcIndex + 2]; // G
      imageData[dstIndex + 2] = srcData[srcIndex + 3]; // B
      imageData[dstIndex + 3] = srcData[srcIndex + 0]; // A
    }
  }
#else
    // Copy and convert pixel data (DXGI_FORMAT_R10G10B10A2_UNORM)
    const uint32_t* srcData = static_cast<const uint32_t*>(mappedData);
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            UINT srcIndex = y * (footprint.Footprint.RowPitch / 4) +
                            x; // Divide by 4 because we're using uint32_t
            UINT dstIndex = (y * width + x) * 4;

            // Unpack R10G10B10A2 format
            uint32_t pixel = srcData[srcIndex];

            // Extract components (10 bits R, 10 bits G, 10 bits B, 2 bits A)
            uint32_t r = (pixel >> 0) & 0x3FF;  // Lower 10 bits
            uint32_t g = (pixel >> 10) & 0x3FF; // Next 10 bits
            uint32_t b = (pixel >> 20) & 0x3FF; // Next 10 bits
            // uint32_t a = (pixel >> 30) & 0x3;   // Upper 2 bits

            // Convert to 8-bit (downscale from 10/2 bit to 8 bit)
            imageData[dstIndex + 0] =
                static_cast<unsigned char>((r * 255) / 1023); // R
            imageData[dstIndex + 1] =
                static_cast<unsigned char>((g * 255) / 1023); // G
            imageData[dstIndex + 2] =
                static_cast<unsigned char>((b * 255) / 1023); // B
            // imageData[dstIndex + 3] = static_cast<unsigned char>((a * 255) /
            // 3); // A The alpha value is always 0, so we just force it to 255
            // instead.
            imageData[dstIndex + 3] = 255;

            // if (x < 5 && y < 5) {
            //   logDEBUG("Pixel ({},{}) - Raw: 0x{:08X}, R:{}, G:{}, B:{},
            //   A:{}", x, y,
            //            pixel, imageData[dstIndex + 0], imageData[dstIndex +
            //            1], imageData[dstIndex + 2], imageData[dstIndex + 3]);
            // }
        }
    }
#endif

    // Save the image
    if (stbi_write_png(filename, width, height, 4, imageData.data(),
                       width * 4) != 0) {
        logDEBUG("Saved image: {}", filename);
    } else {
        logERROR("Failed to save image: {}", filename);
    }

    // Unmap the buffer
    D3D12_RANGE writeRange = {0, 0}; // We didn't write anything
    _readbackBuffer->Unmap(0, &writeRange);
}

void DX12Engine::createReadbackBuffer(U32 size) {
    // Create readback buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&_readbackBuffer));

    if (FAILED(hr)) {
        logERROR("Failed to create readback buffer");
    }

    setCopyDstState(_readbackBuffer.Get());
    _readbackBufferSize = size;
}

auto DX12Engine::getRequiredReadBufferSize(ID3D12Resource* tex) -> U32 {
    // Calculate buffer size
    auto desc = tex->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    U64 size = 0;
    _device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows,
                                   &rowSizeInBytes, &size);
    return size;
}

} // namespace nv
