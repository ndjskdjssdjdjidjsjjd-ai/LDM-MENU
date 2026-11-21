#pragma once
#include <Geode/ui/Notification.hpp>
#include <cmath>
#include <vector>
#include "widgets.hpp"
#include "extras.hpp"

class PMController : public CCNode {
public:
    struct Ember {
        float x = 0.f;
        float y = 0.f;
        float vy = 16.f;
        float drift = 0.f;
        float size = 2.f;
        float life = 0.f;
    };

    CCNode* panel = nullptr;
    CCDrawNode* fireBg = nullptr;
    std::vector<Ember> embers;

    CCNode* playerPage = nullptr;
    CCNode* levelPage = nullptr;
    CCNode* settingsPage = nullptr;

    PMBtn* tabPlayer = nullptr;
    PMBtn* tabLevel = nullptr;
    PMBtn* tabSettings = nullptr;
    int activeTab = 0;

    PMBtn* noclipT = nullptr;
    PMBtn* safeT = nullptr;
    PMBtn* hitboxShowT = nullptr;
    PMBtn* liveTrailT = nullptr;
    PMBtn* hitboxT = nullptr;
    PMBtn* instantT = nullptr;
    PMBtn* noDeathT = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    CCLabelBMFont* fpsLabel = nullptr;
    CCLabelBMFont* trailLabel = nullptr;
    CCLabelBMFont* colorLabel = nullptr;

    PMBtn* bestOrbsT = nullptr;
    PMBtn* smartSpT = nullptr;
    PMBtn* autoCpT = nullptr;
    PMBtn* recT = nullptr;
    PMBtn* replayT = nullptr;
    CCLabelBMFont* spLabel = nullptr;
    CCLabelBMFont* delayLabel = nullptr;
    CCLabelBMFont* macroLabel = nullptr;

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

    void initEmbers(float pw, float ph) {
        embers.clear();
        embers.reserve(28);
        for (int i = 0; i < 28; ++i) {
            Ember e;
            e.x = ((i * 37) % std::max(1, static_cast<int>(pw) - 12)) + 6.f;
            e.y = ((i * 19) % std::max(1, static_cast<int>(ph) - 28)) + 4.f;
            e.vy = 12.f + static_cast<float>(i % 7) * 3.5f;
            e.drift = (i % 2 == 0 ? 1.f : -1.f) * (6.f + static_cast<float>(i % 5) * 2.f);
            e.size = 1.6f + static_cast<float>(i % 3) * 0.9f;
            e.life = static_cast<float>(i) * 0.25f;
            embers.push_back(e);
        }
    }

    void updateFireBackground(float dt, float pw, float ph) {
        if (!fireBg) return;
        fireBg->clear();

        CCPoint baseV[4] = {
            {2.f, 2.f},
            {pw - 2.f, 2.f},
            {pw - 2.f, 26.f},
            {2.f, 26.f}
        };
        fireBg->drawPolygon(baseV, 4, ccColor4F{0.42f, 0.08f, 0.02f, 0.22f}, 0.f,
                            ccColor4F{0.f, 0.f, 0.f, 0.f});

        for (auto& e : embers) {
            e.life += dt;
            e.y += e.vy * dt;
            e.x += std::sin(e.life * 2.4f + e.drift) * e.drift * dt;
            if (e.y > ph - 24.f) {
                e.y = 4.f;
                e.x = std::fmod(std::abs(e.x + 43.f), std::max(12.f, pw - 12.f)) + 6.f;
            }
            float normY = std::clamp(e.y / std::max(1.f, ph - 24.f), 0.f, 1.f);
            float alpha = (1.f - normY * 0.78f) * 0.55f;
            ccColor4F col = (normY < 0.35f)
                ? ccColor4F{1.0f, 0.68f, 0.14f, alpha}
                : ccColor4F{0.96f, 0.26f, 0.06f, alpha};
            fireBg->drawDot({e.x, e.y}, e.size, col);
        }
    }

    void update(float dt) override {
        if (panel && panel->isVisible()) {
            updateFireBackground(dt, 356.f, 208.f);
        }
    }

    bool init(bool game) {
        if (!CCNode::init()) return false;
        inGame = game;
        auto dir = CCDirector::get();
        auto win = dir ? dir->getWinSize() : CCSize{480.f, 320.f};

        const float PW = 356.f;
        const float PH = 208.f;
        const char* FONT = "chatFont.fnt";

        auto p = PMBtn::box({PW, PH}, {16, 11, 14}, 250);
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
            panel->addChild(blocker, 0);
        }

        fireBg = CCDrawNode::create();
        if (fireBg) {
            initEmbers(PW, PH);
            updateFireBackground(0.016f, PW, PH);
            panel->addChild(fireBg, 1);
        }

        auto header = PMBtn::box({PW - 6.f, 17.f}, {44, 18, 22}, 255);
        if (header) {
            header->setPosition({3.f, PH - 20.f});
            panel->addChild(header, 2);
        }

        auto accentLine = PMBtn::box({PW - 6.f, 1.5f}, {255, 115, 25}, 255);
        if (accentLine) {
            accentLine->setPosition({3.f, PH - 21.f});
            panel->addChild(accentLine, 2);
        }

        auto title = CCLabelBMFont::create("MEGA CRACK", "goldFont.fnt");
        if (title) {
            title->setScale(0.32f);
            title->setPosition({PW * 0.5f, PH - 11.5f});
            panel->addChild(title, 3);
        }

        if (auto drag = DragBar::create(panel, {PW, 20.f}, {PW, PH})) {
            drag->setPosition({0.f, PH - 20.f});
            panel->addChild(drag, 4);
        }

        tabPlayer = PMBtn::createText({54.f, 12.f}, {214, 78, 28}, "Player", FONT, 0.42f,
                                      [this]() { setTab(0); });
        tabLevel = PMBtn::createText({54.f, 12.f}, {30, 18, 22}, "Level", FONT, 0.42f,
                                     [this]() { setTab(1); });
        tabSettings = PMBtn::createText({58.f, 12.f}, {30, 18, 22}, "Settings", FONT, 0.42f,
                                       [this]() { setTab(2); });

        if (tabPlayer) {
            tabPlayer->setPosition({8.f, PH - 36.f});
            panel->addChild(tabPlayer, 3);
        }
        if (tabLevel) {
            tabLevel->setPosition({66.f, PH - 36.f});
            panel->addChild(tabLevel, 3);
        }
        if (tabSettings) {
            tabSettings->setPosition({124.f, PH - 36.f});
            panel->addChild(tabSettings, 3);
        }

        auto makePage = [&]() -> CCNode* {
            auto node = CCNode::create();
            if (node) {
                node->setContentSize({PW, PH - 40.f});
                node->setPosition({0.f, 0.f});
                panel->addChild(node, 3);
            }
            return node;
        };

        playerPage = makePage();
        levelPage = makePage();
        settingsPage = makePage();

        auto addLabel = [&](CCNode* parent, const char* text, float x, float y, float scale,
                            bool left, ccColor3B col = {245, 232, 222}) -> CCLabelBMFont* {
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
            b->setPosition({x, y - 6.f});
            parent->addChild(b);
            addLabel(parent, text, x + 15.f, y, 0.45f, true);
            return b;
        };

        auto addStepper = [&](CCNode* parent, float xMinus, float xLabel, float xPlus, float y,
                              std::function<void()> minus, std::function<void()> plus) -> CCLabelBMFont* {
            if (!parent) return nullptr;
            auto m = PMBtn::createText({16.f, 12.f}, {54, 24, 26}, "-", "bigFont.fnt", 0.22f, minus);
            if (m) {
                m->setPosition({xMinus, y - 6.f});
                parent->addChild(m);
            }
            auto l = addLabel(parent, "", xLabel, y, 0.45f, false, {255, 212, 155});
            auto pl = PMBtn::createText({16.f, 12.f}, {54, 24, 26}, "+", "bigFont.fnt", 0.22f, plus);
            if (pl) {
                pl->setPosition({xPlus, y - 6.f});
                parent->addChild(pl);
            }
            return l;
        };

        auto addBtn = [&](CCNode* parent, const char* text, float x, float y, float w,
                            ccColor3B color, std::function<void()> cb) {
            if (!parent) return;
            auto b = PMBtn::createText({w, 12.f}, color, text, FONT, 0.41f, cb);
            if (b) {
                b->setPosition({x, y - 6.f});
                parent->addChild(b);
            }
        };

        noclipT = addToggle(playerPage, "No-Clip", 8.f, 150.f, [this]() { onNoclip(); });
        safeT = addToggle(playerPage, "Safe Mode", 8.f, 130.f, [this]() { onSafe(); });
        hitboxShowT = addToggle(playerPage, "Hitbox Show", 8.f, 110.f, [this]() { onHitboxShow(); });
        liveTrailT = addToggle(playerPage, "Live Trail", 8.f, 90.f, [this]() { onLiveTrail(); });
        hitboxT = addToggle(playerPage, "Death Box", 8.f, 70.f, [this]() { onHitbox(); });
        instantT = addToggle(playerPage, "Instant Retry", 8.f, 50.f, [this]() { onInstantRespawn(); });
        noDeathT = addToggle(playerPage, "No Death FX", 8.f, 30.f, [this]() { onNoDeathEffect(); });

        speedLabel = addStepper(playerPage, 174.f, 254.f, 326.f, 150.f,
                                [this]() { changeSpeed(-0.25f); },
                                [this]() { changeSpeed(0.25f); });
        fpsLabel = addStepper(playerPage, 174.f, 254.f, 326.f, 130.f,
                              [this]() { changeFps(-1); },
                              [this]() { changeFps(1); });
        trailLabel = addStepper(playerPage, 174.f, 254.f, 326.f, 110.f,
                                [this]() { changeTrail(-25); },
                                [this]() { changeTrail(25); });
        colorLabel = addStepper(playerPage, 174.f, 254.f, 326.f, 90.f,
                                [this]() { changeColor(-1); },
                                [this]() { changeColor(1); });

        bestOrbsT = addToggle(levelPage, "No Best Orbs", 8.f, 150.f, [this]() { onBestOrbs(); });
        smartSpT = addToggle(levelPage, "Smart StartPos", 8.f, 130.f, [this]() { onSmartStartPos(); });
        autoCpT = addToggle(levelPage, "Auto Checkpoints", 8.f, 110.f, [this]() { onAutoCheckpoints(); });
        if (inGame) {
            recT = addToggle(levelPage, "Record Macro", 8.f, 90.f, [this]() { onRecord(); });
            replayT = addToggle(levelPage, "Replay Macro", 8.f, 70.f, [this]() { onReplay(); });
            macroLabel = addLabel(levelPage, "", 8.f, 48.f, 0.43f, true, {218, 176, 152});
        }

        if (inGame) {
            spLabel = addStepper(levelPage, 174.f, 254.f, 326.f, 150.f,
                                 [this]() { changeStartPos(-1); },
                                 [this]() { changeStartPos(1); });
        }
        delayLabel = addStepper(levelPage, 174.f, 254.f, 326.f, 130.f,
                                [this]() { changeDelay(-0.25f); },
                                [this]() { changeDelay(0.25f); });

        hideInLevelT = addToggle(settingsPage, "Hide In Level", 8.f, 150.f, [this]() { onHideInLevel(); });
        unlockT = addToggle(settingsPage, "Unlock Icons", 8.f, 130.f, [this]() { onUnlock(); });
        fastMenuT = addToggle(settingsPage, "Fast Menu", 8.f, 110.f, [this]() { onFastMenu(); });

        addBtn(settingsPage, "CBF", 174.f, 150.f, 54.f, {138, 52, 128}, []() { openCBF(); });
        addBtn(settingsPage, "Lite Mode", 234.f, 150.f, 58.f, {42, 134, 72}, [this]() { onLite(); });

        addBtn(panel, "All Off", 8.f, 12.f, 54.f, {186, 42, 46}, [this]() { onAllCheatsOff(); });
        addBtn(panel, "Hide Menu", 68.f, 12.f, 58.f, {58, 34, 38}, [this]() { onHide(); });

        auto mcText = CCLabelBMFont::create("MC", "bigFont.fnt");
        if (mcText) mcText->setScale(0.26f);
        auto circle = CircleButtonSprite::create(mcText, CircleBaseColor::Green, CircleBaseSize::Small);
        if (circle) circle->setScale(0.44f);
        const float R = 11.5f;
        auto mcBtn = PMButton::create(circle, R, [this]() {
            if (!panel) return;
            panel->setVisible(!panel->isVisible());
            refresh();
        });
        if (mcBtn) {
            float bx = Mod::get()->getSavedValue<float>("btn-x", std::max(0.f, win.width - 30.f));
            float by = Mod::get()->getSavedValue<float>("btn-y", std::max(0.f, win.height - 30.f));
            bx = std::clamp(bx, 0.f, std::max(0.f, win.width - R * 2.f));
            by = std::clamp(by, 0.f, std::max(0.f, win.height - R * 2.f));
            mcBtn->setPosition({bx, by});
            this->addChild(mcBtn, 2);
        }

        this->scheduleUpdate();
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
        if (hitboxShowT) hitboxShowT->setOn(s.hitboxShow);
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
    void onHitboxShow() { S().hitboxShow = !S().hitboxShow; saveAll(); refresh(); }
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
        s.trail = std::clamp(s.trail + d, 0, 1000);
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
        s.hitboxShow = false;
        s.hitboxOnDeath = false;
        s.respawnDelay = 0.f;
        saveAll();
        refresh();
        pmNotify("Lite mode enabled");
    }

    void onHide() {
        Mod::get()->setSettingValue<bool>("hide-menu", true);
        this->setVisible(false);
        pmNotify("MEGA CRACK hidden. Re-enable in Geode mod settings
