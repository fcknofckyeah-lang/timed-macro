#include "Macro.hpp"
#include <gdr/gdr.hpp>
#include <algorithm>
#include <limits>

using namespace geode::prelude;

// ---------------------------------------------------------------------
// GDR / GDR2 parsing.
// This is the ONE place that touches the GDReplayFormat library. If its
// API differs slightly from what's below, check its README and adjust
// only this section (struct names, importData's return type, field names).
// ---------------------------------------------------------------------
struct TInput : gdr::Input {
    using gdr::Input::Input;
    TInput() = default;
};

struct TReplay : gdr::Replay<TReplay, TInput> {
    TReplay() : Replay("TimedMacro", 1) {}
};

MacroState& MacroState::get() {
    static MacroState instance;
    return instance;
}

Result<size_t> MacroState::load(std::filesystem::path const& path) {
    auto data = file::readBinary(path);
    if (data.isErr()) return Err("Couldn't read that file");

    auto replay = TReplay::importData(data.unwrap());   // ADAPT if it returns a Result/optional

    std::vector<MacroJump> out;
    for (auto const& in : replay.inputs) {
        int button = static_cast<int>(in.button);
        if (in.down) {
            MacroJump j;
            j.press = in.frame;
            j.release = std::numeric_limits<uint32_t>::max();
            j.button = button;
            j.player2 = in.player2;
            out.push_back(j);
        } else {
            // match to the most recent unreleased press of the same button/player
            for (auto it = out.rbegin(); it != out.rend(); ++it) {
                if (it->button == button && it->player2 == in.player2 &&
                    it->release == std::numeric_limits<uint32_t>::max()) {
                    it->release = in.frame;
                    break;
                }
            }
        }
    }
    for (auto& j : out) {
        if (j.release == std::numeric_limits<uint32_t>::max()) j.release = j.press + 1;
    }
    if (out.empty()) return Err("No inputs found in that macro");

    std::stable_sort(out.begin(), out.end(), [](auto const& a, auto const& b) {
        return a.press < b.press;
    });

    jumps = std::move(out);
    fps = replay.framerate > 0 ? static_cast<uint32_t>(replay.framerate) : 240;
    name = path.filename().string();
    resetRun();
    return Ok(jumps.size());
}

void MacroState::clear() {
    jumps.clear();
    name.clear();
    cursor = 0;
}

void MacroState::resetRun() {
    cursor = 0;
    for (auto& j : jumps) {
        j.armed = j.pressed = j.released = false;
    }
}

bool MacroState::isActive() const {
    return !jumps.empty() && Mod::get()->getSettingValue<bool>("enabled");
}

uint32_t MacroState::windowFrames() const {
    double secs = Mod::get()->getSettingValue<double>("window-seconds");
    return static_cast<uint32_t>(secs * fps);
}

void MacroState::onUserClick(uint32_t frame, int button, bool isPlayer1) {
    uint32_t win = windowFrames();
    for (size_t i = cursor; i < jumps.size(); ++i) {
        auto& j = jumps[i];
        if (j.press > frame + win) break;                 // too far in the future
        if (j.armed || j.press < frame) continue;         // already handled / passed
        if (j.button != button) continue;
        if (j.player2 == isPlayer1) continue;             // wrong player
        j.armed = true;
        return;
    }
    // click outside every window: ignored
}
