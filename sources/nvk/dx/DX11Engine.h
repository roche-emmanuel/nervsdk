#ifndef DX11ENGINE_H_
#define DX11ENGINE_H_

#include <nvk/dx/dx_common.h>

#include <d3d11.h>

namespace nv {

class DX11InputLayoutDesc {
  public:
    DX11InputLayoutDesc() = default;

    DX11InputLayoutDesc(
        std::initializer_list<std::pair<std::string, DXGI_FORMAT>> elements) {
        for (const auto& element : elements) {
            add(element.first, element.second);
        }
    }

    void add(const std::string& semanticName, DXGI_FORMAT format) {

        names.push_back(semanticName);
        D3D11_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName = nullptr;
        elementDesc.SemanticIndex = 0;
        elementDesc.Format = format;
        elementDesc.InputSlot = 0;
        elementDesc.AlignedByteOffset =
            layout.empty() ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        layout.push_back(elementDesc);
    }

    void add_per_instance(const std::string& semanticName, DXGI_FORMAT format,
                          UINT semIdx = 0, UINT slot = 1) {

        names.push_back(semanticName);
        D3D11_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName = nullptr;
        elementDesc.SemanticIndex = semIdx;
        elementDesc.Format = format;
        elementDesc.InputSlot = slot;
        elementDesc.AlignedByteOffset =
            layout.empty() ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
        elementDesc.InstanceDataStepRate = 1;

        layout.push_back(elementDesc);
    }

    auto data() const -> const D3D11_INPUT_ELEMENT_DESC* {
        // Assign the name pointers here:
        int idx = 0;
        for (auto& desc : layout) {
            desc.SemanticName = names[idx++].c_str();
        }

        return layout.data();
    }

    auto size() const -> UINT { return static_cast<UINT>(layout.size()); }

  private:
    std::vector<std::string> names;
    mutable std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
};

struct DX11Program {
    ID3D11VertexShader* vertexShader{nullptr};
    ID3D11PixelShader* pixelShader{nullptr};
    ID3D11ComputeShader* computeShader{nullptr};
    ID3D11InputLayout* inputLayout{nullptr};
    DX11InputLayoutDesc inputDesc;
    std::string filename;

    //! Time of the last update check:
    std::time_t lastCheckTime{0};

    //! Last time this program was updated.
    std::time_t lastUpdateTime{0};

    //! List of files on which this shader depends:
    std::unordered_set<std::string> files;

    bool isCompute{false};
};

struct SimpleVertex {
    Vec3f Pos;
    Vec3f Normal;
    Vec4f Color;
};

struct DX11State {
    ID3D11DepthStencilState* depthStencil{nullptr};
    ID3D11BlendState* blend{nullptr};
    ID3D11SamplerState* sampler{nullptr};
    ID3D11RasterizerState* raster{nullptr};
    DX11Program program;
    D3D_PRIMITIVE_TOPOLOGY topology{D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST};
    U32 stencilRef{0};
};

/** Helper class handling the rendering part. This should really be what the
backend does on its own, and our BackendInterface below is what we need for
integration into simcore. */
class DX11Engine {
  public:
    static auto instance(ID3D11Device* device = nullptr) -> DX11Engine&;

    // Explicit construction from existing device:
    explicit DX11Engine(ID3D11Device* device);

    // Get the engine for a given device
    static auto get(ID3D11Device* device = nullptr) -> DX11Engine&;

    // Delete copy/move constructors and assignment operators
    DX11Engine(const DX11Engine&) = delete;
    auto operator=(const DX11Engine&) -> DX11Engine& = delete;
    DX11Engine(DX11Engine&&) = delete;
    auto operator=(DX11Engine&&) -> DX11Engine& = delete;

    auto device() -> ID3D11Device* { return _device; }
    auto context() -> ID3D11DeviceContext* { return _context; }

    /** Create a render target view */
    auto createRenderTargetView(ID3D11Texture2D* texture)
        -> ID3D11RenderTargetView*;

    void printGPUInfos();

    /** Create depth stencil view */
    auto createDepthStencilView(U32 width, U32 height,
                                ID3D11ShaderResourceView** outSRV)
        -> ID3D11DepthStencilView*;

    /** Create depth stencil state */
    auto createDepthStencilState(bool depthEnabled) -> ID3D11DepthStencilState*;

    /** Create a blend state */
    auto createBlendState(bool enabled) -> ID3D11BlendState*;

    /** Create linear wrap sampler */
    auto createLinearWrapSamplerState() -> ID3D11SamplerState*;

    /** Create a specific program */
    auto createProgram(const std::string& filename,
                       const DX11InputLayoutDesc& desc) -> DX11Program;

    /** Create a compute program */
    auto createComputeProgram(const std::string& filename) -> DX11Program;

    /** Read shader file content */
    auto readShaderFile(const std::string& filename,
                        std::unordered_set<std::string>& fileList)
        -> std::string;

    /** Create a vertex shader */
    auto createVertexShader(const std::string& source, const std::string& hint,
                            const D3D11_INPUT_ELEMENT_DESC* polygonLayout,
                            uint32_t numElements,
                            ID3D11InputLayout** pLayout = nullptr,
                            const std::string& funcName = "vs_main",
                            const std::string& profile = "vs_5_0")
        -> ID3D11VertexShader*;

    auto createVertexShader(const std::string& source, const std::string& hint,
                            const DX11InputLayoutDesc& desc,
                            ID3D11InputLayout** inputLayout = nullptr)
        -> ID3D11VertexShader* {
        return createVertexShader(source, hint, desc.data(), desc.size(),
                                  inputLayout);
    }

    /** Create a pixel shader */
    auto createPixelShader(const std::string& source, const std::string& hint,
                           const std::string& funcName = "ps_main",
                           const std::string& profile = "ps_5_0")
        -> ID3D11PixelShader*;

    /** Create a compute shader */
    auto createComputeShader(const std::string& source, const std::string& hint,
                             const std::string& funcName = "cs_main",
                             const std::string& profile = "cs_5_0")
        -> ID3D11ComputeShader*;

    auto compileShaderSource(const std::string& source, const std::string& hint,
                             const std::string& funcName,
                             const std::string& profile) -> ID3DBlob*;

    auto createVertexBuffer(const void* data, U32 size) -> ID3D11Buffer*;

    template <typename T>
    auto createVertexBuffer(const std::vector<T>& elements) -> ID3D11Buffer* {
        return createVertexBuffer(elements.data(), sizeof(T) * elements.size());
    }

    auto createIndexBuffer(const void* data, U32 size) -> ID3D11Buffer*;

    template <typename T>
    auto createIndexBuffer(const std::vector<T>& elements) -> ID3D11Buffer* {
        return createIndexBuffer(elements.data(), sizeof(T) * elements.size());
    }

    auto createConstantBuffer(U32 size) -> ID3D11Buffer*;

    template <typename T> auto createConstantBuffer() -> ID3D11Buffer* {
        return createConstantBuffer(sizeof(T));
    }

    auto createBuffer(U32 elemSize, U32 numElems, D3D11_USAGE usage,
                      U32 bindFlags, U32 cpuFlags = 0) -> ID3D11Buffer*;

    void setViewport(U32 width, U32 height);

    void setDepthStencilState(ID3D11DepthStencilState* state, U32 stencilRef);

    void setProgram(const DX11Program& prog);

    void check_live_reload(DX11Program& prog);

    void setRenderTargets(ID3D11RenderTargetView* renderTgt,
                          ID3D11DepthStencilView* depthTgt) {
        _context->OMSetRenderTargets(1, &renderTgt, depthTgt);
    };

    void createCube(float size, const Vec3f& center,
                    std::vector<SimpleVertex>& vertices,
                    std::vector<unsigned int>& indices);

    void createCubeGrid(U32 gridSize, float size, float spaceFactor,
                        std::vector<SimpleVertex>& vertices,
                        std::vector<unsigned int>& indices);

    void setVertexBuffer(ID3D11Buffer* buffer, U32 stride);
    void setIndexBuffer(ID3D11Buffer* buffer,
                        DXGI_FORMAT fmt = DXGI_FORMAT_R32_UINT);

    /** Set the draw topology to triangle list */
    void setTriangleList();

    auto toDX(const Vec3f& vec) -> Vec3f { return vec; }

    auto buildViewMatrix(const Vec3f& position, const Vec3f& forward,
                         const Vec3f& up) -> DirectX::XMMATRIX;

    auto invertMatrix(const DirectX::XMMATRIX& matrix) -> DirectX::XMMATRIX;

    auto buildPerspectiveProjectionMatrix(float fovYRadians, float aspectRatio,
                                          float nearZ, float farZ)
        -> DirectX::XMMATRIX;

    auto createRasterState(D3D11_CULL_MODE mode, bool frontccw)
        -> ID3D11RasterizerState*;

    auto getCurrentTime() -> double;

    /** Create a cubemap view */
    auto createTextureCube(const std::string& folder,
                           const std::vector<std::string>& filenames)
        -> ID3D11ShaderResourceView*;

    auto createTexture2D(U32 width, U32 height,
                         U32 bindFlags = D3D11_BIND_RENDER_TARGET |
                                         D3D11_BIND_SHADER_RESOURCE,
                         DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
                         U32 miscFlags = 0, U32 usage = D3D11_USAGE_DEFAULT)
        -> ID3D11Texture2D*;

    auto createSharedTexture2D(HANDLE* sharedHandle, U32 width, U32 height,
                               U32 bindFlags = D3D11_BIND_RENDER_TARGET |
                                               D3D11_BIND_SHADER_RESOURCE,
                               DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
                               bool nthandle = true, bool keyedMutex = false,
                               DWORD access = DXGI_SHARED_RESOURCE_READ |
                                              DXGI_SHARED_RESOURCE_WRITE)
        -> ComPtr<ID3D11Texture2D>;

    auto createReadOnlySharedTexture2D(
        HANDLE* sharedHandle, U32 width, U32 height,
        U32 bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool nthandle = true,
        bool keyedMutex = false) -> ComPtr<ID3D11Texture2D>;

    auto createShaderResourceView(ID3D11Texture2D* tex)
        -> ComPtr<ID3D11ShaderResourceView>;

    /** Create a texture 2D from a shared handle */
    auto createTexture2DFromSharedHandle(HANDLE handle) -> ID3D11Texture2D*;

    auto createTexture2DFromSharedHandle(HANDLE handle, bool isNTHandle)
        -> ID3D11Texture2D*;

    /** Setup the viewport from the render target texture. */
    void setViewportFromRenderTargetTexture(ID3D11Texture2D* tex);

    /** Set the render target and viewport */
    void setRenderTarget(ID3D11RenderTargetView* rtgt);

    /** apply a given state */
    void applyState(DX11State& state);

    /** Set the shader include dir */
    void setShaderIncludeDir(const std::string& dir);

    auto gen_f32(F32 mini = 0.0F, F32 maxi = 1.0F) -> F32;

    /** Retrieve KeyedMutex from DX11Texture. */
    auto getKeyedMutex(ID3D11Texture2D* texture) -> ComPtr<IDXGIKeyedMutex>;

    auto saveTextureToFile(ID3D11Texture2D* sourceTexture, const char* filename)
        -> bool;

    void unbindResources();

  private:
    ID3D11Device* _device{nullptr};
    ID3D11DeviceContext* _context{nullptr};

    ID3D11Texture2D* _stagingTexture2d{nullptr};
    UINT _stagingWidth{0};
    UINT _stagingHeight{0};
    DXGI_FORMAT _stagingFormat{DXGI_FORMAT_UNKNOWN};

    /** Shader include dir */
    std::string _shaderIncludeDir;

    mutable std::mt19937 _gen;

    auto convertAndSaveImage(const D3D11_MAPPED_SUBRESOURCE& mappedResource,
                             const char* filename) -> bool;

    auto initializeStagingTexture2d(UINT width, UINT height, DXGI_FORMAT format)
        -> bool;

    /** Perform actual program loading. */
    auto updateProgram(DX11Program& prog) -> bool;
};

auto acquireKeyedMutex(ComPtr<IDXGIKeyedMutex>& keyedMutex, I32 key) -> bool;
auto releaseKeyedMutex(ComPtr<IDXGIKeyedMutex>& keyedMutex, I32 key) -> bool;

} // namespace nv

#endif
