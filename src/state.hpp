#pragma once
#include <Geode/Geode.hpp>
#include <deque>
#include <vector>
#include <chrono>
#include <functional>
#include <algorithm>

using namespace geode::prelude;

struct MacroInput { int frame; bool down; int button; bool p1; };

struct TrailColor { const char* name; ccColor3B color; };

inline const std::vector<int> FPS_STEPS = {30, 60, 90, 120, 150, 180, 210, 240};

inline const std::vector<TrailColor> TRAIL_COLORS = {
    {"Yellow", {255, 220, 0}},
    {"Red", {255, 55, 55}},
    {"Green", {50, 245, 110}},
    {"Blue", {70, 145, 255}},
    {"Pink", {255, 105, 205}},
    {"White", {245, 248, 255}}
};

struct PMState {
    bool noclip = false;
    bool safe = false;
    bool noBestOrbs = false;
    bool smartStartPos = false;
    float speed = 1.f;
    int fpsIndex = 1;
    float respawnDelay = 0.f;
    int trail = 0;
    int trailColor = 0;
    int startPosIndex = -1; // -1 = normal start (no StartPos forced)
    bool hitboxOnDeath = false;
    bool recording = false;
    bool replaying = false;
    bool injecting = false;
    int frame = 0;
    size_t replayIndex = 0;
    std::vector<MacroInput> macro;
    std::deque<double> clicks;
};

inline PMState& S() {
    static PMState s;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        auto m = Mod::get();
        s.noclip = m->getSavedValue<bool>("noclip", false);
        s.safe = m->getSavedValue<bool>("safe", false);
        s.noBestOrbs = m->getSavedValue<bool>("no-best-orbs", false);
        s.smartStartPos = m->getSavedValue<bool>("smart-startpos", false);
        s.speed = std::clamp(m->getSavedValue<float>("speed", 1.f), 0.25f, 2.f);
        s.fpsIndex = std::clamp(m->getSavedValue<int>("fps-index", 1), 0,
                                static_cast<int>(FPS_STEPS.size()) - 1);
        s.respawnDelay = std::clamp(m->getSavedValue<float>("delay", 0.f), 0.f, 2.f);
        s.trail = std::clamp(m->getSavedValue<int>("trail", 0), 0, 120);
        s.trailColor = std::clamp(m->getSavedValue<int>("trail-color", 0), 0,
                                  static_cast<int>(TRAIL_COLORS.size()) - 1);
        s.hitboxOnDeath = m->getSavedValue<bool>("hitbox-death", false);
    }
    return s;
}

inline void saveAll() {
    auto& s = S();
    auto m = Mod::get();
    m->setSavedValue<bool>("noclip", s.noclip);
    m->setSavedValue<bool>("safe", s.safe);
    m->setSavedValue<bool>("no-best-orbs", s.noBestOrbs);
    m->setSavedValue<bool>("smart-startpos", s.smartStartPos);
    m->setSavedValue<float>("speed", s.speed);
    m->setSavedValue<int>("fps-index", s.fpsIndex);
    m->setSavedValue<float>("delay", s.respawnDelay);
    m->setSavedValue<int>("trail", s.trail);
    m->setSavedValue<int>("trail-color", s.trailColor);
    m->setSavedValue<bool>("hitbox-death", s.hitboxOnDeath);
}

inline double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

inline int targetFps() {
    auto& s = S();
    return s.safe ? 60 : FPS_STEPS[s.fpsIndex];
}

inline void applyFps() {
    CCDirector::get()->setAnimationInterval(1.0 / targetFps());
}

inline bool unlockAll() {
    return Mod::get()->getSavedValue<bool>("unlock-all", true);
}

inline bool menuHidden() {
    return Mod::get()->getSettingValue<bool>("hide-menu");
}

inline bool chainVisible(CCNode* n) {
    for (; n; n = n->getParent()) {
        if (!n->isVisible()) return false;
    }
    return true;
}

inline bool pmTouchAllowed(CCNode* n) {
    if (!chainVisible(n)) return false;
    bool inPause = false;
    for (CCNode* p = n; p; p = p->getParent()) {
        if (typeinfo_cast<PauseLayer*>(p)) {
            inPause = true;
            break;
        }
    }
    bool paused = false;
    if (auto scene = CCDirector::get()->getRunningScene()) {
        if (auto kids = scene->getChildren()) {
            for (auto child : CCArrayExt<CCNode*>(kids)) {
                if (typeinfo_cast<PauseLayer*>(child)) {
                    paused = true;
                    break;
                }
            }
        }
    }
    return !paused || inPause;
}
