#pragma once

#include "Common.h"
#include <hidapi.h>

class DualSenseController {
public:
    DualSenseController();
    ~DualSenseController();

    // Connection
    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }

    // Reading
    bool update();
    const ControllerState& getState() const { return m_state; }
    NormalizedState getNormalizedState() const;

    // LED and haptics (optional features)
    void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
    void setPlayerLED(uint8_t pattern);

    // Info
    std::string getDevicePath() const { return m_devicePath; }
    bool isUSB() const { return m_isUSB; }
    bool isBluetooth() const { return !m_isUSB; }

private:
    void parseInputReport(const uint8_t* data, size_t length);
    void parseUSBReport(const uint8_t* data);
    void parseBluetoothReport(const uint8_t* data);
    bool sendOutputReport();

    hid_device* m_device = nullptr;
    ControllerState m_state;
    ControllerState m_prevState;
    std::string m_devicePath;
    bool m_connected = false;
    bool m_isUSB = true;

    // Output report state
    uint8_t m_ledR = 0;
    uint8_t m_ledG = 0;
    uint8_t m_ledB = 255;
    uint8_t m_playerLED = 0;
    bool m_outputDirty = true;

    // Timing
    std::chrono::high_resolution_clock::time_point m_lastUpdate;

    // Report buffer
    static constexpr size_t REPORT_SIZE = 78;
    uint8_t m_reportBuffer[REPORT_SIZE];
};
