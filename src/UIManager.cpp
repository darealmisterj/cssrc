#include "UIManager.h"
#include "ResourceManager.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <stdexcept>

UIManager::UIManager()
    : m_initialized(false)
    , m_hwnd(nullptr)
    , m_device(nullptr)
    , m_deviceContext(nullptr) {
}

UIManager::~UIManager() {
    shutdown();
}

void UIManager::initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext) {
    if (m_initialized) {
        return;
    }

    m_hwnd = hwnd;
    m_device = device;
    m_deviceContext = deviceContext;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Platform/Renderer backends
    if (!ImGui_ImplWin32_Init(m_hwnd)) {
        throw std::runtime_error("Failed to initialize ImGui Win32 implementation");
    }
    if (!ImGui_ImplDX11_Init(m_device, m_deviceContext)) {
        ImGui_ImplWin32_Shutdown();
        throw std::runtime_error("Failed to initialize ImGui DirectX11 implementation");
    }

    setupTheme();
    m_initialized = true;
}

void UIManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_device = nullptr;
    m_deviceContext = nullptr;
    m_hwnd = nullptr;
    m_initialized = false;
}

void UIManager::beginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::setupTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Set up modern dark theme
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    // Window styling
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
}

bool UIManager::createWindow(const char* name, const WindowCreateInfo& info) {
    bool isOpen = true;
    ImGui::SetNextWindowSize(ImVec2(info.width, info.height), ImGuiCond_FirstUseEver);
    if (info.pos.has_value()) {
        ImGui::SetNextWindowPos(ImVec2(info.pos->x, info.pos->y), ImGuiCond_FirstUseEver);
    }

    ImGuiWindowFlags flags = 0;
    if (info.flags.noTitleBar) flags |= ImGuiWindowFlags_NoTitleBar;
    if (info.flags.noResize) flags |= ImGuiWindowFlags_NoResize;
    if (info.flags.noMove) flags |= ImGuiWindowFlags_NoMove;
    if (info.flags.noScrollbar) flags |= ImGuiWindowFlags_NoScrollbar;
    if (info.flags.noCollapse) flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin(name, &isOpen, flags);
    m_activeWindow = name;
    return isOpen;
}

void UIManager::endWindow() {
    ImGui::End();
    m_activeWindow.clear();
}

bool UIManager::processInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!m_initialized) {
        return false;
    }
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void UIManager::loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Load default font
    io.Fonts->AddFontDefault();
    
    // Upload font textures to GPU
    ImGui_ImplDX11_CreateDeviceObjects();
}

bool UIManager::loadTexture(const std::string& name, const std::string& path) {
    return ResourceManager::getInstance().loadTexture(name, path);
}

ImTextureID UIManager::getTexture(const std::string& name) {
    return ResourceManager::getInstance().getTexture(name);
}

