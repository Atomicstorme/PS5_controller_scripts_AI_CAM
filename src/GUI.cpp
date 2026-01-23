#include "GUI.h"
#include "ConfigManager.h"
#include "Application.h"
#include "HotkeyManager.h"
#include "Overlay.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <fstream>
#include <sstream>

GUI::GUI() {
}

GUI::~GUI() {
    shutdown();
}

bool GUI::initialize(void* hwnd, void* device, void* deviceContext) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigDebugIsDebuggerPresent = false;  // Disable ImGui debug breaks when running in debugger

    // Set up styling
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Modern rounded look
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.ChildRounding = 4.0f;

    // Spacing
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // Custom color scheme - Modern dark blue
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.5f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.75f, 0.55f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.55f, 0.75f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.65f, 0.85f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.60f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.70f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.55f, 0.75f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.55f, 0.70f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.50f, 0.65f, 0.85f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.55f, 0.70f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.30f, 0.50f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.50f, 0.70f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.45f, 0.65f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.50f, 0.70f, 0.40f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(device),
                        static_cast<ID3D11DeviceContext*>(deviceContext));

    m_initialized = true;
    return true;
}

void GUI::shutdown() {
    if (m_initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void GUI::render(InputProcessor& processor) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Suspend/resume hotkeys based on text input state
    {
        Application* app = Application::getInstance();
        HotkeyManager& hotkeys = app->getHotkeys();
        bool textInputActive = ImGui::GetIO().WantTextInput;

        if (textInputActive && !m_wasTextInputActive) {
            // Text input just became active - suspend hotkeys
            hotkeys.suspend();
        } else if (!textInputActive && m_wasTextInputActive) {
            // Text input just became inactive - resume hotkeys
            hotkeys.resume();
        }
        m_wasTextInputActive = textInputActive;
    }

    // Handle hotkey capture mode
    if (m_capturingHotkey) {
        Application* app = Application::getInstance();
        HotkeyManager& hotkeys = app->getHotkeys();

        // Check for key presses using the newer ImGui API
        ImGuiIO& io = ImGui::GetIO();

        // Define keys to check for hotkey capture
        struct KeyMapping { ImGuiKey imguiKey; int vk; };
        static const KeyMapping keyMappings[] = {
            // Function keys
            {ImGuiKey_F1, VK_F1}, {ImGuiKey_F2, VK_F2}, {ImGuiKey_F3, VK_F3}, {ImGuiKey_F4, VK_F4},
            {ImGuiKey_F5, VK_F5}, {ImGuiKey_F6, VK_F6}, {ImGuiKey_F7, VK_F7}, {ImGuiKey_F8, VK_F8},
            {ImGuiKey_F9, VK_F9}, {ImGuiKey_F10, VK_F10}, {ImGuiKey_F11, VK_F11}, {ImGuiKey_F12, VK_F12},
            // Numpad
            {ImGuiKey_Keypad0, VK_NUMPAD0}, {ImGuiKey_Keypad1, VK_NUMPAD1}, {ImGuiKey_Keypad2, VK_NUMPAD2},
            {ImGuiKey_Keypad3, VK_NUMPAD3}, {ImGuiKey_Keypad4, VK_NUMPAD4}, {ImGuiKey_Keypad5, VK_NUMPAD5},
            {ImGuiKey_Keypad6, VK_NUMPAD6}, {ImGuiKey_Keypad7, VK_NUMPAD7}, {ImGuiKey_Keypad8, VK_NUMPAD8},
            {ImGuiKey_Keypad9, VK_NUMPAD9}, {ImGuiKey_KeypadMultiply, VK_MULTIPLY}, {ImGuiKey_KeypadAdd, VK_ADD},
            {ImGuiKey_KeypadSubtract, VK_SUBTRACT}, {ImGuiKey_KeypadDivide, VK_DIVIDE},
            // Letters
            {ImGuiKey_A, 'A'}, {ImGuiKey_B, 'B'}, {ImGuiKey_C, 'C'}, {ImGuiKey_D, 'D'}, {ImGuiKey_E, 'E'},
            {ImGuiKey_F, 'F'}, {ImGuiKey_G, 'G'}, {ImGuiKey_H, 'H'}, {ImGuiKey_I, 'I'}, {ImGuiKey_J, 'J'},
            {ImGuiKey_K, 'K'}, {ImGuiKey_L, 'L'}, {ImGuiKey_M, 'M'}, {ImGuiKey_N, 'N'}, {ImGuiKey_O, 'O'},
            {ImGuiKey_P, 'P'}, {ImGuiKey_Q, 'Q'}, {ImGuiKey_R, 'R'}, {ImGuiKey_S, 'S'}, {ImGuiKey_T, 'T'},
            {ImGuiKey_U, 'U'}, {ImGuiKey_V, 'V'}, {ImGuiKey_W, 'W'}, {ImGuiKey_X, 'X'}, {ImGuiKey_Y, 'Y'},
            {ImGuiKey_Z, 'Z'},
            // Numbers
            {ImGuiKey_0, '0'}, {ImGuiKey_1, '1'}, {ImGuiKey_2, '2'}, {ImGuiKey_3, '3'}, {ImGuiKey_4, '4'},
            {ImGuiKey_5, '5'}, {ImGuiKey_6, '6'}, {ImGuiKey_7, '7'}, {ImGuiKey_8, '8'}, {ImGuiKey_9, '9'},
            // Special keys
            {ImGuiKey_Escape, VK_ESCAPE}, {ImGuiKey_Delete, VK_DELETE}, {ImGuiKey_Backspace, VK_BACK},
            {ImGuiKey_Insert, VK_INSERT}, {ImGuiKey_Home, VK_HOME}, {ImGuiKey_End, VK_END},
            {ImGuiKey_PageUp, VK_PRIOR}, {ImGuiKey_PageDown, VK_NEXT}, {ImGuiKey_Pause, VK_PAUSE},
            {ImGuiKey_ScrollLock, VK_SCROLL}, {ImGuiKey_GraveAccent, VK_OEM_3},
        };

        for (const auto& mapping : keyMappings) {
            if (ImGui::IsKeyPressed(mapping.imguiKey)) {
                int vk = mapping.vk;
                int modifiers = 0;
                if (io.KeyCtrl) modifiers |= MOD_CONTROL;
                if (io.KeyAlt) modifiers |= MOD_ALT;
                if (io.KeyShift) modifiers |= MOD_SHIFT;

                bool wasWeaponCapture = hotkeys.isCapturingWeapon();
                std::string captureTarget = hotkeys.getCaptureTarget();

                if (hotkeys.processCaptureKey(vk, modifiers)) {
                    m_capturingHotkey = false;
                    m_hotkeyTarget.clear();

                    if (wasWeaponCapture) {
                        // Update weapon preset with hotkey info
                        const Hotkey* weaponHk = hotkeys.getWeaponHotkey(captureTarget);
                        if (weaponHk) {
                            const WeaponPreset* preset = app->getConfig().getWeaponPreset(captureTarget);
                            if (preset) {
                                WeaponPreset updatedPreset = *preset;
                                updatedPreset.hotkeyVk = weaponHk->virtualKey;
                                updatedPreset.hotkeyModifiers = weaponHk->modifiers;
                                app->getConfig().updateWeaponPreset(updatedPreset);
                            }
                        } else {
                            // Hotkey was cleared - update preset to remove hotkey
                            const WeaponPreset* preset = app->getConfig().getWeaponPreset(captureTarget);
                            if (preset) {
                                WeaponPreset updatedPreset = *preset;
                                updatedPreset.hotkeyVk = 0;
                                updatedPreset.hotkeyModifiers = 0;
                                app->getConfig().updateWeaponPreset(updatedPreset);
                            }
                        }
                    } else {
                        // Save script hotkeys immediately
                        app->getConfig().setHotkeys(hotkeys.serializeHotkeys());
                    }
                }
                break;
            }
        }
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Settings", "Ctrl+,")) {
                m_showSettings = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_shouldClose = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scripts")) {
            if (ImGui::MenuItem("Refresh Scripts", "F5")) {
                processor.getScriptManager().rescanScripts();
            }
            if (ImGui::MenuItem("Open Scripts Folder")) {
                std::string folder = processor.getScriptManager().getScriptsFolder();
                std::string cmd = "explorer \"" + folder + "\"";
                system(cmd.c_str());
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Demo Window", nullptr, &m_showDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Status bar at bottom
    renderStatusBar(processor);

    // Controller visualization (left panel)
    renderControllerView(processor);

    // Script list with parameters (right panel)
    renderScriptList(processor);

    // Settings window
    if (m_showSettings) {
        renderSettings(processor);
    }

    // Script editor
    if (m_showScriptEditor) {
        renderScriptEditor();
    }

    // Demo window
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void GUI::renderStatusBar(InputProcessor& processor) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float height = 32.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - height));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, height));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.08f, 1.0f));
    ImGui::Begin("StatusBar", nullptr, flags);

    // DualSense status with icon-like indicator
    bool dsConnected = processor.isDualSenseConnected();
    ImGui::PushStyleColor(ImGuiCol_Text, dsConnected ? ImVec4(0.40f, 0.90f, 0.50f, 1.0f) : ImVec4(0.90f, 0.40f, 0.40f, 1.0f));
    ImGui::Text("[*]");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::Text("DualSense");

    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "|");
    ImGui::SameLine(0, 20);

    // Virtual controller status
    bool vcConnected = processor.isVirtualConnected();
    ImGui::PushStyleColor(ImGuiCol_Text, vcConnected ? ImVec4(0.40f, 0.90f, 0.50f, 1.0f) : ImVec4(0.90f, 0.40f, 0.40f, 1.0f));
    ImGui::Text("[*]");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::Text("Virtual Controller");

    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "|");
    ImGui::SameLine(0, 20);

    // Processing status
    bool running = processor.isRunning();
    ImGui::PushStyleColor(ImGuiCol_Text, running ? ImVec4(0.40f, 0.90f, 0.50f, 1.0f) : ImVec4(0.90f, 0.90f, 0.40f, 1.0f));
    ImGui::Text(running ? "[>]" : "[||]");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::Text(running ? "Processing" : "Paused");

    // Controls on right side
    ImGui::SameLine(ImGui::GetWindowWidth() - 180);
    if (running) {
        if (ImGui::Button("Stop", ImVec2(60, 0))) {
            processor.stop();
        }
    } else {
        if (ImGui::Button("Start", ImVec2(60, 0))) {
            processor.start();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reconnect", ImVec2(80, 0))) {
        processor.reconnectDualSense();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void GUI::renderControllerView(InputProcessor& processor) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menuBarHeight = 22.0f;
    float statusBarHeight = 32.0f;
    float panelWidth = 380.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 8, viewport->WorkPos.y + menuBarHeight + 8));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, viewport->WorkSize.y - menuBarHeight - statusBarHeight - 24));

    ImGui::Begin("Controller Preview", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    const NormalizedState& input = processor.getInputState();
    const NormalizedState& output = processor.getOutputState();

    // Tabs for Input/Output comparison
    if (ImGui::BeginTabBar("ControllerTabs")) {
        if (ImGui::BeginTabItem("Side by Side")) {
            ImGui::Columns(2, "ControllerColumns", true);
            ImGui::SetColumnWidth(0, 175);

            // INPUT COLUMN
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("RAW INPUT");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Left Stick");
            ImGui::Text("  X: %+.2f", input.leftStickX);
            ImGui::Text("  Y: %+.2f", input.leftStickY);

            ImGui::Spacing();
            ImGui::Text("Right Stick");
            ImGui::Text("  X: %+.2f", input.rightStickX);
            ImGui::Text("  Y: %+.2f", input.rightStickY);

            ImGui::Spacing();
            ImGui::Text("Triggers");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            ImGui::ProgressBar(input.leftTrigger, ImVec2(-1, 14), "");
            ImGui::ProgressBar(input.rightTrigger, ImVec2(-1, 14), "");
            ImGui::PopStyleColor();

            ImGui::NextColumn();

            // OUTPUT COLUMN
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.6f, 1.0f));
            ImGui::Text("MODIFIED OUTPUT");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Left Stick");
            ImGui::Text("  X: %+.2f", output.leftStickX);
            ImGui::Text("  Y: %+.2f", output.leftStickY);

            ImGui::Spacing();
            ImGui::Text("Right Stick");
            ImGui::Text("  X: %+.2f", output.rightStickX);
            ImGui::Text("  Y: %+.2f", output.rightStickY);

            ImGui::Spacing();
            ImGui::Text("Triggers");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));
            ImGui::ProgressBar(output.leftTrigger, ImVec2(-1, 14), "");
            ImGui::ProgressBar(output.rightTrigger, ImVec2(-1, 14), "");
            ImGui::PopStyleColor();

            ImGui::Columns(1);

            // Buttons section (shared)
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Buttons (Output)");
            ImGui::Spacing();

            // Face buttons
            ImGui::TextColored(output.cross ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "X");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.circle ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "O");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.square ? ImVec4(0.8f, 0.4f, 0.8f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "[]");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.triangle ? ImVec4(0.4f, 0.8f, 0.8f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "/\\");

            ImGui::SameLine(0, 20);
            ImGui::TextColored(output.l1 ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "L1");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.r1 ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "R1");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.l3 ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "L3");
            ImGui::SameLine(0, 8);
            ImGui::TextColored(output.r3 ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.35f, 0.35f, 0.40f, 1.0f), "R3");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Detailed")) {
            ImGui::Text("Input State Details");
            ImGui::Separator();
            ImGui::Text("Left Stick:  X=%+.3f  Y=%+.3f", input.leftStickX, input.leftStickY);
            ImGui::Text("Right Stick: X=%+.3f  Y=%+.3f", input.rightStickX, input.rightStickY);
            ImGui::Text("L2: %.3f  R2: %.3f", input.leftTrigger, input.rightTrigger);
            ImGui::Text("D-Pad: %d", input.dpad);
            ImGui::Spacing();

            ImGui::Text("Output State Details");
            ImGui::Separator();
            ImGui::Text("Left Stick:  X=%+.3f  Y=%+.3f", output.leftStickX, output.leftStickY);
            ImGui::Text("Right Stick: X=%+.3f  Y=%+.3f", output.rightStickX, output.rightStickY);
            ImGui::Text("L2: %.3f  R2: %.3f", output.leftTrigger, output.rightTrigger);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void GUI::renderScriptList(InputProcessor& processor) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menuBarHeight = 22.0f;
    float statusBarHeight = 32.0f;
    float leftPanelWidth = 388.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + leftPanelWidth + 16, viewport->WorkPos.y + menuBarHeight + 8));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x - leftPanelWidth - 32, viewport->WorkSize.y - menuBarHeight - statusBarHeight - 24));

    ImGui::Begin("Scripts", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Profile selector section
    ConfigManager* config = processor.getConfigManager();
    if (config) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
        ImGui::BeginChild("ProfileSection", ImVec2(0, 60), true);

        ImGui::Text("Profile:");
        ImGui::SameLine();

        // Get profile names
        std::vector<std::string> profileNames = config->getProfileNames();
        std::string currentProfile = config->getCurrentProfileName();

        // Find current profile index
        int currentIdx = 0;
        for (size_t i = 0; i < profileNames.size(); i++) {
            if (profileNames[i] == currentProfile) {
                currentIdx = static_cast<int>(i);
                break;
            }
        }

        // Profile dropdown
        ImGui::SetNextItemWidth(180);
        if (ImGui::BeginCombo("##ProfileCombo", currentProfile.c_str())) {
            for (size_t i = 0; i < profileNames.size(); i++) {
                bool isSelected = (profileNames[i] == currentProfile);
                if (ImGui::Selectable(profileNames[i].c_str(), isSelected)) {
                    if (profileNames[i] != currentProfile) {
                        config->switchProfile(profileNames[i]);
                        // Rescan scripts to apply new profile settings
                        processor.getScriptManager().rescanScripts();
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        // New profile button
        if (ImGui::Button("+##NewProfile")) {
            m_showCreateProfile = true;
            memset(m_newProfileName, 0, sizeof(m_newProfileName));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Create new profile");
        }

        ImGui::SameLine();

        // Rename profile button
        if (ImGui::Button("R##RenameProfile")) {
            if (currentProfile != "Default") {
                m_showRenameProfile = true;
                m_profileToRename = currentProfile;
                strncpy_s(m_renameProfileBuffer, currentProfile.c_str(), sizeof(m_renameProfileBuffer) - 1);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(currentProfile == "Default" ? "Cannot rename Default profile" : "Rename profile");
        }

        ImGui::SameLine();

        // Delete profile button
        if (ImGui::Button("X##DeleteProfile")) {
            if (currentProfile != "Default") {
                m_showDeleteConfirm = true;
                m_profileToDelete = currentProfile;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(currentProfile == "Default" ? "Cannot delete Default profile" : "Delete profile");
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Create profile popup
        if (m_showCreateProfile) {
            ImGui::OpenPopup("Create Profile");
            m_showCreateProfile = false;
        }
        if (ImGui::BeginPopupModal("Create Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter new profile name:");
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##NewProfileName", m_newProfileName, sizeof(m_newProfileName));

            ImGui::Spacing();
            if (ImGui::Button("Create", ImVec2(90, 0))) {
                if (strlen(m_newProfileName) > 0) {
                    if (config->createProfile(m_newProfileName)) {
                        config->switchProfile(m_newProfileName);
                        processor.getScriptManager().rescanScripts();
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Rename profile popup
        if (m_showRenameProfile) {
            ImGui::OpenPopup("Rename Profile");
            m_showRenameProfile = false;
        }
        if (ImGui::BeginPopupModal("Rename Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Rename profile '%s' to:", m_profileToRename.c_str());
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##RenameProfileName", m_renameProfileBuffer, sizeof(m_renameProfileBuffer));

            ImGui::Spacing();
            if (ImGui::Button("Rename", ImVec2(90, 0))) {
                if (strlen(m_renameProfileBuffer) > 0 && m_renameProfileBuffer != m_profileToRename) {
                    config->renameProfile(m_profileToRename, m_renameProfileBuffer);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Delete confirmation popup
        if (m_showDeleteConfirm) {
            ImGui::OpenPopup("Delete Profile?");
            m_showDeleteConfirm = false;
        }
        if (ImGui::BeginPopupModal("Delete Profile?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete profile '%s'?", m_profileToDelete.c_str());
            ImGui::Text("This action cannot be undone.");

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(90, 0))) {
                config->deleteProfile(m_profileToDelete);
                processor.getScriptManager().rescanScripts();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();

        // Weapon Presets Section (collapsible)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.35f, 0.45f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.40f, 0.50f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.45f, 0.55f, 1.0f));

        if (ImGui::CollapsingHeader("Weapon Presets (Anti-Recoil)", m_weaponPresetExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
            m_weaponPresetExpanded = true;

            std::vector<std::string> weaponNames = config->getWeaponPresetNames();
            std::string activeWeapon = config->getActiveWeaponName();

            ImGui::Text("Weapon:");
            ImGui::SameLine();

            // Weapon dropdown
            ImGui::SetNextItemWidth(150);
            const char* previewName = activeWeapon.empty() ? "(No weapons)" : activeWeapon.c_str();
            if (ImGui::BeginCombo("##WeaponCombo", previewName)) {
                for (size_t i = 0; i < weaponNames.size(); i++) {
                    bool isSelected = (weaponNames[i] == activeWeapon);
                    if (ImGui::Selectable(weaponNames[i].c_str(), isSelected)) {
                        config->setActiveWeapon(weaponNames[i]);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            // Add weapon button
            if (ImGui::Button("+##NewWeapon")) {
                m_showCreateWeapon = true;
                memset(m_newWeaponName, 0, sizeof(m_newWeaponName));
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Add new weapon preset");
            }

            ImGui::SameLine();

            // Delete weapon button
            bool hasWeapons = !weaponNames.empty();
            if (ImGui::Button("X##DeleteWeapon") && hasWeapons) {
                m_showDeleteWeaponConfirm = true;
                m_weaponToDelete = activeWeapon;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(hasWeapons ? "Delete current weapon preset" : "No weapon to delete");
            }

            // Show weapon settings if a weapon is selected
            const WeaponPreset* preset = config->getActiveWeaponPreset();
            if (preset) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Create a mutable copy for editing
                WeaponPreset editPreset = *preset;
                bool changed = false;

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));

                ImGui::Text("ADS Strength:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##AdsStrength", &editPreset.adsStrength, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.adsStrength);

                ImGui::Text("Hip-Fire Strength:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##HipFireStrength", &editPreset.hipFireStrength, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.hipFireStrength);

                ImGui::Text("Horizontal:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##HorizontalStrength", &editPreset.horizontalStrength, -1.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.horizontalStrength);

                ImGui::Text("ADS Threshold:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##AdsThreshold", &editPreset.adsThreshold, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.adsThreshold);

                ImGui::Text("Fire Threshold:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##FireThreshold", &editPreset.fireThreshold, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.fireThreshold);

                ImGui::Text("Smoothing:");
                ImGui::SameLine(140);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SliderFloat("##Smoothing", &editPreset.smoothing, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("%.2f", editPreset.smoothing);

                ImGui::PopStyleVar();

                // Weapon hotkey
                ImGui::Spacing();
                ImGui::Text("Hotkey:");
                ImGui::SameLine(140);

                Application* app = Application::getInstance();
                HotkeyManager& hotkeys = app->getHotkeys();
                const Hotkey* weaponHk = hotkeys.getWeaponHotkey(editPreset.name);
                std::string weaponHotkeyLabel = weaponHk && weaponHk->virtualKey != 0 ? weaponHk->getDisplayName() : "Not Set";

                // Check if we're capturing a hotkey for this weapon
                if (m_capturingHotkey && m_hotkeyTarget == ("weapon:" + editPreset.name)) {
                    weaponHotkeyLabel = "[Press Key]";
                }

                if (ImGui::Button(weaponHotkeyLabel.c_str(), ImVec2(100, 0))) {
                    if (!m_capturingHotkey) {
                        m_capturingHotkey = true;
                        m_hotkeyTarget = "weapon:" + editPreset.name;
                        hotkeys.startWeaponCapture(editPreset.name);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click to set hotkey for quick weapon switch (Esc=cancel, Del=clear)");
                }

                if (changed) {
                    config->updateWeaponPreset(editPreset);
                }
            } else if (weaponNames.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No weapon presets. Click + to add one.");
            }

            // Create weapon popup
            if (m_showCreateWeapon) {
                ImGui::OpenPopup("Create Weapon Preset");
                m_showCreateWeapon = false;
            }
            if (ImGui::BeginPopupModal("Create Weapon Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Enter weapon name:");
                ImGui::SetNextItemWidth(200);
                ImGui::InputText("##NewWeaponName", m_newWeaponName, sizeof(m_newWeaponName));

                ImGui::Spacing();
                if (ImGui::Button("Create", ImVec2(90, 0))) {
                    if (strlen(m_newWeaponName) > 0) {
                        if (config->createWeaponPreset(m_newWeaponName)) {
                            config->setActiveWeapon(m_newWeaponName);
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // Delete weapon confirmation popup
            if (m_showDeleteWeaponConfirm) {
                ImGui::OpenPopup("Delete Weapon?");
                m_showDeleteWeaponConfirm = false;
            }
            if (ImGui::BeginPopupModal("Delete Weapon?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete weapon preset '%s'?", m_weaponToDelete.c_str());

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Delete", ImVec2(90, 0))) {
                    config->deleteWeaponPreset(m_weaponToDelete);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        } else {
            m_weaponPresetExpanded = false;
        }

        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }

    auto& scripts = processor.getScriptManager().getScripts();

    if (scripts.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No scripts found.\n\nPlace .lua files in the 'scripts' folder and click Refresh.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        if (ImGui::Button("Open Scripts Folder")) {
            std::string folder = processor.getScriptManager().getScriptsFolder();
            std::string cmd = "explorer \"" + folder + "\"";
            system(cmd.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            processor.getScriptManager().rescanScripts();
        }
    } else {
        // Header row
        ImGui::Text("%zu script(s) loaded", scripts.size());
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        if (ImGui::Button("Refresh All")) {
            processor.getScriptManager().rescanScripts();
        }
        ImGui::Separator();
        ImGui::Spacing();

        // Script list with collapsible settings
        for (size_t i = 0; i < scripts.size(); i++) {
            auto& script = scripts[i];
            ImGui::PushID(static_cast<int>(i));

            // Script header with checkbox and collapsible
            bool enabled = script.config.enabled;

            // Enable checkbox with colored background based on state
            if (!script.loaded) {
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
            }
            if (ImGui::Checkbox("##enabled", &enabled)) {
                processor.getScriptManager().setScriptEnabled(script.config.name, enabled);
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Collapsible header for script
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed;
            if (i == 0) nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

            bool nodeOpen = ImGui::TreeNodeEx(script.config.name.c_str(), nodeFlags);

            // Error indicator in header
            if (!script.loaded && script.engine) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "(Error)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", script.engine->getLastError().c_str());
                }
            }

            // Hotkey button and reorder buttons on the right
            Application* app = Application::getInstance();
            HotkeyManager& hotkeys = app->getHotkeys();
            const Hotkey* hk = hotkeys.getHotkey(script.config.name);
            std::string hotkeyLabel = hk && hk->virtualKey != 0 ? hk->getDisplayName() : "...";

            // Check if we're capturing a hotkey for this script
            if (m_capturingHotkey && m_hotkeyTarget == script.config.name) {
                hotkeyLabel = "[Press Key]";
            }

            ImGui::SameLine(ImGui::GetWindowWidth() - 140);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
            if (ImGui::Button(hotkeyLabel.c_str(), ImVec2(60, 20))) {
                if (!m_capturingHotkey) {
                    m_capturingHotkey = true;
                    m_hotkeyTarget = script.config.name;
                    hotkeys.startCapture(script.config.name);
                }
            }
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to set hotkey (Esc=cancel, Del=clear)");
            }

            // Reorder buttons on the right
            ImGui::SameLine(ImGui::GetWindowWidth() - 70);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
            if (i > 0) {
                if (ImGui::Button("^", ImVec2(24, 20))) {
                    processor.getScriptManager().moveScriptUp(i);
                }
            } else {
                ImGui::Dummy(ImVec2(24, 20));
            }
            ImGui::SameLine(0, 4);
            if (i < scripts.size() - 1) {
                if (ImGui::Button("v", ImVec2(24, 20))) {
                    processor.getScriptManager().moveScriptDown(i);
                }
            } else {
                ImGui::Dummy(ImVec2(24, 20));
            }
            ImGui::PopStyleVar();

            if (nodeOpen) {
                ImGui::Indent(20.0f);

                // Script info
                if (!script.config.description.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::TextWrapped("%s", script.config.description.c_str());
                    ImGui::PopStyleColor();
                }

                if (!script.config.author.empty() || !script.config.version.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
                    if (!script.config.author.empty() && !script.config.version.empty()) {
                        ImGui::Text("by %s | v%s", script.config.author.c_str(), script.config.version.c_str());
                    } else if (!script.config.author.empty()) {
                        ImGui::Text("by %s", script.config.author.c_str());
                    } else {
                        ImGui::Text("v%s", script.config.version.c_str());
                    }
                    ImGui::PopStyleColor();
                }

                // Parameters section
                if (!script.config.parameters.empty()) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.85f, 1.0f));
                    ImGui::Text("Settings");
                    ImGui::PopStyleColor();
                    ImGui::Spacing();

                    for (auto& param : script.config.parameters) {
                        ImGui::PushID(param.key.c_str());

                        // Parameter label with tooltip
                        ImGui::Text("%s", param.displayName.c_str());
                        if (!param.description.empty() && ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", param.description.c_str());
                        }

                        ImGui::SameLine(180);
                        ImGui::SetNextItemWidth(-1);

                        bool valueChanged = false;
                        float newValue = param.value;

                        switch (param.type) {
                            case ParamType::Float: {
                                valueChanged = ImGui::SliderFloat("##value", &newValue,
                                    param.minValue, param.maxValue, "%.2f");
                                break;
                            }
                            case ParamType::Int: {
                                int intVal = static_cast<int>(param.value);
                                if (ImGui::SliderInt("##value", &intVal,
                                    static_cast<int>(param.minValue),
                                    static_cast<int>(param.maxValue))) {
                                    newValue = static_cast<float>(intVal);
                                    valueChanged = true;
                                }
                                break;
                            }
                            case ParamType::Bool: {
                                bool boolVal = param.value > 0.5f;
                                if (ImGui::Checkbox("##value", &boolVal)) {
                                    newValue = boolVal ? 1.0f : 0.0f;
                                    valueChanged = true;
                                }
                                break;
                            }
                            case ParamType::Choice: {
                                if (!param.choices.empty()) {
                                    int choiceIdx = static_cast<int>(param.value);
                                    if (choiceIdx >= 0 && choiceIdx < static_cast<int>(param.choices.size())) {
                                        if (ImGui::BeginCombo("##value", param.choices[choiceIdx].c_str())) {
                                            for (int c = 0; c < static_cast<int>(param.choices.size()); c++) {
                                                bool selected = (c == choiceIdx);
                                                if (ImGui::Selectable(param.choices[c].c_str(), selected)) {
                                                    newValue = static_cast<float>(c);
                                                    valueChanged = true;
                                                }
                                            }
                                            ImGui::EndCombo();
                                        }
                                    }
                                }
                                break;
                            }
                        }

                        // Update parameter value in script engine and save to config
                        if (valueChanged) {
                            param.value = newValue;
                            processor.getScriptManager().setScriptParameter(script.config.name, param.key, newValue);
                        }

                        ImGui::PopID();
                    }

                    // Reset to defaults button
                    ImGui::Spacing();
                    if (ImGui::Button("Reset to Defaults")) {
                        for (auto& param : script.config.parameters) {
                            param.value = param.defaultValue;
                            processor.getScriptManager().setScriptParameter(script.config.name, param.key, param.defaultValue);
                        }
                    }
                }

                ImGui::Unindent(20.0f);
                ImGui::TreePop();
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    ImGui::End();
}

void GUI::renderScriptEditor() {
    ImGui::Begin("Script Editor", &m_showScriptEditor, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                // Save script
                if (!m_currentEditingScript.empty()) {
                    std::ofstream file(m_currentEditingScript);
                    if (file.is_open()) {
                        file << m_scriptEditorBuffer;
                        file.close();
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::InputTextMultiline("##ScriptSource", m_scriptEditorBuffer, sizeof(m_scriptEditorBuffer),
                               ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);

    ImGui::End();
}

void GUI::renderSettings(InputProcessor& processor) {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Settings", &m_showSettings);

    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Poll Rate");
        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##PollRate", &m_pollRate, 100.0f, 1000.0f, "%.0f Hz")) {
            processor.setPollRate(m_pollRate);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
        ImGui::TextWrapped("Higher values mean lower latency but more CPU usage.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Controller LED", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float ledColor[3] = {0.0f, 0.5f, 1.0f};
        ImGui::Text("LED Color");
        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit3("##LED", ledColor, ImGuiColorEditFlags_NoInputs)) {
            processor.getDualSense().setLEDColor(
                static_cast<uint8_t>(ledColor[0] * 255),
                static_cast<uint8_t>(ledColor[1] * 255),
                static_cast<uint8_t>(ledColor[2] * 255)
            );
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Overlay", ImGuiTreeNodeFlags_DefaultOpen)) {
        Application* app = Application::getInstance();
        Overlay& overlay = app->getOverlay();
        ConfigManager& config = app->getConfig();

        // Overlay enabled checkbox
        bool overlayEnabled = overlay.isVisible();
        ImGui::Text("Show Overlay");
        ImGui::SameLine(120);
        if (ImGui::Checkbox("##OverlayEnabled", &overlayEnabled)) {
            if (overlayEnabled) {
                overlay.show();
            } else {
                overlay.hide();
            }
            config.getSettings().overlayEnabled = overlayEnabled;
        }

        // Position dropdown
        ImGui::Text("Position");
        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(-1);
        const char* positions[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
        int currentPos = static_cast<int>(overlay.getPosition());
        if (ImGui::Combo("##OverlayPosition", &currentPos, positions, 4)) {
            overlay.setPosition(static_cast<OverlayPosition>(currentPos));
            config.getSettings().overlayPosition = static_cast<OverlayPosition>(currentPos);
        }

        // Opacity slider
        ImGui::Text("Opacity");
        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(-1);
        float opacity = overlay.getOpacity();
        if (ImGui::SliderFloat("##OverlayOpacity", &opacity, 0.1f, 1.0f, "%.0f%%", ImGuiSliderFlags_None)) {
            overlay.setOpacity(opacity);
            config.getSettings().overlayOpacity = opacity;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
        ImGui::TextWrapped("Press F11 to toggle overlay visibility.");
        ImGui::PopStyleColor();
    }

    ImGui::End();
}
