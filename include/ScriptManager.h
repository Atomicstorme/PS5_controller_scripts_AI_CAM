#pragma once

#include "Common.h"
#include "ScriptEngine.h"
#include <filesystem>

struct LoadedScript {
    std::unique_ptr<ScriptEngine> engine;
    ScriptConfig config;
    bool loaded = false;
};

class ScriptManager {
public:
    ScriptManager();
    ~ScriptManager();

    // Initialize and scan for scripts
    bool initialize(const std::string& scriptsFolder = "scripts");

    // Reload all scripts from folder
    void rescanScripts();

    // Load a specific script
    bool loadScript(const std::string& filename);

    // Enable/disable a script
    void setScriptEnabled(const std::string& name, bool enabled);
    bool isScriptEnabled(const std::string& name) const;

    // Process input through all enabled scripts (in order)
    NormalizedState process(const NormalizedState& input, float deltaTime);

    // Get list of available scripts
    const std::vector<LoadedScript>& getScripts() const { return m_scripts; }
    std::vector<LoadedScript>& getScripts() { return m_scripts; }

    // Set script parameter
    void setScriptParameter(const std::string& scriptName, const std::string& param, float value);

    // Reorder scripts
    void moveScriptUp(size_t index);
    void moveScriptDown(size_t index);

    // Get scripts folder
    const std::string& getScriptsFolder() const { return m_scriptsFolder; }

private:
    std::vector<LoadedScript> m_scripts;
    std::string m_scriptsFolder;
};
