#include "ScriptManager.h"
#include "ConfigManager.h"
#include <algorithm>

namespace fs = std::filesystem;

ScriptManager::ScriptManager() {
}

ScriptManager::~ScriptManager() {
}

bool ScriptManager::initialize(const std::string& scriptsFolder, ConfigManager* config) {
    m_scriptsFolder = scriptsFolder;
    m_config = config;

    // Create scripts folder if it doesn't exist
    if (!fs::exists(m_scriptsFolder)) {
        fs::create_directories(m_scriptsFolder);
    }

    rescanScripts();
    return true;
}

void ScriptManager::rescanScripts() {
    // Keep track of currently enabled scripts
    std::unordered_map<std::string, bool> enabledState;
    for (const auto& script : m_scripts) {
        enabledState[script.config.name] = script.config.enabled;
    }

    m_scripts.clear();

    if (!fs::exists(m_scriptsFolder)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(m_scriptsFolder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filename = entry.path().filename().string();
            std::string filepath = entry.path().string();

            LoadedScript script;
            script.config.name = entry.path().stem().string();
            script.config.filename = filepath;

            // Restore enabled state if it was previously loaded
            auto it = enabledState.find(script.config.name);
            if (it != enabledState.end()) {
                script.config.enabled = it->second;
            }

            // Try to load the script
            script.engine = std::make_unique<ScriptEngine>();
            if (script.engine->initialize() && script.engine->loadScript(filepath)) {
                script.loaded = true;

                // Get script info (name, description, parameters) from the Lua script
                ScriptConfig info = script.engine->getScriptInfo();
                script.config.name = info.name.empty() ? script.config.name : info.name;
                script.config.description = info.description;
                script.config.author = info.author;
                script.config.version = info.version;
                script.config.parameters = info.parameters;

                // Restore saved settings from config
                if (m_config) {
                    m_config->updateScriptConfig(script.config);
                }

                // Initialize parameters in the engine (use restored values or defaults)
                for (const auto& param : script.config.parameters) {
                    script.engine->setParameter(param.key, param.value);
                }

                script.engine->callInit();
            } else {
                script.loaded = false;
            }

            m_scripts.push_back(std::move(script));
        }
    }
}

bool ScriptManager::loadScript(const std::string& filename) {
    std::string filepath = m_scriptsFolder + "/" + filename;
    if (!fs::exists(filepath)) {
        filepath = filename;  // Try as absolute path
    }

    if (!fs::exists(filepath)) {
        return false;
    }

    // Check if already loaded
    for (auto& script : m_scripts) {
        if (script.config.filename == filepath) {
            // Reload
            script.engine = std::make_unique<ScriptEngine>();
            if (script.engine->initialize() && script.engine->loadScript(filepath)) {
                script.loaded = true;
                script.engine->callInit();
                return true;
            }
            script.loaded = false;
            return false;
        }
    }

    // New script
    LoadedScript script;
    script.config.name = fs::path(filepath).stem().string();
    script.config.filename = filepath;
    script.engine = std::make_unique<ScriptEngine>();

    if (script.engine->initialize() && script.engine->loadScript(filepath)) {
        script.loaded = true;
        script.engine->callInit();
        m_scripts.push_back(std::move(script));
        return true;
    }

    return false;
}

void ScriptManager::setScriptEnabled(const std::string& name, bool enabled) {
    for (auto& script : m_scripts) {
        if (script.config.name == name) {
            script.config.enabled = enabled;
            // Save to config
            if (m_config) {
                m_config->setScriptEnabled(name, enabled);
            }
            return;
        }
    }
}

bool ScriptManager::isScriptEnabled(const std::string& name) const {
    for (const auto& script : m_scripts) {
        if (script.config.name == name) {
            return script.config.enabled;
        }
    }
    return false;
}

NormalizedState ScriptManager::process(const NormalizedState& input, float deltaTime) {
    NormalizedState current = input;

    // Get active weapon preset if available
    const WeaponPreset* activePreset = nullptr;
    if (m_config) {
        activePreset = m_config->getActiveWeaponPreset();
    }

    for (auto& script : m_scripts) {
        if (script.config.enabled && script.loaded && script.engine) {
            // Apply weapon preset to anti-recoil script if available
            if (activePreset && (script.config.name == "Anti-Recoil" ||
                                  script.config.name.find("anti") != std::string::npos ||
                                  script.config.name.find("recoil") != std::string::npos)) {
                script.engine->applyWeaponPreset(activePreset);
            }
            current = script.engine->process(current, deltaTime);
        }
    }

    return current;
}

void ScriptManager::setScriptParameter(const std::string& scriptName, const std::string& param, float value) {
    for (auto& script : m_scripts) {
        if (script.config.name == scriptName && script.engine) {
            script.engine->setParameter(param, value);
            // Save to config
            if (m_config) {
                m_config->setScriptParameter(scriptName, param, value);
            }
            return;
        }
    }
}

void ScriptManager::moveScriptUp(size_t index) {
    if (index > 0 && index < m_scripts.size()) {
        std::swap(m_scripts[index], m_scripts[index - 1]);
    }
}

void ScriptManager::moveScriptDown(size_t index) {
    if (index < m_scripts.size() - 1) {
        std::swap(m_scripts[index], m_scripts[index + 1]);
    }
}
