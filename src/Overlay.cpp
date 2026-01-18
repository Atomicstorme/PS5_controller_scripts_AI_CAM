#include "Overlay.h"
#include "InputProcessor.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"

// GDI+ requires COM headers (not included with WIN32_LEAN_AND_MEAN)
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

Overlay::Overlay() {
}

Overlay::~Overlay() {
    shutdown();
}

bool Overlay::initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        return false;
    }

    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;  // No background
    wc.lpszClassName = "PS5OverlayClass";

    if (!RegisterClassExA(&wc)) {
        return false;
    }

    // Create layered, topmost, click-through window
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP;

    m_hwnd = CreateWindowExA(
        exStyle,
        wc.lpszClassName,
        "PS5 Overlay",
        style,
        0, 0, m_width, m_height,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) {
        return false;
    }

    // Set initial position
    updateWindowPosition();

    // Show the window
    if (m_visible) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    }

    return true;
}

void Overlay::shutdown() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    if (m_hInstance) {
        UnregisterClassA("PS5OverlayClass", m_hInstance);
        m_hInstance = nullptr;
    }
}

void Overlay::update(InputProcessor& processor, ConfigManager& config, HotkeyManager& hotkeys) {
    if (!m_hwnd || !m_visible) return;

    // Update cached data
    m_data.dualSenseConnected = processor.isDualSenseConnected();
    m_data.processingActive = processor.isRunning();

    // Get active weapon
    m_data.activeWeapon = config.getActiveWeaponName();
    const WeaponPreset* preset = config.getActiveWeaponPreset();
    if (preset) {
        m_data.adsStrength = preset->adsStrength;
        m_data.hipStrength = preset->hipFireStrength;
    } else {
        m_data.adsStrength = 0.0f;
        m_data.hipStrength = 0.0f;
    }

    // Get scripts
    m_data.scripts.clear();
    auto& scripts = processor.getScriptManager().getScripts();
    for (const auto& script : scripts) {
        m_data.scripts.push_back({script.config.name, script.config.enabled});
    }

    // Get hotkeys
    m_data.hotkeys.clear();
    const auto& allHotkeys = hotkeys.getAllHotkeys();
    for (const auto& hk : allHotkeys) {
        if (hk.virtualKey != 0) {
            m_data.hotkeys.push_back({hk.id, hk.getDisplayName()});
        }
    }

    // Render the overlay
    render();
}

void Overlay::show() {
    m_visible = true;
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    }
}

void Overlay::hide() {
    m_visible = false;
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void Overlay::toggle() {
    if (m_visible) {
        hide();
    } else {
        show();
    }
}

void Overlay::setPosition(OverlayPosition pos) {
    m_position = pos;
    updateWindowPosition();
}

void Overlay::setOpacity(float opacity) {
    m_opacity = std::clamp(opacity, 0.1f, 1.0f);
    // Opacity is applied during render
}

void Overlay::updateWindowPosition() {
    if (!m_hwnd) return;

    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = m_margin;
    int y = m_margin;

    switch (m_position) {
        case OverlayPosition::TopLeft:
            x = m_margin;
            y = m_margin;
            break;
        case OverlayPosition::TopRight:
            x = screenWidth - m_width - m_margin;
            y = m_margin;
            break;
        case OverlayPosition::BottomLeft:
            x = m_margin;
            y = screenHeight - m_height - m_margin;
            break;
        case OverlayPosition::BottomRight:
            x = screenWidth - m_width - m_margin;
            y = screenHeight - m_height - m_margin;
            break;
    }

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_width, m_height, SWP_NOACTIVATE);
}

void Overlay::render() {
    if (!m_hwnd || !m_visible) return;

    // Create a compatible DC and bitmap for layered window
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_width;
    bmi.bmiHeader.biHeight = -m_height;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Create GDI+ graphics from the memory DC
    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Calculate alpha (0-255)
    BYTE alpha = static_cast<BYTE>(m_opacity * 255);

    // Background color with alpha
    Color bgColor(static_cast<BYTE>(alpha * 0.9f), 20, 20, 25);
    SolidBrush bgBrush(bgColor);
    graphics.FillRectangle(&bgBrush, 0, 0, m_width, m_height);

    // Border
    Color borderColor(alpha, 60, 60, 70);
    Pen borderPen(borderColor, 1.0f);
    graphics.DrawRectangle(&borderPen, 0, 0, m_width - 1, m_height - 1);

    // Fonts
    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font normalFont(&fontFamily, 10, FontStyleRegular, UnitPixel);
    Font smallFont(&fontFamily, 9, FontStyleRegular, UnitPixel);

    // Colors
    Color textColor(alpha, 220, 220, 220);
    Color greenColor(alpha, 100, 220, 120);
    Color redColor(alpha, 220, 100, 100);
    Color grayColor(alpha, 140, 140, 140);
    Color accentColor(alpha, 100, 180, 220);

    SolidBrush textBrush(textColor);
    SolidBrush greenBrush(greenColor);
    SolidBrush redBrush(redColor);
    SolidBrush grayBrush(grayColor);
    SolidBrush accentBrush(accentColor);

    float y = 8.0f;
    float x = 10.0f;
    float lineHeight = 16.0f;

    // Title
    graphics.DrawString(L"PS5 Controller Scripts", -1, &titleFont, PointF(x, y), &accentBrush);
    y += lineHeight + 4;

    // Separator line
    Color sepColor(alpha, 50, 50, 60);
    Pen sepPen(sepColor, 1.0f);
    graphics.DrawLine(&sepPen, x, y, m_width - x, y);
    y += 6;

    // Connection status
    std::wstring dsStatus = m_data.dualSenseConnected ? L"[*] DualSense Connected" : L"[X] DualSense Disconnected";
    graphics.DrawString(dsStatus.c_str(), -1, &normalFont, PointF(x, y),
                       m_data.dualSenseConnected ? &greenBrush : &redBrush);
    y += lineHeight;

    std::wstring procStatus = m_data.processingActive ? L"[>] Processing Active" : L"[||] Processing Paused";
    graphics.DrawString(procStatus.c_str(), -1, &normalFont, PointF(x, y),
                       m_data.processingActive ? &greenBrush : &grayBrush);
    y += lineHeight + 8;

    // Weapon section
    graphics.DrawLine(&sepPen, x, y, m_width - x, y);
    y += 6;

    graphics.DrawString(L"Weapon:", -1, &titleFont, PointF(x, y), &textBrush);
    y += lineHeight;

    if (!m_data.activeWeapon.empty()) {
        std::wstring weaponName(m_data.activeWeapon.begin(), m_data.activeWeapon.end());
        graphics.DrawString(weaponName.c_str(), -1, &normalFont, PointF(x + 8, y), &accentBrush);
        y += lineHeight;

        wchar_t statsBuffer[64];
        swprintf_s(statsBuffer, L"  ADS: %.2f | Hip: %.2f", m_data.adsStrength, m_data.hipStrength);
        graphics.DrawString(statsBuffer, -1, &smallFont, PointF(x, y), &grayBrush);
        y += lineHeight;
    } else {
        graphics.DrawString(L"  (None)", -1, &normalFont, PointF(x, y), &grayBrush);
        y += lineHeight;
    }
    y += 4;

    // Scripts section
    graphics.DrawLine(&sepPen, x, y, m_width - x, y);
    y += 6;

    graphics.DrawString(L"Scripts:", -1, &titleFont, PointF(x, y), &textBrush);
    y += lineHeight;

    if (m_data.scripts.empty()) {
        graphics.DrawString(L"  (No scripts)", -1, &normalFont, PointF(x, y), &grayBrush);
        y += lineHeight;
    } else {
        for (const auto& script : m_data.scripts) {
            std::wstring status = script.second ? L"[ON]  " : L"[OFF] ";
            std::wstring name(script.first.begin(), script.first.end());
            std::wstring line = status + name;

            graphics.DrawString(line.c_str(), -1, &smallFont, PointF(x + 4, y),
                               script.second ? &greenBrush : &grayBrush);
            y += lineHeight - 2;

            // Limit to prevent overflow
            if (y > m_height - 60) break;
        }
    }
    y += 4;

    // Hotkeys section (if space permits)
    if (y < m_height - 50 && !m_data.hotkeys.empty()) {
        graphics.DrawLine(&sepPen, x, y, m_width - x, y);
        y += 6;

        graphics.DrawString(L"Hotkeys:", -1, &titleFont, PointF(x, y), &textBrush);
        y += lineHeight;

        for (const auto& hk : m_data.hotkeys) {
            std::wstring name(hk.first.begin(), hk.first.end());
            std::wstring key(hk.second.begin(), hk.second.end());
            std::wstring line = key + L": " + name;

            graphics.DrawString(line.c_str(), -1, &smallFont, PointF(x + 4, y), &grayBrush);
            y += lineHeight - 2;

            // Limit display
            if (y > m_height - 20) break;
        }
    }

    // Update the layered window
    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {m_width, m_height};
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;  // Per-pixel alpha from bitmap
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptDst;
    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    ptDst.x = rect.left;
    ptDst.y = rect.top;

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    // Cleanup
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

LRESULT CALLBACK Overlay::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    Overlay* overlay = reinterpret_cast<Overlay*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_DESTROY:
            return 0;

        case WM_DISPLAYCHANGE:
            // Screen resolution changed, update position
            if (overlay) {
                overlay->updateWindowPosition();
            }
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
