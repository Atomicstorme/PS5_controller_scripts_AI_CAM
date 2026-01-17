#pragma once

#include "Common.h"

// Forward declarations for ViGEm types
struct _VIGEM_CLIENT_T;
struct _VIGEM_TARGET_T;
typedef struct _VIGEM_CLIENT_T* PVIGEM_CLIENT;
typedef struct _VIGEM_TARGET_T* PVIGEM_TARGET;

class VirtualController {
public:
    VirtualController();
    ~VirtualController();

    // Connection
    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }

    // Update virtual controller with state
    bool update(const NormalizedState& state);

    // Direct update from raw state
    bool updateRaw(const ControllerState& state);

    // Get last error message
    const std::string& getLastError() const { return m_lastError; }

private:
    PVIGEM_CLIENT m_client = nullptr;
    PVIGEM_TARGET m_target = nullptr;
    bool m_connected = false;
    std::string m_lastError;
};
