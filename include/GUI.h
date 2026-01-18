#pragma once

#include "Common.h"
#include "InputProcessor.h"

class ConfigManager;

class GUI {
public:
    GUI();
    ~GUI();

    // Initialize ImGui
    bool initialize(void* hwnd, void* device, void* deviceContext);
    void shutdown();

    // Render frame
    void render(InputProcessor& processor);

    // Is window requesting close?
    bool shouldClose() const { return m_shouldClose; }

private:
    void renderStatusBar(InputProcessor& processor);
    void renderControllerView(InputProcessor& processor);
    void renderScriptList(InputProcessor& processor);
    void renderScriptEditor();
    void renderSettings(InputProcessor& processor);

    bool m_initialized = false;
    bool m_shouldClose = false;

    // UI State
    bool m_showDemo = false;
    bool m_showSettings = false;
    bool m_showScriptEditor = false;
    int m_selectedScript = -1;
    char m_scriptEditorBuffer[65536] = {0};
    std::string m_currentEditingScript;

    // Settings
    float m_pollRate = 1000.0f;

    // Profile management UI state
    char m_newProfileName[64] = {0};
    char m_renameProfileBuffer[64] = {0};
    bool m_showCreateProfile = false;
    bool m_showRenameProfile = false;
    bool m_showDeleteConfirm = false;
    std::string m_profileToRename;
    std::string m_profileToDelete;

    // Weapon preset UI state
    char m_newWeaponName[64] = {0};
    bool m_showCreateWeapon = false;
    bool m_showDeleteWeaponConfirm = false;
    std::string m_weaponToDelete;
    bool m_weaponPresetExpanded = false;

    // Hotkey UI state
    bool m_capturingHotkey = false;
    std::string m_hotkeyTarget;
    bool m_wasTextInputActive = false;
};
