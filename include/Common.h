#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>

// PS5 DualSense vendor/product IDs
constexpr uint16_t DUALSENSE_VENDOR_ID = 0x054C;
constexpr uint16_t DUALSENSE_PRODUCT_ID = 0x0CE6;
constexpr uint16_t DUALSENSE_EDGE_PRODUCT_ID = 0x0DF2;

// Controller state structure
struct ControllerState {
    // Sticks (0-255, 128 = center)
    uint8_t leftStickX = 128;
    uint8_t leftStickY = 128;
    uint8_t rightStickX = 128;
    uint8_t rightStickY = 128;

    // Triggers (0-255)
    uint8_t leftTrigger = 0;
    uint8_t rightTrigger = 0;

    // D-Pad (0-7 for directions, 8 = released)
    uint8_t dpad = 8;

    // Buttons
    bool square = false;
    bool cross = false;
    bool circle = false;
    bool triangle = false;
    bool l1 = false;
    bool r1 = false;
    bool l2Button = false;  // L2 as button (fully pressed)
    bool r2Button = false;  // R2 as button
    bool share = false;     // Create button on PS5
    bool options = false;
    bool l3 = false;        // Left stick press
    bool r3 = false;        // Right stick press
    bool ps = false;        // PS button
    bool touchpad = false;  // Touchpad press
    bool mute = false;      // Mute button

    // Touchpad coordinates (if needed)
    int16_t touchX = 0;
    int16_t touchY = 0;
    bool touchActive = false;

    // Gyro/Accelerometer (raw values)
    int16_t gyroX = 0;
    int16_t gyroY = 0;
    int16_t gyroZ = 0;
    int16_t accelX = 0;
    int16_t accelY = 0;
    int16_t accelZ = 0;

    // Timestamp
    uint64_t timestamp = 0;
};

// Normalized controller state for scripts (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
struct NormalizedState {
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;

    // Buttons same as ControllerState
    bool square = false;
    bool cross = false;
    bool circle = false;
    bool triangle = false;
    bool l1 = false;
    bool r1 = false;
    bool l2Button = false;
    bool r2Button = false;
    bool share = false;
    bool options = false;
    bool l3 = false;
    bool r3 = false;
    bool ps = false;
    bool touchpad = false;
    bool mute = false;
    uint8_t dpad = 8;

    // Gyro normalized
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;

    // Delta time since last update (seconds)
    float deltaTime = 0.0f;
};

// Script parameter types
enum class ParamType {
    Float,      // Slider
    Int,        // Integer slider
    Bool,       // Checkbox
    Choice      // Dropdown
};

// Script parameter definition (for UI)
struct ScriptParameter {
    std::string key;            // Internal name used in script
    std::string displayName;    // User-friendly name
    std::string description;    // Tooltip text
    ParamType type = ParamType::Float;

    float value = 0.0f;         // Current value
    float defaultValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.01f;         // Step size for sliders

    std::vector<std::string> choices;  // For Choice type
    int choiceIndex = 0;               // Selected choice index
};

// Script configuration
struct ScriptConfig {
    std::string name;
    std::string filename;
    std::string description;    // Script description for UI
    std::string author;         // Script author
    std::string version;        // Script version
    bool enabled = false;
    std::vector<ScriptParameter> parameters;
};

// Application settings
struct AppSettings {
    float pollRate = 1000.0f;  // Hz
    bool showDemo = false;
    bool minimizeToTray = true;
    std::vector<ScriptConfig> scripts;
};

// Utility functions
inline float normalizeStick(uint8_t value) {
    return (static_cast<float>(value) - 128.0f) / 127.0f;
}

inline uint8_t denormalizeStick(float value) {
    return static_cast<uint8_t>(std::clamp((value * 127.0f) + 128.0f, 0.0f, 255.0f));
}

inline float normalizeTrigger(uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}

inline uint8_t denormalizeTrigger(float value) {
    return static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f));
}
