#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <Windows.h>

// Hotkey definition
struct Hotkey {
    std::string id;           // Unique identifier (e.g., script name)
    int virtualKey = 0;       // Virtual key code (VK_F1, VK_NUMPAD1, etc.)
    int modifiers = 0;        // Modifier keys (MOD_CONTROL, MOD_ALT, MOD_SHIFT)
    bool enabled = false;

    std::string getDisplayName() const;
    static std::string keyCodeToString(int vk);
    static std::string modifiersToString(int mods);
};

class HotkeyManager {
public:
    HotkeyManager();
    ~HotkeyManager();

    // Initialize with window handle (needed for RegisterHotKey)
    bool initialize(HWND hwnd);
    void shutdown();

    // Register a hotkey for a script
    bool registerHotkey(const std::string& scriptName, int virtualKey, int modifiers);
    void unregisterHotkey(const std::string& scriptName);
    void unregisterAll();

    // Get/set hotkey for a script
    Hotkey* getHotkey(const std::string& scriptName);
    const Hotkey* getHotkey(const std::string& scriptName) const;
    const std::vector<Hotkey>& getAllHotkeys() const { return m_hotkeys; }

    // Set callback for when a script hotkey is pressed
    using HotkeyCallback = std::function<void(const std::string& scriptName)>;
    void setCallback(HotkeyCallback callback) { m_callback = callback; }

    // Set callback for when a weapon hotkey is pressed
    using WeaponHotkeyCallback = std::function<void(const std::string& weaponName)>;
    void setWeaponCallback(WeaponHotkeyCallback callback) { m_weaponCallback = callback; }

    // Register/unregister weapon hotkeys
    bool registerWeaponHotkey(const std::string& weaponName, int virtualKey, int modifiers);
    void unregisterWeaponHotkey(const std::string& weaponName);
    void unregisterAllWeaponHotkeys();

    // Process Windows message (call from message loop)
    // Returns true if the message was a hotkey message
    bool processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // Check if currently capturing a hotkey
    bool isCapturing() const { return m_capturing; }
    void startCapture(const std::string& scriptName);
    void cancelCapture();

    // Process key during capture mode
    bool processCaptureKey(int virtualKey, int modifiers);
    const std::string& getCaptureTarget() const { return m_captureTarget; }

    // Serialize hotkeys for saving
    std::vector<std::pair<std::string, std::pair<int, int>>> serializeHotkeys() const;
    void loadHotkeys(const std::vector<std::pair<std::string, std::pair<int, int>>>& data);

private:
    HWND m_hwnd = nullptr;
    std::vector<Hotkey> m_hotkeys;
    std::vector<Hotkey> m_weaponHotkeys;
    HotkeyCallback m_callback;
    WeaponHotkeyCallback m_weaponCallback;
    int m_nextId = 1;  // Windows hotkey ID counter
    std::unordered_map<int, std::string> m_idToScript;  // Map Windows hotkey ID to script name
    std::unordered_map<int, std::string> m_idToWeapon;  // Map Windows hotkey ID to weapon name

    bool m_capturing = false;
    std::string m_captureTarget;
    bool m_capturingWeapon = false;  // True if capturing for weapon, false for script

    int getNextId() { return m_nextId++; }

public:
    // Weapon hotkey capture
    void startWeaponCapture(const std::string& weaponName);
    bool isCapturingWeapon() const { return m_capturingWeapon; }
    Hotkey* getWeaponHotkey(const std::string& weaponName);
    const Hotkey* getWeaponHotkey(const std::string& weaponName) const;

    // Temporarily suspend/resume all hotkeys (for when text input is active)
    void suspend();
    void resume();
    bool isSuspended() const { return m_suspended; }

private:
    bool m_suspended = false;
};
