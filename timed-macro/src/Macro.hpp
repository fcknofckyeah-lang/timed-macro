#pragma once
#include <Geode/Geode.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// One recorded jump (a press and its matching release).
struct MacroJump {
    uint32_t press = 0;
    uint32_t release = 0;
    int button = 1;        // 1 = jump, 2 = left, 3 = right (platformer)
    bool player2 = false;

    // runtime state, cleared on every level reset
    bool armed = false;    // the user clicked inside the window
    bool pressed = false;
    bool released = false;
};

class MacroState {
public:
    static MacroState& get();

    std::vector<MacroJump> jumps;
    uint32_t fps = 240;
    size_t cursor = 0;
    std::string name;

    geode::Result<size_t> load(std::filesystem::path const& path);
    void clear();
    void resetRun();
    bool isActive() const;
    uint32_t windowFrames() const;

    // Called when the player physically clicks. Arms the earliest
    // matching jump whose window contains `frame`.
    void onUserClick(uint32_t frame, int button, bool isPlayer1);
};
