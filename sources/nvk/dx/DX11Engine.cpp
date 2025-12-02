#ifdef _WIN32

#include <nvk/dx/DX11Engine.h>

#include <d3d11_1.h> // For ID3D11Device1

// #include <dxgi1_2.h>
// #include <fstream>

// #include <chrono>
// #include <filesystem>
// #include <random>
// #include <regex>

#include <external/stb/stb_image.h>
#include <external/stb/stb_image_write.h>

using namespace DirectX;

#define DX11_USE_INCLUDE_HANDLER 1

namespace nv {

#if DX11_USE_INCLUDE_HANDLER
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

auto DX11Engine::instance(ID3D11Device* device) -> DX11Engine& {
    static std::unique_ptr<DX11Engine> singleton;
    if (singleton == nullptr) {
        logDEBUG("Creating DX11Engine.");
        singleton.reset(new DX11Engine(device));
    }
    return *singleton;
}

auto DX11Engine::get(ID3D11Device* device) -> DX11Engine& {
    // Check if the device is the one used for the instance:
    auto& inst = instance(device);
    if (inst.device() == device) {
        return inst;
    }

    // Otherwise check the auxiliary engines:
    static std::unordered_map<ID3D11Device*, std::unique_ptr<DX11Engine>>
        engineMap;

    auto it = engineMap.find(device);
    if (it != engineMap.end()) {
        return *it->second;
    }

    // Create a new device:
    auto res = engineMap.insert(std::make_pair(device, new DX11Engine(device)));
    NVCHK(res.second, "Could not insert new dx11 engine.");
    return *res.first->second;
}

DX11Engine::DX11Engine(ID3D11Device* device) : _device(device) {
    if (_device == nullptr) {
        logDEBUG("DX11Engine: allocating dedicated DX11 device.");

        UINT createDeviceFlags = 0;
#ifndef NDEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1};
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                          createDeviceFlags, featureLevels, 1,
                          D3D11_SDK_VERSION, &_device, nullptr, &_context);
    } else {
        _device->GetImmediateContext(&_context);
    }
    NVCHK(_device != nullptr, "Cannot create DX11 device.");
    NVCHK(_context != nullptr, "Cannot retrieve DX11 context.");
}

auto DX11Engine::createDepthStencilView(U32 width, U32 height,
                                        ID3D11ShaderResourceView** outSRV)
    -> ID3D11DepthStencilView* {

    // Create a depth-stencil buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc = {};
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    // depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT; // Compatible for SRV
    depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Compatible for SRV
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags =
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // Needed for SRV
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    // Create the depth-stencil texture
    ID3D11Texture2D* depthStencilTexture = nullptr;
    CHECK_RESULT(_device->CreateTexture2D(&depthStencilDesc, nullptr,
                                          &depthStencilTexture),
                 "Cannot create depthstencil texture.");

    // Create the depth-stencil view (DSV)
    ID3D11DepthStencilView* depthStencilView = nullptr;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    CHECK_RESULT(_device->CreateDepthStencilView(depthStencilTexture, &dsvDesc,
                                                 &depthStencilView),
                 "Cannot create depthstencil view");

    // If the caller wants an SRV, create it
    if (outSRV != nullptr) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // Required for depth reading
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        CHECK_RESULT(
            _device->CreateShaderResourceView(depthStencilTexture, &srvDesc,
                                              outSRV),
            "Cannot create shader resource view for depthstencil buffer.");
    }

    SAFERELEASE(depthStencilTexture);
    return depthStencilView;
}

auto DX11Engine::createDepthStencilState(bool depthEnabled)
    -> ID3D11DepthStencilState* {

    ID3D11DepthStencilState* depthStencilState{nullptr};
    D3D11_DEPTH_STENCIL_DESC depthStateDesc;
    ZeroMemory(&depthStateDesc, sizeof(depthStateDesc));

    depthStateDesc.DepthEnable = depthEnabled ? TRUE : FALSE;
    depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;

    depthStateDesc.StencilEnable = FALSE;

    CHECK_RESULT(
        _device->CreateDepthStencilState(&depthStateDesc, &depthStencilState),
        "Cannot create depthStencilState");
    return depthStencilState;
};

auto DX11Engine::createBlendState(bool enabled) -> ID3D11BlendState* {
    // cf.
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d11/d3d10-graphics-programming-guide-blend-state

    D3D11_BLEND_DESC blendStateDesc;
    ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
    blendStateDesc.AlphaToCoverageEnable = FALSE;
    blendStateDesc.IndependentBlendEnable = FALSE;
    blendStateDesc.RenderTarget[0].BlendEnable = enabled ? TRUE : FALSE;
    blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
    blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* blendState = nullptr;
    CHECK_RESULT(_device->CreateBlendState(&blendStateDesc, &blendState),
                 "Cannot create blend state");
    return blendState;
}

auto DX11Engine::updateProgram(DX11Program& prog) -> bool {
    std::unordered_set<std::string> fileList{prog.filename};

    auto source = readShaderFile(prog.filename, fileList);
    if (prog.isCompute) {
        auto* computeShader = createComputeShader(source, prog.filename);
        if (computeShader == nullptr) {
            // Could not load the new DX11program:
            return false;
        }

        // Otherwise we keep updating the structure:
        prog.computeShader = computeShader;
    } else {
        auto* vertexShader = createVertexShader(
            source, prog.filename, prog.inputDesc, &prog.inputLayout);
        auto* pixelShader = createPixelShader(source, prog.filename);

        if (vertexShader == nullptr || pixelShader == nullptr) {
            // Could not load the new program:
            return false;
        }

        // Otherwise we keep updating the structure:
        prog.vertexShader = vertexShader;
        prog.pixelShader = pixelShader;
    }

    prog.files.swap(fileList);
    return true;
}

auto DX11Engine::createProgram(const std::string& filename,
                               const DX11InputLayoutDesc& desc) -> DX11Program {
    DX11Program res;

    res.filename = filename;
    res.inputDesc = desc;

    if (!updateProgram(res)) {
        THROW_MSG("Could not create HLSL program from {}", res.filename);
    }

    res.lastCheckTime = std::time(nullptr);
    res.lastUpdateTime = res.lastCheckTime;

    return res;
}

auto DX11Engine::createComputeProgram(const std::string& filename)
    -> DX11Program {
    DX11Program prog;
    prog.filename = filename;
    prog.isCompute = true;

    if (!updateProgram(prog)) {
        THROW_MSG("Could not create HLSL program from {}", prog.filename);
    }

    prog.lastCheckTime = std::time(nullptr);
    prog.lastUpdateTime = prog.lastCheckTime;

    return prog;
}

auto DX11Engine::readShaderFile(const std::string& filename,
                                std::unordered_set<std::string>& fileList)
    -> std::string {
    // Open the file
    String content = read_virtual_file(filename);

    // std::ifstream fileStream(filename);
    // if (!fileStream.is_open()) {
    //     THROW_MSG("Could not open file: {}", filename);
    // }

    // // Use a stringstream to read the file content into a string
    // std::stringstream buffer;
    // buffer << fileStream.rdbuf();

    // // Close the file (handled by destructor of ifstream, but explicitly for
    // // clarity)
    // fileStream.close();

    // std::string content = buffer.str();

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
        } else {
            // logDEBUG("Shader file " << full_path << " was already
            // inserted.");
        }

        // Move past the current match
        searchStart = match.suffix().first;
    }

    // Append remaining source content after the last include
    processedSource.append(searchStart, content.cend());

#if DX11_USE_INCLUDE_HANDLER
    return content;
#else
    // Return the content of the file as a string
    return processedSource;
#endif
}

// Method used to create a Vertex Shader from a string:
auto DX11Engine::createVertexShader(
    const std::string& source, const std::string& hint,
    const D3D11_INPUT_ELEMENT_DESC* polygonLayout, uint32_t numElements,
    ID3D11InputLayout** pLayout, const std::string& funcName,
    const std::string& profile) -> ID3D11VertexShader* {

    // First we should compile the blob:
    ID3DBlob* buffer = compileShaderSource(source, hint, funcName, profile);
    if (buffer == nullptr) {
        logDEBUG("ERROR: Cannot compiler vertex shader from source.");
        return nullptr;
    }

    ID3D11VertexShader* shader = nullptr;
    HRESULT result = _device->CreateVertexShader(
        buffer->GetBufferPointer(), buffer->GetBufferSize(), nullptr, &shader);

    // Also check here if we should create a layout:
    if ((polygonLayout != nullptr) && numElements > 0 && pLayout != nullptr) {
        HRESULT res2 = _device->CreateInputLayout(
            polygonLayout, numElements, buffer->GetBufferPointer(),
            buffer->GetBufferSize(), pLayout);
        CHECK_RESULT(res2, "Cannot create input layout.");
        NVCHK(pLayout != nullptr, "Invalid input layout.");
    }

    SAFERELEASE(buffer);
    if (FAILED(result)) {
        logDEBUG("ERROR: Cannot create vertex shader from buffer.");
        return nullptr;
    }

    return shader;
};

// Method used to create a Pixel Shader from a string:
auto DX11Engine::createPixelShader(const std::string& source,
                                   const std::string& hint,
                                   const std::string& funcName,
                                   const std::string& profile)
    -> ID3D11PixelShader* {

    // First we should compile the blob:
    ID3DBlob* buffer = compileShaderSource(source, hint, funcName, profile);
    if (buffer == nullptr) {
        logDEBUG("ERROR: Cannot compiler Pixel shader from source.");
        return nullptr;
    }

    ID3D11PixelShader* shader = nullptr;
    HRESULT result = _device->CreatePixelShader(
        buffer->GetBufferPointer(), buffer->GetBufferSize(), nullptr, &shader);
    SAFERELEASE(buffer);

    if (FAILED(result)) {
        logDEBUG("ERROR: Cannot create Pixel shader from buffer.");
        return nullptr;
    }

    return shader;
};

auto DX11Engine::createComputeShader(const std::string& source,
                                     const std::string& hint,
                                     const std::string& funcName,
                                     const std::string& profile)
    -> ID3D11ComputeShader* {

    // First we should compile the blob:
    ID3DBlob* buffer = compileShaderSource(source, hint, funcName, profile);
    if (buffer == nullptr) {
        logDEBUG("ERROR: Cannot compiler compute shader from source.");
        return nullptr;
    }

    ID3D11ComputeShader* shader = nullptr;
    HRESULT result = _device->CreateComputeShader(
        buffer->GetBufferPointer(), buffer->GetBufferSize(), nullptr, &shader);
    SAFERELEASE(buffer);

    if (FAILED(result)) {
        logDEBUG("ERROR: Cannot create Compute shader from buffer.");
        return nullptr;
    }

    return shader;
};

// cf.
// https://stackoverflow.com/questions/50155656/creating-dx11-shaders-from-string
auto DX11Engine::compileShaderSource(const std::string& source,
                                     const std::string& hint,
                                     const std::string& funcName,
                                     const std::string& profile) -> ID3DBlob* {
    ID3DBlob* blob = nullptr;
    ID3DBlob* pErrorMsgs = nullptr;
    ID3DInclude* pInclude = nullptr;
#if DX11_USE_INCLUDE_HANDLER
    ShaderIncludeHandler includeHandler(_shaderIncludeDir);
    pInclude = &includeHandler;
#endif

    HRESULT result = D3DCompile(source.data(), source.size(), nullptr, nullptr,
                                pInclude, funcName.c_str(), profile.c_str(),
                                D3DCOMPILE_ENABLE_STRICTNESS, 0, // 0, 0,
                                &blob, &pErrorMsgs);

    if (pErrorMsgs != nullptr) {
        logDEBUG("ERROR: Shader compilation errors in {}:\n{}", hint,
                 (char*)pErrorMsgs->GetBufferPointer());
        SAFERELEASE(pErrorMsgs);
    }

    if (FAILED(result)) {
        logDEBUG("ERROR: Invalid shader code in source string.");
        return nullptr;
    }

    // Otherwise we return the blob:
    return blob;
};

auto DX11Engine::createVertexBuffer(const void* data, U32 size)
    -> ID3D11Buffer* {
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = size;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = data;
    ID3D11Buffer* vertexBuffer = nullptr;
    CHECK_RESULT(_device->CreateBuffer(&bd, &InitData, &vertexBuffer),
                 "Cannot create vertex buffer");
    return vertexBuffer;
}

auto DX11Engine::createIndexBuffer(const void* data, U32 size)
    -> ID3D11Buffer* {
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = size;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = data;
    ID3D11Buffer* indexBuffer = nullptr;
    CHECK_RESULT(_device->CreateBuffer(&bd, &InitData, &indexBuffer),
                 "Cannot create index buffer");
    return indexBuffer;
}

auto DX11Engine::createConstantBuffer(U32 size) -> ID3D11Buffer* {
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = size;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    ID3D11Buffer* buf = nullptr;
    CHECK_RESULT(_device->CreateBuffer(&bd, nullptr, &buf),
                 "Cannot create constant buffer");
    return buf;
}

auto DX11Engine::createBuffer(U32 elemSize, U32 numElems, D3D11_USAGE usage,
                              U32 bindFlags, U32 cpuFlags) -> ID3D11Buffer* {
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.ByteWidth = elemSize * numElems;
    bd.Usage = usage;
    bd.BindFlags = bindFlags;
    bd.CPUAccessFlags = cpuFlags;
    bd.StructureByteStride = elemSize;

    ID3D11Buffer* buf = nullptr;
    CHECK_RESULT(_device->CreateBuffer(&bd, nullptr, &buf),
                 "Cannot create buffer");
    return buf;
}

void DX11Engine::setViewport(U32 width, U32 height) {
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0F;
    vp.MaxDepth = 1.0F;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _context->RSSetViewports(1, &vp);
}

void DX11Engine::setDepthStencilState(ID3D11DepthStencilState* state,
                                      U32 stencilRef) {
    _context->OMSetDepthStencilState(state, stencilRef);
}

void DX11Engine::check_live_reload(DX11Program& prog) {

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

void DX11Engine::setProgram(const DX11Program& prog) {
    check_live_reload(const_cast<DX11Program&>(prog));
    if (prog.isCompute) {
        _context->CSSetShader(prog.computeShader, nullptr, 0);
    } else {
        _context->IASetInputLayout(prog.inputLayout);
        _context->VSSetShader(prog.vertexShader, nullptr, 0);
        _context->PSSetShader(prog.pixelShader, nullptr, 0);
    }
}

void DX11Engine::createCube(F32 size, const Vec3f& center,
                            std::vector<SimpleVertex>& vertices,
                            std::vector<U32>& indices) {
    // F32 halfSize = size * 0.5F;
    F32 x = center.x();
    F32 y = center.y();
    F32 z = center.z();

    std::vector<SimpleVertex> verts{
        {{x - 1.0f, y + 1.0f, z - 1.0f}, {}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{x + 1.0f, y + 1.0f, z - 1.0f}, {}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{x + 1.0f, y + 1.0f, z + 1.0f}, {}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{x - 1.0f, y + 1.0f, z + 1.0f}, {}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{x - 1.0f, y - 1.0f, z - 1.0f}, {}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{x + 1.0f, y - 1.0f, z - 1.0f}, {}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{x + 1.0f, y - 1.0f, z + 1.0f}, {}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{x - 1.0f, y - 1.0f, z + 1.0f}, {}, {0.0f, 0.0f, 0.0f, 1.0f}},
    };

    // Setup the normals:
    for (auto& vert : verts) {
        vert.Normal = (vert.Pos - center).normalized();
    }
    U32 offset = vertices.size();

    for (auto vert : verts) {
        vertices.push_back(vert);
    }

    std::vector<U32> inds{
        3, 1, 0, 2, 1, 3,

        0, 5, 4, 1, 5, 0,

        3, 4, 7, 0, 4, 3,

        1, 6, 5, 2, 6, 1,

        2, 7, 6, 3, 7, 2,

        6, 4, 5, 7, 4, 6,
    };

    for (auto idx : inds) {
        indices.push_back(offset + idx);
    }
}

void DX11Engine::createCubeGrid(U32 gridSize, F32 size, F32 spaceFactor,
                                std::vector<SimpleVertex>& vertices,
                                std::vector<U32>& indices) {
    F32 tsize = (F32)gridSize * size + F32(gridSize - 1) * size * spaceFactor;
    F32 orig = -tsize * 0.5F + size * 0.5F;

    // Create a grid of cubes
    for (I32 r = 0; r < gridSize; ++r) {
        for (I32 c = 0; c < gridSize; ++c) {
            createCube(size,
                       {orig + F32(c) * size * (1.0F + spaceFactor), 0.0,
                        orig + F32(r) * size * (1.0F + spaceFactor)},
                       vertices, indices);
        }
    }
}

void DX11Engine::setVertexBuffer(ID3D11Buffer* buffer, U32 stride) {
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
}

void DX11Engine::setIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT fmt) {
    _context->IASetIndexBuffer(buffer, fmt, 0);
}

void DX11Engine::setTriangleList() {
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
auto DX11Engine::buildViewMatrix(const Vec3f& position, const Vec3f& forward,
                                 const Vec3f& up) -> XMMATRIX {
    // Create XMVECTORs from the input vectors
    auto pos = toDX(position);
    auto fwd = toDX(forward);
    auto up2 = toDX(up);

    XMVECTOR camPosition = XMVectorSet(pos.x(), pos.y(), pos.z(), 1.0);
    XMVECTOR camForward = XMVectorSet(fwd.x(), fwd.y(), fwd.z(), 0.0);
    XMVECTOR camUp = XMVectorSet(up2.x(), up2.y(), up2.z(), 0.0);

    // Normalize the forward and up vectors
    camForward = XMVector3Normalize(camForward);
    camUp = XMVector3Normalize(camUp);

    // Build the view matrix
    return XMMatrixLookToRH(camPosition, camForward, camUp);
}

auto DX11Engine::invertMatrix(const XMMATRIX& matrix) -> XMMATRIX {
    // Calculate the inverse of the matrix
    return XMMatrixInverse(nullptr, matrix);
}

auto DX11Engine::buildPerspectiveProjectionMatrix(F32 fovYRadians,
                                                  F32 aspectRatio, F32 nearZ,
                                                  F32 farZ) -> XMMATRIX {
    // Calculate the aspect ratio
    // F32 fovY = 2.0f * atanf(tanf(fovRadians * 0.5f) / aspectRatio);

    // Build the perspective projection matrix
    return XMMatrixPerspectiveFovRH(fovYRadians, aspectRatio, nearZ, farZ);
}

auto DX11Engine::createRasterState(D3D11_CULL_MODE mode, bool frontccw)
    -> ID3D11RasterizerState* {

    // create our raster state:
    D3D11_RASTERIZER_DESC rDesc;
    ZeroMemory(&rDesc, sizeof(D3D11_RASTERIZER_DESC));
    rDesc.FillMode = D3D11_FILL_SOLID;
    rDesc.CullMode = mode;
    rDesc.FrontCounterClockwise = frontccw ? TRUE : FALSE;
    rDesc.DepthBias = 0;
    rDesc.DepthBiasClamp = 0.0;
    rDesc.SlopeScaledDepthBias = 0.0;
    rDesc.DepthClipEnable = FALSE;
    rDesc.ScissorEnable = FALSE;
    rDesc.MultisampleEnable = FALSE;
    rDesc.AntialiasedLineEnable = FALSE;

    ID3D11RasterizerState* rasterState = nullptr;
    CHECK_RESULT(_device->CreateRasterizerState(&rDesc, &rasterState),
                 "Cannot create raster state");
    return rasterState;
}
auto DX11Engine::createLinearWrapSamplerState() -> ID3D11SamplerState* {
    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0F;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the texture sampler state.
    ID3D11SamplerState* samplerState = nullptr;
    CHECK_RESULT(_device->CreateSamplerState(&samplerDesc, &samplerState),
                 "Cannot create SamplerState");
    return samplerState;
}

auto DX11Engine::createTextureCube(const std::string& folder,
                                   const std::vector<std::string>& filenames)
    -> ID3D11ShaderResourceView* {

    NVCHK(filenames.size() == 6, "Invalid number of images.");

    // Array to hold image data for each face
    std::vector<std::vector<unsigned char>> imageData(6);

    int width = 0;
    int height = 0;

    // Load and process each image
    for (int i = 0; i < 6; ++i) {
        int channels = 0;
        // Read from the full path:
        std::string full_path = folder + "/" + filenames[i];
        unsigned char* data = stbi_load(full_path.c_str(), &width, &height,
                                        &channels, 4); // Force RGBA
        if (data == nullptr) {
            // Handle error
            return nullptr;
        }

        // Store the image data
        imageData[i].assign(data, data + width * height * 4);
        stbi_image_free(data);

        // Ensure all images are the same size
        if (i > 0 &&
            (width != height ||
             width != static_cast<int>(imageData[0].size()) / (4 * height))) {
            // Handle error: images are not the same size or not square
            THROW_MSG("Mismatch in cubemap images dimensions");
        }
    }

    // Create the texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 6;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    ID3D11Texture2D* cubeTexture = nullptr;
    D3D11_SUBRESOURCE_DATA initData[6];
    for (int i = 0; i < 6; ++i) {
        initData[i].pSysMem = imageData[i].data();
        initData[i].SysMemPitch = width * 4;
        initData[i].SysMemSlicePitch = 0;
    }

    CHECK_RESULT(_device->CreateTexture2D(&texDesc, initData, &cubeTexture),
                 "Cannot create cube texture");

    // Create the shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;

    ID3D11ShaderResourceView* cubeMapSRV = nullptr;
    CHECK_RESULT(
        _device->CreateShaderResourceView(cubeTexture, &srvDesc, &cubeMapSRV),
        "Cannot create shader resource");

    // Release the texture as we don't need it anymore
    cubeTexture->Release();

    return cubeMapSRV;
}

void DX11Engine::applyState(DX11State& state) {
    if (!state.program.isCompute) {
        _context->IASetPrimitiveTopology(state.topology);
        _context->OMSetDepthStencilState(state.depthStencil, state.stencilRef);
        _context->OMSetBlendState(state.blend, nullptr, 0xFFFFFF);
        _context->PSSetSamplers(0, 1, &state.sampler);
        _context->RSSetState(state.raster);
    }

    setProgram(state.program);
}

auto DX11Engine::createTexture2D(U32 width, U32 height, U32 bindFlags,
                                 DXGI_FORMAT format, U32 miscFlags, U32 usage)
    -> ID3D11Texture2D* {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = (D3D11_USAGE)usage;
    textureDesc.BindFlags = bindFlags;
    textureDesc.MiscFlags = miscFlags;
    ID3D11Texture2D* tex = nullptr;

    _device->CreateTexture2D(&textureDesc, nullptr, &tex);
    NVCHK(tex != nullptr, "Cannot create texture 2d.");
    return tex;
}

auto DX11Engine::createSharedTexture2D(HANDLE* sharedHandle, U32 width,
                                       U32 height, U32 bindFlags,
                                       DXGI_FORMAT format, bool nthandle,
                                       bool keyedMutex, DWORD access)
    -> ComPtr<ID3D11Texture2D> {
    U32 flags = keyedMutex ? D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
                           : D3D11_RESOURCE_MISC_SHARED;
    if (nthandle) {
        flags |= D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    }

    auto* tex = createTexture2D(width, height, bindFlags, format, flags);
    NVCHK(tex != nullptr, "Cannot create shared texture 2d.");

    if (nthandle) {
        // 2. Get the shared handle (NT handle)
        IDXGIResource1* dxgiResource1 = nullptr;
        tex->QueryInterface(__uuidof(IDXGIResource1), (void**)&dxgiResource1);
        NVCHK(dxgiResource1 != nullptr,
              "Cannot retrieve IDXGIResource1 interface.");

        CHECK_RESULT(
            dxgiResource1->CreateSharedHandle(nullptr, // security attributes
                                              access,
                                              nullptr, // name
                                              sharedHandle),
            "Cannot create shared handle.");
        SAFERELEASE(dxgiResource1);
    } else {
        IDXGIResource* pDXGIResource = nullptr;
        CHECK_RESULT(
            tex->QueryInterface(__uuidof(IDXGIResource),
                                (void**)&pDXGIResource),
            "Cannot retrieve DXGIResource interface from texture object.");

        CHECK_RESULT(pDXGIResource->GetSharedHandle(sharedHandle),
                     "Cannot retrieve shared handle from DXGI resource.");

        // Check that the shared texture creation worked:
        NVCHK(sharedHandle, "Invalid sharedHandle for shared texture.");
        SAFERELEASE(pDXGIResource);
    }

    return tex;
}

void DX11Engine::setShaderIncludeDir(const std::string& dir) {
    _shaderIncludeDir = dir;
}

auto DX11Engine::getCurrentTime() -> double {
    static const auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();

    // Use std::chrono::duration<double> for proper conversion to seconds
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

auto DX11Engine::gen_f32(F32 mini, F32 maxi) -> F32 {
    std::uniform_real_distribution<F32> dis(mini, maxi);
    return dis(_gen);
}

auto DX11Engine::createTexture2DFromSharedHandle(HANDLE handle)
    -> ID3D11Texture2D* {
    NVCHK(handle != nullptr,
          "createTexture2DFromSharedHandle: Invalid shared handle.");

    // Open the shared resource in DX11
    ID3D11Texture2D* d3d11Texture = nullptr;
    CHECK_RESULT(_device->OpenSharedResource(handle, __uuidof(ID3D11Texture2D),
                                             (void**)&d3d11Texture),
                 "Cannot open shared resource in DX11.");

    return d3d11Texture;
}

auto DX11Engine::getKeyedMutex(ID3D11Texture2D* texture)
    -> ComPtr<IDXGIKeyedMutex> {
    NVCHK(texture != nullptr, "getKeyedMutex: invalid texture.");
    IDXGIKeyedMutex* keyedMutex = nullptr;
    CHECK_RESULT(
        texture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex),
        "Cannot retrieve texture2D keyed mutex.");
    NVCHK(keyedMutex != nullptr, "Invalid KeyedMutex interface");

    return keyedMutex;
}

auto DX11Engine::initializeStagingTexture2d(UINT width, UINT height,
                                            DXGI_FORMAT format) -> bool {
    // Clean up existing staging texture
    if (_stagingTexture2d != nullptr) {
        _stagingTexture2d->Release();
        _stagingTexture2d = nullptr;
    }

    // Create staging texture description
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = width;
    stagingDesc.Height = height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = format;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    // Create the staging texture
    CHECK_RESULT(
        _device->CreateTexture2D(&stagingDesc, nullptr, &_stagingTexture2d),
        "Cannot create staging texture");

    _stagingWidth = width;
    _stagingHeight = height;
    _stagingFormat = format;

    return true;
}

auto DX11Engine::saveTextureToFile(ID3D11Texture2D* sourceTexture,
                                   const char* filename) -> bool {
    if ((_device == nullptr) || (_context == nullptr) ||
        (sourceTexture == nullptr)) {
        THROW_MSG("saveTextureToFile: Invalid source texture.");
    }

    // Get source texture description
    D3D11_TEXTURE2D_DESC sourceDesc;
    sourceTexture->GetDesc(&sourceDesc);

    // Initialize staging texture if needed
    if (_stagingWidth != sourceDesc.Width ||
        _stagingHeight != sourceDesc.Height ||
        _stagingFormat != sourceDesc.Format) {

        NVCHK(initializeStagingTexture2d(sourceDesc.Width, sourceDesc.Height,
                                         sourceDesc.Format),
              "Cannot init stagin texture.");
    }

    // Copy source texture to staging texture
    _context->CopyResource(_stagingTexture2d, sourceTexture);

    // Map the staging texture for reading
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    CHECK_RESULT(
        _context->Map(_stagingTexture2d, 0, D3D11_MAP_READ, 0, &mappedResource),
        "Cannot map staging texture 2d.");

    // Convert and save the image
    bool success = convertAndSaveImage(mappedResource, filename);

    // Unmap the staging texture
    _context->Unmap(_stagingTexture2d, 0);

    return success;
}

auto DX11Engine::convertAndSaveImage(
    const D3D11_MAPPED_SUBRESOURCE& mappedResource, const char* filename)
    -> bool {
    if (_stagingFormat != DXGI_FORMAT_R10G10B10A2_UNORM) {
        // For now, only handle R10G10B10A2_UNORM format
        THROW_MSG("convertAndSaveImage: only format "
                  "DXGI_FORMAT_R10G10B10A2_UNORM supported for now.");
        return false;
    }

    // Prepare image data buffer
    std::vector<unsigned char> imageData(_stagingWidth * _stagingHeight *
                                         4); // RGBA

    // Convert pixel data
    const uint32_t* srcData =
        static_cast<const uint32_t*>(mappedResource.pData);
    UINT srcRowPitch =
        mappedResource.RowPitch / 4; // Convert to uint32_t stride

    for (UINT y = 0; y < _stagingHeight; ++y) {
        for (UINT x = 0; x < _stagingWidth; ++x) {
            UINT srcIndex = y * srcRowPitch + x;
            UINT dstIndex = (y * _stagingWidth + x) * 4;

            // Unpack R10G10B10A2 format (assuming BGR order)
            uint32_t pixel = srcData[srcIndex];

            uint32_t r = (pixel >> 0) & 0x3FF;  // Lower 10 bits - Red
            uint32_t g = (pixel >> 10) & 0x3FF; // Next 10 bits - Green
            uint32_t b = (pixel >> 20) & 0x3FF; // Next 10 bits - Blue
            // uint32_t a = (pixel >> 30) & 0x3;   // Upper 2 bits - Alpha

            // Convert to 8-bit
            imageData[dstIndex + 0] =
                static_cast<unsigned char>((r * 255) / 1023); // R
            imageData[dstIndex + 1] =
                static_cast<unsigned char>((g * 255) / 1023); // G
            imageData[dstIndex + 2] =
                static_cast<unsigned char>((b * 255) / 1023); // B
            imageData[dstIndex + 3] = 255; // Force alpha to 255 (opaque)
        }
    }

    // Save the image using stb_image
    return stbi_write_png(filename, _stagingWidth, _stagingHeight, 4,
                          imageData.data(), _stagingWidth * 4) != 0;
}

void DX11Engine::printGPUInfos() {
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    // Get the DXGI device from the D3D11 device
    CHECK_RESULT(_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)),
                 "Failed to get DXGI device from D3D11 device");

    // Get the adapter from the DXGI device
    CHECK_RESULT(dxgiDevice->GetAdapter(
                     reinterpret_cast<IDXGIAdapter**>(adapter.GetAddressOf())),
                 "Failed to get adapter from DXGI device");

    DXGI_ADAPTER_DESC1 desc;
    CHECK_RESULT(adapter->GetDesc1(&desc), "Failed to get adapter description");

    // Convert wide string to regular string
    std::wstring wDeviceName(desc.Description);
    std::string deviceName = WStringToString(wDeviceName);
    logDEBUG("DX11Engine GPU infos:");
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

auto DX11Engine::createShaderResourceView(ID3D11Texture2D* tex)
    -> ComPtr<ID3D11ShaderResourceView> {
    NVCHK(tex != nullptr, "DX11 create shaderResourceView: Invalid texture.");
    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = desc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view:
    ComPtr<ID3D11ShaderResourceView> srv;
    CHECK_RESULT(
        _device->CreateShaderResourceView(tex, &shaderResourceViewDesc, &srv),
        "Cannot create shader resource view.");

    return srv;
}

auto DX11Engine::createRenderTargetView(ID3D11Texture2D* texture)
    -> ID3D11RenderTargetView* {
    NVCHK(texture != nullptr,
          "Invalid input texture in createRenderTargetView().");

    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    NVCHK(desc.BindFlags & D3D11_BIND_RENDER_TARGET,
          "Missing D3D11_BIND_RENDER_TARGET flag in createRenderTargetView()");

    // We continue with setting up the render target view desc:
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    renderTargetViewDesc.Format = desc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // We create the render target view:
    ID3D11RenderTargetView* renderTargetView = nullptr;
    CHECK_RESULT(_device->CreateRenderTargetView(texture, &renderTargetViewDesc,
                                                 &renderTargetView),
                 "Cannot create render target view");
    return renderTargetView;
};

void DX11Engine::setViewportFromRenderTargetTexture(ID3D11Texture2D* tex) {
    D3D11_TEXTURE2D_DESC desc2;
    tex->GetDesc(&desc2);
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0F;
    viewport.TopLeftY = 0.0F;
    viewport.Width = (F32)desc2.Width;
    viewport.Height = (F32)desc2.Height;
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;

    // We assign the viewport to the context:
    _context->RSSetViewports(1, &viewport);
}

void DX11Engine::setRenderTarget(ID3D11RenderTargetView* rtgt) {
    ComPtr<ID3D11Resource> resource;
    rtgt->GetResource(&resource);

    ComPtr<ID3D11Texture2D> texture;
    if (SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) {
        setViewportFromRenderTargetTexture(texture.Get());
    }

    _context->OMSetRenderTargets(1, &rtgt, nullptr);
}

auto DX11Engine::createReadOnlySharedTexture2D(HANDLE* sharedHandle, U32 width,
                                               U32 height, U32 bindFlags,
                                               DXGI_FORMAT format,
                                               bool nthandle, bool keyedMutex)
    -> ComPtr<ID3D11Texture2D> {
    return createSharedTexture2D(sharedHandle, width, height, bindFlags, format,
                                 nthandle, keyedMutex,
                                 DXGI_SHARED_RESOURCE_READ);
}

void DX11Engine::unbindResources() {
    _context->OMSetRenderTargets(0, nullptr, nullptr);
    // Unbind multiple shader resources at once
    ID3D11ShaderResourceView* nullSRVs[8] = {nullptr};
    _context->PSSetShaderResources(0, 8, nullSRVs);

    // Unbind multiple constant buffers at once
    ID3D11Buffer* nullBuffers[4] = {nullptr};
    _context->PSSetConstantBuffers(0, 4, nullBuffers);
}

auto DX11Engine::createTexture2DFromSharedHandle(HANDLE handle, bool isNTHandle)
    -> ID3D11Texture2D* {
    NVCHK(handle != nullptr,
          "createTexture2DFromSharedHandle: Invalid shared handle.");

    ID3D11Texture2D* d3d11Texture = nullptr;
    HRESULT hr;

    if (isNTHandle) {
        // For NT handles, use OpenSharedResource1
        ID3D11Device1* device1 = nullptr;
        hr = _device->QueryInterface(__uuidof(ID3D11Device1), (void**)&device1);
        if (SUCCEEDED(hr) && (device1 != nullptr)) {
            hr = device1->OpenSharedResource1(handle, __uuidof(ID3D11Texture2D),
                                              (void**)&d3d11Texture);
            SAFERELEASE(device1);
            CHECK_RESULT(hr, "Cannot open shared NT handle resource in DX11.");
        } else {
            NVCHK(false,
                  "ID3D11Device1 interface not available for NT handle.");
        }
    } else {
        // For legacy handles, use OpenSharedResource
        hr = _device->OpenSharedResource(handle, __uuidof(ID3D11Texture2D),
                                         (void**)&d3d11Texture);
        CHECK_RESULT(hr, "Cannot open shared legacy handle resource in DX11.");
    }

    return d3d11Texture;
}

auto acquireKeyedMutex(ComPtr<IDXGIKeyedMutex>& keyedMutex, I32 key) -> bool {
    if (keyedMutex == nullptr) {
        logDEBUG("acquireKeyedMutex: mutex is null.");
        return true;
    }

    // Use 0ms timeout - fail immediately if mutex not available
    HRESULT hr = keyedMutex->AcquireSync(key, 0);
    if (hr == WAIT_TIMEOUT) {
        // Reader hasn't released yet - skip this frame
        // This is normal when window is hidden or app is busy
        // logDEBUG("Failed to acquire keyed mutex with key {} (timeout)", key);
        return false;
    }
    if (FAILED(hr)) {
        _com_error err(hr);
        logWARN("Failed to acquire keyed mutex with key {} (error={})", key,
                err.ErrorMessage());
        return false;
    }

    return true;
}

auto releaseKeyedMutex(ComPtr<IDXGIKeyedMutex>& keyedMutex, I32 key) -> bool {
    if (keyedMutex == nullptr) {
        logDEBUG("releaseKeyedMutex: mutex is null.");
        return true;
    }

    HRESULT hr = keyedMutex->ReleaseSync(key);
    if (FAILED(hr)) {
        _com_error err(hr);
        logWARN("Failed to release keyed mutex with key {} (error={})", key,
                err.ErrorMessage());
        return false;
    }

    return true;
}

} // namespace nv

#endif
