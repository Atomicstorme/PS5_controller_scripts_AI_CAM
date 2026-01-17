#include "InputProcessor.h"
#include <iostream>

InputProcessor::InputProcessor() {
    m_lastUpdate = std::chrono::high_resolution_clock::now();
}

InputProcessor::~InputProcessor() {
    stop();
}

bool InputProcessor::initialize() {
    // Initialize script manager
    if (!m_scriptManager.initialize("scripts")) {
        std::cerr << "Warning: Failed to initialize script manager" << std::endl;
    }

    // Try to connect to DualSense
    if (!m_dualSense.connect()) {
        std::cerr << "Warning: DualSense controller not found" << std::endl;
    } else {
        // Set LED to indicate we're connected
        m_dualSense.setLEDColor(0, 255, 128);  // Cyan-green
    }

    // Try to connect virtual controller
    if (!m_virtual.connect()) {
        std::cerr << "Error: Failed to connect virtual controller: " << m_virtual.getLastError() << std::endl;
        return false;
    }

    return true;
}

bool InputProcessor::start() {
    if (m_running) {
        return true;
    }

    if (!m_virtual.isConnected()) {
        if (!m_virtual.connect()) {
            return false;
        }
    }

    m_shouldStop = false;
    m_running = true;
    m_thread = std::thread(&InputProcessor::processingLoop, this);

    return true;
}

void InputProcessor::stop() {
    if (!m_running) {
        return;
    }

    m_shouldStop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_running = false;
}

bool InputProcessor::update() {
    auto now = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_lastUpdate).count();
    m_lastUpdate = now;

    // Try to reconnect DualSense if disconnected
    if (!m_dualSense.isConnected()) {
        m_dualSense.connect();
    }

    // Read from DualSense
    bool hasInput = m_dualSense.update();

    if (hasInput) {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        // Get normalized input
        m_inputState = m_dualSense.getNormalizedState();
        m_inputState.deltaTime = deltaTime;

        // Process through scripts
        m_outputState = m_scriptManager.process(m_inputState, deltaTime);

        // Send to virtual controller
        m_virtual.update(m_outputState);
    }

    return hasInput;
}

void InputProcessor::processingLoop() {
    using namespace std::chrono;

    while (!m_shouldStop) {
        auto start = high_resolution_clock::now();

        update();

        // Calculate sleep time based on poll rate
        auto elapsed = high_resolution_clock::now() - start;
        auto targetDuration = microseconds(static_cast<long long>(1000000.0 / m_pollRateHz));

        if (elapsed < targetDuration) {
            std::this_thread::sleep_for(targetDuration - elapsed);
        }
    }
}

bool InputProcessor::reconnectDualSense() {
    m_dualSense.disconnect();
    bool connected = m_dualSense.connect();
    if (connected) {
        m_dualSense.setLEDColor(0, 255, 128);
    }
    return connected;
}

bool InputProcessor::reconnectVirtual() {
    m_virtual.disconnect();
    return m_virtual.connect();
}
