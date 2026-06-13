#pragma once

#ifdef GAME_DEBUG

#include <imgui.h>
#include <string>
#include <variant>
#include <algorithm>
#include "Settings.h"

namespace DevPanel {

class DevPanel {
public:
    void draw(bool* open) {
        ImGui::SetNextWindowSize(ImVec2(450, 500), ImGuiCond_FirstUseEver);
        
        if (!ImGui::Begin("Developer Settings", open)) {
            ImGui::End();
            return;
        }

        ImGui::TextDisabled("Modify engine parameters in real-time.");
        ImGui::Separator();

        // Iterate over our settings registry and generate appropriate UI controls automatically
        for (const auto& s : Settings::getRegistry()) {
            std::string label(s.name);
            
            // Render UI based on the actual type wrapped inside the Variant pointer
            std::visit([&s, &label](auto&& targetPtr) {
                using T = std::decay_t<decltype(*targetPtr)>;
                
                // Get typed min/max bounds from metadata
                T minBound = std::get<T>(s.minv);
                T maxBound = std::get<T>(s.maxv);

                if constexpr (std::is_same_v<T, float>) {
                    // Floating point settings get a standard slider
                    if (ImGui::SliderFloat(label.c_str(), targetPtr, minBound, maxBound, "%.4f")) {
                        Settings::saveToFile();
                    }
                } 
                else if constexpr (std::is_same_v<T, int>) {
                    // Handle specific edge-cases based on setting naming conventions
                    if (label.find("MODE") != std::string::npos || label.find("USE_") != std::string::npos) {
                        // Binary flags (0 or 1) get rendered as clean checkboxes
                        bool isChecked = (*targetPtr != 0);
                        if (ImGui::Checkbox(label.c_str(), &isChecked)) {
                            *targetPtr = isChecked ? 1 : 0;
                            Settings::saveToFile();
                        }
                    } 
                    else if (label == "DITHER_PALETTE") {
                        // Special dropdown selection combo-box for the dither palette setting
                        const char* palettes[] = { "256 Colors", "4096 Colors", "32768 Colors" };
                        int currentIdx = 2; // Default fallback to 32768
                        
                        if (*targetPtr == 256) currentIdx = 0;
                        else if (*targetPtr == 4096) currentIdx = 1;

                        if (ImGui::Combo(label.c_str(), &currentIdx, palettes, IM_ARRAYSIZE(palettes))) {
                            if (currentIdx == 0) *targetPtr = 256;
                            else if (currentIdx == 1) *targetPtr = 4096;
                            else *targetPtr = 32768;
                            
                            Settings::saveToFile();
                        }
                    } 
                    else {
                        // Generic integer values fall back to a standard integer slider
                        if (ImGui::SliderInt(label.c_str(), targetPtr, minBound, maxBound)) {
                            Settings::saveToFile();
                        }
                    }
                }
            }, s.ptr);

            // Display the helpful description string underneath the control item
            if (ImGui::IsItemHovered() && !s.desc.empty()) {
                ImGui::SetTooltip("%s", s.desc.data());
            }
        }

        ImGui::End();
    }
};

inline DevPanel& get() {
    static DevPanel instance;
    return instance;
}

} // namespace DevPanel

#endif
