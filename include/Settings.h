#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cctype>
#include <string_view>
#include <variant>
#include <functional>

namespace Settings {

constexpr std::string_view SETTINGS_FILE = "settings.cfg";

// Use pointers inside variant to eliminate data duplication and synchronization
using SettingPtr = std::variant<int*, float*>;
using SettingValue = std::variant<int, float>;

struct Setting {
    std::string_view name;
    SettingPtr ptr;
    SettingValue minv;
    SettingValue maxv;
    std::string_view desc;
    std::function<void()> onValidate = nullptr;

    Setting(std::string_view n, SettingPtr p, SettingValue min_val, SettingValue max_val, 
            std::string_view d, std::function<void()> validate = nullptr)
        : name(n), ptr(p), minv(min_val), maxv(max_val), desc(d), onValidate(validate) {}
};

// Variable storage
inline float LIGHT_RADIUS_NEAR        = 1.0f;
inline float LIGHT_RADIUS_FAR         = 5.0f;
inline float AMBIENT_LIGHT            = 0.01f;
inline float LIGHT_TRANSITION_SOFTNESS= 0.6f;
inline float LIGHT_SHARPNESS          = 5.0f;
inline float AO_LIGHT_BLEND           = 0.1f;
inline float FOV_DEGREES              = 60.0f;
inline int   DEBUG_WHITEBOX_MODE      = 0;
inline float SCREEN_DISTORTION        = 0.08f;
inline float ANISOTROPY_LEVEL         = 4.0f;
inline int   USE_DITHER               = 1;
inline float DITHER_AMOUNT            = 0.06f;
inline int   DITHER_PALETTE           = 32768;
inline float PLAYER_SPEED             = 1.5f;

// Custom validation routines
inline void validatePalette() {
    if (DITHER_PALETTE <= 256) DITHER_PALETTE = 256;
    else if (DITHER_PALETTE <= 4096) DITHER_PALETTE = 4096;
    else DITHER_PALETTE = 32768;
}

// Global registry containing metadata for all settings
inline const std::vector<Setting>& getRegistry() {
    static const std::vector<Setting> registry = {
        Setting("LIGHT_RADIUS_NEAR",         &LIGHT_RADIUS_NEAR,         0.0f,    50.0f,    "near radius"),
        Setting("LIGHT_RADIUS_FAR",          &LIGHT_RADIUS_FAR,          0.01f,   100.0f,   "far radius"),
        Setting("AMBIENT_LIGHT",             &AMBIENT_LIGHT,             0.0f,    1.0f,     "ambient"),
        Setting("LIGHT_TRANSITION_SOFTNESS", &LIGHT_TRANSITION_SOFTNESS, 0.0f,    1.0f,     "transition softness"),
        Setting("LIGHT_SHARPNESS",           &LIGHT_SHARPNESS,           0.0001f, 50.0f,    "sharpness"),
        Setting("AO_LIGHT_BLEND",            &AO_LIGHT_BLEND,            0.0f,    1.0f,     "AO blend"),
        Setting("FOV_DEGREES",               &FOV_DEGREES,               10.0f,   140.0f,   "FOV degrees"),
        Setting("DEBUG_WHITEBOX_MODE",       &DEBUG_WHITEBOX_MODE,       0,       1,        "debug whitebox (0/1)"),
        Setting("SCREEN_DISTORTION",         &SCREEN_DISTORTION,         0.0f,    0.5f,     "screen distortion"),
        Setting("ANISOTROPY_LEVEL",          &ANISOTROPY_LEVEL,          1.0f,    16.0f,    "anisotropy level"),
        Setting("USE_DITHER",                &USE_DITHER,                0,       1,        "use dither (0/1)"),
        Setting("DITHER_AMOUNT",             &DITHER_AMOUNT,             0.0f,    0.5f,     "dither amount"),
        Setting("DITHER_PALETTE",            &DITHER_PALETTE,            256,     32768,    "dither palette", validatePalette),
        Setting("PLAYER_SPEED",              &PLAYER_SPEED,              0.01f,   8.0f,     "player speed"),
    };
    return registry;
}

// Formats pointing variables for string outputs
inline std::string valueToString(const SettingPtr& ptr) {
    std::ostringstream ss; ss << std::fixed << std::setprecision(4);
    std::visit([&ss](auto&& arg) { ss << *arg; }, ptr);
    return ss.str();
}

// Applies typesafe clamp bounds directly via pointers
inline void applyValue(const Setting& setting, float rawValue) {
    std::visit([rawValue, &setting](auto&& targetPtr) {
        using T = std::decay_t<decltype(*targetPtr)>;
        T typedValue = static_cast<T>(rawValue);
        T minBound = std::get<T>(setting.minv);
        T maxBound = std::get<T>(setting.maxv);
        
        *targetPtr = std::clamp(typedValue, minBound, maxBound);
    }, setting.ptr);

    if (setting.onValidate) {
        setting.onValidate();
    }
}

// File persistence routines
inline bool saveToFile(const std::string &path = std::string(SETTINGS_FILE)) {
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << "# Settings saved\n";
    for (const auto& s : getRegistry()) {
        ofs << s.name << "=" << valueToString(s.ptr) << "\n";
    }
    return true;
}

inline const Setting* findSetting(const std::string& name) {
    for (const auto& s : getRegistry()) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

inline std::string setByName(const std::string &name, float value) {
    if (const auto* s = findSetting(name)) {
        applyValue(*s, value);
        saveToFile();
        return std::string(s->name) + " = " + valueToString(s->ptr);
    }
    return "Unknown setting: " + name;
}

inline std::string getByName(const std::string &name) {
    if (const auto* s = findSetting(name)) {
        return std::string(s->name) + " = " + valueToString(s->ptr);
    }
    return "Unknown setting: " + name;
}

inline std::string listAll() {
    std::ostringstream ss;
    for (const auto& s : getRegistry()) {
        ss << s.name << "=" << valueToString(s.ptr) << " # " << s.desc << "\n";
    }
    return ss.str();
}

inline bool loadFromFile(const std::string &path = std::string(SETTINGS_FILE)) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        
        if (const auto* s = findSetting(key)) {
            try {
                applyValue(*s, std::stof(val));
            } catch(...) {}
        }
    }
    return true;
}

} // namespace Settings
