#pragma once
#include <Geode/ui/Notification.hpp>
#include "widgets.hpp"
#include "extras.hpp"

// the whole menu: PM button + panel
class PMController : public CCNode {
public:
    CCNode* panel = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    CCLabelBMFont* fpsLabel = nullptr;
    CCLabelBMFont* delayLabel = nullptr;
    CCLabelBMFont* trailLabel = nullptr;
    CCLabelBMFont* colorLabel = nullptr;
    CCLabelBMFont* macroLabel = nullptr;
    PMBtn* noclipT = nullptr;
    PMBtn* safeT = nullptr;
    PMBtn* hitboxT = nullptr;
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

        const float PW = 420.f;
        const float PH = 265.f;
        auto p = CCLayerColor::create({0, 0, 0, 190}, PW, PH);
        float px = Mod::get()->getSavedValue<float>("panel-x", (win.width - PW) / 2.f);
        float py = Mod::get()->getSavedValue<float>("panel-y", (win.height - PH) / 2.f);
        px = std::clamp(px, 0.f, std::max(0.f, win.width - PW));
        py = std::clamp(py, 0.f, std::max(0.f, win.height - PH));
        p->setPosition({px, py});
        p->setVisible(false);
        this->addChild(p, 1);
        panel = p;

        panel->addChild(TouchBlocker::create({PW, PH}));

        auto label = [&](const char* text, float x, float y, float scale, bool left) -> CCLabelBMFont* {
            auto l = CCLabelBMFont::create(text, "bigFont.fnt");
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
            label(text, 45.f, y, 0.38f, true);
            return b;
        };

        auto stepper = [&](float y, std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            auto m = PMBtn::createText({26.f, 22.f}, {90, 90, 90}, "-", 0.55f, minus);
            m->setPosition({212.f, y - 11.f});
            panel->addChild(m);
            auto l = label("", 305.f, y, 0.38f, false);
            auto pl = PMBtn::createText({26.f, 22.f}, {90, 90, 90}, "+", 0.55f, plus);
            pl->setPosition({372.f, y - 11.f});
            panel->addChild(pl);
            return l;
        };

        auto button = [&](const char* text, float cx, float y, std::function<void()> cb) {
            auto b = PMBtn::createText({84.f, 22.f}, {40, 110, 200}, text, 0.34f, cb);
            b->setPosition({cx - 42.f, y - 11.f});
            panel->addChild(b);
        };

        auto title = label("Practice Menu (drag here)", PW / 2.f, 250.f, 0.5f, false);
        title->setFntFile("goldFont.fnt");

        auto drag = DragBar::create(panel, {PW, 30.f}, {PW, PH});
        drag->setPosition({0.f, PH - 30.f});
        panel->addChild(drag);

        // left column
        noclipT = toggle("Noclip", 220.f, [this]() { onNoclip(); });
        safeT = toggle("Safe mode", 192.f, [this]() { onSafe(); });
        hitboxT = toggle("Hitbox on death", 164.f, [this]() { onHitbox(); });
        unlockT = toggle("Unlock icons", 136.f, [this]() { onUnlock(); });
        if (inGame) {
            recT = toggle("Record macro", 108.f, [this]() { onRecord(); });
            replayT = toggle("Replay macro", 80.f, [this]() { onReplay(); });
            macroLabel = label("", 14.f, 52.f, 0.3f, true);
        }

        // right column
        speedLabel = stepper(220.f, [this]() { changeSpeed(-0.25f); },
                             [this]() { changeSpeed(0.25f); });
        fpsLabel = stepper(192.f, [this]() { changeFps(-1); },
                           [this]() { changeFps(1); });
        delayLabel = stepper(164.f, [this]() { changeDelay(-0.25f); },
                             [this]() { changeDelay(0.25f); });
        trailLabel = stepper(136.f, [this]() { changeTrail(-30); },
                             [this]() { changeTrail(30); });
        colorLabel = stepper(108.f, [this]() { changeColor(-1); },
                             [this]() { changeColor(1); });

        button("Cheats off", 265.f, 76.f, [this]() { onCheatsOff(); });
        button("Hide menu", 355.f, 76.f, [this]() { onHide(); });
        button("Lite mode", 265.f, 48.f, [this]() { onLite(); });
        button("CBF", 355.f, 48.f, []() { openCBF(); });
        button("Save icons", 265.f, 20.f, []() { saveIcons(); });
        button("Load icons", 355.f, 20.f, []() { loadIcons(); });

        // the round PM button
        auto pmText = CCLabelBMFont::create("PM", "bigFont.fnt");
        pmText->setScale(0.6f);
        auto circle = CircleButtonSprite::create(pmText, CircleBaseColor::Green, CircleBaseSize::Small);
        circle->setScale(0.8f);
        const float R = 20.f;
        auto pmBtn = PMButton::create(circle, R, [this]() {
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        float bx = Mod::get()->getSavedValue<float>("btn-x", win.width - 50.f);
        float by = Mod::get()->getSavedValue<float>("btn-y", win.height - 50.f);
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
        unlockT->setOn(unlockAll());
        if (recT) recT->setOn(s.recording);
        if (replayT) replayT->setOn(s.replaying);
        speedLabel->setString(fmt::format("Speed: {:.2f}x", s.speed).c_str());
        fpsLabel->setString(fmt::format("FPS: {}", FPS_STEPS[s.fpsIndex]).c_str());
        delayLabel->setString(fmt::format("Delay: {:.2f}s", s.respawnDelay).c_str());
        if (s.trail == 0) trailLabel->setString("Trail: off");
        else trailLabel->setString(fmt::format("Trail: {}", s.trail).c_str());
        colorLabel->setString(fmt::format("Color: {}", TRAIL_COLORS[s.trailColor].name).c_str());
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
        s.trail = std::clamp(s.trail + d, 0, 240);
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
