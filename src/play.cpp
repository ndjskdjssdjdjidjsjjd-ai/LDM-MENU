#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "controller.hpp"

// PM menu on the pause screen
class $modify(PMPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!menuHidden()) {
            if (auto pm = PMController::create(true)) {
                this->addChild(pm, 200);
            }
        }
    }
};

class $modify(PMBase, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& s = S();
        if (PlayLayer::get()) {
            if (s.replaying && !s.safe && !s.injecting) return;
            if (!s.injecting) {
                if (s.recording) s.macro.push_back({s.frame, down, button, isPlayer1});
                if (down && isPlayer1) s.clicks.push_back(nowSec());
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(PMPlay, PlayLayer) {
    struct Fields {
        CCLabelBMFont* stats = nullptr;
        CCDrawNode* dot = nullptr;
        CCDrawNode* trailNode = nullptr;
        std::deque<CCRect> trailRects;
        PMController* pm = nullptr;
        int totalFrames = 0;
        int hitFrames = 0;
        int deaths = 0;
        int lastHit = -10;
        int fpsCount = 0;
        double fpsStart = 0.0;
        int fps = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        auto win = CCDirector::get()->getWinSize();
        applyFps();

        if (m_objectLayer) {
            m_fields->trailNode = CCDrawNode::create();
            if (m_fields->trailNode) {
                m_objectLayer->addChild(m_fields->trailNode, 5);
            }
        }

        m_fields->dot = CCDrawNode::create();
        if (m_fields->dot) {
            m_fields->dot->setPosition({14.f, win.height - 14.f});
            this->addChild(m_fields->dot, 100);
        }

        m_fields->stats = CCLabelBMFont::create("", "bigFont.fnt");
        if (m_fields->stats) {
            m_fields->stats->setScale(0.33f);
            m_fields->stats->setOpacity(190);
            m_fields->stats->setAnchorPoint({0.f, 1.f});
            m_fields->stats->setPosition({28.f, win.height - 4.f});
            this->addChild(m_fields->stats, 100);
        }

        m_fields->pm = PMController::create(true);
        if (m_fields->pm) {
            if (menuHidden()) m_fields->pm->setVisible(false);
            this->addChild(m_fields->pm, 200);
        }
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto& s = S();
        s.frame = 0;
        s.replayIndex = 0;
        if (s.recording) s.macro.clear();
        m_fields->totalFrames = 0;
        m_fields->hitFrames = 0;
        m_fields->deaths = 0;
        m_fields->lastHit = -10;
        m_fields->trailRects.clear();
        if (m_fields->trailNode) {
            m_fields->trailNode->clear();
        }
        if (s.hitboxOnDeath) {
            m_isDebugDrawEnabled = false;
            this->updateDebugDraw();
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& s = S();
        if (s.noclip && !s.safe && object != m_anticheatSpike) {
            m_fields->hitFrames++;
            if (m_fields->totalFrames - m_fields->lastHit > 1) m_fields->deaths++;
            m_fields->lastHit = m_fields->totalFrames;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
        if (s.hitboxOnDeath) {
            m_isDebugDrawEnabled = true;
            this->updateDebugDraw();
        }
    }

    void delayedResetLevel() {
        float d = S().respawnDelay;
        if (d <= 0.f) {
            PlayLayer::delayedResetLevel();
            return;
        }
        this->runAction(CCSequence::create(
            CCDelayTime::create(d),
            CCCallFunc::create(this, callfunc_selector(PMPlay::doDelayedReset)),
            nullptr));
    }

    void doDelayedReset() {
        PlayLayer::delayedResetLevel();
    }

    // hitbox trail: copies of the player's hitbox, fading out behind you
    void updateTrail() {
        auto& s = S();
        auto& rects = m_fields->trailRects;
        auto node = m_fields->trailNode;
        if (!node || !m_player1) {
            rects.clear();
            return;
        }
        if (s.trail <= 0) {
            if (!rects.empty()) {
                rects.clear();
                node->clear();
            }
            return;
        }
        rects.push_back(m_player1->getObjectRect());
        while (static_cast<int>(rects.size()) > s.trail) rects.pop_front();

        node->clear();
        int colorIdx = std::clamp(s.trailColor, 0, std::max(0, static_cast<int>(TRAIL_COLORS.size()) - 1));
        auto c = TRAIL_COLORS[colorIdx].color;
        float r = c.r / 255.f;
        float g = c.g / 255.f;
        float b = c.b / 255.f;
        int n = static_cast<int>(rects.size());
        int step = std::max(1, n / 80);
        for (int i = 0; i < n; i += step) {
            float a = static_cast<float>(i + 1) / static_cast<float>(n);
            auto& rc = rects[i];
            CCPoint v[4] = {
                {rc.origin.x, rc.origin.y},
                {rc.origin.x + rc.size.width, rc.origin.y},
                {rc.origin.x + rc.size.width, rc.origin.y + rc.size.height},
                {rc.origin.x, rc.origin.y + rc.size.height}
            };
            node->drawPolygon(v, 4, ccColor4F{r, g, b, a * 0.2f}, 0.5f,
                              ccColor4F{r, g, b, a * 0.9f});
        }
    }

    void postUpdate(float dt) {
        auto& s = S();

        if (s.replaying && !s.safe) {
            s.injecting = true;
            while (s.replayIndex < s.macro.size() && s.macro[s.replayIndex].frame <= s.frame) {
                auto& in = s.macro[s.replayIndex++];
                this->handleButton(in.down, in.button, in.p1);
            }
            s.injecting = false;
        }

        PlayLayer::postUpdate(dt);
        s.frame++;
        m_fields->totalFrames++;
        this->updateTrail();

        if (auto sched = CCDirector::get()->getScheduler()) {
            sched->setTimeScale(s.safe ? 1.f : s.speed);
        }

        double now = nowSec();
        m_fields->fpsCount++;
        if (now - m_fields->fpsStart >= 1.0) {
            m_fields->fps = m_fields->fpsCount;
            m_fields->fpsCount = 0;
            m_fields->fpsStart = now;
        }

        while (!s.clicks.empty() && now - s.clicks.front() > 1.0) s.clicks.pop_front();

        bool noclipOn = s.noclip && !s.safe;
        bool speedOn = !s.safe && s.speed != 1.f;
        bool replayOn = s.replaying && !s.safe;
        bool fpsOn = targetFps() != 60;
        bool cheating = noclipOn || speedOn || replayOn || fpsOn;

        std::string text = fmt::format("FPS: {} | CPS: {}", m_fields->fps, s.clicks.size());
        if (noclipOn) {
            float acc = 100.f * (1.f - static_cast<float>(m_fields->hitFrames) /
                                       static_cast<float>(std::max(1, m_fields->totalFrames)));
            text += fmt::format("\nNoclip acc: {:.2f}%\nNoclip deaths: {}", acc, m_fields->deaths);
        }
        if (speedOn) text += fmt::format("\nSpeed: {:.2f}x", s.speed);
        if (fpsOn) text += fmt::format("\nFPS target: {}", targetFps());
        if (s.recording) text += "\nMACRO: recording";
        if (replayOn) text += "\nMACRO: replaying";
        if (s.safe) text += "\nSAFE MODE";
        if (m_fields->stats) {
            m_fields->stats->setString(text.c_str());
        }

        // honest indicator: red = cheats on, green = clean
        if (m_fields->dot) {
            m_fields->dot->clear();
            ccColor4F color = cheating ? ccColor4F{1.f, 0.15f, 0.15f, 1.f}
                                       : ccColor4F{0.1f, 1.f, 0.2f, 1.f};
            m_fields->dot->drawDot({0.f, 0.f}, 10.f, ccColor4F{0.f, 0.f, 0.f, 1.f});
            m_fields->dot->drawDot({0.f, 0.f}, 8.f, color);
        }
    }

    void onQuit() {
        if (auto sched = CCDirector::get()->getScheduler()) {
            sched->setTimeScale(1.f);
        }
        CCDirector::get()->setAnimationInterval(1.0 / 60.0);
        PlayLayer::onQuit();
    }
};
