#pragma once
#include <Geode/ui/Notification.hpp>
#include "widgets.hpp"
#include "extras.hpp"

class PMController : public CCNode {
public:
    CCNode* panel = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    CCLabelBMFont* fpsLabel = nullptr;
    CCLabelBMFont* delayLabel = nullptr;
    CCLabelBMFont* trailLabel = nullptr;
    CCLabelBMFont* colorLabel = nullptr;
    CCLabelBMFont* spLabel = nullptr;
    CCLabelBMFont* macroLabel = nullptr;
    PMBtn* noclipT = nullptr;
    PMBtn* safeT = nullptr;
    PMBtn* bestOrbsT = nullptr;
    PMBtn* hitboxT = nullptr;
    PMBtn* smartSpT = nullptr;
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
        auto dir = CCDirector::get();
        auto win = dir ? dir->getWinSize() : CCSize{480.f, 320.f};

        const float PW = 396.f;
        const float PH = 244.f;
        const char* FONT = "chatFont.fnt";

        auto p = PMBtn::box({PW, PH}, {18, 20, 34}, 244);
        if (!p) return false;
        float px = Mod::get()->getSavedValue<float>("panel-x", (win.width - PW) * 0.5f);
        float py = Mod::get()->getSavedValue<float>("panel-y", (win.height - PH) * 0.5f);
        px = std::clamp(px, 0.f, std::max(0.f, win.width - PW));
        py = std::clamp(py, 0.f, std::max(0.f, win.height - PH));
        p->setPosition({px, py});
        p->setVisible(false);
        this->addChild(p, 1);
        panel = p;

        if (auto blocker = TouchBlocker::create({PW, PH})) {
            panel->addChild(blocker);
        }

        auto header = PMBtn::box({PW - 6.f, 22.f}, {42, 48, 84}, 255);
        if (header) {
            header->setPosition({3.f, PH - 25.f});
            panel->addChild(header);
        }

        auto title = CCLabelBMFont::create("Practice Menu", "goldFont.fnt");
        if (title) {
            title->setScale(0.41f);
            title->setPosition({PW * 0.5f, PH - 14.f});
            panel->addChild(title);
        }

        auto label = [&](const char* text, float x, float y, float scale, bool left,
                         ccColor3B col = {235, 238, 248}) -> CCLabelBMFont* {
            auto l = CCLabelBMFont::create(text, FONT);
            if (!l) return nullptr;
            l->setScale(scale);
            l->setColor(col);
            if (left) l->setAnchorPoint({0.f, 0.5f});
            l->setPosition({x, y});
            panel->addChild(l);
            return l;
        };

        auto toggle = [&](const char* text, float y, std::function<void()> cb) -> PMBtn* {
            auto b = PMBtn::createToggle(cb);
            if (!b) return nullptr;
            b->setPosition({12.f, y - 9.f});
            panel->addChild(b);
            label(text, 36.f, y, 0.62f, true);
            return b;
        };

        auto stepper = [&](float y, std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            auto m = PMBtn::createText({22.f, 18.f}, {54, 60, 92}, "-", "bigFont.fnt", 0.36f, minus);
            if (m) {
                m->setPosition({202.f, y - 9.f});
                panel->addChild(m);
            }
            auto l = label("", 294.f, y, 0.62f, false, {220, 232, 255});
            auto pl = PMBtn::createText({22.f, 18.f}, {54, 60, 92}, "+", "bigFont.fnt", 0.36f, plus);
            if (pl) {
                pl->setPosition({364.f, y - 9.f});
                panel->addChild(pl);
            }
            return l;
        };

        auto button = [&](const char* text, float cx, float y, ccColor3B color, std::function<void()> cb) {
            auto b = PMBtn::createText({70.f, 18.f}, color, text, FONT, 0.55f, cb);
            if (b) {
                b->setPosition({cx - 35.f, y - 9.f});
                panel->addChild(b);
            }
        };

        if (auto drag = DragBar::create(panel, {PW, 25.f}, {PW, PH})) {
            drag->setPosition({0.f, PH - 25.f});
            panel->addChild(drag);
        }

        // Left column: gameplay toggles & StartPos / macro controls
        noclipT = toggle("Noclip", 198.f, [this]() { onNoclip(); });
        safeT = toggle("Safe mode", 174.f, [this]() { onSafe(); });
        bestOrbsT = toggle("No best orbs", 150.f, [this]() { onBestOrbs(); });
        hitboxT = toggle("Death hitbox", 126.f, [this]() { onHitbox(); });
        unlockT = toggle("Unlock icons", 102.f, [this]() { onUnlock(); });
        if (inGame) {
            smartSpT = toggle("Smart StartPos", 78.f, [this]() { onSmartStartPos(); });
            recT = toggle("Record macro", 54.f, [this]() { onRecord(); });
            replayT = toggle("Replay macro", 30.f, [this]() { onReplay(); });
            macroLabel = label("", 12.f, 11.f, 0.54f, true, {175, 190, 220});
        }

        // Right column: steppers & action buttons
        speedLabel = stepper(198.f, [this]() { changeSpeed(-0.25f); },
                             [this]() { changeSpeed(0.25f); });
        fpsLabel = stepper(174.f, [this]() { changeFps(-1); },
                           [this]() { changeFps(1); });
        delayLabel = stepper(150.f, [this]() { changeDelay(-0.25f); },
                             [this]() { changeDelay(0.25f); });
        trailLabel = stepper(126.f, [this]() { changeTrail(-15); },
                             [this]() { changeTrail(15); });
        colorLabel = stepper(102.f, [this]() { changeColor(-1); },
                             [this]() { changeColor(1); });
        if (inGame) {
            spLabel = stepper(78.f, [this]() { changeStartPos(-1); },
                              [this]() { changeStartPos(1); });
        }

        button("All off", 254.f, 52.f, {190, 56, 56}, [this]() { onAllCheatsOff(); });
        button("Hide menu", 338.f, 52.f, {76, 84, 114}, [this]() { onHide(); });
        button("Lite mode", 254.f, 31.f, {42, 142, 82}, [this]() { onLite(); });
        button("CBF", 338.f, 31.f, {112, 78, 178}, []() { openCBF(); });
        button("Save icons", 254.f, 10.f, {44, 106, 190}, []() { saveIcons(); });
        button("Load icons", 338.f, 10.f, {44, 106, 190}, []() { loadIcons(); });

        // Compact floating PM button
        auto pmText = CCLabelBMFont::create("PM", "bigFont.fnt");
        if (pmText) pmText->setScale(0.38f);
        auto circle = CircleButtonSprite::create(pmText, CircleBaseColor::Green, CircleBaseSize::Small);
        if (circle) circle->setScale(0.62f);
        const float R = 16.f;
        auto pmBtn = PMButton::create(circle, R, [this]() {
            if (!panel) return;
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        if (pmBtn) {
            float bx = Mod::get()->getSavedValue<float>("btn-x", std::max(0.f, win.width - 42.f));
            float by = Mod::get()->getSavedValue<float>("btn-y", std::max(0.f, win.height - 42.f));
            bx = std::clamp(bx, 0.f, std::max(0.f, win.width - R * 2.f));
            by = std::clamp(by, 0.f, std::max(0.f, win.height - R * 2.f));
            pmBtn->setPosition({bx, by});
            this->addChild(pmBtn, 2);
        }

        refresh();
        return true;
    }

    void refresh() {
        auto& s = S();
        if (noclipT) noclipT->setOn(s.noclip);
        if (safeT) safeT->setOn(s.safe);
        if (bestOrbsT) bestOrbsT->setOn(s.noBestOrbs);
        if (hitboxT) hitboxT->setOn(s.hitboxOnDeath);
        if (smartSpT) smartSpT->setOn(s.smartStartPos);
        if (unlockT) unlockT->setOn(unlockAll());
        if (recT) recT->setOn(s.recording);
        if (replayT) replayT->setOn(s.replaying);

        int fpsIdx = std::clamp(s.fpsIndex, 0, std::max(0, static_cast<int>(FPS_STEPS.size()) - 1));
        int colIdx = std::clamp(s.trailColor, 0, std::max(0, static_cast<int>(TRAIL_COLORS.size()) - 1));

        if (speedLabel) speedLabel->setString(fmt::format("Speed: {:.2f}x", s.speed).c_str());
        if (fpsLabel) fpsLabel->setString(fmt::format("FPS: {}", FPS_STEPS[fpsIdx]).c_str());
        if (delayLabel) delayLabel->setString(fmt::format("Delay: {:.2f}s", s.respawnDelay).c_str());
        if (trailLabel) {
            if (s.trail <= 0) trailLabel->setString("Trail: off");
            else trailLabel->setString(fmt::format("Trail: {}", s.trail).c_str());
        }
        if (colorLabel) colorLabel->setString(fmt::format("Color: {}", TRAIL_COLORS[colIdx].name).c_str());
        if (spLabel) {
            if (s.startPosIndex < 0) spLabel->setString("SP: Default");
            else spLabel->setString(fmt::format("SP: #{}", s.startPosIndex + 1).c_str());
        }
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

    void onBestOrbs() {
        S().noBestOrbs = !S().noBestOrbs;
        saveAll();
        refresh();
    }

    void onHitbox() {
        S().hitboxOnDeath = !S().hitboxOnDeath;
        saveAll();
        refresh();
    }

    void onSmartStartPos() {
        auto& s = S();
        s.smartStartPos = !s.smartStartPos;
        saveAll();
        if (s.smartStartPos) {
            pmNotify("Smart StartPos on: jumps to best StartPos on reset");
        } else {
            pmNotify("Smart StartPos off");
        }
        refresh();
    }

    void changeStartPos(int dir) {
        auto& s = S();
        s.smartStartPos = false;
        // Cycle from -1 (Default) through 0..19
        const int minIdx = -1;
        const int maxIdx = 19;
        int next = s.startPosIndex + dir;
        if (next > maxIdx) next = minIdx;
        if (next < minIdx) next = maxIdx;
        s.startPosIndex = next;
        saveAll();
        refresh();
        if (auto pl = PlayLayer::get()) {
            pl->resetLevel();
        }
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
        int maxIdx = std::max(0, static_cast<int>(FPS_STEPS.size()) - 1);
        s.fpsIndex = std::clamp(s.fpsIndex + d, 0, maxIdx);
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
        s.trail = std::clamp(s.trail + d, 0, 120);
        saveAll();
        refresh();
    }

    void changeColor(int d) {
        auto& s = S();
        int n = std::max(1, static_cast<int>(TRAIL_COLORS.size()));
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

    // Disables all cheats cleanly
    void onAllCheatsOff() {
        auto& s = S();
        s.noclip = false;
        s.safe = false;
        s.speed = 1.f;
        s.fpsIndex = 1; // 60 FPS
        s.recording = false;
        s.replaying = false;
        s.injecting = false;
        saveAll();
        applyFps();
        if (auto dir = CCDirector::get()) {
            if (auto sched = dir->getScheduler()) {
                sched->setTimeScale(1.f);
            }
        }
        refresh();
        pmNotify("All cheats disabled");
    }

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
