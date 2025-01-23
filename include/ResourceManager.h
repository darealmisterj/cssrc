#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>

class ResourceManager {
public:
    // Singleton access
    static ResourceManager& GetInstance();

    // Delete copy and move constructors and operators
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // Initialization and cleanup
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Cleanup();

    // Texture management
    bool LoadTexture(const std::wstring& name, const std::wstring& filePath);
    ID3D11ShaderResourceView* GetTexture(const std::wstring& name) const;
    void ReleaseTexture(const std::wstring& name);

    // Buffer management
    template<typename T>
    bool CreateBuffer(const std::wstring& name, const T* data, UINT count,
                    D3D11_BIND_FLAG bindFlags, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
    template<typename T>
    ID3D11Buffer* GetBuffer(const std::wstring& name) const;
    void ReleaseBuffer(const std::wstring& name);

    // Shader management
    bool LoadVertexShader(const std::wstring& name, const std::wstring& filePath,
                        const D3D11_INPUT_ELEMENT_DESC* layoutDesc, UINT numElements);
    bool LoadPixelShader(const std::wstring& name, const std::wstring& filePath);
    ID3D11VertexShader* GetVertexShader(const std::wstring& name) const;
    ID3D11PixelShader* GetPixelShader(const std::wstring& name) const;
    ID3D11InputLayout* GetInputLayout(const std::wstring& name) const;

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // DirectX device references
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;

    // Resource storage
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textures;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11Buffer>> m_buffers;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11VertexShader>> m_vertexShaders;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11PixelShader>> m_pixelShaders;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11InputLayout>> m_inputLayouts;

    // Thread safety
    mutable std::mutex m_mutex;
};

// Template implementation
template<typename T>
bool ResourceManager::CreateBuffer(const std::wstring& name, const T* data, UINT count,
                                D3D11_BIND_FLAG bindFlags, D3D11_USAGE usage) {
    std::lock_guard<std::mutex> lock(m_mutex);

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(T) * count;
    desc.Usage = usage;
    desc.BindFlags = bindFlags;
    desc.CPUAccessFlags = (usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = m_device->CreateBuffer(&desc, data ? &initData : nullptr, &buffer);
    if (FAILED(hr)) return false;

    m_buffers[name] = buffer;
    return true;
}

template<typename T>
ID3D11Buffer* ResourceManager::GetBuffer(const std::wstring& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_buffers.find(name);
    return (it != m_buffers.end()) ? it->second.Get() : nullptr;
}

