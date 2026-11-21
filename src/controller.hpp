#pragma once
#include <cmath>
#include <Geode/ui/Notification.hpp>
#include "widgets.hpp"
#include "extras.hpp"

inline float uiScale() {
    return std::clamp(Mod::get()->getSavedValue<float>("ui-scale", 0.75f), 0.5f, 0.95f);
}

inline bool showStats() {
    return Mod::get()->getSavedValue<bool>("show-stats", true);
}

inline bool showHitboxes() {
    return Mod::get()->getSavedValue<bool>("show-hitboxes", false);
}

inline bool trailOn() {
    return Mod::get()->getSavedValue<bool>("trail-on", false);
}

// the whole menu: PM button + panel
class PMController : public CCNode {
public:
    static constexpr float PW = 420.f;
    static constexpr float PH = 330.f;

    CCNode* panel = nullptr;
    DragBar* dragBar = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    CCLabelBMFont* fpsLabel = nullptr;
    CCLabelBMFont* delayLabel = nullptr;
    CCLabelBMFont* trailLabel = nullptr;
    CCLabelBMFont* colorLabel = nullptr;
    CCLabelBMFont* sizeLabel = nullptr;
    CCLabelBMFont* macroLabel = nullptr;
    PMBtn* noclipT = nullptr;
    PMBtn* safeT = nullptr;
    PMBtn* hitboxT = nullptr;
    PMBtn* boxesT = nullptr;
    PMBtn* trailT = nullptr;
    PMBtn* statsT = nullptr;
    PMBtn* unlockT = nullptr;
    PMBtn* recT = nullptr;
    PMBtn* replayT = nullptr;
    bool inGame = false;

    static PMController* create(bool inGame) {
        auto r = new PMController();
        if (r && r->init(inGame)) {
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }

    bool init(bool game) {
        if (!CCNode::init()) return false;
        inGame = game;
        auto win = CCDirector::get()->getWinSize();
        const char* FONT = "chatFont.fnt";
        float sc = uiScale();

        // dark rounded panel
        auto p = PMBtn::box({PW, PH}, {24, 26, 44}, 238);
        p->setScale(sc);
        float px = Mod::get()->getSavedValue<float>("panel-x", (win.width - PW * sc) / 2.f);
        float py = Mod::get()->getSavedValue<float>("panel-y", (win.height - PH * sc) / 2.f);
        px = std::clamp(px, 0.f, std::max(0.f, win.width - PW * sc));
        py = std::clamp(py, 0.f, std::max(0.f, win.height - PH * sc));
        p->setPosition({px, py});
        p->setVisible(false);
        this->addChild(p, 1);
        panel = p;

        panel->addChild(TouchBlocker::create({PW, PH}));

        // header strip + title
        auto header = PMBtn::box({PW - 8.f, 26.f}, {60, 66, 112}, 255);
        header->setPosition({4.f, PH - 30.f});
        panel->addChild(header);

        auto title = CCLabelBMFont::create("Practice Menu", "goldFont.fnt");
        title->setScale(0.5f);
        title->setPosition({PW / 2.f, PH - 17.f});
        panel->addChild(title);

        auto label = [&](const char* text, float x, float y, float scale, bool left) -> CCLabelBMFont* {
            auto l = CCLabelBMFont::create(text, FONT);
            l->setScale(scale);
            if (left) l->setAnchorPoint({0.f, 0.5f});
            l->setPosition({x, y});
            panel->addChild(l);
            return l;
        };

        auto toggle = [&](const char* text, float y, std::function<void()> cb) -> PMBtn* {
            auto b = PMBtn::createToggle(cb);
            b->setPosition({14.f, y - 11.f});
            panel->addChild(b);
            label(text, 45.f, y, 0.8f, true);
            return b;
        };

        auto stepper = [&](float y, std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            auto m = PMBtn::createText({26.f, 22.f}, {70, 76, 112}, "-", "bigFont.fnt", 0.5f, minus);
            m->setPosition({212.f, y - 11.f});
            panel->addChild(m);
            auto l = label("", 305.f, y, 0.8f, false);
            auto pl = PMBtn::createText({26.f, 22.f}, {70, 76, 112}, "+", "bigFont.fnt", 0.5f, plus);
            pl->setPosition({372.f, y - 11.f});
            panel->addChild(pl);
            return l;
        };

        auto button = [&](const char* text, float cx, float y, ccColor3B color, std::function<void()> cb) {
            auto b = PMBtn::createText({84.f, 22.f}, color, text, FONT, 0.7f, cb);
            b->setPosition({cx - 42.f, y - 11.f});
            panel->addChild(b);
        };

        dragBar = DragBar::create(panel, {PW, 30.f}, {PW * sc, PH * sc});
        dragBar->setPosition({0.f, PH - 30.f});
        panel->addChild(dragBar);

        // left column
        noclipT = toggle("Noclip", 268.f, [this]() { onNoclip(); });
        safeT = toggle("Safe mode", 240.f, [this]() { onSafe(); });
        hitboxT = toggle("Hitbox on death", 212.f, [this]() { onHitbox(); });
        boxesT = toggle("Show hitboxes", 184.f, [this]() { onBoxes(); });
        trailT = toggle("Hitbox trail", 156.f, [this]() { onTrail(); });
        statsT = toggle("Show stats", 128.f, [this]() { onStats(); });
        unlockT = toggle("Unlock icons", 100.f, [this]() { onUnlock(); });
        if (inGame) {
            recT = toggle("Record macro", 72.f, [this]() { onRecord(); });
            replayT = toggle("Replay macro", 44.f, [this]() { onReplay(); });
            macroLabel = label("", 14.f, 16.f, 0.65f, true);
        }

        // right column
        speedLabel = stepper(268.f, [this]() { changeSpeed(-0.25f); },
                             [this]() { changeSpeed(0.25f); });
        fpsLabel = stepper(240.f, [this]() { changeFps(-1); },
                           [this]() { changeFps(1); });
        delayLabel = stepper(212.f, [this]() { changeDelay(-0.25f); },
                             [this]() { changeDelay(0.25f); });
        trailLabel = stepper(184.f, [this]() { changeTrail(-50); },
                             [this]() { changeTrail(50); });
        colorLabel = stepper(156.f, [this]() { changeColor(-1); },
                             [this]() { changeColor(1); });
        sizeLabel = stepper(128.f, [this]() { changeSize(-0.05f); },
                            [this]() { changeSize(0.05f); });

        button("Cheats off", 265.f, 100.f, {200, 70, 70}, [this]() { onCheatsOff(); });
        button("Hide menu", 355.f, 100.f, {90, 95, 125}, [this]() { onHide(); });
        button("Lite mode", 265.f, 72.f, {50, 160, 90}, [this]() { onLite(); });
        button("CBF", 355.f, 72.f, {130, 90, 200}, []() { openCBF(); });
        button("Save icons", 265.f, 44.f, {50, 120, 215}, []() { saveIcons(); });
        button("Load icons", 355.f, 44.f, {50, 120, 215}, []() { loadIcons(); });

        // the round PM button (smaller)
        auto pmText = CCLabelBMFont::create("PM", "bigFont.fnt");
        pmText->setScale(0.6f);
        auto circle = CircleButtonSprite::create(pmText, CircleBaseColor::Green, CircleBaseSize::Small);
        circle->setScale(0.55f);
        const float R = 14.f;
        auto pmBtn = PMButton::create(circle, R, [this]() {
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        float bx = Mod::get()->getSavedValue<float>("btn-x", win.width - 40.f);
        float by = Mod::get()->getSavedValue<float>("btn-y", win.height - 40.f);
        bx = std::clamp(bx, 0.f, win.width - R * 2);
        by = std::clamp(by, 0.f, win.height - R * 2);
        pmBtn->setPosition({bx, by});
        this->addChild(pmBtn, 2);

        refresh();
        return true;
    }

    void refresh() {
        auto& s = S();
        noclipT->setOn(s.noclip);
        safeT->setOn(s.safe);
        hitboxT->setOn(s.hitboxOnDeath);
        boxesT->setOn(showHitboxes());
        trailT->setOn(trailOn());
        statsT->setOn(showStats());
        unlockT->setOn(unlockAll());
        if (recT) recT->setOn(s.recording);
        if (replayT) replayT->setOn(s.replaying);
        speedLabel->setString(fmt::format("Speed: {:.2f}x", s.speed).c_str());
        fpsLabel->setString(fmt::format("FPS: {}", FPS_STEPS[s.fpsIndex]).c_str());
        delayLabel->setString(fmt::format("Delay: {:.2f}s", s.respawnDelay).c_str());
        trailLabel->setString(fmt::format("Length: {}", s.trail).c_str());
        colorLabel->setString(fmt::format("Color: {}", TRAIL_COLORS[s.trailColor].name).c_str());
        sizeLabel->setString(fmt::format("Size: {}%", static_cast<int>(std::round(uiScale() * 100.f))).c_str());
        if (macroLabel) macroLabel->setString(fmt::format("{} inputs", s.macro.size()).c_str());
    }

    void onNoclip() {
        S().noclip = !S().noclip;
        saveAll();
        refresh();
    }

    void onSafe() {
        S().safe = !S().safe;
        saveAll();
        applyFps();
        refresh();
    }

    void onHitbox() {
        S().hitboxOnDeath = !S().hitboxOnDeath;
        saveAll();
        refresh();
    }

    void onBoxes() {
        Mod::get()->setSavedValue<bool>("show-hitboxes", !showHitboxes());
        refresh();
    }

    void onTrail() {
        Mod::get()->setSavedValue<bool>("trail-on", !trailOn());
        refresh();
    }

    void onStats() {
        Mod::get()->setSavedValue<bool>("show-stats", !showStats());
        refresh();
    }

    void onUnlock() {
        Mod::get()->setSavedValue<bool>("unlock-all", !unlockAll());
        refresh();
    }

    void changeSpeed(float d) {
        auto& s = S();
        s.speed = std::clamp(s.speed + d, 0.25f, 2.f);
        saveAll();
        refresh();
    }

    void changeFps(int d) {
        auto& s = S();
        s.fpsIndex = std::clamp(s.fpsIndex + d, 0, static_cast<int>(FPS_STEPS.size()) - 1);
        saveAll();
        applyFps();
        refresh();
    }

    void changeDelay(float d) {
        auto& s = S();
        s.respawnDelay = std::clamp(s.respawnDelay + d, 0.f, 2.f);
        saveAll();
        refresh();
    }

    void changeTrail(int d) {
        auto& s = S();
        s.trail = std::clamp(s.trail + d, 0, 1000);
        saveAll();
        refresh();
    }

    void changeColor(int d) {
        auto& s = S();
        int n = static_cast<int>(TRAIL_COLORS.size());
        s.trailColor = (s.trailColor + d + n) % n;
        saveAll();
        refresh();
    }

    void changeSize(float d) {
        float sc = std::clamp(uiScale() + d, 0.5f, 0.95f);
        sc = std::round(sc * 20.f) / 20.f;
        Mod::get()->setSavedValue<float>("ui-scale", sc);
        panel->setScale(sc);
        dragBar->panelSize = CCSize{PW * sc, PH * sc};
        auto win = CCDirector::get()->getWinSize();
        auto pos = panel->getPosition();
        pos.x = std::clamp(pos.x, 0.f, std::max(0.f, win.width - PW * sc));
        pos.y = std::clamp(pos.y, 0.f, std::max(0.f, win.height - PH * sc));
        panel->setPosition(pos);
        refresh();
    }

    void onRecord() {
        auto& s = S();
        s.recording = !s.recording;
        if (s.recording) {
            s.replaying = false;
            s.macro.clear();
            if (auto pl = PlayLayer::get()) pl->resetLevel();
        }
        refresh();
    }

    void onReplay() {
        auto& s = S();
        s.replaying = !s.replaying;
        if (s.replaying) {
            s.recording = false;
            s.replayIndex = 0;
            if (auto pl = PlayLayer::get()) pl->resetLevel();
        }
        refresh();
    }

    void onCheatsOff() {
        auto& s = S();
        s.noclip = false;
        s.speed = 1.f;
        s.fpsIndex = 1;
        s.respawnDelay = 0.f;
        s.trail = 0;
        s.hitboxOnDeath = false;
        s.recording = false;
        s.replaying = false;
        Mod::get()->setSavedValue<bool>("trail-on", false);
        saveAll();
        applyFps();
        refresh();
    }

    // turns off the mod's own heavier extras
    void onLite() {
        auto& s = S();
        s.trail = 0;
        s.hitboxOnDeath = false;
        s.respawnDelay = 0.f;
        Mod::get()->setSavedValue<bool>("trail-on", false);
        saveAll();
        refresh();
        pmNotify("Lite mode on. Also try Low Detail Mode in GD settings");
    }

    void onHide() {
        Mod::get()->setSettingValue<bool>("hide-menu", true);
        this->setVisible(false);
        pmNotify("PM hidden. Turn it back on in Geode > Practice Mod > settings");
    }
};
