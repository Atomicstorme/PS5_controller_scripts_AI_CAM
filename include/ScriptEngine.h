#pragma once

#include "Common.h"
#include <lua.hpp>

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    // Initialize the Lua state
    bool initialize();

    // Load a script from file
    bool loadScript(const std::string& filename);

    // Load script from string
    bool loadScriptString(const std::string& script, const std::string& name = "inline");

    // Process input through the script
    // Returns modified state
    NormalizedState process(const NormalizedState& input, float deltaTime);

    // Call script's init function
    bool callInit();

    // Call script's cleanup function
    void callCleanup();

    // Set a parameter accessible from script
    void setParameter(const std::string& name, float value);
    float getParameter(const std::string& name, float defaultValue = 0.0f);

    // Get script info and parameters (call after loading)
    ScriptConfig getScriptInfo() const;

    // Check if script has required functions
    bool hasProcessFunction() const { return m_hasProcess; }

    // Get last error
    const std::string& getLastError() const { return m_lastError; }

    // Get script name
    const std::string& getScriptName() const { return m_scriptName; }

    // Sync parameters from config (after loading from saved settings)
    void syncParameters(const std::vector<ScriptParameter>& params);

    // Apply weapon preset overrides (for anti-recoil script)
    void applyWeaponPreset(const WeaponPreset* preset);

private:
    // Register C functions for Lua
    void registerFunctions();

    // Push NormalizedState as a Lua table
    void pushState(const NormalizedState& state);

    // Read NormalizedState from Lua table at stack index
    NormalizedState readState(int index);

    lua_State* m_lua = nullptr;
    std::string m_lastError;
    std::string m_scriptName;
    bool m_hasProcess = false;
    std::unordered_map<std::string, float> m_parameters;
};
