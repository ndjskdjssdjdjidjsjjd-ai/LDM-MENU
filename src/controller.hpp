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
        auto dir = CCDirector::get();
        auto win = dir ? dir->getWinSize() : CCSize{480.f, 320.f};

        const float PW = 412.f;
        const float PH = 256.f;
        const char* FONT = "chatFont.fnt";

        auto p = PMBtn::box({PW, PH}, {20, 22, 38}, 242);
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

        auto header = PMBtn::box({PW - 8.f, 24.f}, {52, 58, 98}, 255);
        if (header) {
            header->setPosition({4.f, PH - 28.f});
            panel->addChild(header);
        }

        auto title = CCLabelBMFont::create("Practice Menu", "goldFont.fnt");
        if (title) {
            title->setScale(0.46f);
            title->setPosition({PW * 0.5f, PH - 16.f});
            panel->addChild(title);
        }

        auto label = [&](const char* text, float x, float y, float scale, bool left) -> CCLabelBMFont* {
            auto l = CCLabelBMFont::create(text, FONT);
            if (!l) return nullptr;
            l->setScale(scale);
            if (left) l->setAnchorPoint({0.f, 0.5f});
            l->setPosition({x, y});
            panel->addChild(l);
            return l;
        };

        auto toggle = [&](const char* text, float y, std::function<void()> cb) -> PMBtn* {
            auto b = PMBtn::createToggle(cb);
            if (!b) return nullptr;
            b->setPosition({14.f, y - 10.f});
            panel->addChild(b);
            label(text, 42.f, y, 0.72f, true);
            return b;
        };

        auto stepper = [&](float y, std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            auto m = PMBtn::createText({24.f, 20.f}, {64, 70, 104}, "-", "bigFont.fnt", 0.44f, minus);
            if (m) {
                m->setPosition({208.f, y - 10.f});
                panel->addChild(m);
            }
            auto l = label("", 304.f, y, 0.7f, false);
            auto pl = PMBtn::createText({24.f, 20.f}, {64, 70, 104}, "+", "bigFont.fnt", 0.44f, plus);
            if (pl) {
                pl->setPosition({376.f, y - 10.f});
                panel->addChild(pl);
            }
            return l;
        };

        auto button = [&](const char* text, float cx, float y, ccColor3B color, std::function<void()> cb) {
            auto b = PMBtn::createText({78.f, 20.f}, color, text, FONT, 0.64f, cb);
            if (b) {
                b->setPosition({cx - 39.f, y - 10.f});
                panel->addChild(b);
            }
        };

        if (auto drag = DragBar::create(panel, {PW, 28.f}, {PW, PH})) {
            drag->setPosition({0.f, PH - 28.f});
            panel->addChild(drag);
        }

        // Left column: gameplay toggles & macro controls
        noclipT = toggle("Noclip", 202.f, [this]() { onNoclip(); });
        safeT = toggle("Safe mode", 174.f, [this]() { onSafe(); });
        hitboxT = toggle("Death hitbox", 146.f, [this]() { onHitbox(); });
        unlockT = toggle("Unlock icons", 118.f, [this]() { onUnlock(); });
        if (inGame) {
            recT = toggle("Record macro", 90.f, [this]() { onRecord(); });
            replayT = toggle("Replay macro", 62.f, [this]() { onReplay(); });
            macroLabel = label("", 14.f, 35.f, 0.62f, true);
        }

        // Right column: steppers & quick actions
        speedLabel = stepper(202.f, [this]() { changeSpeed(-0.25f); },
                             [this]() { changeSpeed(0.25f); });
        fpsLabel = stepper(174.f, [this]() { changeFps(-1); },
                           [this]() { changeFps(1); });
        delayLabel = stepper(146.f, [this]() { changeDelay(-0.25f); },
                             [this]() { changeDelay(0.25f); });
        trailLabel = stepper(118.f, [this]() { changeTrail(-15); },
                             [this]() { changeTrail(15); });
        colorLabel = stepper(90.f, [this]() { changeColor(-1); },
                             [this]() { changeColor(1); });

        button("Cheats off", 264.f, 60.f, {195, 68, 68}, [this]() { onCheatsOff(); });
        button("Hide menu", 352.f, 60.f, {85, 92, 122}, [this]() { onHide(); });
        button("Lite mode", 264.f, 34.f, {46, 155, 88}, [this]() { onLite(); });
        button("CBF", 352.f, 34.f, {125, 86, 195}, []() { openCBF(); });
        button("Save icons", 264.f, 12.f, {48, 116, 208}, []() { saveIcons(); });
        button("Load icons", 352.f, 12.f, {48, 116, 208}, []() { loadIcons(); });

        // Compact floating PM button
        auto pmText = CCLabelBMFont::create("PM", "bigFont.fnt");
        if (pmText) pmText->setScale(0.46f);
        auto circle = CircleButtonSprite::create(pmText, CircleBaseColor::Green, CircleBaseSize::Small);
        if (circle) circle->setScale(0.72f);
        const float R = 18.f;
        auto pmBtn = PMButton::create(circle, R, [this]() {
            if (!panel) return;
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        if (pmBtn) {
            float bx = Mod::get()->getSavedValue<float>("btn-x", std::max(0.f, win.width - 46.f));
            float by = Mod::get()->getSavedValue<float>("btn-y", std::max(0.f, win.height - 46.f));
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
        if (hitboxT) hitboxT->setOn(s.hitboxOnDeath);
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
