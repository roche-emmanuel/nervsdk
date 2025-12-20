#ifndef DX12ENGINE_H_
#define DX12ENGINE_H_

#include <nvk/dx/dx_common.h>

namespace nv {

class DX12Engine;
struct DX12Program;

struct CommandListContext {
    U32 index;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    U64 fenceValue = 0;
    bool isRecording = false;
    DX12Engine& eng;

    // Add a resource transition:
    void addTransition(ID3D12Resource* res, D3D12_RESOURCE_STATES sBefore,
                       D3D12_RESOURCE_STATES sAfter);

    void addTransition(ID3D12Resource* res, D3D12_RESOURCE_STATES sAfter);
    void addCopyDstTransition(ID3D12Resource* res);
    void addCopySrcTransition(ID3D12Resource* res);
    void addCommonTransition(ID3D12Resource* res);
    void addRenderTgtTransition(ID3D12Resource* res);

    void addCopyFullTextureToTexture(ID3D12Resource* src, ID3D12Resource* dst);
    void addCopyFullTextureToBuffer(ID3D12Resource* src, ID3D12Resource* dst);

    // Compute shader execution
    void setComputeProgram(const DX12Program& prog);

    void dispatch(U32 threadGroupCountX, U32 threadGroupCountY = 1,
                  U32 threadGroupCountZ = 1);

    void clearRenderTarget(ID3D12DescriptorHeap* descHeap, F32 r, F32 g, F32 b,
                           F32 a, U32 slotIndex = 0);
};

class DX12InputLayoutDesc {
  public:
    DX12InputLayoutDesc() = default;

    DX12InputLayoutDesc(
        std::initializer_list<std::pair<std::string, DXGI_FORMAT>> elements) {
        for (const auto& element : elements) {
            add(element.first, element.second);
        }
    }

    void add(const std::string& semanticName, DXGI_FORMAT format) {
        names.push_back(semanticName);
        D3D12_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName = nullptr;
        elementDesc.SemanticIndex = 0;
        elementDesc.Format = format;
        elementDesc.InputSlot = 0;
        elementDesc.AlignedByteOffset =
            layout.empty() ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;
        layout.push_back(elementDesc);
    }

    auto data() const -> const D3D12_INPUT_ELEMENT_DESC* {
        int idx = 0;
        for (auto& desc : layout) {
            desc.SemanticName = names[idx++].c_str();
        }
        return layout.data();
    }

    auto size() const -> UINT { return static_cast<UINT>(layout.size()); }

  private:
    std::vector<std::string> names;
    mutable std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
};

struct DX12Program {
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;
    ComPtr<ID3DBlob> computeShaderBlob;
    DX12InputLayoutDesc inputDesc;
    std::string filename;

    // Time tracking (same as DX11)
    std::time_t lastCheckTime{0};
    std::time_t lastUpdateTime{0};
    std::unordered_set<std::string> files;

    bool isCompute{false};
};

class DX12Engine {
  public:
    static auto instance(ID3D12Device* device = nullptr) -> DX12Engine&;

    // Delete copy/move constructors and assignment operators
    DX12Engine(const DX12Engine&) = delete;
    auto operator=(const DX12Engine&) -> DX12Engine& = delete;
    DX12Engine(DX12Engine&&) = delete;
    auto operator=(DX12Engine&&) -> DX12Engine& = delete;

    auto device() const -> ID3D12Device* { return _device.Get(); }
    auto cmdQueue() const -> ID3D12CommandQueue* { return _cmdQueue.Get(); }

    void executeCommands(CommandListContext& ctx);
    void waitForGpu();

    void printGPUInfos();

    void saveTextureToFile(ID3D12Resource* tex, const char* filename);

    //    Get the current resource state:
    auto
    getCurrentState(ID3D12Resource* res,
                    D3D12_RESOURCE_STATES defval = D3D12_RESOURCE_STATE_COMMON)
        -> D3D12_RESOURCE_STATES;

    void setCurrentState(ID3D12Resource* res, D3D12_RESOURCE_STATES state);

    void setCopyDstState(ID3D12Resource* res);
    void setCopySrcState(ID3D12Resource* res);
    void setCommonState(ID3D12Resource* res);
    void setRenderTgtState(ID3D12Resource* res);

    // Resource creation
    auto createVertexBuffer(const void* data, U32 size)
        -> ComPtr<ID3D12Resource>;

    // Shader compilation and program creation
    auto readShaderFile(const std::string& filename,
                        std::unordered_set<std::string>& fileList)
        -> std::string;

    auto compileShaderSource(const std::string& source, const std::string& hint,
                             const std::string& funcName,
                             const std::string& profile) -> ComPtr<ID3DBlob>;

    auto createComputeShader(const std::string& source, const std::string& hint,
                             const std::string& funcName = "cs_main",
                             const std::string& profile = "cs_5_0")
        -> ComPtr<ID3DBlob>;

    auto createComputeProgram(const std::string& filename) -> DX12Program;

    auto createComputeRootSignature() -> ComPtr<ID3D12RootSignature>;

    auto createComputePipelineState(ID3D12RootSignature* rootSig,
                                    ID3DBlob* computeShader)
        -> ComPtr<ID3D12PipelineState>;

    // Shader hot-reload support
    void check_live_reload(DX12Program& prog);

    // Buffer creation for compute
    auto createStructuredBuffer(
        U32 elemSize, U32 numElems,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        -> ComPtr<ID3D12Resource>;

    // auto createReadbackBuffer(U32 size) -> ComPtr<ID3D12Resource>;

    // Descriptor heap management
    auto createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
                              U32 numDescriptors, bool shaderVisible = false)
        -> ComPtr<ID3D12DescriptorHeap>;

    auto createViewHeap(U32 numDescriptors, bool shaderVisible = false)
        -> ComPtr<ID3D12DescriptorHeap>;

    auto createRTVHeap(U32 numDescriptors) -> ComPtr<ID3D12DescriptorHeap>;

    auto createDSVHeap(U32 numDescriptors) -> ComPtr<ID3D12DescriptorHeap>;

    void createRenderTargetView(ID3D12DescriptorHeap* descHeap,
                                ID3D12Resource* texture, U32 slotIndex = 0);

    // UAV creation
    void createUnorderedAccessView(ID3D12Resource* resource,
                                   D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor,
                                   U32 numElements, U32 structureByteStride);

    // SRV creation
    void createShaderResourceView(ID3D12Resource* resource,
                                  D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor,
                                  U32 numElements, U32 structureByteStride);

    // Set shader include directory
    void setShaderIncludeDir(const std::string& dir);

    auto getReadbackBuffer(U32 size) -> ComPtr<ID3D12Resource>;

    auto createTexture2D(
        U32 width, U32 height,
        D3D12_RESOURCE_FLAGS resourceFlags =
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT)
        -> ComPtr<ID3D12Resource>;

    auto getRequiredReadBufferSize(ID3D12Resource* tex) -> U32;

    auto beginCmdList() -> CommandListContext&;

    auto getCmdList(I32 idx = -1) -> CommandListContext&;

  private:
    // Private constructor for singleton pattern
    explicit DX12Engine(ID3D12Device* device);

    // Core DX12 objects
    ComPtr<ID3D12Device> _device;
    ComPtr<ID3D12CommandQueue> _cmdQueue;
    ComPtr<ID3D12Fence> _fence;

    // Replace existing command objects with pool
    Vector<CommandListContext> _cmdListPool;
    U32 _currentCmdListIndex = 0;

    // Synchronization
    HANDLE _fenceEvent;
    UINT64 _fenceValue{0};

    // Upload heap for resource initialization
    ComPtr<ID3D12Resource> _uploadBuffer;
    U32 _uploadBufferSize{0};
    U32 _uploadBufferOffset{0};

    ComPtr<ID3D12Resource> _readbackBuffer;
    U64 _readbackBufferSize{0};

    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> _stateMap;

    /** Shader include dir */
    std::string _shaderIncludeDir;

    mutable std::mt19937 _gen;

    void createCommandListContext(CommandListContext& ctx);
    void retireCommandList(U32 index);

    // Helper method to create device if none provided
    void createDevice();
    void createCommandObjects();
    void createSyncObjects();
    void uploadToResource(ID3D12Resource* resource, const void* data, U32 size);
    void createUploadBuffer(U32 size);
    auto updateProgram(DX12Program& prog) -> bool;
};

} // namespace nv

#endif
