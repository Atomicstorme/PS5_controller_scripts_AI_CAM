#pragma once

#include "Common.h"
#include "InputProcessor.h"
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
    InputProcessor m_processor;
    GUI m_gui;

    bool m_running = false;
};
