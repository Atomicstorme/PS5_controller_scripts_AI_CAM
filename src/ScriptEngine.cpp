#include "ScriptEngine.h"
#include <fstream>
#include <sstream>
#include <cmath>

// Global pointer for Lua callbacks (simple approach for single-threaded use)
static ScriptEngine* g_currentEngine = nullptr;

// Lua C functions
static int lua_getParameter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float defaultVal = luaL_optnumber(L, 2, 0.0);
    if (g_currentEngine) {
        lua_pushnumber(L, g_currentEngine->getParameter(name, defaultVal));
    } else {
        lua_pushnumber(L, defaultVal);
    }
    return 1;
}

static int lua_setParameter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float value = luaL_checknumber(L, 2);
    if (g_currentEngine) {
        g_currentEngine->setParameter(name, value);
    }
    return 0;
}

static int lua_clamp(lua_State* L) {
    float value = luaL_checknumber(L, 1);
    float min = luaL_checknumber(L, 2);
    float max = luaL_checknumber(L, 3);
    lua_pushnumber(L, std::clamp(value, min, max));
    return 1;
}

static int lua_lerp(lua_State* L) {
    float a = luaL_checknumber(L, 1);
    float b = luaL_checknumber(L, 2);
    float t = luaL_checknumber(L, 3);
    lua_pushnumber(L, a + (b - a) * t);
    return 1;
}

static int lua_deadzone(lua_State* L) {
    float value = luaL_checknumber(L, 1);
    float deadzone = luaL_checknumber(L, 2);
    if (std::abs(value) < deadzone) {
        lua_pushnumber(L, 0.0);
    } else {
        // Scale the remaining range to 0-1
        float sign = value > 0 ? 1.0f : -1.0f;
        float scaled = (std::abs(value) - deadzone) / (1.0f - deadzone);
        lua_pushnumber(L, sign * scaled);
    }
    return 1;
}

static int lua_print(lua_State* L) {
    int nargs = lua_gettop(L);
    std::string output;
    for (int i = 1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            output += lua_tostring(L, i);
        } else if (lua_isnumber(L, i)) {
            output += std::to_string(lua_tonumber(L, i));
        } else if (lua_isboolean(L, i)) {
            output += lua_toboolean(L, i) ? "true" : "false";
        } else {
            output += lua_typename(L, lua_type(L, i));
        }
        if (i < nargs) output += "\t";
    }
    // In a real app, you'd route this to a log window
    printf("[Script] %s\n", output.c_str());
    return 0;
}

ScriptEngine::ScriptEngine() {
}

ScriptEngine::~ScriptEngine() {
    if (m_lua) {
        callCleanup();
        lua_close(m_lua);
        m_lua = nullptr;
    }
    if (g_currentEngine == this) {
        g_currentEngine = nullptr;
    }
}

bool ScriptEngine::initialize() {
    if (m_lua) {
        lua_close(m_lua);
    }

    m_lua = luaL_newstate();
    if (!m_lua) {
        m_lastError = "Failed to create Lua state";
        return false;
    }

    // Open standard libraries (safe subset)
    luaL_requiref(m_lua, "_G", luaopen_base, 1);
    luaL_requiref(m_lua, "math", luaopen_math, 1);
    luaL_requiref(m_lua, "string", luaopen_string, 1);
    luaL_requiref(m_lua, "table", luaopen_table, 1);
    lua_pop(m_lua, 4);

    registerFunctions();

    g_currentEngine = this;
    m_lastError.clear();
    return true;
}

void ScriptEngine::registerFunctions() {
    // Register helper functions
    lua_register(m_lua, "get_param", lua_getParameter);
    lua_register(m_lua, "set_param", lua_setParameter);
    lua_register(m_lua, "clamp", lua_clamp);
    lua_register(m_lua, "lerp", lua_lerp);
    lua_register(m_lua, "deadzone", lua_deadzone);
    lua_register(m_lua, "print", lua_print);
}

bool ScriptEngine::loadScript(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        m_lastError = "Failed to open script file: " + filename;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Extract script name from filename
    size_t lastSlash = filename.find_last_of("/\\");
    m_scriptName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;

    return loadScriptString(buffer.str(), m_scriptName);
}

bool ScriptEngine::loadScriptString(const std::string& script, const std::string& name) {
    if (!m_lua) {
        if (!initialize()) {
            return false;
        }
    }

    m_scriptName = name;
    g_currentEngine = this;

    // Load and execute the script
    int result = luaL_loadbuffer(m_lua, script.c_str(), script.size(), name.c_str());
    if (result != LUA_OK) {
        m_lastError = "Script load error: " + std::string(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    result = lua_pcall(m_lua, 0, 0, 0);
    if (result != LUA_OK) {
        m_lastError = "Script execution error: " + std::string(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    // Check if process function exists
    lua_getglobal(m_lua, "process");
    m_hasProcess = lua_isfunction(m_lua, -1);
    lua_pop(m_lua, 1);

    if (!m_hasProcess) {
        m_lastError = "Script missing 'process' function";
        return false;
    }

    m_lastError.clear();
    return true;
}

bool ScriptEngine::callInit() {
    if (!m_lua) return false;

    g_currentEngine = this;

    lua_getglobal(m_lua, "init");
    if (!lua_isfunction(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return true;  // init is optional
    }

    int result = lua_pcall(m_lua, 0, 0, 0);
    if (result != LUA_OK) {
        m_lastError = "Script init error: " + std::string(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    return true;
}

void ScriptEngine::callCleanup() {
    if (!m_lua) return;

    g_currentEngine = this;

    lua_getglobal(m_lua, "cleanup");
    if (lua_isfunction(m_lua, -1)) {
        lua_pcall(m_lua, 0, 0, 0);
    } else {
        lua_pop(m_lua, 1);
    }
}

void ScriptEngine::pushState(const NormalizedState& state) {
    lua_newtable(m_lua);

    // Sticks
    lua_pushnumber(m_lua, state.leftStickX);
    lua_setfield(m_lua, -2, "left_x");
    lua_pushnumber(m_lua, state.leftStickY);
    lua_setfield(m_lua, -2, "left_y");
    lua_pushnumber(m_lua, state.rightStickX);
    lua_setfield(m_lua, -2, "right_x");
    lua_pushnumber(m_lua, state.rightStickY);
    lua_setfield(m_lua, -2, "right_y");

    // Triggers
    lua_pushnumber(m_lua, state.leftTrigger);
    lua_setfield(m_lua, -2, "left_trigger");
    lua_pushnumber(m_lua, state.rightTrigger);
    lua_setfield(m_lua, -2, "right_trigger");

    // Buttons
    lua_pushboolean(m_lua, state.square);
    lua_setfield(m_lua, -2, "square");
    lua_pushboolean(m_lua, state.cross);
    lua_setfield(m_lua, -2, "cross");
    lua_pushboolean(m_lua, state.circle);
    lua_setfield(m_lua, -2, "circle");
    lua_pushboolean(m_lua, state.triangle);
    lua_setfield(m_lua, -2, "triangle");
    lua_pushboolean(m_lua, state.l1);
    lua_setfield(m_lua, -2, "l1");
    lua_pushboolean(m_lua, state.r1);
    lua_setfield(m_lua, -2, "r1");
    lua_pushboolean(m_lua, state.l2Button);
    lua_setfield(m_lua, -2, "l2_button");
    lua_pushboolean(m_lua, state.r2Button);
    lua_setfield(m_lua, -2, "r2_button");
    lua_pushboolean(m_lua, state.share);
    lua_setfield(m_lua, -2, "share");
    lua_pushboolean(m_lua, state.options);
    lua_setfield(m_lua, -2, "options");
    lua_pushboolean(m_lua, state.l3);
    lua_setfield(m_lua, -2, "l3");
    lua_pushboolean(m_lua, state.r3);
    lua_setfield(m_lua, -2, "r3");
    lua_pushboolean(m_lua, state.ps);
    lua_setfield(m_lua, -2, "ps");
    lua_pushboolean(m_lua, state.touchpad);
    lua_setfield(m_lua, -2, "touchpad");
    lua_pushboolean(m_lua, state.mute);
    lua_setfield(m_lua, -2, "mute");

    // D-Pad
    lua_pushinteger(m_lua, state.dpad);
    lua_setfield(m_lua, -2, "dpad");

    // Gyro
    lua_pushnumber(m_lua, state.gyroX);
    lua_setfield(m_lua, -2, "gyro_x");
    lua_pushnumber(m_lua, state.gyroY);
    lua_setfield(m_lua, -2, "gyro_y");
    lua_pushnumber(m_lua, state.gyroZ);
    lua_setfield(m_lua, -2, "gyro_z");

    // Delta time
    lua_pushnumber(m_lua, state.deltaTime);
    lua_setfield(m_lua, -2, "dt");
}

NormalizedState ScriptEngine::readState(int index) {
    NormalizedState state;

    if (!lua_istable(m_lua, index)) {
        return state;
    }

    // Helper to read number fields
    auto getNumber = [this, index](const char* field, float defaultVal = 0.0f) -> float {
        lua_getfield(m_lua, index, field);
        float val = lua_isnumber(m_lua, -1) ? lua_tonumber(m_lua, -1) : defaultVal;
        lua_pop(m_lua, 1);
        return val;
    };

    // Helper to read boolean fields
    auto getBool = [this, index](const char* field, bool defaultVal = false) -> bool {
        lua_getfield(m_lua, index, field);
        bool val = lua_isboolean(m_lua, -1) ? lua_toboolean(m_lua, -1) : defaultVal;
        lua_pop(m_lua, 1);
        return val;
    };

    // Read sticks
    state.leftStickX = getNumber("left_x");
    state.leftStickY = getNumber("left_y");
    state.rightStickX = getNumber("right_x");
    state.rightStickY = getNumber("right_y");

    // Read triggers
    state.leftTrigger = getNumber("left_trigger");
    state.rightTrigger = getNumber("right_trigger");

    // Read buttons
    state.square = getBool("square");
    state.cross = getBool("cross");
    state.circle = getBool("circle");
    state.triangle = getBool("triangle");
    state.l1 = getBool("l1");
    state.r1 = getBool("r1");
    state.l2Button = getBool("l2_button");
    state.r2Button = getBool("r2_button");
    state.share = getBool("share");
    state.options = getBool("options");
    state.l3 = getBool("l3");
    state.r3 = getBool("r3");
    state.ps = getBool("ps");
    state.touchpad = getBool("touchpad");
    state.mute = getBool("mute");

    // D-Pad
    lua_getfield(m_lua, index, "dpad");
    state.dpad = lua_isinteger(m_lua, -1) ? static_cast<uint8_t>(lua_tointeger(m_lua, -1)) : 8;
    lua_pop(m_lua, 1);

    // Gyro
    state.gyroX = getNumber("gyro_x");
    state.gyroY = getNumber("gyro_y");
    state.gyroZ = getNumber("gyro_z");

    return state;
}

NormalizedState ScriptEngine::process(const NormalizedState& input, float deltaTime) {
    if (!m_lua || !m_hasProcess) {
        return input;
    }

    g_currentEngine = this;

    // Get the process function
    lua_getglobal(m_lua, "process");
    if (!lua_isfunction(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return input;
    }

    // Create input table with delta time
    NormalizedState inputWithDt = input;
    inputWithDt.deltaTime = deltaTime;
    pushState(inputWithDt);

    // Call process(input) -> output
    int result = lua_pcall(m_lua, 1, 1, 0);
    if (result != LUA_OK) {
        m_lastError = "Script process error: " + std::string(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return input;
    }

    // Read output state
    NormalizedState output = readState(-1);
    lua_pop(m_lua, 1);

    return output;
}

void ScriptEngine::setParameter(const std::string& name, float value) {
    m_parameters[name] = value;
}

float ScriptEngine::getParameter(const std::string& name, float defaultValue) {
    auto it = m_parameters.find(name);
    if (it != m_parameters.end()) {
        return it->second;
    }
    return defaultValue;
}

ScriptConfig ScriptEngine::getScriptInfo() const {
    ScriptConfig config;
    config.name = m_scriptName;

    if (!m_lua) return config;

    // Try to read script_info table
    lua_getglobal(m_lua, "script_info");
    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return config;
    }

    // Read basic info
    lua_getfield(m_lua, -1, "name");
    if (lua_isstring(m_lua, -1)) {
        config.name = lua_tostring(m_lua, -1);
    }
    lua_pop(m_lua, 1);

    lua_getfield(m_lua, -1, "description");
    if (lua_isstring(m_lua, -1)) {
        config.description = lua_tostring(m_lua, -1);
    }
    lua_pop(m_lua, 1);

    lua_getfield(m_lua, -1, "author");
    if (lua_isstring(m_lua, -1)) {
        config.author = lua_tostring(m_lua, -1);
    }
    lua_pop(m_lua, 1);

    lua_getfield(m_lua, -1, "version");
    if (lua_isstring(m_lua, -1)) {
        config.version = lua_tostring(m_lua, -1);
    }
    lua_pop(m_lua, 1);

    // Read parameters array
    lua_getfield(m_lua, -1, "parameters");
    if (lua_istable(m_lua, -1)) {
        int paramTableIndex = lua_gettop(m_lua);
        lua_pushnil(m_lua);

        while (lua_next(m_lua, paramTableIndex) != 0) {
            if (lua_istable(m_lua, -1)) {
                ScriptParameter param;

                lua_getfield(m_lua, -1, "key");
                if (lua_isstring(m_lua, -1)) param.key = lua_tostring(m_lua, -1);
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "name");
                if (lua_isstring(m_lua, -1)) param.displayName = lua_tostring(m_lua, -1);
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "description");
                if (lua_isstring(m_lua, -1)) param.description = lua_tostring(m_lua, -1);
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "type");
                if (lua_isstring(m_lua, -1)) {
                    std::string typeStr = lua_tostring(m_lua, -1);
                    if (typeStr == "float") param.type = ParamType::Float;
                    else if (typeStr == "int") param.type = ParamType::Int;
                    else if (typeStr == "bool") param.type = ParamType::Bool;
                    else if (typeStr == "choice") param.type = ParamType::Choice;
                }
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "default");
                if (lua_isnumber(m_lua, -1)) {
                    param.defaultValue = static_cast<float>(lua_tonumber(m_lua, -1));
                    param.value = param.defaultValue;
                } else if (lua_isboolean(m_lua, -1)) {
                    param.defaultValue = lua_toboolean(m_lua, -1) ? 1.0f : 0.0f;
                    param.value = param.defaultValue;
                }
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "min");
                if (lua_isnumber(m_lua, -1)) param.minValue = static_cast<float>(lua_tonumber(m_lua, -1));
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "max");
                if (lua_isnumber(m_lua, -1)) param.maxValue = static_cast<float>(lua_tonumber(m_lua, -1));
                lua_pop(m_lua, 1);

                lua_getfield(m_lua, -1, "step");
                if (lua_isnumber(m_lua, -1)) param.step = static_cast<float>(lua_tonumber(m_lua, -1));
                lua_pop(m_lua, 1);

                // Read choices for choice type
                lua_getfield(m_lua, -1, "choices");
                if (lua_istable(m_lua, -1)) {
                    int choicesIndex = lua_gettop(m_lua);
                    lua_pushnil(m_lua);
                    while (lua_next(m_lua, choicesIndex) != 0) {
                        if (lua_isstring(m_lua, -1)) {
                            param.choices.push_back(lua_tostring(m_lua, -1));
                        }
                        lua_pop(m_lua, 1);
                    }
                }
                lua_pop(m_lua, 1);

                if (!param.key.empty()) {
                    config.parameters.push_back(param);
                }
            }
            lua_pop(m_lua, 1);
        }
    }
    lua_pop(m_lua, 1);  // pop parameters table

    lua_pop(m_lua, 1);  // pop script_info table

    return config;
}

void ScriptEngine::syncParameters(const std::vector<ScriptParameter>& params) {
    for (const auto& param : params) {
        m_parameters[param.key] = param.value;
    }
}

void ScriptEngine::applyWeaponPreset(const WeaponPreset* preset) {
    if (!preset) return;

    // Map weapon preset fields to standard anti-recoil script parameters
    m_parameters["strength_ads"] = preset->adsStrength;
    m_parameters["strength_hipfire"] = preset->hipFireStrength;
    m_parameters["horizontal_strength"] = preset->horizontalStrength;
    m_parameters["ads_threshold"] = preset->adsThreshold;
    m_parameters["fire_threshold"] = preset->fireThreshold;
    m_parameters["smoothing"] = preset->smoothing;
}
