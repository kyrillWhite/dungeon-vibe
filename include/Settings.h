#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cctype>

namespace Settings {

constexpr char * SETTINGS_FILE = "settings.cfg";

// Default values
inline float LIGHT_RADIUS_NEAR        = 1.0f;
inline float LIGHT_RADIUS_FAR         = 5.0f;
inline float AMBIENT_LIGHT            = 0.01f;
inline float LIGHT_TRANSITION_SOFTNESS= 0.6f;   // 0..1
inline float LIGHT_SHARPNESS          = 5.0f;   // >= 0.0001
inline float AO_LIGHT_BLEND           = 0.1f;   // 0..1
inline float FOV_DEGREES              = 60.0f;
inline int   DEBUG_WHITEBOX_MODE      = 0;      // 0/1
inline float SCREEN_DISTORTION        = 0.08f;  // 0..0.5
inline float ANISOTROPY_LEVEL         = 4.0f;   // 1..max supported (typical 16)
inline int   USE_DITHER               = 1;      // 0/1
inline int   DITHER_PALETTE           = 32768;    // 256 / 4096 / 32768
inline float DITHER_AMOUNT            = 0.06f;  // legacy, kept but not used for FS

struct Meta { float minv, maxv; const char* desc; };

// Limits
constexpr Meta M_LIGHT_RADIUS_NEAR         = {0.0f, 50.0f, "near radius"};
constexpr Meta M_LIGHT_RADIUS_FAR          = {0.01f, 100.0f, "far radius"};
constexpr Meta M_AMBIENT_LIGHT             = {0.0f, 1.0f, "ambient"};
constexpr Meta M_LIGHT_TRANSITION_SOFTNESS = {0.0f, 1.0f, "transition softness"};
constexpr Meta M_LIGHT_SHARPNESS           = {0.0001f, 50.0f, "sharpness"};
constexpr Meta M_AO_LIGHT_BLEND            = {0.0f, 1.0f, "AO blend"};
constexpr Meta M_FOV_DEGREES               = {10.0f, 140.0f, "FOV degrees"};
constexpr Meta M_SCREEN_DISTORTION         = {0.0f, 0.5f, "screen distortion"};
constexpr Meta M_DEBUG_WHITEBOX_MODE       = {0.0f, 1.0f, "debug whitebox (0/1)"};
constexpr Meta M_ANISOTROPY_LEVEL         = {1.0f, 16.0f, "anisotropy level"};
constexpr Meta M_USE_DITHER               = {0.0f, 1.0f, "use dither (0/1)"};
constexpr Meta M_DITHER_AMOUNT            = {0.0f, 0.5f, "dither amount"};
constexpr Meta M_DITHER_PALETTE           = {0.0f, 32768.0f, "dither palette (256/4096/32768)"};

inline void clampInPlace(float &v, const Meta &m) { if (v < m.minv) v = m.minv; if (v > m.maxv) v = m.maxv; }

inline void applySettingNoSave(const std::string &nameUpper, float value) {
    if (nameUpper == "LIGHT_RADIUS_NEAR") { clampInPlace(value, M_LIGHT_RADIUS_NEAR); LIGHT_RADIUS_NEAR = value; }
    else if (nameUpper == "LIGHT_RADIUS_FAR") { clampInPlace(value, M_LIGHT_RADIUS_FAR); LIGHT_RADIUS_FAR = value; }
    else if (nameUpper == "AMBIENT_LIGHT") { clampInPlace(value, M_AMBIENT_LIGHT); AMBIENT_LIGHT = value; }
    else if (nameUpper == "LIGHT_TRANSITION_SOFTNESS") { clampInPlace(value, M_LIGHT_TRANSITION_SOFTNESS); LIGHT_TRANSITION_SOFTNESS = value; }
    else if (nameUpper == "LIGHT_SHARPNESS") { clampInPlace(value, M_LIGHT_SHARPNESS); LIGHT_SHARPNESS = value; }
    else if (nameUpper == "AO_LIGHT_BLEND") { clampInPlace(value, M_AO_LIGHT_BLEND); AO_LIGHT_BLEND = value; }
    else if (nameUpper == "FOV_DEGREES") { clampInPlace(value, M_FOV_DEGREES); FOV_DEGREES = value; }
    else if (nameUpper == "DEBUG_WHITEBOX_MODE") { DEBUG_WHITEBOX_MODE = (value >= 0.5f) ? 1 : 0; }
    else if (nameUpper == "SCREEN_DISTORTION") { clampInPlace(value, M_SCREEN_DISTORTION); SCREEN_DISTORTION = value; }
    else if (nameUpper == "ANISOTROPY_LEVEL") { clampInPlace(value, M_ANISOTROPY_LEVEL); ANISOTROPY_LEVEL = value; }
    else if (nameUpper == "USE_DITHER") { value = (value >= 0.5f) ? 1.0f : 0.0f; clampInPlace(value, M_USE_DITHER); USE_DITHER = static_cast<int>(value); }
    else if (nameUpper == "DITHER_AMOUNT") { clampInPlace(value, M_DITHER_AMOUNT); DITHER_AMOUNT = value; }
    else if (nameUpper == "DITHER_PALETTE") {
        // accept only 256, 4096, 32768; otherwise choose closest
        int v = static_cast<int>(value);
        if (v <= 256) DITHER_PALETTE = 256;
        else if (v <= 4096) DITHER_PALETTE = 4096;
        else DITHER_PALETTE = 32768;
    }
}

// Save current settings to file (human-readable key=value, comments allowed)
inline bool saveToFile(const std::string &path = std::string(SETTINGS_FILE)) {
    std::ofstream ofs(path, std::ofstream::out | std::ofstream::trunc);
    if (!ofs.is_open()) return false;
    ofs << "# Settings saved\n";
    ofs << "LIGHT_RADIUS_NEAR=" << LIGHT_RADIUS_NEAR << "\n";
    ofs << "LIGHT_RADIUS_FAR=" << LIGHT_RADIUS_FAR << "\n";
    ofs << "AMBIENT_LIGHT=" << AMBIENT_LIGHT << "\n";
    ofs << "LIGHT_TRANSITION_SOFTNESS=" << LIGHT_TRANSITION_SOFTNESS << "\n";
    ofs << "LIGHT_SHARPNESS=" << LIGHT_SHARPNESS << "\n";
    ofs << "AO_LIGHT_BLEND=" << AO_LIGHT_BLEND << "\n";
    ofs << "FOV_DEGREES=" << FOV_DEGREES << "\n";
    ofs << "DEBUG_WHITEBOX_MODE=" << DEBUG_WHITEBOX_MODE << "\n";
    ofs << "SCREEN_DISTORTION=" << SCREEN_DISTORTION << "\n";
    ofs << "ANISOTROPY_LEVEL=" << ANISOTROPY_LEVEL << "\n";
    ofs << "USE_DITHER=" << USE_DITHER << "\n";
    ofs << "DITHER_AMOUNT=" << DITHER_AMOUNT << "\n";
    ofs << "DITHER_PALETTE=" << DITHER_PALETTE << "\n";
    ofs.close();
    return true;
}

inline std::string setByName(const std::string &name, float value) {
    // normalize name -> UPPER, trim
    auto trim = [](std::string &s){
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch){ return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch){ return !std::isspace(ch); }).base(), s.end());
    };
    std::string n = name;
    trim(n);
    for (auto &c: n) c = std::toupper((unsigned char)c);
    applySettingNoSave(n, value);
    saveToFile(); // persist immediately
    std::ostringstream ss; ss << n << " = " << std::fixed << std::setprecision(4) << value;
    return ss.str();
}

inline std::string getByName(const std::string &name) {
    std::string n = name;
    // Normalize
    n.erase(n.begin(), std::find_if(n.begin(), n.end(), [](int ch){ return !std::isspace(ch); }));
    n.erase(std::find_if(n.rbegin(), n.rend(), [](int ch){ return !std::isspace(ch); }).base(), n.end());
    for (auto &c: n) c = std::toupper((unsigned char)c);

    std::ostringstream ss; ss << std::fixed << std::setprecision(4);
    if (n == "LIGHT_RADIUS_NEAR") ss << LIGHT_RADIUS_NEAR;
    else if (n == "LIGHT_RADIUS_FAR") ss << LIGHT_RADIUS_FAR;
    else if (n == "AMBIENT_LIGHT") ss << AMBIENT_LIGHT;
    else if (n == "LIGHT_TRANSITION_SOFTNESS") ss << LIGHT_TRANSITION_SOFTNESS;
    else if (n == "LIGHT_SHARPNESS") ss << LIGHT_SHARPNESS;
    else if (n == "AO_LIGHT_BLEND") ss << AO_LIGHT_BLEND;
    else if (n == "FOV_DEGREES") ss << FOV_DEGREES;
    else if (n == "DEBUG_WHITEBOX_MODE") ss << DEBUG_WHITEBOX_MODE;
    else if (n == "SCREEN_DISTORTION") ss << SCREEN_DISTORTION;
    else if (n == "ANISOTROPY_LEVEL") ss << ANISOTROPY_LEVEL;
    else if (n == "USE_DITHER") ss << USE_DITHER;
    else if (n == "DITHER_AMOUNT") ss << DITHER_AMOUNT;
    else if (n == "DITHER_PALETTE") ss << DITHER_PALETTE;
    else return "Unknown setting: " + name;
    return n + " = " + ss.str();
}

inline std::string listAll() {
    std::ostringstream ss; ss << std::fixed << std::setprecision(4);
    ss << "LIGHT_RADIUS_NEAR=" << LIGHT_RADIUS_NEAR << "\n";
    ss << "LIGHT_RADIUS_FAR=" << LIGHT_RADIUS_FAR << "\n";
    ss << "AMBIENT_LIGHT=" << AMBIENT_LIGHT << "\n";
    ss << "LIGHT_TRANSITION_SOFTNESS=" << LIGHT_TRANSITION_SOFTNESS << "\n";
    ss << "LIGHT_SHARPNESS=" << LIGHT_SHARPNESS << "\n";
    ss << "AO_LIGHT_BLEND=" << AO_LIGHT_BLEND << "\n";
    ss << "FOV_DEGREES=" << FOV_DEGREES << "\n";
    ss << "DEBUG_WHITEBOX_MODE=" << DEBUG_WHITEBOX_MODE << "\n";
    ss << "SCREEN_DISTORTION=" << SCREEN_DISTORTION << "\n";
    ss << "ANISOTROPY_LEVEL=" << ANISOTROPY_LEVEL << "\n";
    ss << "USE_DITHER=" << USE_DITHER << "\n";
    ss << "DITHER_AMOUNT=" << DITHER_AMOUNT << "\n";
    ss << "DITHER_PALETTE=" << DITHER_PALETTE << "\n";
    return ss.str();
}

inline bool loadFromFile(const std::string &path = std::string(SETTINGS_FILE)) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        // Trim
        auto l = line;
        l.erase(l.begin(), std::find_if(l.begin(), l.end(), [](int ch){ return !std::isspace(ch); }));
        if (l.empty() || l[0]=='#') continue;
        auto pos = l.find('=');
        if (pos == std::string::npos) continue;
        std::string key = l.substr(0,pos);
        std::string val = l.substr(pos+1);
        // Trim both
        auto trim = [](std::string &s){
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch){ return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch){ return !std::isspace(ch); }).base(), s.end());
        };
        trim(key); trim(val);
        if (key.empty() || val.empty()) continue;
        try {
            float v = std::stof(val);
            std::string ku = key;
            for (auto &c: ku) c = std::toupper((unsigned char)c);
            applySettingNoSave(ku, v);
        } catch(...) {
            // Skip invalid values
        }
    }
    return true;
}

// Console parser (set/get/list/save/load)
inline std::string execConsoleCommand(const std::string &cmd) {
    std::istringstream iss(cmd);
    std::string op;
    if (!(iss >> op)) return "";
    for (auto &c: op) c = std::tolower((unsigned char)c);
    if (op == "set") {
        std::string name; float val;
        if (!(iss >> name >> val)) return "Usage: set NAME VALUE";
        return setByName(name, val);
    } else if (op == "get") {
        std::string name; if (!(iss >> name)) return "Usage: get NAME";
        return getByName(name);
    } else if (op == "list") {
        return listAll();
    } else if (op == "save") {
        return saveToFile() ? "Saved to " + std::string(SETTINGS_FILE) : "Save failed";
    } else if (op == "load") {
        return loadFromFile() ? "Loaded " + std::string(SETTINGS_FILE) : ("Load failed or file missing: " + std::string(SETTINGS_FILE));
    } else {
        return std::string("Unknown command: ") + op + " (supported: set/get/list/save/load)";
    }
}

} // namespace Settings
