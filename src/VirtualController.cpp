#include "VirtualController.h"

// Windows headers (must come before ViGEm)
#include <Windows.h>

// ViGEmClient includes
#include <ViGEm/Client.h>
#include <ViGEm/Common.h>

VirtualController::VirtualController() {
}

VirtualController::~VirtualController() {
    disconnect();
}

bool VirtualController::connect() {
    if (m_connected) {
        disconnect();
    }

    // Allocate ViGEm client
    m_client = vigem_alloc();
    if (!m_client) {
        m_lastError = "Failed to allocate ViGEm client";
        return false;
    }

    // Connect to ViGEmBus driver
    VIGEM_ERROR error = vigem_connect(m_client);
    if (!VIGEM_SUCCESS(error)) {
        m_lastError = "Failed to connect to ViGEmBus. Make sure ViGEmBus driver is installed. Error: " +
                      std::to_string(static_cast<int>(error));
        vigem_free(m_client);
        m_client = nullptr;
        return false;
    }

    // Allocate Xbox 360 controller target
    m_target = vigem_target_x360_alloc();
    if (!m_target) {
        m_lastError = "Failed to allocate virtual Xbox 360 controller";
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
        return false;
    }

    // Add target to bus
    error = vigem_target_add(m_client, m_target);
    if (!VIGEM_SUCCESS(error)) {
        m_lastError = "Failed to add virtual controller to bus. Error: " +
                      std::to_string(static_cast<int>(error));
        vigem_target_free(m_target);
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_target = nullptr;
        m_client = nullptr;
        return false;
    }

    m_connected = true;
    m_lastError.clear();
    return true;
}

void VirtualController::disconnect() {
    if (m_target) {
        vigem_target_remove(m_client, m_target);
        vigem_target_free(m_target);
        m_target = nullptr;
    }

    if (m_client) {
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
    }

    m_connected = false;
}

bool VirtualController::update(const NormalizedState& state) {
    if (!m_connected || !m_client || !m_target) {
        return false;
    }

    XUSB_REPORT report = {};

    // Convert normalized sticks (-1.0 to 1.0) to Xbox format (-32768 to 32767)
    report.sThumbLX = static_cast<SHORT>(std::clamp(state.leftStickX * 32767.0f, -32768.0f, 32767.0f));
    report.sThumbLY = static_cast<SHORT>(std::clamp(-state.leftStickY * 32767.0f, -32768.0f, 32767.0f));  // Inverted Y
    report.sThumbRX = static_cast<SHORT>(std::clamp(state.rightStickX * 32767.0f, -32768.0f, 32767.0f));
    report.sThumbRY = static_cast<SHORT>(std::clamp(-state.rightStickY * 32767.0f, -32768.0f, 32767.0f));  // Inverted Y

    // Convert triggers (0.0 to 1.0) to Xbox format (0 to 255)
    report.bLeftTrigger = static_cast<BYTE>(std::clamp(state.leftTrigger * 255.0f, 0.0f, 255.0f));
    report.bRightTrigger = static_cast<BYTE>(std::clamp(state.rightTrigger * 255.0f, 0.0f, 255.0f));

    // Map buttons
    // PS5 -> Xbox mapping:
    // Cross -> A, Circle -> B, Square -> X, Triangle -> Y
    // L1 -> LB, R1 -> RB, L3 -> LS, R3 -> RS
    // Share -> Back, Options -> Start, PS -> Guide
    report.wButtons = 0;

    if (state.cross)     report.wButtons |= XUSB_GAMEPAD_A;
    if (state.circle)    report.wButtons |= XUSB_GAMEPAD_B;
    if (state.square)    report.wButtons |= XUSB_GAMEPAD_X;
    if (state.triangle)  report.wButtons |= XUSB_GAMEPAD_Y;
    if (state.l1)        report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
    if (state.r1)        report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
    if (state.l3)        report.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
    if (state.r3)        report.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;
    if (state.share)     report.wButtons |= XUSB_GAMEPAD_BACK;
    if (state.options)   report.wButtons |= XUSB_GAMEPAD_START;
    if (state.ps)        report.wButtons |= XUSB_GAMEPAD_GUIDE;

    // D-Pad mapping (PS5 uses 0-7 for directions, 8 = released)
    switch (state.dpad) {
        case 0: report.wButtons |= XUSB_GAMEPAD_DPAD_UP; break;
        case 1: report.wButtons |= XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_DPAD_RIGHT; break;
        case 2: report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT; break;
        case 3: report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN | XUSB_GAMEPAD_DPAD_RIGHT; break;
        case 4: report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN; break;
        case 5: report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN | XUSB_GAMEPAD_DPAD_LEFT; break;
        case 6: report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT; break;
        case 7: report.wButtons |= XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_DPAD_LEFT; break;
        default: break;  // 8 = released
    }

    // Send report
    VIGEM_ERROR error = vigem_target_x360_update(m_client, m_target, report);
    return VIGEM_SUCCESS(error);
}

bool VirtualController::updateRaw(const ControllerState& state) {
    // Convert raw state to normalized and update
    NormalizedState norm;

    norm.leftStickX = normalizeStick(state.leftStickX);
    norm.leftStickY = normalizeStick(state.leftStickY);
    norm.rightStickX = normalizeStick(state.rightStickX);
    norm.rightStickY = normalizeStick(state.rightStickY);
    norm.leftTrigger = normalizeTrigger(state.leftTrigger);
    norm.rightTrigger = normalizeTrigger(state.rightTrigger);

    norm.square = state.square;
    norm.cross = state.cross;
    norm.circle = state.circle;
    norm.triangle = state.triangle;
    norm.l1 = state.l1;
    norm.r1 = state.r1;
    norm.l2Button = state.l2Button;
    norm.r2Button = state.r2Button;
    norm.share = state.share;
    norm.options = state.options;
    norm.l3 = state.l3;
    norm.r3 = state.r3;
    norm.ps = state.ps;
    norm.touchpad = state.touchpad;
    norm.mute = state.mute;
    norm.dpad = state.dpad;

    return update(norm);
}
