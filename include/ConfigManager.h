#pragma once

#include "Common.h"
#include <string>
#include <vector>

// Profile structure for per-game settings
struct GameProfile {
    std::string name;           // Profile name (e.g., "Call of Duty", "Default")
    std::string executableHint; // Optional: exe name to auto-detect (e.g., "cod.exe")
    std::vector<ScriptConfig> scripts;
    std::vector<WeaponPreset> weaponPresets;  // Per-gun recoil patterns
    std::string activeWeapon;   // Currently selected weapon preset name
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Set config file path
    void setConfigPath(const std::string& path) { m_configPath = path; }

    // Load configuration from file
    bool load();

    // Save configuration to file
    bool save();

    // Profile management
    std::vector<std::string> getProfileNames() const;
    std::string getCurrentProfileName() const { return m_currentProfile; }
    bool createProfile(const std::string& name);
    bool deleteProfile(const std::string& name);
    bool switchProfile(const std::string& name);
    bool renameProfile(const std::string& oldName, const std::string& newName);

    // Get/set app settings
    AppSettings& getSettings() { return m_settings; }
    const AppSettings& getSettings() const { return m_settings; }

    // Script configuration helpers
    void setScriptEnabled(const std::string& scriptName, bool enabled);
    bool getScriptEnabled(const std::string& scriptName) const;

    void setScriptParameter(const std::string& scriptName, const std::string& paramKey, float value);
    float getScriptParameter(const std::string& scriptName, const std::string& paramKey, float defaultValue) const;

    // Get saved parameters for a script (to restore after loading)
    std::vector<ScriptParameter> getScriptParameters(const std::string& scriptName) const;

    // Update script config (call after script loads to merge saved settings)
    void updateScriptConfig(ScriptConfig& config);

    // Mark config as dirty (needs saving)
    void markDirty() { m_dirty = true; }
    bool isDirty() const { return m_dirty; }

    // Auto-save if dirty
    void autoSave();

    // Weapon preset management
    std::vector<std::string> getWeaponPresetNames() const;
    std::string getActiveWeaponName() const;
    const WeaponPreset* getActiveWeaponPreset() const;
    bool createWeaponPreset(const std::string& name);
    bool deleteWeaponPreset(const std::string& name);
    bool setActiveWeapon(const std::string& name);
    bool updateWeaponPreset(const WeaponPreset& preset);
    const WeaponPreset* getWeaponPreset(const std::string& name) const;
    std::vector<WeaponPreset>& getWeaponPresets();
    const std::vector<WeaponPreset>& getWeaponPresets() const;

    // Hotkey management
    std::vector<std::pair<std::string, std::pair<int, int>>> getHotkeys() const { return m_hotkeys; }
    void setHotkeys(const std::vector<std::pair<std::string, std::pair<int, int>>>& hotkeys) {
        m_hotkeys = hotkeys;
        m_dirty = true;
    }

private:
    std::string m_configPath = "config.json";
    AppSettings m_settings;
    bool m_dirty = false;

    // Profiles
    std::vector<GameProfile> m_profiles;
    std::string m_currentProfile = "Default";

    // Hotkeys (scriptName -> {virtualKey, modifiers})
    std::vector<std::pair<std::string, std::pair<int, int>>> m_hotkeys;

    // Simple JSON helpers
    std::string serializeToJson() const;
    bool parseFromJson(const std::string& json);

    // Helper to find script config by name
    ScriptConfig* findScriptConfig(const std::string& name);
    const ScriptConfig* findScriptConfig(const std::string& name) const;

    // Helper to find profile by name
    GameProfile* findProfile(const std::string& name);
    const GameProfile* findProfile(const std::string& name) const;
};
