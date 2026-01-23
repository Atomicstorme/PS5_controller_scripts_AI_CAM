#include "Application.h"
#include <imgui.h>
#include <imgui_impl_win32.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

Application::~Application() {
    shutdown();
    s_instance = nullptr;
}

bool Application::initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Load saved configuration
    m_config.load();

    if (!createWindow(hInstance)) {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!createD3D()) {
        MessageBoxA(nullptr, "Failed to initialize DirectX 11", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!m_gui.initialize(m_hwnd, m_device, m_context)) {
        MessageBoxA(nullptr, "Failed to initialize GUI", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!m_processor.initialize(&m_config)) {
        MessageBoxA(nullptr, "Failed to initialize input processor.\n\n"
                    "Make sure ViGEmBus driver is installed:\n"
                    "https://github.com/ViGEm/ViGEmBus/releases",
                    "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Initialize hotkey manager
    m_hotkeys.initialize(m_hwnd);

    // Set up hotkey callback to toggle scripts (and overlay)
    m_hotkeys.setCallback([this](const std::string& scriptName) {
        // Special case: overlay toggle
        if (scriptName == "__overlay_toggle__") {
            m_overlay.toggle();
            // Update config setting
            m_config.getSettings().overlayEnabled = m_overlay.isVisible();
            return;
        }

        // Normal script toggle
        auto& scripts = m_processor.getScriptManager().getScripts();
        for (auto& script : scripts) {
            if (script.config.name == scriptName) {
                bool newState = !script.config.enabled;
                m_processor.getScriptManager().setScriptEnabled(scriptName, newState);
                break;
            }
        }
    });

    // Set up weapon hotkey callback to switch active weapon
    m_hotkeys.setWeaponCallback([this](const std::string& weaponName) {
        m_config.setActiveWeapon(weaponName);
    });

    // Load saved hotkeys from config
    auto hotkeyData = m_config.getHotkeys();
    m_hotkeys.loadHotkeys(hotkeyData);

    // Load weapon hotkeys from saved presets
    const auto& weaponPresets = m_config.getWeaponPresets();
    for (const auto& preset : weaponPresets) {
        if (preset.hotkeyVk != 0) {
            m_hotkeys.registerWeaponHotkey(preset.name, preset.hotkeyVk, preset.hotkeyModifiers);
        }
    }

    // Initialize overlay
    if (!m_overlay.initialize(hInstance)) {
        // Overlay is optional, just warn but continue
        MessageBoxA(nullptr, "Failed to initialize overlay. Overlay will be disabled.",
                    "Warning", MB_OK | MB_ICONWARNING);
    } else {
        // Apply saved overlay settings
        const auto& settings = m_config.getSettings();
        m_overlay.setPosition(settings.overlayPosition);
        m_overlay.setOpacity(settings.overlayOpacity);
        if (!settings.overlayEnabled) {
            m_overlay.hide();
        }

        // Register overlay toggle hotkey (F11 - F12 triggers Windows debugger break)
        m_hotkeys.registerHotkey("__overlay_toggle__", VK_F11, 0);
    }

    // Auto-start processing
    m_processor.start();

    return true;
}

bool Application::createWindow(HINSTANCE hInstance) {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "PS5ControllerScriptsClass";

    if (!RegisterClassExA(&wc)) {
        return false;
    }

    RECT rect = {0, 0, m_width, m_height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "PS5 Controller Scripts",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hwnd) {
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);

    return true;
}

bool Application::createD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    // Try hardware first, then WARP fallback
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,  // No debug flag - requires special SDK
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    // Fallback to WARP (software renderer) if hardware fails
    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &m_swapChain,
            &m_device,
            &featureLevel,
            &m_context
        );
    }

    if (FAILED(hr)) {
        return false;
    }

    // Create render target
    ID3D11Texture2D* backBuffer = nullptr;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTarget);
    backBuffer->Release();

    return true;
}

void Application::destroyD3D() {
    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

void Application::handleResize(int width, int height) {
    if (m_device && width > 0 && height > 0) {
        m_width = width;
        m_height = height;

        if (m_renderTarget) {
            m_renderTarget->Release();
            m_renderTarget = nullptr;
        }

        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

        ID3D11Texture2D* backBuffer = nullptr;
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTarget);
        backBuffer->Release();
    }
}

int Application::run() {
    m_running = true;
    MSG msg = {};

    while (m_running) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);

            if (msg.message == WM_QUIT) {
                m_running = false;
            }
        }

        if (!m_running) break;

        // Check if GUI wants to close
        if (m_gui.shouldClose()) {
            m_running = false;
            break;
        }

        // Update overlay
        m_overlay.update(m_processor, m_config, m_hotkeys);

        // Render main GUI
        float clearColor[4] = {0.1f, 0.1f, 0.12f, 1.0f};
        m_context->OMSetRenderTargets(1, &m_renderTarget, nullptr);
        m_context->ClearRenderTargetView(m_renderTarget, clearColor);

        m_gui.render(m_processor);

        m_swapChain->Present(1, 0);  // VSync on
    }

    return static_cast<int>(msg.wParam);
}

void Application::shutdown() {
    // Save hotkeys to config
    m_config.setHotkeys(m_hotkeys.serializeHotkeys());

    // Save overlay settings
    m_config.getSettings().overlayEnabled = m_overlay.isVisible();
    m_config.getSettings().overlayPosition = m_overlay.getPosition();
    m_config.getSettings().overlayOpacity = m_overlay.getOpacity();

    // Save configuration before shutting down
    m_config.save();

    m_overlay.shutdown();
    m_hotkeys.shutdown();
    m_processor.stop();
    m_gui.shutdown();
    destroyD3D();

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }

    Application* app = Application::getInstance();

    // Process hotkey messages (but not if ImGui wants keyboard input for text fields)
    bool wantTextInput = ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
    if (app && !wantTextInput && app->m_hotkeys.processMessage(msg, wParam, lParam)) {
        return 0;
    }

    switch (msg) {
        case WM_SIZE:
            if (app && wParam != SIZE_MINIMIZED) {
                app->handleResize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
