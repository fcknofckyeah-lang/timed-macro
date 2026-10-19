#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "Macro.hpp"
#include "MacroPopup.hpp"

using namespace geode::prelude;

// Current physics step. If a GD update moves this field, fix it here only.
static uint32_t currentFrame(GJBaseGameLayer* layer) {
    return layer->m_gameState.m_currentProgress;
}

class $modify(TMGameLayer, GJBaseGameLayer) {
    bool assistActive() {
        // only inside a real level, never the editor
        return typeinfo_cast<PlayLayer*>(this) && MacroState::get().isActive();
    }

    // Runs every physics step: fire the macro's jumps that the user armed.
    void processCommands(float dt) {
        if (this->assistActive()) {
            auto& s = MacroState::get();
            uint32_t frame = currentFrame(this);

            // skip jumps that are finished or whose window already closed
            while (s.cursor < s.jumps.size()) {
                auto& j = s.jumps[s.cursor];
                if (j.released || (!j.armed && j.press < frame)) s.cursor++;
                else break;
            }

            for (size_t i = s.cursor; i < s.jumps.size(); ++i) {
                auto& j = s.jumps[i];
                if (j.press > frame) break;
                if (!j.armed || j.released) continue;

                // qualified calls go straight to the game, bypassing our own hooks below
                if (!j.pressed) {
                    GJBaseGameLayer::pushButton(j.button, !j.player2);
                    j.pressed = true;
                }
                if (frame >= j.release) {
                    GJBaseGameLayer::releaseButton(j.button, !j.player2);
                    j.released = true;
                }
            }
        }
        GJBaseGameLayer::processCommands(dt);
    }

    // The player's real click: don't jump, just arm the matching macro jump.
    void pushButton(int button, bool isPlayer1) {
        if (!this->assistActive()) return GJBaseGameLayer::pushButton(button, isPlayer1);
        MacroState::get().onUserClick(currentFrame(this), button, isPlayer1);
    }

    // The player's real release is ignored too; the macro handles releases itself.
    void releaseButton(int button, bool isPlayer1) {
        if (!this->assistActive()) return GJBaseGameLayer::releaseButton(button, isPlayer1);
    }
};

class $modify(TMPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        MacroState::get().resetRun();
    }
};

class $modify(TMPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        auto spr = ButtonSprite::create("Macro", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(TMPauseLayer::onMacro));
        menu->addChild(btn);
        menu->setPosition({55.f, winSize.height - 30.f});
        this->addChild(menu);
    }

    void onMacro(CCObject*) {
        if (auto popup = MacroPopup::create()) popup->show();
    }
};
