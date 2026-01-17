#include "DualSenseController.h"
#include <cstring>
#include <iostream>

DualSenseController::DualSenseController() {
    hid_init();
    m_lastUpdate = std::chrono::high_resolution_clock::now();
    std::memset(m_reportBuffer, 0, REPORT_SIZE);
}

DualSenseController::~DualSenseController() {
    disconnect();
    hid_exit();
}

bool DualSenseController::connect() {
    if (m_connected) {
        disconnect();
    }

    // Try to find DualSense controller
    hid_device_info* devices = hid_enumerate(DUALSENSE_VENDOR_ID, 0);
    hid_device_info* current = devices;

    while (current) {
        if (current->product_id == DUALSENSE_PRODUCT_ID ||
            current->product_id == DUALSENSE_EDGE_PRODUCT_ID) {

            m_device = hid_open_path(current->path);
            if (m_device) {
                m_devicePath = current->path;
                // USB reports have different structure than Bluetooth
                // USB interface 3 is the one we want, or check usage page
                m_isUSB = (current->interface_number == 3) ||
                          (current->usage_page == 1 && current->usage == 5);
                m_connected = true;

                // Set non-blocking mode
                hid_set_nonblocking(m_device, 1);

                // Initialize LED
                m_outputDirty = true;
                sendOutputReport();

                break;
            }
        }
        current = current->next;
    }

    hid_free_enumeration(devices);
    return m_connected;
}

void DualSenseController::disconnect() {
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }
    m_connected = false;
    m_devicePath.clear();
}

bool DualSenseController::update() {
    if (!m_connected || !m_device) {
        return false;
    }

    m_prevState = m_state;

    // Read input report
    int bytesRead = hid_read(m_device, m_reportBuffer, REPORT_SIZE);

    if (bytesRead > 0) {
        parseInputReport(m_reportBuffer, bytesRead);

        // Update timestamp
        auto now = std::chrono::high_resolution_clock::now();
        m_state.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        m_lastUpdate = now;

        return true;
    } else if (bytesRead < 0) {
        // Error - device likely disconnected
        disconnect();
        return false;
    }

    // No data available (non-blocking)
    return false;
}

void DualSenseController::parseInputReport(const uint8_t* data, size_t length) {
    if (length < 10) return;

    // Report ID determines USB vs Bluetooth
    uint8_t reportId = data[0];

    if (reportId == 0x01) {
        // USB report
        parseUSBReport(data);
    } else if (reportId == 0x31) {
        // Bluetooth report
        parseBluetoothReport(data);
    }
}

void DualSenseController::parseUSBReport(const uint8_t* data) {
    // USB report structure (offset by 1 for report ID)
    m_state.leftStickX = data[1];
    m_state.leftStickY = data[2];
    m_state.rightStickX = data[3];
    m_state.rightStickY = data[4];
    m_state.leftTrigger = data[5];
    m_state.rightTrigger = data[6];

    // Buttons byte 8
    uint8_t buttons1 = data[8];
    m_state.dpad = buttons1 & 0x0F;
    m_state.square = (buttons1 & 0x10) != 0;
    m_state.cross = (buttons1 & 0x20) != 0;
    m_state.circle = (buttons1 & 0x40) != 0;
    m_state.triangle = (buttons1 & 0x80) != 0;

    // Buttons byte 9
    uint8_t buttons2 = data[9];
    m_state.l1 = (buttons2 & 0x01) != 0;
    m_state.r1 = (buttons2 & 0x02) != 0;
    m_state.l2Button = (buttons2 & 0x04) != 0;
    m_state.r2Button = (buttons2 & 0x08) != 0;
    m_state.share = (buttons2 & 0x10) != 0;
    m_state.options = (buttons2 & 0x20) != 0;
    m_state.l3 = (buttons2 & 0x40) != 0;
    m_state.r3 = (buttons2 & 0x80) != 0;

    // Buttons byte 10
    uint8_t buttons3 = data[10];
    m_state.ps = (buttons3 & 0x01) != 0;
    m_state.touchpad = (buttons3 & 0x02) != 0;
    m_state.mute = (buttons3 & 0x04) != 0;

    // Gyro and accelerometer (bytes 16-27)
    m_state.gyroX = static_cast<int16_t>(data[16] | (data[17] << 8));
    m_state.gyroY = static_cast<int16_t>(data[18] | (data[19] << 8));
    m_state.gyroZ = static_cast<int16_t>(data[20] | (data[21] << 8));
    m_state.accelX = static_cast<int16_t>(data[22] | (data[23] << 8));
    m_state.accelY = static_cast<int16_t>(data[24] | (data[25] << 8));
    m_state.accelZ = static_cast<int16_t>(data[26] | (data[27] << 8));

    // Touch data (bytes 33+)
    if (data[33] & 0x80) {
        m_state.touchActive = false;
    } else {
        m_state.touchActive = true;
        m_state.touchX = ((data[35] & 0x0F) << 8) | data[34];
        m_state.touchY = (data[36] << 4) | ((data[35] & 0xF0) >> 4);
    }
}

void DualSenseController::parseBluetoothReport(const uint8_t* data) {
    // Bluetooth report has 1 byte offset compared to USB
    m_state.leftStickX = data[2];
    m_state.leftStickY = data[3];
    m_state.rightStickX = data[4];
    m_state.rightStickY = data[5];
    m_state.leftTrigger = data[6];
    m_state.rightTrigger = data[7];

    // Buttons
    uint8_t buttons1 = data[9];
    m_state.dpad = buttons1 & 0x0F;
    m_state.square = (buttons1 & 0x10) != 0;
    m_state.cross = (buttons1 & 0x20) != 0;
    m_state.circle = (buttons1 & 0x40) != 0;
    m_state.triangle = (buttons1 & 0x80) != 0;

    uint8_t buttons2 = data[10];
    m_state.l1 = (buttons2 & 0x01) != 0;
    m_state.r1 = (buttons2 & 0x02) != 0;
    m_state.l2Button = (buttons2 & 0x04) != 0;
    m_state.r2Button = (buttons2 & 0x08) != 0;
    m_state.share = (buttons2 & 0x10) != 0;
    m_state.options = (buttons2 & 0x20) != 0;
    m_state.l3 = (buttons2 & 0x40) != 0;
    m_state.r3 = (buttons2 & 0x80) != 0;

    uint8_t buttons3 = data[11];
    m_state.ps = (buttons3 & 0x01) != 0;
    m_state.touchpad = (buttons3 & 0x02) != 0;
    m_state.mute = (buttons3 & 0x04) != 0;

    // Gyro/Accel at different offsets for Bluetooth
    m_state.gyroX = static_cast<int16_t>(data[17] | (data[18] << 8));
    m_state.gyroY = static_cast<int16_t>(data[19] | (data[20] << 8));
    m_state.gyroZ = static_cast<int16_t>(data[21] | (data[22] << 8));
    m_state.accelX = static_cast<int16_t>(data[23] | (data[24] << 8));
    m_state.accelY = static_cast<int16_t>(data[25] | (data[26] << 8));
    m_state.accelZ = static_cast<int16_t>(data[27] | (data[28] << 8));
}

NormalizedState DualSenseController::getNormalizedState() const {
    NormalizedState norm;

    // Normalize sticks to -1.0 to 1.0
    norm.leftStickX = normalizeStick(m_state.leftStickX);
    norm.leftStickY = normalizeStick(m_state.leftStickY);
    norm.rightStickX = normalizeStick(m_state.rightStickX);
    norm.rightStickY = normalizeStick(m_state.rightStickY);

    // Normalize triggers to 0.0 to 1.0
    norm.leftTrigger = normalizeTrigger(m_state.leftTrigger);
    norm.rightTrigger = normalizeTrigger(m_state.rightTrigger);

    // Copy buttons
    norm.square = m_state.square;
    norm.cross = m_state.cross;
    norm.circle = m_state.circle;
    norm.triangle = m_state.triangle;
    norm.l1 = m_state.l1;
    norm.r1 = m_state.r1;
    norm.l2Button = m_state.l2Button;
    norm.r2Button = m_state.r2Button;
    norm.share = m_state.share;
    norm.options = m_state.options;
    norm.l3 = m_state.l3;
    norm.r3 = m_state.r3;
    norm.ps = m_state.ps;
    norm.touchpad = m_state.touchpad;
    norm.mute = m_state.mute;
    norm.dpad = m_state.dpad;

    // Normalize gyro (typical range is about -2000 to 2000 for normal movement)
    constexpr float GYRO_SCALE = 1.0f / 2000.0f;
    norm.gyroX = static_cast<float>(m_state.gyroX) * GYRO_SCALE;
    norm.gyroY = static_cast<float>(m_state.gyroY) * GYRO_SCALE;
    norm.gyroZ = static_cast<float>(m_state.gyroZ) * GYRO_SCALE;

    return norm;
}

void DualSenseController::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    if (m_ledR != r || m_ledG != g || m_ledB != b) {
        m_ledR = r;
        m_ledG = g;
        m_ledB = b;
        m_outputDirty = true;
        sendOutputReport();
    }
}

void DualSenseController::setPlayerLED(uint8_t pattern) {
    if (m_playerLED != pattern) {
        m_playerLED = pattern;
        m_outputDirty = true;
        sendOutputReport();
    }
}

bool DualSenseController::sendOutputReport() {
    if (!m_connected || !m_device || !m_outputDirty) {
        return false;
    }

    uint8_t outputReport[48] = {0};

    if (m_isUSB) {
        outputReport[0] = 0x02;  // USB output report ID
        outputReport[1] = 0xFF;  // Feature flags 1
        outputReport[2] = 0x04;  // Feature flags 2 (LED)

        // LED color
        outputReport[45] = m_ledR;
        outputReport[46] = m_ledG;
        outputReport[47] = m_ledB;

        // Player LEDs
        outputReport[44] = m_playerLED;

        int result = hid_write(m_device, outputReport, sizeof(outputReport));
        m_outputDirty = (result < 0);
        return result >= 0;
    } else {
        // Bluetooth output report
        outputReport[0] = 0x31;  // Bluetooth output report ID
        outputReport[1] = 0x02;  // Sequence tag
        outputReport[2] = 0xFF;  // Feature flags 1
        outputReport[3] = 0x04;  // Feature flags 2

        outputReport[46] = m_ledR;
        outputReport[47] = m_ledG;
        // Note: Bluetooth needs CRC which we're skipping for simplicity

        int result = hid_write(m_device, outputReport, sizeof(outputReport));
        m_outputDirty = (result < 0);
        return result >= 0;
    }
}
