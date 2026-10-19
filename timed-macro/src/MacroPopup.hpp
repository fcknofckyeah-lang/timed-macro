#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/ui/Notification.hpp>
#include <optional>
#include "Macro.hpp"

using namespace geode::prelude;

// Written for Geode v5 (non-templated Popup, async file picker).
class MacroPopup : public Popup {
protected:
    async::TaskHolder<Result<std::optional<std::filesystem::path>>> m_pick;
    CCLabelBMFont* m_status = nullptr;

    bool init() {
        if (!Popup::init(300.f, 220.f)) return false;

        this->setTitle("Timed Macro");

        m_status = CCLabelBMFont::create("", "bigFont.fnt");
        m_status->setScale(0.4f);
        m_mainLayer->addChildAtPosition(m_status, Anchor::Center, {0.f, 45.f});
        this->refresh();

        auto loadBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Load .gdr / .gdr2"), this, menu_selector(MacroPopup::onLoad)
        );
        m_buttonMenu->addChildAtPosition(loadBtn, Anchor::Center, {0.f, 5.f});

        auto clearBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Clear", "goldFont.fnt", "GJ_button_06.png", 0.8f),
            this, menu_selector(MacroPopup::onClear)
        );
        m_buttonMenu->addChildAtPosition(clearBtn, Anchor::Center, {0.f, -30.f});

        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(MacroPopup::onToggle), 0.7f
        );
        toggler->toggle(Mod::get()->getSettingValue<bool>("enabled"));
        m_buttonMenu->addChildAtPosition(toggler, Anchor::Center, {-45.f, -65.f});

        auto lbl = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        lbl->setScale(0.45f);
        lbl->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChildAtPosition(lbl, Anchor::Center, {-25.f, -65.f});

        return true;
    }

    void refresh() {
        auto& s = MacroState::get();
        if (s.jumps.empty()) {
            m_status->setString("No macro loaded");
        } else {
            m_status->setString(
                fmt::format("{} - {} jumps @ {} fps", s.name, s.jumps.size(), s.fps).c_str()
            );
        }
    }

    void onLoad(CCObject*) {
        file::FilePickOptions opts;
        opts.filters = {{"Geometry Dash Replay", {"*.gdr", "*.gdr2"}}};

        // TaskHolder cancels this automatically if the popup closes first
        m_pick.spawn(
            file::pick(file::PickMode::OpenFile, opts),
            [this](Result<std::optional<std::filesystem::path>> result) {
                if (result.isErr()) return;
                auto picked = result.unwrap();
                if (!picked) return;   // user cancelled the dialog

                auto loaded = MacroState::get().load(*picked);
                if (loaded.isOk()) {
                    Notification::create(
                        fmt::format("Loaded {} jumps", loaded.unwrap()),
                        NotificationIcon::Success
                    )->show();
                } else {
                    Notification::create(loaded.unwrapErr(), NotificationIcon::Error)->show();
                }
                this->refresh();
            }
        );
    }

    void onClear(CCObject*) {
        MacroState::get().clear();
        this->refresh();
    }

    void onToggle(CCObject* sender) {
        // toggler state is the value *before* this click
        bool now = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
        Mod::get()->setSettingValue<bool>("enabled", now);
    }

public:
    static MacroPopup* create() {
        auto ret = new MacroPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
