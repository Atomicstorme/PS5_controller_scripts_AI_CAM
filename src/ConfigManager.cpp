#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>

ConfigManager::ConfigManager() {
    // Create default profile
    GameProfile defaultProfile;
    defaultProfile.name = "Default";
    m_profiles.push_back(defaultProfile);
}

ConfigManager::~ConfigManager() {
    // Auto-save on destruction if dirty
    if (m_dirty) {
        save();
    }
}

bool ConfigManager::load() {
    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        // No config file yet, that's okay
        return true;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return parseFromJson(buffer.str());
}

bool ConfigManager::save() {
    std::string json = serializeToJson();

    std::ofstream file(m_configPath);
    if (!file.is_open()) {
        return false;
    }

    file << json;
    file.close();
    m_dirty = false;
    return true;
}

void ConfigManager::setScriptEnabled(const std::string& scriptName, bool enabled) {
    ScriptConfig* config = findScriptConfig(scriptName);
    if (config) {
        config->enabled = enabled;
    } else {
        ScriptConfig newConfig;
        newConfig.name = scriptName;
        newConfig.enabled = enabled;
        m_settings.scripts.push_back(newConfig);
    }
    m_dirty = true;
}

bool ConfigManager::getScriptEnabled(const std::string& scriptName) const {
    const ScriptConfig* config = findScriptConfig(scriptName);
    return config ? config->enabled : false;
}

void ConfigManager::setScriptParameter(const std::string& scriptName, const std::string& paramKey, float value) {
    ScriptConfig* config = findScriptConfig(scriptName);
    if (!config) {
        ScriptConfig newConfig;
        newConfig.name = scriptName;
        m_settings.scripts.push_back(newConfig);
        config = &m_settings.scripts.back();
    }

    // Find or add parameter
    for (auto& param : config->parameters) {
        if (param.key == paramKey) {
            param.value = value;
            m_dirty = true;
            return;
        }
    }

    // Add new parameter
    ScriptParameter param;
    param.key = paramKey;
    param.value = value;
    config->parameters.push_back(param);
    m_dirty = true;
}

float ConfigManager::getScriptParameter(const std::string& scriptName, const std::string& paramKey, float defaultValue) const {
    const ScriptConfig* config = findScriptConfig(scriptName);
    if (!config) return defaultValue;

    for (const auto& param : config->parameters) {
        if (param.key == paramKey) {
            return param.value;
        }
    }
    return defaultValue;
}

std::vector<ScriptParameter> ConfigManager::getScriptParameters(const std::string& scriptName) const {
    const ScriptConfig* config = findScriptConfig(scriptName);
    if (config) {
        return config->parameters;
    }
    return {};
}

void ConfigManager::updateScriptConfig(ScriptConfig& config) {
    const ScriptConfig* saved = findScriptConfig(config.name);
    if (!saved) return;

    // Restore enabled state
    config.enabled = saved->enabled;

    // Restore parameter values
    for (auto& param : config.parameters) {
        for (const auto& savedParam : saved->parameters) {
            if (param.key == savedParam.key) {
                param.value = savedParam.value;
                break;
            }
        }
    }
}

void ConfigManager::autoSave() {
    if (m_dirty) {
        save();
    }
}

// Profile management
std::vector<std::string> ConfigManager::getProfileNames() const {
    std::vector<std::string> names;
    for (const auto& profile : m_profiles) {
        names.push_back(profile.name);
    }
    return names;
}

bool ConfigManager::createProfile(const std::string& name) {
    // Check if profile already exists
    if (findProfile(name)) {
        return false;
    }

    GameProfile newProfile;
    newProfile.name = name;
    // Copy current profile's settings as starting point
    const GameProfile* current = findProfile(m_currentProfile);
    if (current) {
        newProfile.scripts = current->scripts;
    }
    m_profiles.push_back(newProfile);
    m_dirty = true;
    return true;
}

bool ConfigManager::deleteProfile(const std::string& name) {
    // Can't delete the default profile
    if (name == "Default") {
        return false;
    }

    for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if (it->name == name) {
            m_profiles.erase(it);
            // If we deleted the current profile, switch to default
            if (m_currentProfile == name) {
                switchProfile("Default");
            }
            m_dirty = true;
            return true;
        }
    }
    return false;
}

bool ConfigManager::switchProfile(const std::string& name) {
    GameProfile* profile = findProfile(name);
    if (!profile) {
        return false;
    }

    // Save current profile's settings first
    GameProfile* currentProfile = findProfile(m_currentProfile);
    if (currentProfile) {
        currentProfile->scripts = m_settings.scripts;
    }

    // Switch to new profile
    m_currentProfile = name;
    m_settings.scripts = profile->scripts;
    m_dirty = true;
    return true;
}

bool ConfigManager::renameProfile(const std::string& oldName, const std::string& newName) {
    // Can't rename default profile
    if (oldName == "Default") {
        return false;
    }

    // Check if new name already exists
    if (findProfile(newName)) {
        return false;
    }

    GameProfile* profile = findProfile(oldName);
    if (!profile) {
        return false;
    }

    profile->name = newName;
    if (m_currentProfile == oldName) {
        m_currentProfile = newName;
    }
    m_dirty = true;
    return true;
}

GameProfile* ConfigManager::findProfile(const std::string& name) {
    for (auto& profile : m_profiles) {
        if (profile.name == name) {
            return &profile;
        }
    }
    return nullptr;
}

const GameProfile* ConfigManager::findProfile(const std::string& name) const {
    for (const auto& profile : m_profiles) {
        if (profile.name == name) {
            return &profile;
        }
    }
    return nullptr;
}

ScriptConfig* ConfigManager::findScriptConfig(const std::string& name) {
    for (auto& config : m_settings.scripts) {
        if (config.name == name) {
            return &config;
        }
    }
    return nullptr;
}

const ScriptConfig* ConfigManager::findScriptConfig(const std::string& name) const {
    for (const auto& config : m_settings.scripts) {
        if (config.name == name) {
            return &config;
        }
    }
    return nullptr;
}

// Helper to serialize a scripts array
static void serializeScriptsArray(std::ostringstream& ss, const std::vector<ScriptConfig>& scripts, const std::string& indent) {
    ss << indent << "[\n";
    for (size_t i = 0; i < scripts.size(); i++) {
        const auto& script = scripts[i];
        ss << indent << "  {\n";
        ss << indent << "    \"name\": \"" << script.name << "\",\n";
        ss << indent << "    \"enabled\": " << (script.enabled ? "true" : "false") << ",\n";
        ss << indent << "    \"parameters\": [\n";

        for (size_t j = 0; j < script.parameters.size(); j++) {
            const auto& param = script.parameters[j];
            ss << indent << "      {\n";
            ss << indent << "        \"key\": \"" << param.key << "\",\n";
            ss << indent << "        \"value\": " << param.value << "\n";
            ss << indent << "      }";
            if (j < script.parameters.size() - 1) ss << ",";
            ss << "\n";
        }

        ss << indent << "    ]\n";
        ss << indent << "  }";
        if (i < scripts.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << indent << "]";
}

// Helper to serialize weapon presets array
static void serializeWeaponPresetsArray(std::ostringstream& ss, const std::vector<WeaponPreset>& presets, const std::string& indent) {
    ss << indent << "[\n";
    for (size_t i = 0; i < presets.size(); i++) {
        const auto& preset = presets[i];
        ss << indent << "  {\n";
        ss << indent << "    \"name\": \"" << preset.name << "\",\n";
        ss << indent << "    \"adsStrength\": " << preset.adsStrength << ",\n";
        ss << indent << "    \"hipFireStrength\": " << preset.hipFireStrength << ",\n";
        ss << indent << "    \"horizontalStrength\": " << preset.horizontalStrength << ",\n";
        ss << indent << "    \"adsThreshold\": " << preset.adsThreshold << ",\n";
        ss << indent << "    \"fireThreshold\": " << preset.fireThreshold << ",\n";
        ss << indent << "    \"smoothing\": " << preset.smoothing << ",\n";
        ss << indent << "    \"hotkeyVk\": " << preset.hotkeyVk << ",\n";
        ss << indent << "    \"hotkeyModifiers\": " << preset.hotkeyModifiers << "\n";
        ss << indent << "  }";
        if (i < presets.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << indent << "]";
}

// Simple JSON serialization (no external library needed)
std::string ConfigManager::serializeToJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"pollRate\": " << m_settings.pollRate << ",\n";
    ss << "  \"showDemo\": " << (m_settings.showDemo ? "true" : "false") << ",\n";
    ss << "  \"minimizeToTray\": " << (m_settings.minimizeToTray ? "true" : "false") << ",\n";
    ss << "  \"overlayEnabled\": " << (m_settings.overlayEnabled ? "true" : "false") << ",\n";
    ss << "  \"overlayPosition\": " << static_cast<int>(m_settings.overlayPosition) << ",\n";
    ss << "  \"overlayOpacity\": " << m_settings.overlayOpacity << ",\n";
    ss << "  \"currentProfile\": \"" << m_currentProfile << "\",\n";

    // Serialize profiles
    ss << "  \"profiles\": [\n";
    for (size_t p = 0; p < m_profiles.size(); p++) {
        const auto& profile = m_profiles[p];
        ss << "    {\n";
        ss << "      \"name\": \"" << profile.name << "\",\n";
        ss << "      \"executableHint\": \"" << profile.executableHint << "\",\n";
        ss << "      \"activeWeapon\": \"" << profile.activeWeapon << "\",\n";
        ss << "      \"weaponPresets\": ";
        serializeWeaponPresetsArray(ss, profile.weaponPresets, "      ");
        ss << ",\n";
        ss << "      \"scripts\": ";
        serializeScriptsArray(ss, profile.scripts, "      ");
        ss << "\n    }";
        if (p < m_profiles.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    // Serialize hotkeys
    ss << "  \"hotkeys\": [\n";
    for (size_t h = 0; h < m_hotkeys.size(); h++) {
        const auto& hk = m_hotkeys[h];
        ss << "    {\n";
        ss << "      \"script\": \"" << hk.first << "\",\n";
        ss << "      \"key\": " << hk.second.first << ",\n";
        ss << "      \"modifiers\": " << hk.second.second << "\n";
        ss << "    }";
        if (h < m_hotkeys.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    // Also serialize current scripts for backwards compatibility
    ss << "  \"scripts\": ";
    serializeScriptsArray(ss, m_settings.scripts, "  ");
    ss << "\n}\n";

    return ss.str();
}

// Helper function to parse a scripts array from JSON
static std::vector<ScriptConfig> parseScriptsArray(const std::string& json, size_t arrayStart, size_t arrayEnd,
    const std::function<std::pair<size_t, size_t>(const std::string&, size_t)>& findValue,
    const std::function<std::string(size_t)>& extractString,
    const std::function<float(size_t)>& extractNumber,
    const std::function<bool(size_t)>& extractBool) {

    std::vector<ScriptConfig> scripts;
    size_t pos = arrayStart;

    while (pos < arrayEnd) {
        size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos || objStart >= arrayEnd) break;

        // Find matching closing brace (handle nested objects)
        int braceCount = 1;
        size_t searchPos = objStart + 1;
        while (braceCount > 0 && searchPos < json.size()) {
            if (json[searchPos] == '{') braceCount++;
            else if (json[searchPos] == '}') braceCount--;
            searchPos++;
        }
        size_t objEnd = searchPos - 1;

        if (objEnd == std::string::npos || objEnd >= arrayEnd + 100) break;

        ScriptConfig scriptConfig;

        // Parse name
        auto [nameKey, nameVal] = findValue("name", objStart);
        if (nameVal != std::string::npos && nameVal < objEnd) {
            scriptConfig.name = extractString(nameVal);
        }

        // Parse enabled
        auto [enKey, enVal] = findValue("enabled", objStart);
        if (enVal != std::string::npos && enVal < objEnd) {
            scriptConfig.enabled = extractBool(enVal);
        }

        // Parse parameters
        size_t paramsPos = json.find("\"parameters\"", objStart);
        if (paramsPos != std::string::npos && paramsPos < objEnd) {
            size_t paramsArrayStart = json.find('[', paramsPos);
            size_t paramsArrayEnd = json.find(']', paramsArrayStart);

            if (paramsArrayStart != std::string::npos && paramsArrayEnd != std::string::npos) {
                size_t paramPos = paramsArrayStart;

                while (paramPos < paramsArrayEnd) {
                    size_t paramObjStart = json.find('{', paramPos);
                    if (paramObjStart == std::string::npos || paramObjStart >= paramsArrayEnd) break;

                    size_t paramObjEnd = json.find('}', paramObjStart);
                    if (paramObjEnd == std::string::npos) break;

                    ScriptParameter param;

                    // Parse key
                    auto [keyKey, keyVal] = findValue("key", paramObjStart);
                    if (keyVal != std::string::npos && keyVal < paramObjEnd) {
                        param.key = extractString(keyVal);
                    }

                    // Parse value
                    auto [valKey, valVal] = findValue("value", paramObjStart);
                    if (valVal != std::string::npos && valVal < paramObjEnd) {
                        param.value = extractNumber(valVal);
                    }

                    if (!param.key.empty()) {
                        scriptConfig.parameters.push_back(param);
                    }

                    paramPos = paramObjEnd + 1;
                }
            }
        }

        if (!scriptConfig.name.empty()) {
            scripts.push_back(scriptConfig);
        }

        pos = objEnd + 1;
    }

    return scripts;
}

// Simple JSON parsing (basic implementation)
bool ConfigManager::parseFromJson(const std::string& json) {
    // Clear existing
    m_settings.scripts.clear();
    m_profiles.clear();

    // Very basic JSON parsing - looks for key patterns
    // This is not a full JSON parser but works for our simple config format

    auto findValue = [&json](const std::string& key, size_t startPos = 0) -> std::pair<size_t, size_t> {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey, startPos);
        if (keyPos == std::string::npos) return {std::string::npos, std::string::npos};

        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return {std::string::npos, std::string::npos};

        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\n' || json[valueStart] == '\r' || json[valueStart] == '\t')) {
            valueStart++;
        }

        return {keyPos, valueStart};
    };

    auto extractString = [&json](size_t start) -> std::string {
        if (start >= json.size() || json[start] != '"') return "";
        size_t end = json.find('"', start + 1);
        if (end == std::string::npos) return "";
        return json.substr(start + 1, end - start - 1);
    };

    auto extractNumber = [&json](size_t start) -> float {
        size_t end = start;
        while (end < json.size() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-' || json[end] == '+')) {
            end++;
        }
        if (end == start) return 0.0f;
        return std::stof(json.substr(start, end - start));
    };

    auto extractBool = [&json](size_t start) -> bool {
        if (json.substr(start, 4) == "true") return true;
        return false;
    };

    // Parse poll rate
    auto [prKey, prVal] = findValue("pollRate");
    if (prVal != std::string::npos) {
        m_settings.pollRate = extractNumber(prVal);
    }

    // Parse showDemo
    auto [sdKey, sdVal] = findValue("showDemo");
    if (sdVal != std::string::npos) {
        m_settings.showDemo = extractBool(sdVal);
    }

    // Parse minimizeToTray
    auto [mtKey, mtVal] = findValue("minimizeToTray");
    if (mtVal != std::string::npos) {
        m_settings.minimizeToTray = extractBool(mtVal);
    }

    // Parse overlay settings
    auto [oeKey, oeVal] = findValue("overlayEnabled");
    if (oeVal != std::string::npos) {
        m_settings.overlayEnabled = extractBool(oeVal);
    }

    auto [opKey, opVal] = findValue("overlayPosition");
    if (opVal != std::string::npos) {
        int pos = static_cast<int>(extractNumber(opVal));
        if (pos >= 0 && pos <= 3) {
            m_settings.overlayPosition = static_cast<OverlayPosition>(pos);
        }
    }

    auto [ooKey, ooVal] = findValue("overlayOpacity");
    if (ooVal != std::string::npos) {
        m_settings.overlayOpacity = extractNumber(ooVal);
    }

    // Parse currentProfile
    auto [cpKey, cpVal] = findValue("currentProfile");
    if (cpVal != std::string::npos) {
        m_currentProfile = extractString(cpVal);
    }

    // Parse profiles array
    size_t profilesPos = json.find("\"profiles\"");
    if (profilesPos != std::string::npos) {
        size_t profilesArrayStart = json.find('[', profilesPos);
        if (profilesArrayStart != std::string::npos) {
            // Find the matching closing bracket for profiles array
            int bracketCount = 1;
            size_t searchPos = profilesArrayStart + 1;
            while (bracketCount > 0 && searchPos < json.size()) {
                if (json[searchPos] == '[') bracketCount++;
                else if (json[searchPos] == ']') bracketCount--;
                searchPos++;
            }
            size_t profilesArrayEnd = searchPos - 1;

            size_t pos = profilesArrayStart;
            while (pos < profilesArrayEnd) {
                size_t profileObjStart = json.find('{', pos);
                if (profileObjStart == std::string::npos || profileObjStart >= profilesArrayEnd) break;

                // Find matching closing brace for profile object
                int braceCount = 1;
                size_t braceSearchPos = profileObjStart + 1;
                while (braceCount > 0 && braceSearchPos < json.size()) {
                    if (json[braceSearchPos] == '{') braceCount++;
                    else if (json[braceSearchPos] == '}') braceCount--;
                    braceSearchPos++;
                }
                size_t profileObjEnd = braceSearchPos - 1;

                GameProfile profile;

                // Parse profile name
                auto [pnKey, pnVal] = findValue("name", profileObjStart);
                if (pnVal != std::string::npos && pnVal < profileObjEnd) {
                    profile.name = extractString(pnVal);
                }

                // Parse executableHint
                auto [ehKey, ehVal] = findValue("executableHint", profileObjStart);
                if (ehVal != std::string::npos && ehVal < profileObjEnd) {
                    profile.executableHint = extractString(ehVal);
                }

                // Parse activeWeapon
                auto [awKey, awVal] = findValue("activeWeapon", profileObjStart);
                if (awVal != std::string::npos && awVal < profileObjEnd) {
                    profile.activeWeapon = extractString(awVal);
                }

                // Parse weapon presets
                size_t weaponPresetsPos = json.find("\"weaponPresets\"", profileObjStart);
                if (weaponPresetsPos != std::string::npos && weaponPresetsPos < profileObjEnd) {
                    size_t wpArrayStart = json.find('[', weaponPresetsPos);
                    if (wpArrayStart != std::string::npos && wpArrayStart < profileObjEnd) {
                        // Find matching closing bracket
                        int wpBracketCount = 1;
                        size_t wpSearchPos = wpArrayStart + 1;
                        while (wpBracketCount > 0 && wpSearchPos < json.size()) {
                            if (json[wpSearchPos] == '[') wpBracketCount++;
                            else if (json[wpSearchPos] == ']') wpBracketCount--;
                            wpSearchPos++;
                        }
                        size_t wpArrayEnd = wpSearchPos - 1;

                        // Parse each weapon preset
                        size_t wpPos = wpArrayStart;
                        while (wpPos < wpArrayEnd) {
                            size_t wpObjStart = json.find('{', wpPos);
                            if (wpObjStart == std::string::npos || wpObjStart >= wpArrayEnd) break;

                            size_t wpObjEnd = json.find('}', wpObjStart);
                            if (wpObjEnd == std::string::npos || wpObjEnd > wpArrayEnd) break;

                            WeaponPreset preset;

                            auto [wpnKey, wpnVal] = findValue("name", wpObjStart);
                            if (wpnVal != std::string::npos && wpnVal < wpObjEnd) {
                                preset.name = extractString(wpnVal);
                            }

                            auto [adsKey, adsVal] = findValue("adsStrength", wpObjStart);
                            if (adsVal != std::string::npos && adsVal < wpObjEnd) {
                                preset.adsStrength = extractNumber(adsVal);
                            }

                            auto [hfKey, hfVal] = findValue("hipFireStrength", wpObjStart);
                            if (hfVal != std::string::npos && hfVal < wpObjEnd) {
                                preset.hipFireStrength = extractNumber(hfVal);
                            }

                            auto [hsKey, hsVal] = findValue("horizontalStrength", wpObjStart);
                            if (hsVal != std::string::npos && hsVal < wpObjEnd) {
                                preset.horizontalStrength = extractNumber(hsVal);
                            }

                            auto [atKey, atVal] = findValue("adsThreshold", wpObjStart);
                            if (atVal != std::string::npos && atVal < wpObjEnd) {
                                preset.adsThreshold = extractNumber(atVal);
                            }

                            auto [ftKey, ftVal] = findValue("fireThreshold", wpObjStart);
                            if (ftVal != std::string::npos && ftVal < wpObjEnd) {
                                preset.fireThreshold = extractNumber(ftVal);
                            }

                            auto [smKey, smVal] = findValue("smoothing", wpObjStart);
                            if (smVal != std::string::npos && smVal < wpObjEnd) {
                                preset.smoothing = extractNumber(smVal);
                            }

                            auto [hkKey, hkVal] = findValue("hotkeyVk", wpObjStart);
                            if (hkVal != std::string::npos && hkVal < wpObjEnd) {
                                preset.hotkeyVk = static_cast<int>(extractNumber(hkVal));
                            }

                            auto [hmKey, hmVal] = findValue("hotkeyModifiers", wpObjStart);
                            if (hmVal != std::string::npos && hmVal < wpObjEnd) {
                                preset.hotkeyModifiers = static_cast<int>(extractNumber(hmVal));
                            }

                            if (!preset.name.empty()) {
                                profile.weaponPresets.push_back(preset);
                            }

                            wpPos = wpObjEnd + 1;
                        }
                    }
                }

                // Parse profile scripts
                size_t profileScriptsPos = json.find("\"scripts\"", profileObjStart);
                if (profileScriptsPos != std::string::npos && profileScriptsPos < profileObjEnd) {
                    size_t scriptsArrayStart = json.find('[', profileScriptsPos);
                    if (scriptsArrayStart != std::string::npos && scriptsArrayStart < profileObjEnd) {
                        // Find matching closing bracket
                        int sBracketCount = 1;
                        size_t sSearchPos = scriptsArrayStart + 1;
                        while (sBracketCount > 0 && sSearchPos < json.size()) {
                            if (json[sSearchPos] == '[') sBracketCount++;
                            else if (json[sSearchPos] == ']') sBracketCount--;
                            sSearchPos++;
                        }
                        size_t scriptsArrayEnd = sSearchPos - 1;

                        profile.scripts = parseScriptsArray(json, scriptsArrayStart, scriptsArrayEnd,
                            findValue, extractString, extractNumber, extractBool);
                    }
                }

                if (!profile.name.empty()) {
                    m_profiles.push_back(profile);
                }

                pos = profileObjEnd + 1;
            }
        }
    }

    // If no profiles were loaded, create default profile
    if (m_profiles.empty()) {
        GameProfile defaultProfile;
        defaultProfile.name = "Default";
        m_profiles.push_back(defaultProfile);
        m_currentProfile = "Default";
    }

    // Parse scripts array (for backwards compatibility and current active scripts)
    size_t scriptsPos = json.find("\"scripts\"");
    // Make sure we're finding the top-level scripts, not one inside profiles
    size_t profilesEnd = json.rfind("\"profiles\"");
    if (profilesEnd != std::string::npos) {
        // Find the scripts key that comes after profiles array ends
        size_t afterProfiles = json.find(']', profilesEnd);
        if (afterProfiles != std::string::npos) {
            scriptsPos = json.find("\"scripts\"", afterProfiles);
        }
    }

    if (scriptsPos != std::string::npos) {
        size_t arrayStart = json.find('[', scriptsPos);
        size_t arrayEnd = json.rfind(']');

        if (arrayStart != std::string::npos && arrayEnd != std::string::npos && arrayStart < arrayEnd) {
            m_settings.scripts = parseScriptsArray(json, arrayStart, arrayEnd,
                findValue, extractString, extractNumber, extractBool);
        }
    }

    // If we loaded profiles but no current scripts, use the current profile's scripts
    if (m_settings.scripts.empty() && !m_profiles.empty()) {
        GameProfile* currentProfile = findProfile(m_currentProfile);
        if (currentProfile) {
            m_settings.scripts = currentProfile->scripts;
        }
    }

    // Parse hotkeys
    m_hotkeys.clear();
    size_t hotkeysPos = json.find("\"hotkeys\"");
    if (hotkeysPos != std::string::npos) {
        size_t hkArrayStart = json.find('[', hotkeysPos);
        if (hkArrayStart != std::string::npos) {
            // Find matching closing bracket
            int hkBracketCount = 1;
            size_t hkSearchPos = hkArrayStart + 1;
            while (hkBracketCount > 0 && hkSearchPos < json.size()) {
                if (json[hkSearchPos] == '[') hkBracketCount++;
                else if (json[hkSearchPos] == ']') hkBracketCount--;
                hkSearchPos++;
            }
            size_t hkArrayEnd = hkSearchPos - 1;

            size_t hkPos = hkArrayStart;
            while (hkPos < hkArrayEnd) {
                size_t hkObjStart = json.find('{', hkPos);
                if (hkObjStart == std::string::npos || hkObjStart >= hkArrayEnd) break;

                size_t hkObjEnd = json.find('}', hkObjStart);
                if (hkObjEnd == std::string::npos || hkObjEnd > hkArrayEnd) break;

                std::string scriptName;
                int key = 0;
                int modifiers = 0;

                auto [sKey, sVal] = findValue("script", hkObjStart);
                if (sVal != std::string::npos && sVal < hkObjEnd) {
                    scriptName = extractString(sVal);
                }

                auto [kKey, kVal] = findValue("key", hkObjStart);
                if (kVal != std::string::npos && kVal < hkObjEnd) {
                    key = static_cast<int>(extractNumber(kVal));
                }

                auto [mKey, mVal] = findValue("modifiers", hkObjStart);
                if (mVal != std::string::npos && mVal < hkObjEnd) {
                    modifiers = static_cast<int>(extractNumber(mVal));
                }

                if (!scriptName.empty() && key != 0) {
                    m_hotkeys.push_back({scriptName, {key, modifiers}});
                }

                hkPos = hkObjEnd + 1;
            }
        }
    }

    return true;
}

// Weapon preset management
std::vector<std::string> ConfigManager::getWeaponPresetNames() const {
    std::vector<std::string> names;
    const GameProfile* profile = findProfile(m_currentProfile);
    if (profile) {
        for (const auto& preset : profile->weaponPresets) {
            names.push_back(preset.name);
        }
    }
    return names;
}

std::string ConfigManager::getActiveWeaponName() const {
    const GameProfile* profile = findProfile(m_currentProfile);
    if (profile) {
        return profile->activeWeapon;
    }
    return "";
}

const WeaponPreset* ConfigManager::getActiveWeaponPreset() const {
    const GameProfile* profile = findProfile(m_currentProfile);
    if (profile && !profile->activeWeapon.empty()) {
        for (const auto& preset : profile->weaponPresets) {
            if (preset.name == profile->activeWeapon) {
                return &preset;
            }
        }
    }
    return nullptr;
}

bool ConfigManager::createWeaponPreset(const std::string& name) {
    GameProfile* profile = findProfile(m_currentProfile);
    if (!profile) return false;

    // Check if preset already exists
    for (const auto& preset : profile->weaponPresets) {
        if (preset.name == name) {
            return false;
        }
    }

    WeaponPreset newPreset;
    newPreset.name = name;
    profile->weaponPresets.push_back(newPreset);

    // If this is the first preset, make it active
    if (profile->weaponPresets.size() == 1) {
        profile->activeWeapon = name;
    }

    m_dirty = true;
    return true;
}

bool ConfigManager::deleteWeaponPreset(const std::string& name) {
    GameProfile* profile = findProfile(m_currentProfile);
    if (!profile) return false;

    for (auto it = profile->weaponPresets.begin(); it != profile->weaponPresets.end(); ++it) {
        if (it->name == name) {
            profile->weaponPresets.erase(it);

            // If we deleted the active weapon, switch to first available or clear
            if (profile->activeWeapon == name) {
                if (!profile->weaponPresets.empty()) {
                    profile->activeWeapon = profile->weaponPresets[0].name;
                } else {
                    profile->activeWeapon = "";
                }
            }

            m_dirty = true;
            return true;
        }
    }
    return false;
}

bool ConfigManager::setActiveWeapon(const std::string& name) {
    GameProfile* profile = findProfile(m_currentProfile);
    if (!profile) return false;

    // Verify the preset exists
    for (const auto& preset : profile->weaponPresets) {
        if (preset.name == name) {
            profile->activeWeapon = name;
            m_dirty = true;
            return true;
        }
    }
    return false;
}

bool ConfigManager::updateWeaponPreset(const WeaponPreset& preset) {
    GameProfile* profile = findProfile(m_currentProfile);
    if (!profile) return false;

    for (auto& existing : profile->weaponPresets) {
        if (existing.name == preset.name) {
            existing = preset;
            m_dirty = true;
            return true;
        }
    }
    return false;
}

const WeaponPreset* ConfigManager::getWeaponPreset(const std::string& name) const {
    const GameProfile* profile = findProfile(m_currentProfile);
    if (profile) {
        for (const auto& preset : profile->weaponPresets) {
            if (preset.name == name) {
                return &preset;
            }
        }
    }
    return nullptr;
}

std::vector<WeaponPreset>& ConfigManager::getWeaponPresets() {
    GameProfile* profile = findProfile(m_currentProfile);
    if (profile) {
        return profile->weaponPresets;
    }
    // Return empty static vector if no profile found (shouldn't happen)
    static std::vector<WeaponPreset> empty;
    return empty;
}

const std::vector<WeaponPreset>& ConfigManager::getWeaponPresets() const {
    const GameProfile* profile = findProfile(m_currentProfile);
    if (profile) {
        return profile->weaponPresets;
    }
    // Return empty static vector if no profile found (shouldn't happen)
    static const std::vector<WeaponPreset> empty;
    return empty;
}
