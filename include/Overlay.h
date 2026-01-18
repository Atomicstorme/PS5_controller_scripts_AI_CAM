#pragma once

#include "Common.h"
#include <Windows.h>
#include <string>
#include <vector>

// Forward declarations
class InputProcessor;
class ConfigManager;
class HotkeyManager;

class Overlay {
public:
    Overlay();
    ~Overlay();

    // Initialize the overlay window
    bool initialize(HINSTANCE hInstance);
    void shutdown();

    // Update and render the overlay
    void update(InputProcessor& processor, ConfigManager& config, HotkeyManager& hotkeys);

    // Visibility control
    void show();
    void hide();
    void toggle();
    bool isVisible() const { return m_visible; }

    // Settings
    void setPosition(OverlayPosition pos);
    OverlayPosition getPosition() const { return m_position; }
    void setOpacity(float opacity);
    float getOpacity() const { return m_opacity; }

    // Window handle access (for message processing)
    HWND getHwnd() const { return m_hwnd; }

private:
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_visible = true;
    OverlayPosition m_position = OverlayPosition::TopLeft;
    float m_opacity = 0.85f;

    // Window dimensions
    int m_width = 220;
    int m_height = 300;
    int m_margin = 10;

    // GDI+ token
    ULONG_PTR m_gdiplusToken = 0;

    // Cached display data
    struct DisplayData {
        bool dualSenseConnected = false;
        bool processingActive = false;
        std::string activeWeapon;
        float adsStrength = 0.0f;
        float hipStrength = 0.0f;
        std::vector<std::pair<std::string, bool>> scripts; // name, enabled
        std::vector<std::pair<std::string, std::string>> hotkeys; // name, key display
    };
    DisplayData m_data;

    // Rendering
    void render();
    void updateWindowPosition();

    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
