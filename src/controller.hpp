#pragma once
#include <Geode/ui/Notification.hpp>
#include "widgets.hpp"
#include "extras.hpp"

class PMController : public CCNode {
public:
    CCNode* panel = nullptr;
    CCNode* playerPage = nullptr;
    CCNode* levelPage = nullptr;
    CCNode* settingsPage = nullptr;

    PMBtn* tabPlayer = nullptr;
    PMBtn* tabLevel = nullptr;
    PMBtn* tabSettings = nullptr;
    int activeTab = 0;

    // Player tab
    PMBtn* noclipT = nullptr;
    PMBtn* safeT = nullptr;
    PMBtn* liveTrailT = nullptr;
    PMBtn* hitboxT = nullptr;
    PMBtn* instantT = nullptr;
    PMBtn* noDeathT = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    CCLabelBMFont* fpsLabel = nullptr;
    CCLabelBMFont* trailLabel = nullptr;
    CCLabelBMFont* colorLabel = nullptr;

    // Level tab
    PMBtn* bestOrbsT = nullptr;
    PMBtn* smartSpT = nullptr;
    PMBtn* autoCpT = nullptr;
    PMBtn* recT = nullptr;
    PMBtn* replayT = nullptr;
    CCLabelBMFont* spLabel = nullptr;
    CCLabelBMFont* delayLabel = nullptr;
    CCLabelBMFont* macroLabel = nullptr;

    // Settings tab
    PMBtn* hideInLevelT = nullptr;
    PMBtn* unlockT = nullptr;
    PMBtn* fastMenuT = nullptr;

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

        const float PW = 368.f;
        const float PH = 216.f;
        const char* FONT = "chatFont.fnt";

        auto p = PMBtn::box({PW, PH}, {11, 13, 22}, 250);
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

        auto header = PMBtn::box({PW - 6.f, 18.f}, {22, 30, 60}, 255);
        if (header) {
            header->setPosition({3.f, PH - 21.f});
            panel->addChild(header);
        }

        auto accentLine = PMBtn::box({PW - 6.f, 1.5f}, {72, 232, 250}, 255);
        if (accentLine) {
            accentLine->setPosition({3.f, PH - 22.f});
            panel->addChild(accentLine);
        }

        auto title = CCLabelBMFont::create("MEGA CRACK", "goldFont.fnt");
        if (title) {
            title->setScale(0.34f);
            title->setPosition({PW * 0.5f, PH - 12.f});
            panel->addChild(title);
        }

        if (auto drag = DragBar::create(panel, {PW, 21.f}, {PW, PH})) {
            drag->setPosition({0.f, PH - 21.f});
            panel->addChild(drag);
        }

        // Compact category tabs
        tabPlayer = PMBtn::createText({60.f, 14.f}, {52, 114, 214}, "Player", FONT, 0.46f,
                                      [this]() { setTab(0); });
        tabLevel = PMBtn::createText({60.f, 14.f}, {24, 29, 50}, "Level", FONT, 0.46f,
                                     [this]() { setTab(1); });
        tabSettings = PMBtn::createText({64.f, 14.f}, {24, 29, 50}, "Settings", FONT, 0.46f,
                                       [this]() { setTab(2); });

        if (tabPlayer) {
            tabPlayer->setPosition({9.f, PH - 39.f});
            panel->addChild(tabPlayer);
        }
        if (tabLevel) {
            tabLevel->setPosition({73.f, PH - 39.f});
            panel->addChild(tabLevel);
        }
        if (tabSettings) {
            tabSettings->setPosition({137.f, PH - 39.f});
            panel->addChild(tabSettings);
        }

        auto makePage = [&]() -> CCNode* {
            auto node = CCNode::create();
            if (node) {
                node->setContentSize({PW, PH - 44.f});
                node->setPosition({0.f, 0.f});
                panel->addChild(node);
            }
            return node;
        };

        playerPage = makePage();
        levelPage = makePage();
        settingsPage = makePage();

        auto addLabel = [&](CCNode* parent, const char* text, float x, float y, float scale,
                            bool left, ccColor3B col = {222, 230, 248}) -> CCLabelBMFont* {
            if (!parent) return nullptr;
            auto l = CCLabelBMFont::create(text, FONT);
            if (!l) return nullptr;
            l->setScale(scale);
            l->setColor(col);
            if (left) l->setAnchorPoint({0.f, 0.5f});
            l->setPosition({x, y});
            parent->addChild(l);
            return l;
        };

        auto addToggle = [&](CCNode* parent, const char* text, float x, float y,
                             std::function<void()> cb) -> PMBtn* {
            if (!parent) return nullptr;
            auto b = PMBtn::createToggle(cb);
            if (!b) return nullptr;
            b->setPosition({x, y - 7.f});
            parent->addChild(b);
            addLabel(parent, text, x + 18.f, y, 0.49f, true);
            return b;
        };

        auto addStepper = [&](CCNode* parent, float xMinus, float xLabel, float xPlus, float y,
                              std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            if (!parent) return nullptr;
            auto m = PMBtn::createText({18.f, 14.f}, {34, 42, 70}, "-", "bigFont.fnt", 0.26f, minus);
            if (m) {
                m->setPosition({xMinus, y - 7.f});
                parent->addChild(m);
            }
            auto l = addLabel(parent, "", xLabel, y, 0.49f, false, {192, 222, 250});
            auto pl = PMBtn::createText({18.f, 14.f}, {34, 42, 70}, "+", "bigFont.fnt", 0.26f, plus);
            if (pl) {
                pl->setPosition({xPlus, y - 7.f});
                parent->addChild(pl);
            }
            return l;
        };

        auto addBtn = [&](CCNode* parent, const char* text, float x, float y, float w,
                            ccColor3B color, std::function<void()> cb) {
            if (!parent) return;
            auto b = PMBtn::createText({w, 14.f}, color, text, FONT, 0.45f, cb);
            if (b) {
                b->setPosition({x, y - 7.f});
                parent->addChild(b);
            }
        };

        // --- TAB 0: PLAYER ---
        noclipT = addToggle(playerPage, "No-Clip", 9.f, 152.f, [this]() { onNoclip(); });
        safeT = addToggle(playerPage, "Safe Mode", 9.f, 130.f, [this]() { onSafe(); });
        liveTrailT = addToggle(playerPage, "Live Trail", 9.f, 108.f, [this]() { onLiveTrail(); });
        hitboxT = addToggle(playerPage, "Death Box", 9.f, 86.f, [this]() { onHitbox(); });
        instantT = addToggle(playerPage, "Instant Retry", 9.f, 64.f, [this]() { onInstantRespawn(); });
        noDeathT = addToggle(playerPage, "No Death FX", 9.f, 42.f, [this]() { onNoDeathEffect(); });

        speedLabel = addStepper(playerPage, 182.f, 264.f, 336.f, 152.f,
                                [this]() { changeSpeed(-0.25f); },
                                [this]() { changeSpeed(0.25f); });
        fpsLabel = addStepper(playerPage, 182.f, 264.f, 336.f, 130.f,
                              [this]() { changeFps(-1); },
                              [this]() { changeFps(1); });
        trailLabel = addStepper(playerPage, 182.f, 264.f, 336.f, 108.f,
                                [this]() { changeTrail(-15); },
                                [this]() { changeTrail(15); });
        colorLabel = addStepper(playerPage, 182.f, 264.f, 336.f, 86.f,
                                [this]() { changeColor(-1); },
                                [this]() { changeColor(1); });

        // --- TAB 1: LEVEL ---
        bestOrbsT = addToggle(levelPage, "No Best Orbs", 9.f, 152.f, [this]() { onBestOrbs(); });
        smartSpT = addToggle(levelPage, "Smart StartPos", 9.f, 130.f, [this]() { onSmartStartPos(); });
        autoCpT = addToggle(levelPage, "Auto Checkpoints", 9.f, 108.f, [this]() { onAutoCheckpoints(); });
        if (inGame) {
            recT = addToggle(levelPage, "Record Macro", 9.f, 86.f, [this]() { onRecord(); });
            replayT = addToggle(levelPage, "Replay Macro", 9.f, 64.f, [this]() { onReplay(); });
            macroLabel = addLabel(levelPage, "", 9.f, 42.f, 0.46f, true, {154, 176, 214});
        }

        if (inGame) {
            spLabel = addStepper(levelPage, 182.f, 264.f, 336.f, 152.f,
                                 [this]() { changeStartPos(-1); },
                                 [this]() { changeStartPos(1); });
        }
        delayLabel = addStepper(levelPage, 182.f, 264.f, 336.f, 130.f,
                                [this]() { changeDelay(-0.25f); },
                                [this]() { changeDelay(0.25f); });

        // --- TAB 2: SETTINGS ---
        hideInLevelT = addToggle(settingsPage, "Hide In Level", 9.f, 152.f, [this]() { onHideInLevel(); });
        unlockT = addToggle(settingsPage, "Unlock Icons", 9.f, 130.f, [this]() { onUnlock(); });
        fastMenuT = addToggle(settingsPage, "Fast Menu", 9.f, 108.f, [this]() { onFastMenu(); });

        addBtn(settingsPage, "CBF", 182.f, 152.f, 60.f, {96, 70, 166}, []() { openCBF(); });
        addBtn(settingsPage, "Lite Mode", 248.f, 152.f, 64.f, {36, 126, 74}, [this]() { onLite(); });

        // Compact bottom bar across all tabs
        addBtn(panel, "All Off", 9.f, 14.f, 60.f, {176, 46, 50}, [this]() { onAllCheatsOff(); });
        addBtn(panel, "Hide Menu", 74.f, 14.f, 64.f, {48, 56, 84}, [this]() { onHide(); });

        // Compact floating MEGA CRACK button
        auto mcText = CCLabelBMFont::create("MC", "bigFont.fnt");
        if (mcText) mcText->setScale(0.30f);
        auto circle = CircleButtonSprite::create(mcText, CircleBaseColor::Green, CircleBaseSize::Small);
        if (circle) circle->setScale(0.50f);
        const float R = 13.f;
        auto mcBtn = PMButton::create(circle, R, [this]() {
            if (!panel) return;
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        if (mcBtn) {
            float bx = Mod::get()->getSavedValue<float>("btn-x", std::max(0.f, win.width - 34.f));
            float by = Mod::get()->getSavedValue<float>("btn-y", std::max(0.f, win.height - 34.f));
            bx = std::clamp(bx, 0.f, std::max(0.f, win.width - R * 2.f));
            by = std::clamp(by, 0.f, std::max(0.f, win.height - R * 2.f));
            mcBtn->setPosition({bx, by});
            this->addChild(mcBtn, 2);
        }

        setTab(0);
        refresh();
        return true;
    }

    void setTab(int idx) {
        activeTab = std::clamp(idx, 0, 2);
        if (playerPage) playerPage->setVisible(activeTab == 0);
        if (levelPage) levelPage->setVisible(activeTab == 1);
        if (settingsPage) settingsPage->setVisible(activeTab == 2);
        if (tabPlayer) tabPlayer->setActiveTab(activeTab == 0);
        if (tabLevel) tabLevel->setActiveTab(activeTab == 1);
        if (tabSettings) tabSettings->setActiveTab(activeTab == 2);
    }

    void refresh() {
        auto& s = S();
        if (noclipT) noclipT->setOn(s.noclip);
        if (safeT) safeT->setOn(s.safe);
        if (liveTrailT) liveTrailT->setOn(s.liveTrail);
        if (hitboxT) hitboxT->setOn(s.hitboxOnDeath);
        if (instantT) instantT->setOn(s.instantRespawn);
        if (noDeathT) noDeathT->setOn(s.noDeathEffect);
        if (bestOrbsT) bestOrbsT->setOn(s.noBestOrbs);
        if (smartSpT) smartSpT->setOn(s.smartStartPos);
        if (autoCpT) autoCpT->setOn(s.autoCheckpoints);
        if (hideInLevelT) hideInLevelT->setOn(s.hideInLevel);
        if (unlockT) unlockT->setOn(unlockAll());
        if (fastMenuT) fastMenuT->setOn(s.fastMenu);
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
        if (macroLabel) macroLabel->setString(fmt::format("Macro: {} inputs", s.macro.size()).c_str());
    }

    void onNoclip() { S().noclip = !S().noclip; saveAll(); refresh(); }
    void onSafe() { S().safe = !S().safe; saveAll(); applyFps(); refresh(); }
    void onLiveTrail() { S().liveTrail = !S().liveTrail; saveAll(); refresh(); }
    void onHitbox() { S().hitboxOnDeath = !S().hitboxOnDeath; saveAll(); refresh(); }
    void onInstantRespawn() { S().instantRespawn = !S().instantRespawn; saveAll(); refresh(); }
    void onNoDeathEffect() { S().noDeathEffect = !S().noDeathEffect; saveAll(); refresh(); }
    void onBestOrbs() { S().noBestOrbs = !S().noBestOrbs; saveAll(); refresh(); }
    void onAutoCheckpoints() { S().autoCheckpoints = !S().autoCheckpoints; saveAll(); refresh(); }
    void onFastMenu() { S().fastMenu = !S().fastMenu; saveAll(); refresh(); }

    void onHideInLevel() {
        auto& s = S();
        s.hideInLevel = !s.hideInLevel;
        saveAll();
        if (inGame) {
            if (panel) panel->setVisible(false);
            this->setVisible(!levelMenuHidden());
        }
        pmNotify(s.hideInLevel ? "MEGA CRACK hidden in levels" : "MEGA CRACK shown in levels");
        refresh();
    }

    void onSmartStartPos() {
        auto& s = S();
        s.smartStartPos = !s.smartStartPos;
        saveAll();
        pmNotify(s.smartStartPos ? "Smart StartPos enabled" : "Smart StartPos disabled");
        refresh();
    }

    void changeStartPos(int dir) {
        auto& s = S();
        s.smartStartPos = false;
        const int minIdx = -1;
        const int maxIdx = 19;
        int next = s.startPosIndex + dir;
        if (next > maxIdx) next = minIdx;
        if (next < minIdx) next = maxIdx;
        s.startPosIndex = next;
        saveAll();
        refresh();
        if (auto pl = PlayLayer::get()) pl->resetLevel();
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

    void onAllCheatsOff() {
        auto& s = S();
        s.noclip = false;
        s.safe = false;
        s.speed = 1.f;
        s.fpsIndex = 1;
        s.recording = false;
        s.replaying = false;
        s.injecting = false;
        saveAll();
        applyFps();
        if (auto dir = CCDirector::get()) {
            if (auto sched = dir->getScheduler()) sched->setTimeScale(1.f);
        }
        refresh();
        pmNotify("All cheats disabled");
    }

    void onLite() {
        auto& s = S();
        s.trail = 0;
        s.liveTrail = false;
        s.hitboxOnDeath = false;
        s.respawnDelay = 0.f;
        saveAll();
        refresh();
        pmNotify("Lite mode enabled");
    }

    void onHide() {
        Mod::get()->setSettingValue<bool>("hide-menu", true);
        this->setVisible(false);
        pmNotify("MEGA CRACK hidden. Re-enable in Geode mod settings");
    }
};
