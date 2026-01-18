#pragma once

#include "Common.h"
#include "InputProcessor.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "Overlay.h"
#include "GUI.h"
#include <d3d11.h>

class Application {
public:
    Application();
    ~Application();

    // Initialize application
    bool initialize(HINSTANCE hInstance);

    // Run main loop
    int run();

    // Cleanup
    void shutdown();

    // Get singleton instance
    static Application* getInstance() { return s_instance; }

    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool createWindow(HINSTANCE hInstance);
    bool createD3D();
    void destroyD3D();
    void handleResize(int width, int height);

    static Application* s_instance;

    HWND m_hwnd = nullptr;
    int m_width = 1280;
    int m_height = 720;

    // DirectX 11
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTarget = nullptr;

    // Components
    ConfigManager m_config;
    InputProcessor m_processor;
    HotkeyManager m_hotkeys;
    Overlay m_overlay;
    GUI m_gui;

    HINSTANCE m_hInstance = nullptr;
    bool m_running = false;

public:
    // Accessors for GUI
    ConfigManager& getConfig() { return m_config; }
    HotkeyManager& getHotkeys() { return m_hotkeys; }
    InputProcessor& getProcessor() { return m_processor; }
    Overlay& getOverlay() { return m_overlay; }
};
