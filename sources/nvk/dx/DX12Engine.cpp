#ifdef _WIN32

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

#ifdef _DEBUG
static bool gDebugLayerEnabled = true;
#else
static bool gDebugLayerEnabled = true;
#endif

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

void DX12Engine::enableDebugLayer(bool enable) { gDebugLayerEnabled = enable; }

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
        _device = createDevice();
    } else {
        logDEBUG("DX12Engine: using provided device.");
        _device = device;
    }

    NVCHK(_device != nullptr, "Cannot create DX12 device.");

    // Create command objects
    createCommandObjects();
    createSyncObjects();

    NVCHK(_cmdQueue != nullptr, "Cannot create DX12 command queue.");
    NVCHK(_fence != nullptr, "Cannot create DX12 fence.");
}

auto DX12Engine::createDevice() -> ComPtr<ID3D12Device> {
    // Enable debug layer in debug builds
    if (gDebugLayerEnabled) {
        logDEBUG("DX12Engine: Trying to enable debug controller...");
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            logDEBUG("DX12Engine: Debug controller enabled.");
            debugController->EnableDebugLayer();
            // debug->SetEnableGPUBasedValidation(TRUE);
        }
    } else {
        logDEBUG("DX12Engine: Debug layer disabled.");
    }

    // Create DXGI factory
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    NVCHK(SUCCEEDED(hr), "Failed to create DXGI factory.");

    ComPtr<ID3D12Device> device;

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
                                        IID_PPV_ARGS(device.GetAddressOf())))) {
            break;
        }
    }

    // Fallback to WARP device if hardware creation failed

    if (device == nullptr) {
        ComPtr<IDXGIAdapter> warpAdapter;
        hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        NVCHK(SUCCEEDED(hr), "Failed to get WARP adapter.");

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
                               IID_PPV_ARGS(device.GetAddressOf()));
        NVCHK(SUCCEEDED(hr), "Failed to create DX12 device with WARP.");
    }

    if (gDebugLayerEnabled) {

        // Configure info queue to break on errors/warnings and print to
        // console
        ComPtr<ID3D12InfoQueue1> infoQueue;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            // Break on severe issues
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,
                                          TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            // Optional: break on warnings too
            // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,
            // TRUE);

            // Enable callback to print messages to console
            DWORD callbackCookie;
            infoQueue->RegisterMessageCallback(
                [](D3D12_MESSAGE_CATEGORY Category,
                   D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID,
                   LPCSTR pDescription, void* pContext) {
                    switch (Severity) {
                    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                        logERROR("[D3D12 CORRUPTION] {}", pDescription);
                        break;
                    case D3D12_MESSAGE_SEVERITY_ERROR:
                        logERROR("[D3D12 ERROR] {}", pDescription);
                        break;
                    // case D3D12_MESSAGE_SEVERITY_WARNING:
                    //     logINFO("[D3D12 WARNING] {}", pDescription);
                    //     break;
                    case D3D12_MESSAGE_SEVERITY_INFO:
                        logINFO("[D3D12 INFO] {}", pDescription);
                        break;
                    default:
                        // logNOTE("[D3D12 INFO] {}", pDescription);
                        break;
                    };
                },
                D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie);
        }
    }

    return device;
}

void DX12Engine::createCommandObjects() {
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr =
        _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue));
    NVCHK(SUCCEEDED(hr), "Failed to create command queue.");
}

void DX12Engine::createSyncObjects() {
    HRESULT hr =
        _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    NVCHK(SUCCEEDED(hr), "Failed to create fence.");

    _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    NVCHK(_fenceEvent != nullptr, "Failed to create fence event.");
}

void DX12Engine::executeCommands(CommandListContext& ctx) {

    // Close the command list:
    NVCHK(ctx.isRecording, "Command list was not recording!");

    HRESULT hr = ctx.cmdList->Close();
    NVCHK(SUCCEEDED(hr), "Failed to close command list.");

    ID3D12CommandList* commandLists[] = {ctx.cmdList.Get()};
    _cmdQueue->ExecuteCommandLists(1, commandLists);

    // Signal fence with new value
    _fenceValue++;
    hr = _cmdQueue->Signal(_fence.Get(), _fenceValue);
    NVCHK(SUCCEEDED(hr), "Failed to signal fence.");

    // Store fence value for this command list
    ctx.fenceValue = _fenceValue;

    // Mark the command context as not recording anymore:
    ctx.isRecording = false;
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
    writeBuffer(vertexBuffer.Get(), data, size);

    // Transition to vertex buffer state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = vertexBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    auto& ctx = beginCmdList();
    ctx.cmdList->ResourceBarrier(1, &barrier);
    executeCommands(ctx);

    return vertexBuffer;
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

void CommandListContext::addTransition(ID3D12Resource* res,
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

    cmdList->ResourceBarrier(1, &barrier);
}

void CommandListContext::addTransition(ID3D12Resource* res,
                                       D3D12_RESOURCE_STATES sAfter) {
    auto sBefore = eng.getCurrentState(res);
    addTransition(res, sBefore, sAfter);
    eng.setCurrentState(res, sAfter);
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

void CommandListContext::addCopyDstTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COPY_DEST);
};
void CommandListContext::addCopySrcTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COPY_SOURCE);
};
void CommandListContext::addCommonTransition(ID3D12Resource* res) {
    addTransition(res, D3D12_RESOURCE_STATE_COMMON);
};
void CommandListContext::addRenderTgtTransition(ID3D12Resource* res) {
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

void CommandListContext::addCopyFullTextureToTexture(ID3D12Resource* src,
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

    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void CommandListContext::addCopyFullTextureToBuffer(ID3D12Resource* src,
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

    eng.device()->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &footprint,
                                        &numRows, &rowSizeInBytes, nullptr);
    dstLocation.PlacedFootprint = footprint;

    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void DX12Engine::saveTextureToFile(ID3D12Resource* tex, const char* filename) {
    NVCHK(tex != nullptr, "DX11 saveTextureToFile: Invalid texture.");

    auto reqSize = getRequiredReadBufferSize(tex);
    NVCHK(reqSize > 0, "Invalid required buffer size.");

    if (_readbackBufferSize < reqSize) {
        logDEBUG("Reallocating readback buffer with size {}", reqSize);
        getReadbackBuffer(reqSize);
    }

    auto& ctx = beginCmdList();
    ctx.addCopyDstTransition(_readbackBuffer.Get());
    ctx.addCopySrcTransition(tex);
    ctx.addCopyFullTextureToBuffer(tex, _readbackBuffer.Get());
    executeCommands(ctx);

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

auto DX12Engine::getReadbackBuffer(U32 size) -> ComPtr<ID3D12Resource> {
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

    return _readbackBuffer;
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

auto DX12Engine::readShaderFile(const std::string& filename,
                                std::unordered_set<std::string>& fileList)
    -> std::string {
    // Open the file
    String content = read_virtual_file(filename);

    // Check if we have any include statement in this file:
    std::regex includeRegex(R"(#include\s+\"([^\"]+)\")");
    std::smatch match;
    std::string processedSource;
    std::string::const_iterator searchStart(content.cbegin());

    while (
        std::regex_search(searchStart, content.cend(), match, includeRegex)) {
        // Append content before #include statement
        processedSource.append(searchStart, match.prefix().first);

        // Extract include file path
        std::string includePath = match[1].str();

        // Recursively load the included file
        std::string full_path = _shaderIncludeDir + "/" + includePath;

        // Add this file to the set:
        if (fileList.insert(full_path).second) {
            // File effectively inserted because it was not already included
            // before.
            std::string includeContent = readShaderFile(full_path, fileList);

            // Append included content
            processedSource.append(includeContent);
        }

        // Move past the current match
        searchStart = match.suffix().first;
    }

    // Append remaining source content after the last include
    processedSource.append(searchStart, content.cend());

    return processedSource;
}

auto DX12Engine::compileShaderSource(const std::string& source,
                                     const std::string& hint,
                                     const std::string& funcName,
                                     const std::string& profile)
    -> ComPtr<ID3DBlob> {
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr =
        D3DCompile(source.c_str(), source.length(), hint.c_str(), nullptr,
                   nullptr, funcName.c_str(), profile.c_str(), compileFlags, 0,
                   shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(hr)) {
        if (errorBlob) {
            std::string errorMsg =
                static_cast<const char*>(errorBlob->GetBufferPointer());
            THROW_MSG("Shader compilation failed ({}): {}", hint, errorMsg);
        }
        THROW_MSG("Shader compilation failed ({}) with HRESULT: 0x{:X}", hint,
                  static_cast<unsigned int>(hr));
    }

    return shaderBlob;
}

auto DX12Engine::createComputeShader(const std::string& source,
                                     const std::string& hint,
                                     const std::string& funcName,
                                     const std::string& profile)
    -> ComPtr<ID3DBlob> {
    return compileShaderSource(source, hint, funcName, profile);
}

auto DX12Engine::createComputePipelineState(ID3D12RootSignature* rootSig,
                                            ID3DBlob* computeShader)
    -> ComPtr<ID3D12PipelineState> {
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.CS = {computeShader->GetBufferPointer(),
                  computeShader->GetBufferSize()};
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = _device->CreateComputePipelineState(
        &psoDesc, IID_PPV_ARGS(pso.GetAddressOf()));

    if (FAILED(hr)) {
        THROW_MSG("Failed to create compute pipeline state: 0x{:X}",
                  static_cast<unsigned int>(hr));
    }

    return pso;
}

auto DX12Engine::createComputeProgram(const std::string& filename,
                                      DX12RootSig& sig) -> DX12Program {
    DX12Program prog;
    prog.filename = filename;
    prog.isCompute = true;

    // Read shader file
    std::unordered_set<std::string> fileList;
    std::string fullPath = _shaderIncludeDir + "/" + filename;
    fileList.insert(fullPath);
    std::string source = readShaderFile(fullPath, fileList);

    // Store file dependencies
    prog.files = fileList;

    // Compile compute shader
    prog.computeShaderBlob =
        createComputeShader(source, filename, "cs_main", "cs_5_0");

    // Create root signature
    prog.rootSignature = sig.getSignature();

    // Create pipeline state
    prog.pipelineState = createComputePipelineState(
        prog.rootSignature.Get(), prog.computeShaderBlob.Get());

    // Update time tracking
    prog.lastUpdateTime = std::time(nullptr);
    prog.lastCheckTime = prog.lastUpdateTime;

    return prog;
}

void CommandListContext::setComputeProgram(const DX12Program& prog) {
    if (!prog.isCompute) {
        THROW_MSG("Attempting to set non-compute program as compute program");
    }

    cmdList->SetPipelineState(prog.pipelineState.Get());
    cmdList->SetComputeRootSignature(prog.rootSignature.Get());
}

void CommandListContext::dispatch(U32 threadGroupCountX, U32 threadGroupCountY,
                                  U32 threadGroupCountZ) {
    cmdList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

auto DX12Engine::createStructuredBuffer(U32 elemSize, U32 numElems,
                                        D3D12_RESOURCE_FLAGS flags)
    -> ComPtr<ID3D12Resource> {
    U32 bufferSize = elemSize * numElems;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = bufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = flags;

    ComPtr<ID3D12Resource> buffer;
    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(buffer.GetAddressOf()));

    if (FAILED(hr)) {
        THROW_MSG("Failed to create structured buffer: 0x{:X}",
                  static_cast<unsigned int>(hr));
    }

    setCommonState(buffer.Get());

    return buffer;
}

// auto DX12Engine::createReadbackBuffer(U32 size) -> ComPtr<ID3D12Resource> {
//     D3D12_HEAP_PROPERTIES heapProps = {};
//     heapProps.Type = D3D12_HEAP_TYPE_READBACK;
//     heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
//     heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

//     D3D12_RESOURCE_DESC resourceDesc = {};
//     resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//     resourceDesc.Alignment = 0;
//     resourceDesc.Width = size;
//     resourceDesc.Height = 1;
//     resourceDesc.DepthOrArraySize = 1;
//     resourceDesc.MipLevels = 1;
//     resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
//     resourceDesc.SampleDesc.Count = 1;
//     resourceDesc.SampleDesc.Quality = 0;
//     resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//     resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

//     ComPtr<ID3D12Resource> buffer;
//     HRESULT hr = _device->CreateCommittedResource(
//         &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
//         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
//         IID_PPV_ARGS(buffer.GetAddressOf()));

//     if (FAILED(hr)) {
//         THROW_MSG("Failed to create readback buffer: 0x{:X}",
//                   static_cast<unsigned int>(hr));
//     }

//     setCopyDstState(buffer.Get());

//     return buffer;
// }

auto DX12Engine::createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
                                      U32 numDescriptors, bool shaderVisible)
    -> ComPtr<ID3D12DescriptorHeap> {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                   : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = _device->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(heap.GetAddressOf()));

    if (FAILED(hr)) {
        THROW_MSG("Failed to create descriptor heap: 0x{:X}",
                  static_cast<unsigned int>(hr));
    }

    return heap;
}

void DX12Engine::createUnorderedAccessView(
    ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor,
    U32 numElements, U32 structureByteStride) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = structureByteStride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    _device->CreateUnorderedAccessView(resource, nullptr, &uavDesc,
                                       destDescriptor);
}

void DX12Engine::createShaderResourceView(
    ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor,
    U32 numElements, U32 structureByteStride) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    _device->CreateShaderResourceView(resource, &srvDesc, destDescriptor);
}

void DX12Engine::setShaderIncludeDir(const std::string& dir) {
    _shaderIncludeDir = dir;
}

void DX12Engine::check_live_reload(DX12Program& prog) {
    // Get the current time:
    auto cur_time = std::time(nullptr);

    // Check if we should check anything:
    if ((cur_time - prog.lastCheckTime) < 1) {
        // Do nothing.
        return;
    }

    prog.lastCheckTime = cur_time;

    // Iterate on all the files needed for this shader and check their time:
    for (const auto& filename : prog.files) {
        // Next we check if the file time changed:
        auto currentFileTime = std::filesystem::last_write_time(filename);
        auto systemTime =
            std::chrono::clock_cast<std::chrono::system_clock>(currentFileTime);
        auto fileTime = std::chrono::system_clock::to_time_t(systemTime);

        if (fileTime > prog.lastUpdateTime) {
            // We should try a live reload of this program:
            logDEBUG("Reloading HLSL program from {}", prog.filename);

            // Update the change/check times anyway:
            if (!updateProgram(prog)) {
                logDEBUG("ERROR: live reload failed for {}", prog.filename);
            }

            // Update the last update time in all cases.
            prog.lastUpdateTime = fileTime;
            break;
        }
    }
}

auto DX12Engine::updateProgram(DX12Program& prog) -> bool {
    try {
        // Read shader file
        std::unordered_set<std::string> fileList;
        std::string fullPath = _shaderIncludeDir + "/" + prog.filename;
        fileList.insert(fullPath);
        std::string source = readShaderFile(fullPath, fileList);

        if (prog.isCompute) {
            // Recompile compute shader
            auto newComputeBlob =
                createComputeShader(source, prog.filename, "cs_main", "cs_5_0");

            // Create new pipeline state
            auto newPSO = createComputePipelineState(prog.rootSignature.Get(),
                                                     newComputeBlob.Get());

            // Update program
            prog.computeShaderBlob = newComputeBlob;
            prog.pipelineState = newPSO;
            prog.files = fileList;

            return true;
        }

        THROW_MSG("No support for graphics reload yet.");

        // Add graphics shader reload here if needed

        return false;
    } catch (const std::exception& e) {
        logERROR("Failed to reload shader {}: {}", prog.filename, e.what());
        return false;
    }
}

void CommandListContext::clearRenderTarget(ID3D12DescriptorHeap* descHeap,
                                           F32 r, F32 g, F32 b, F32 a,
                                           U32 slotIndex) {
// Verify it's actually an RTV heap:
#ifdef _DEBUG
    NVCHK(descHeap != nullptr, "Invalid desc heap.");
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = descHeap->GetDesc();
    if (heapDesc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
        THROW_MSG(
            "clearRenderTarget requires an RTV descriptor heap, got type {}",
            static_cast<int>(heapDesc.Type));
    }
    if (slotIndex >= heapDesc.NumDescriptors) {
        THROW_MSG("slotIndex {} out of bounds for heap with {} descriptors",
                  slotIndex, heapDesc.NumDescriptors);
    }
#endif

    // Clear with the specified color
    const F32 clearColor[4] = {r, g, b, a};

    D3D12_CPU_DESCRIPTOR_HANDLE descHandle =
        descHeap->GetCPUDescriptorHandleForHeapStart();
    auto stride = eng.device()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    descHandle.ptr += (size_t)slotIndex * stride;

    cmdList->ClearRenderTargetView(descHandle, clearColor, 0, nullptr);
}

auto DX12Engine::createViewHeap(U32 numDescriptors, bool shaderVisible)
    -> ComPtr<ID3D12DescriptorHeap> {
    return createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                numDescriptors, shaderVisible);
}

auto DX12Engine::createRTVHeap(U32 numDescriptors)
    -> ComPtr<ID3D12DescriptorHeap> {
    // RTVs are never shader visible
    return createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numDescriptors,
                                false);
}

auto DX12Engine::createDSVHeap(U32 numDescriptors)
    -> ComPtr<ID3D12DescriptorHeap> {
    return createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, numDescriptors,
                                false);
}

void DX12Engine::createRenderTargetView(ID3D12DescriptorHeap* descHeap,
                                        ID3D12Resource* texture,
                                        U32 slotIndex) {
    D3D12_CPU_DESCRIPTOR_HANDLE descHandle =
        descHeap->GetCPUDescriptorHandleForHeapStart();
    auto stride = _device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    descHandle.ptr += (size_t)slotIndex * stride;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texture->GetDesc().Format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    _device->CreateRenderTargetView(texture, &rtvDesc, descHandle);
}

auto DX12Engine::createTexture2D(U32 width, U32 height,
                                 D3D12_RESOURCE_FLAGS resourceFlags,
                                 DXGI_FORMAT format,
                                 D3D12_RESOURCE_STATES initialState,
                                 D3D12_HEAP_TYPE heapType)
    -> ComPtr<ID3D12Resource> {

    // Setup heap properties
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 0;
    heapProps.VisibleNodeMask = 0;

    // Setup resource description
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = resourceFlags;

    // Setup clear value if it's a render target or depth stencil
    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    D3D12_CLEAR_VALUE clearValue = {};

    if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
        clearValue.Format = format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClearValue = &clearValue;
    } else if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        clearValue.Format = format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
    }

    // Create the resource
    ComPtr<ID3D12Resource> texture;
    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
        pClearValue, IID_PPV_ARGS(texture.GetAddressOf()));

    if (FAILED(hr)) {
        THROW_MSG("Failed to create Texture2D: width={}, height={}, format={}, "
                  "hr=0x{:X}",
                  width, height, static_cast<int>(format),
                  static_cast<unsigned int>(hr));
    }

    // Track the initial state
    setCurrentState(texture.Get(), initialState);

    return texture;
}

auto DX12Engine::getCmdList(I32 idx) -> CommandListContext& {
    if (idx < 0) {
        idx = (I32)_currentCmdListIndex;
    }
    NVCHK(idx < _cmdListPool.size(), "Out of range command list index {}", idx);
    return _cmdListPool[idx];
}

auto DX12Engine::beginCmdList() -> CommandListContext& {
    // Get a free command list if any:
    U64 completedValue = _fence->GetCompletedValue();

    for (auto& ctx : _cmdListPool) {
        if (!ctx.isRecording && ctx.fenceValue <= completedValue) {
            HRESULT hr = ctx.allocator->Reset();
            NVCHK(SUCCEEDED(hr), "Failed to reset command allocator.");

            // Reset the command list:
            hr = ctx.cmdList->Reset(ctx.allocator.Get(), nullptr);
            NVCHK(SUCCEEDED(hr), "Failed to reset command list.");

            ctx.isRecording = true;
            return ctx;
        }
    }

    // No free command list available, add a new one:
    auto& ctx = _cmdListPool.emplace_back(CommandListContext{.eng = *this});

    HRESULT hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(&ctx.allocator));
    NVCHK(SUCCEEDED(hr), "Failed to create command allocator.");

    // Create command list
    hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    ctx.allocator.Get(), nullptr,
                                    IID_PPV_ARGS(&ctx.cmdList));
    NVCHK(SUCCEEDED(hr), "Failed to create command list.");

    ctx.index = _cmdListPool.size() - 1;
    logDEBUG("Creating DX12 command list {}", ctx.index);

    ctx.isRecording = true;
    ctx.fenceValue = 0;
    return ctx;
}

void DX12RootSig::addRootCBV(U32 reg, U32 space, U32 visibility) {
    if (_rootSignature != nullptr) {
        logWARN("Resetting root signature.");
        _rootSignature = nullptr;
    }
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = reg;
    param.Descriptor.RegisterSpace = space;
    param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)visibility;
    _rootParams.emplace_back(param);
}

void DX12RootSig::addRootSRVs(U32 num, U32 reg, U32 space, U32 visibility,
                              U32 offset) {
    auto* ptr =
        _descRanges.emplace_back(std::make_unique<D3D12_DESCRIPTOR_RANGE>())
            .get();

    ptr->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ptr->NumDescriptors = num;
    ptr->BaseShaderRegister = reg;
    ptr->RegisterSpace = space;
    ptr->OffsetInDescriptorsFromTableStart = offset;

    D3D12_ROOT_PARAMETER param{};

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = ptr;
    param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)visibility;
    _rootParams.emplace_back(param);
};

void DX12RootSig::addRootUAVs(U32 num, U32 reg, U32 space, U32 visibility,
                              U32 offset) {
    auto* ptr =
        _descRanges.emplace_back(std::make_unique<D3D12_DESCRIPTOR_RANGE>())
            .get();

    ptr->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ptr->NumDescriptors = num;
    ptr->BaseShaderRegister = reg;
    ptr->RegisterSpace = space;
    ptr->OffsetInDescriptorsFromTableStart = offset;

    D3D12_ROOT_PARAMETER param{};

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = ptr;
    param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)visibility;
    _rootParams.emplace_back(param);
};

auto DX12RootSig::getSignature() -> ComPtr<ID3D12RootSignature> {
    if (_rootSignature != nullptr) {
        return _rootSignature;
    }

    NVCHK(!_rootParams.empty(), "No root parameter description provided.");
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _rootParams.size();
    rootSigDesc.pParameters = _rootParams.data();
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(),
        error.GetAddressOf());
    if (FAILED(hr)) {
        if (error != nullptr) {
            std::string errorMsg =
                static_cast<const char*>(error->GetBufferPointer());
            THROW_MSG("Root signature serialization failed: {}", errorMsg);
        }
        THROW_MSG("Root signature serialization failed with HRESULT: 0x{:X}",
                  static_cast<unsigned int>(hr));
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = _eng.device()->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(rootSig.GetAddressOf()));

    if (FAILED(hr)) {
        THROW_MSG("Failed to create root signature: 0x{:X}",
                  static_cast<unsigned int>(hr));
    }

    return rootSig;
}

DX12RootSig::DX12RootSig(DX12Engine& engine) : _eng(engine) {};
auto DX12Engine::makeRootSig() -> DX12RootSig { return DX12RootSig{*this}; }

void DX12Engine::writeBuffer(ID3D12Resource* buffer, const void* data,
                             U32 size) {
    auto& buf = getUploadBuffer(size);

    // Copy data to upload buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, 0}; // We don't intend to read from this buffer
    buf.buffer->Map(0, &readRange, &mappedData);
    memcpy(mappedData, data, size);
    buf.buffer->Unmap(0, nullptr);

    // Create command list for copy
    auto& ctx = beginCmdList();

    // Transition buffer to copy destination
    ctx.addCopyDstTransition(buffer);

    // Copy from upload buffer to destination buffer
    ctx.cmdList->CopyBufferRegion(buffer, 0, buf.buffer.Get(), 0, size);

    // Transition buffer to common state (or appropriate state for reading)
    ctx.addCommonTransition(buffer);

    executeCommands(ctx);

    buf.fenceValue = _fenceValue;

    // Mark the upload buffer as not in use anymore:
    buf.inUse = false;
}

void DX12Engine::readBuffer(ID3D12Resource* readbackBuffer, void* destData,
                            U32 size) {
    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, size};

    HRESULT hr = readbackBuffer->Map(0, &readRange, &mappedData);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map readback buffer");
    }

    memcpy(destData, mappedData, size);

    D3D12_RANGE writeRange = {0, 0}; // We didn't write anything
    readbackBuffer->Unmap(0, &writeRange);
}

auto DX12Engine::getUploadBuffer(U32 requiredSize) -> UploadBuffer& {
    U64 completedFenceValue = _fence->GetCompletedValue();

    // First, try to find an existing buffer that's large enough and not in use
    for (auto& ub : _uploadBufferPool) {
        // Check if this buffer's GPU work has completed
        if (!ub.inUse && ub.fenceValue <= completedFenceValue &&
            ub.size >= requiredSize) {
            ub.inUse = true;
            return ub;
        }
    }

    // No suitable buffer found, create a new one
    U32 bufferSize = std::max(requiredSize, _minUploadBufferSize);

    UploadBuffer newBuffer;
    newBuffer.size = bufferSize;
    newBuffer.inUse = true;
    newBuffer.fenceValue = _fence->GetCompletedValue();

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = _device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(newBuffer.buffer.GetAddressOf()));

    NVCHK(SUCCEEDED(hr), "Failed to create upload buffer");

    _uploadBufferPool.push_back(std::move(newBuffer));
    return _uploadBufferPool.back();
};

} // namespace nv

#endif
