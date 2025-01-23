#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "ResourceManager.h"

class UIComponent {
public:
    virtual ~UIComponent() = default;
    virtual void Render() = 0;
    virtual void Update() = 0;
    
    void SetVisible(bool visible) { isVisible = visible; }
    bool IsVisible() const { return isVisible; }
    
protected:
    bool isVisible = true;
};

class UIManager {
public:
    static UIManager& GetInstance() {
        static UIManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // Initialization and cleanup
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
    void Shutdown();

    // Frame management
    void BeginFrame();
    void EndFrame();
    void Render();

    // Window management
    template<typename T, typename... Args>
    std::shared_ptr<T> CreateWindow(const std::string& name, Args&&... args) {
        auto window = std::make_shared<T>(std::forward<Args>(args)...);
        windows[name] = window;
        return std::static_pointer_cast<T>(window);
    }

    void RemoveWindow(const std::string& name);
    std::shared_ptr<UIComponent> GetWindow(const std::string& name);

    // Style and theme
    void SetTheme(const std::string& themeName);
    void LoadFonts();
    void SetGlobalScale(float scale);

    // Resource management
    ImTextureID LoadTexture(const std::string& path);
    void UnloadTexture(const std::string& path);

    // Input handling
    void ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool WantCaptureKeyboard() const;
    bool WantCaptureMouse() const;

    // State management
    void PushState();
    void PopState();
    void SaveState(const std::string& name);
    void LoadState(const std::string& name);

private:
    UIManager() = default;
    ~UIManager() = default;

    void SetupStyle();

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    std::unordered_map<std::string, std::shared_ptr<UIComponent>> windows;
    std::unordered_map<std::string, ImTextureID> textureCache;
    float globalScale = 1.0f;
    bool initialized = false;

    // Theme and style settings
    ImGuiStyle defaultStyle;
    std::unordered_map<std::string, std::function<void()>> themes;
};

