#pragma once

#include "Common.h"
#include "DualSenseController.h"
#include "VirtualController.h"
#include "ScriptManager.h"

class InputProcessor {
public:
    InputProcessor();
    ~InputProcessor();

    // Initialize all components
    bool initialize();

    // Start/stop processing loop
    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    // Manual update (for testing or custom loop)
    bool update();

    // Access components
    DualSenseController& getDualSense() { return m_dualSense; }
    VirtualController& getVirtual() { return m_virtual; }
    ScriptManager& getScriptManager() { return m_scriptManager; }

    // Get current states
    const NormalizedState& getInputState() const { return m_inputState; }
    const NormalizedState& getOutputState() const { return m_outputState; }

    // Settings
    void setPollRate(float hz) { m_pollRateHz = hz; }
    float getPollRate() const { return m_pollRateHz; }

    // Status
    bool isDualSenseConnected() const { return m_dualSense.isConnected(); }
    bool isVirtualConnected() const { return m_virtual.isConnected(); }

    // Reconnect
    bool reconnectDualSense();
    bool reconnectVirtual();

private:
    void processingLoop();

    DualSenseController m_dualSense;
    VirtualController m_virtual;
    ScriptManager m_scriptManager;

    NormalizedState m_inputState;
    NormalizedState m_outputState;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};

    float m_pollRateHz = 1000.0f;

    std::chrono::high_resolution_clock::time_point m_lastUpdate;
    std::mutex m_stateMutex;
};
