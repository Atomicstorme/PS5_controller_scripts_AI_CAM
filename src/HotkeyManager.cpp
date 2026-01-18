#include "HotkeyManager.h"
#include <sstream>

std::string Hotkey::keyCodeToString(int vk) {
    switch (vk) {
        case 0: return "None";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        case VK_NUMPAD0: return "Num0";
        case VK_NUMPAD1: return "Num1";
        case VK_NUMPAD2: return "Num2";
        case VK_NUMPAD3: return "Num3";
        case VK_NUMPAD4: return "Num4";
        case VK_NUMPAD5: return "Num5";
        case VK_NUMPAD6: return "Num6";
        case VK_NUMPAD7: return "Num7";
        case VK_NUMPAD8: return "Num8";
        case VK_NUMPAD9: return "Num9";
        case VK_MULTIPLY: return "Num*";
        case VK_ADD: return "Num+";
        case VK_SUBTRACT: return "Num-";
        case VK_DIVIDE: return "Num/";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        case VK_PAUSE: return "Pause";
        case VK_SCROLL: return "ScrollLock";
        case VK_OEM_3: return "`";
        default:
            if (vk >= 'A' && vk <= 'Z') {
                return std::string(1, static_cast<char>(vk));
            }
            if (vk >= '0' && vk <= '9') {
                return std::string(1, static_cast<char>(vk));
            }
            return "Key" + std::to_string(vk);
    }
}

std::string Hotkey::modifiersToString(int mods) {
    std::string result;
    if (mods & MOD_CONTROL) result += "Ctrl+";
    if (mods & MOD_ALT) result += "Alt+";
    if (mods & MOD_SHIFT) result += "Shift+";
    return result;
}

std::string Hotkey::getDisplayName() const {
    if (virtualKey == 0) return "Not Set";
    return modifiersToString(modifiers) + keyCodeToString(virtualKey);
}

HotkeyManager::HotkeyManager() {
}

HotkeyManager::~HotkeyManager() {
    shutdown();
}

bool HotkeyManager::initialize(HWND hwnd) {
    m_hwnd = hwnd;
    return true;
}

void HotkeyManager::shutdown() {
    unregisterAll();
    m_hwnd = nullptr;
}

bool HotkeyManager::registerHotkey(const std::string& scriptName, int virtualKey, int modifiers) {
    if (!m_hwnd || virtualKey == 0) return false;

    // First unregister any existing hotkey for this script
    unregisterHotkey(scriptName);

    // Find or create hotkey entry
    Hotkey* hotkey = nullptr;
    for (auto& hk : m_hotkeys) {
        if (hk.id == scriptName) {
            hotkey = &hk;
            break;
        }
    }

    if (!hotkey) {
        m_hotkeys.push_back({});
        hotkey = &m_hotkeys.back();
        hotkey->id = scriptName;
    }

    hotkey->virtualKey = virtualKey;
    hotkey->modifiers = modifiers;

    // Register with Windows
    int id = getNextId();
    if (RegisterHotKey(m_hwnd, id, modifiers | MOD_NOREPEAT, virtualKey)) {
        hotkey->enabled = true;
        m_idToScript[id] = scriptName;
        return true;
    }

    hotkey->enabled = false;
    return false;
}

void HotkeyManager::unregisterHotkey(const std::string& scriptName) {
    if (!m_hwnd) return;

    // Find and unregister
    for (auto it = m_idToScript.begin(); it != m_idToScript.end(); ) {
        if (it->second == scriptName) {
            UnregisterHotKey(m_hwnd, it->first);
            it = m_idToScript.erase(it);
        } else {
            ++it;
        }
    }

    // Update hotkey entry
    for (auto& hk : m_hotkeys) {
        if (hk.id == scriptName) {
            hk.enabled = false;
            break;
        }
    }
}

void HotkeyManager::unregisterAll() {
    if (!m_hwnd) return;

    for (const auto& pair : m_idToScript) {
        UnregisterHotKey(m_hwnd, pair.first);
    }
    m_idToScript.clear();

    for (const auto& pair : m_idToWeapon) {
        UnregisterHotKey(m_hwnd, pair.first);
    }
    m_idToWeapon.clear();

    for (auto& hk : m_hotkeys) {
        hk.enabled = false;
    }

    for (auto& hk : m_weaponHotkeys) {
        hk.enabled = false;
    }
}

Hotkey* HotkeyManager::getHotkey(const std::string& scriptName) {
    for (auto& hk : m_hotkeys) {
        if (hk.id == scriptName) {
            return &hk;
        }
    }
    return nullptr;
}

const Hotkey* HotkeyManager::getHotkey(const std::string& scriptName) const {
    for (const auto& hk : m_hotkeys) {
        if (hk.id == scriptName) {
            return &hk;
        }
    }
    return nullptr;
}

bool HotkeyManager::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        int id = static_cast<int>(wParam);

        // Check script hotkeys first
        auto it = m_idToScript.find(id);
        if (it != m_idToScript.end() && m_callback) {
            m_callback(it->second);
            return true;
        }

        // Check weapon hotkeys
        auto wit = m_idToWeapon.find(id);
        if (wit != m_idToWeapon.end() && m_weaponCallback) {
            m_weaponCallback(wit->second);
            return true;
        }
    }
    return false;
}

void HotkeyManager::startCapture(const std::string& scriptName) {
    m_capturing = true;
    m_capturingWeapon = false;
    m_captureTarget = scriptName;
}

void HotkeyManager::startWeaponCapture(const std::string& weaponName) {
    m_capturing = true;
    m_capturingWeapon = true;
    m_captureTarget = weaponName;
}

void HotkeyManager::cancelCapture() {
    m_capturing = false;
    m_capturingWeapon = false;
    m_captureTarget.clear();
}

bool HotkeyManager::processCaptureKey(int virtualKey, int modifiers) {
    if (!m_capturing) return false;

    // Ignore modifier-only presses
    if (virtualKey == VK_CONTROL || virtualKey == VK_SHIFT ||
        virtualKey == VK_MENU || virtualKey == VK_LCONTROL ||
        virtualKey == VK_RCONTROL || virtualKey == VK_LSHIFT ||
        virtualKey == VK_RSHIFT || virtualKey == VK_LMENU ||
        virtualKey == VK_RMENU) {
        return false;
    }

    // Escape cancels
    if (virtualKey == VK_ESCAPE) {
        cancelCapture();
        return true;
    }

    // Delete/Backspace clears the hotkey
    if (virtualKey == VK_DELETE || virtualKey == VK_BACK) {
        if (m_capturingWeapon) {
            unregisterWeaponHotkey(m_captureTarget);
            for (auto& hk : m_weaponHotkeys) {
                if (hk.id == m_captureTarget) {
                    hk.virtualKey = 0;
                    hk.modifiers = 0;
                    break;
                }
            }
        } else {
            unregisterHotkey(m_captureTarget);
            for (auto& hk : m_hotkeys) {
                if (hk.id == m_captureTarget) {
                    hk.virtualKey = 0;
                    hk.modifiers = 0;
                    break;
                }
            }
        }
        cancelCapture();
        return true;
    }

    // Register the new hotkey
    if (m_capturingWeapon) {
        registerWeaponHotkey(m_captureTarget, virtualKey, modifiers);
    } else {
        registerHotkey(m_captureTarget, virtualKey, modifiers);
    }
    cancelCapture();
    return true;
}

std::vector<std::pair<std::string, std::pair<int, int>>> HotkeyManager::serializeHotkeys() const {
    std::vector<std::pair<std::string, std::pair<int, int>>> result;
    for (const auto& hk : m_hotkeys) {
        if (hk.virtualKey != 0) {
            result.push_back({hk.id, {hk.virtualKey, hk.modifiers}});
        }
    }
    return result;
}

void HotkeyManager::loadHotkeys(const std::vector<std::pair<std::string, std::pair<int, int>>>& data) {
    for (const auto& entry : data) {
        registerHotkey(entry.first, entry.second.first, entry.second.second);
    }
}

// Weapon hotkey methods
bool HotkeyManager::registerWeaponHotkey(const std::string& weaponName, int virtualKey, int modifiers) {
    if (!m_hwnd || virtualKey == 0) return false;

    // First unregister any existing hotkey for this weapon
    unregisterWeaponHotkey(weaponName);

    // Find or create hotkey entry
    Hotkey* hotkey = nullptr;
    for (auto& hk : m_weaponHotkeys) {
        if (hk.id == weaponName) {
            hotkey = &hk;
            break;
        }
    }

    if (!hotkey) {
        m_weaponHotkeys.push_back({});
        hotkey = &m_weaponHotkeys.back();
        hotkey->id = weaponName;
    }

    hotkey->virtualKey = virtualKey;
    hotkey->modifiers = modifiers;

    // Register with Windows
    int id = getNextId();
    if (RegisterHotKey(m_hwnd, id, modifiers | MOD_NOREPEAT, virtualKey)) {
        hotkey->enabled = true;
        m_idToWeapon[id] = weaponName;
        return true;
    }

    hotkey->enabled = false;
    return false;
}

void HotkeyManager::unregisterWeaponHotkey(const std::string& weaponName) {
    if (!m_hwnd) return;

    // Find and unregister
    for (auto it = m_idToWeapon.begin(); it != m_idToWeapon.end(); ) {
        if (it->second == weaponName) {
            UnregisterHotKey(m_hwnd, it->first);
            it = m_idToWeapon.erase(it);
        } else {
            ++it;
        }
    }

    // Update hotkey entry
    for (auto& hk : m_weaponHotkeys) {
        if (hk.id == weaponName) {
            hk.enabled = false;
            break;
        }
    }
}

void HotkeyManager::unregisterAllWeaponHotkeys() {
    if (!m_hwnd) return;

    for (const auto& pair : m_idToWeapon) {
        UnregisterHotKey(m_hwnd, pair.first);
    }
    m_idToWeapon.clear();

    for (auto& hk : m_weaponHotkeys) {
        hk.enabled = false;
    }
    m_weaponHotkeys.clear();
}

Hotkey* HotkeyManager::getWeaponHotkey(const std::string& weaponName) {
    for (auto& hk : m_weaponHotkeys) {
        if (hk.id == weaponName) {
            return &hk;
        }
    }
    return nullptr;
}

const Hotkey* HotkeyManager::getWeaponHotkey(const std::string& weaponName) const {
    for (const auto& hk : m_weaponHotkeys) {
        if (hk.id == weaponName) {
            return &hk;
        }
    }
    return nullptr;
}

void HotkeyManager::suspend() {
    if (m_suspended || !m_hwnd) return;
    m_suspended = true;

    // Unregister all script hotkeys from Windows (but keep the data)
    for (const auto& pair : m_idToScript) {
        UnregisterHotKey(m_hwnd, pair.first);
    }

    // Unregister all weapon hotkeys from Windows (but keep the data)
    for (const auto& pair : m_idToWeapon) {
        UnregisterHotKey(m_hwnd, pair.first);
    }
}

void HotkeyManager::resume() {
    if (!m_suspended || !m_hwnd) return;
    m_suspended = false;

    // Re-register all script hotkeys
    // We need to clear and rebuild the id maps
    m_idToScript.clear();
    for (auto& hk : m_hotkeys) {
        if (hk.virtualKey != 0) {
            int id = getNextId();
            if (RegisterHotKey(m_hwnd, id, hk.modifiers | MOD_NOREPEAT, hk.virtualKey)) {
                hk.enabled = true;
                m_idToScript[id] = hk.id;
            } else {
                hk.enabled = false;
            }
        }
    }

    // Re-register all weapon hotkeys
    m_idToWeapon.clear();
    for (auto& hk : m_weaponHotkeys) {
        if (hk.virtualKey != 0) {
            int id = getNextId();
            if (RegisterHotKey(m_hwnd, id, hk.modifiers | MOD_NOREPEAT, hk.virtualKey)) {
                hk.enabled = true;
                m_idToWeapon[id] = hk.id;
            } else {
                hk.enabled = false;
            }
        }
    }
}
