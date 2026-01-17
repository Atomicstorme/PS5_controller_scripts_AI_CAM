#pragma once

#include "Common.h"
#include "InputProcessor.h"

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
};
