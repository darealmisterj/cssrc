#include "ResourceManager.h"
#include <d3dcompiler.h>
#include <wincodec.h>
#include <mutex>
#include <stdexcept>
#include <format>

// Static member initialization
ResourceManager* ResourceManager::s_instance = nullptr;
std::mutex ResourceManager::s_mutex;

ResourceManager* ResourceManager::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_instance == nullptr) {
        s_instance = new ResourceManager();
    }
    return s_instance;
}

void ResourceManager::Release() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

ResourceManager::ResourceManager() 
    : m_device(nullptr)
    , m_deviceContext(nullptr) {
}

ResourceManager::~ResourceManager() {
    Cleanup();
}

void ResourceManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (!device || !context) {
        throw std::runtime_error("Invalid DirectX device or context");
    }
    m_device = device;
    m_deviceContext = context;
}

void ResourceManager::Cleanup() {
    // Clean up textures
    for (auto& pair : m_textureMap) {
        SafeRelease(pair.second);
    }
    m_textureMap.clear();

    // Clean up shaders
    for (auto& pair : m_vertexShaderMap) {
        SafeRelease(pair.second);
    }
    m_vertexShaderMap.clear();

    for (auto& pair : m_pixelShaderMap) {
        SafeRelease(pair.second);
    }
    m_pixelShaderMap.clear();

    // Clear device references
    m_deviceContext = nullptr;
    m_device = nullptr;
}

ID3D11ShaderResourceView* ResourceManager::LoadTexture(const std::wstring& filePath) {
    // Check if texture is already loaded
    auto it = m_textureMap.find(filePath);
    if (it != m_textureMap.end()) {
        return it->second;
    }

    // Create WIC factory
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create WIC factory");
    }

    // Load image from file
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to load texture file");
    }

    // Get frame from image
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get texture frame");
    }

    // Convert format if necessary
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create format converter");
    }

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize format converter");
    }

    // Get image dimensions
    UINT width, height;
    converter->GetSize(&width, &height);

    // Create texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Create texture from image data
    std::vector<BYTE> buffer(width * height * 4);
    hr = converter->CopyPixels(
        nullptr,
        width * 4,
        static_cast<UINT>(buffer.size()),
        buffer.data()
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to copy texture pixels");
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = buffer.data();
    initData.SysMemPitch = width * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    hr = m_device->CreateTexture2D(&textureDesc, &initData, &texture);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create texture");
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv;
    hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create shader resource view");
    }

    // Store and return
    m_textureMap[filePath] = srv;
    return srv;
}

ID3D11VertexShader* ResourceManager::LoadVertexShader(
    const std::wstring& filePath,
    const std::string& entryPoint,
    const std::string& target
) {
    auto key = filePath + L":" + std::wstring(entryPoint.begin(), entryPoint.end());
    auto it = m_vertexShaderMap.find(key);
    if (it != m_vertexShaderMap.end()) {
        return it->second;
    }

    // Compile shader
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        target.c_str(),
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            throw std::runtime_error(static_cast<char*>(errorBlob->GetBufferPointer()));
        }
        throw std::runtime_error("Failed to compile vertex shader");
    }

    // Create shader
    ID3D11VertexShader* shader;
    hr = m_device->CreateVertexShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        &shader
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex shader");
    }

    m_vertexShaderMap[key] = shader;
    return shader;
}

ID3D11PixelShader* ResourceManager::LoadPixelShader(
    const std::wstring& filePath,
    const std::string& entryPoint,
    const std::string& target
) {
    auto key = filePath + L":" + std::wstring(entryPoint.begin(), entryPoint.end());
    auto it = m_pixelShaderMap.find(key);
    if (it != m_pixelShaderMap.end()) {
        return it->second;
    }

    // Compile shader
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        target.c_str(),
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            throw std::runtime_error(static_cast<char*>(errorBlob->GetBufferPointer()));
        }
        throw std::runtime_error("Failed to compile pixel shader");
    }

    // Create shader
    ID3D11PixelShader* shader;
    hr = m_device->CreatePixelShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        &shader
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create pixel shader");
    }

    m_pixelShaderMap[key] = shader;
    return shader;
}

void ResourceManager::SafeRelease(IUnknown* resource) {
    if (resource) {
        resource->Release();
    }
}

