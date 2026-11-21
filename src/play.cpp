#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <cmath>
#include "controller.hpp"

namespace {
    void drawCircleOutline(CCDrawNode* node, CCPoint center, float radius, int segments,
                           float thickness, ccColor4F color) {
        if (!node || segments < 8) return;
        for (int i = 0; i < segments; ++i) {
            float a1 = (static_cast<float>(i) / segments) * 6.2831853f;
            float a2 = (static_cast<float>(i + 1) / segments) * 6.2831853f;
            CCPoint p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
            CCPoint p2{center.x + std::cos(a2) * radius, center.y + std::sin(a2) * radius};
            node->drawSegment(p1, p2, thickness, color);
        }
    }

    void drawRectFrame(CCDrawNode* node, const CCRect& rc, float borderWidth,
                       ccColor4F fillCol, ccColor4F borderCol) {
        if (!node) return;
        CCPoint v[4] = {
            {rc.origin.x, rc.origin.y},
            {rc.origin.x + rc.size.width, rc.origin.y},
            {rc.origin.x + rc.size.width, rc.origin.y + rc.size.height},
            {rc.origin.x, rc.origin.y + rc.size.height}
        };
        node->drawPolygon(v, 4, fillCol, borderWidth, borderCol);
    }
}

class $modify(PMPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!menuHidden()) {
            if (auto pm = PMController::create(true)) {
                this->addChild(pm, 999);
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
        CCDrawNode* indicator = nullptr;
        CCDrawNode* deathNode = nullptr;
        std::deque<CCRect> recentRects;
        std::vector<CCRect> deathFrames;
        PMController* pm = nullptr;
        bool delayPending = false;
        int totalFrames = 0;
        int hitFrames = 0;
        int deaths = 0;
        int lastHit = -10;
        int fpsCount = 0;
        double fpsStart = 0.0;
        int fps = 0;
    };

    void ensureMenuAttached() {
        if (menuHidden()) {
            if (m_fields->pm) m_fields->pm->setVisible(false);
            return;
        }
        if (!m_fields->pm) {
            m_fields->pm = PMController::create(true);
            if (m_fields->pm) {
                this->addChild(m_fields->pm, 999);
            }
        }
        if (m_fields->pm) {
            m_fields->pm->setVisible(true);
            m_fields->pm->setZOrder(999);
            m_fields->pm->refresh();
        }
    }

    void renderDeathHitboxAndTrail() {
        if (!m_fields->deathNode) return;
        m_fields->deathNode->clear();
        if (m_fields->deathFrames.empty()) return;

        auto& s = S();
        int colIdx = std::clamp(s.trailColor, 0, std::max(0, static_cast<int>(TRAIL_COLORS.size()) - 1));
        auto c = TRAIL_COLORS[colIdx].color;
        float r = c.r / 255.f;
        float g = c.g / 255.f;
        float b = c.b / 255.f;

        int count = static_cast<int>(m_fields->deathFrames.size());
        for (int i = 0; i < count; ++i) {
            bool isDeathFrame = (i == count - 1);
            float t = static_cast<float>(i + 1) / static_cast<float>(count);
            if (!isDeathFrame && s.trail <= 0) continue;

            float alpha = isDeathFrame ? 0.95f : (0.18f + 0.52f * t);
            float fillAlpha = isDeathFrame ? 0.24f : (0.04f + 0.12f * t);
            float thick = isDeathFrame ? 1.4f : 0.8f;
            drawRectFrame(
                m_fields->deathNode,
                m_fields->deathFrames[i],
                thick,
                ccColor4F{r, g, b, fillAlpha},
                ccColor4F{r, g, b, alpha}
            );
        }
    }

    void recordDeathSnapshot() {
        CCRect deathBox{};
        bool hasBox = false;
        if (m_player1) {
            deathBox = m_player1->getObjectRect();
            hasBox = true;
        }
        if (!hasBox && !m_fields->recentRects.empty()) {
            deathBox = m_fields->recentRects.back();
            hasBox = true;
        }
        if (!hasBox) return;

        auto& s = S();
        m_fields->deathFrames.clear();
        int trailCount = (s.trail > 0) ? std::clamp(s.trail, 1, 45) : 0;
        if (trailCount > 0 && !m_fields->recentRects.empty()) {
            int start = std::max(0, static_cast<int>(m_fields->recentRects.size()) - trailCount);
            for (int i = start; i < static_cast<int>(m_fields->recentRects.size()); ++i) {
                m_fields->deathFrames.push_back(m_fields->recentRects[i]);
            }
        }
        m_fields->deathFrames.push_back(deathBox);
        renderDeathHitboxAndTrail();
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        auto dir = CCDirector::get();
        auto win = dir ? dir->getWinSize() : CCSize{480.f, 320.f};
        applyFps();

        if (m_objectLayer) {
            m_fields->deathNode = CCDrawNode::create();
            if (m_fields->deathNode) {
                m_objectLayer->addChild(m_fields->deathNode, 25);
            }
        }

        // Circular cheat indicator on top-left HUD
        m_fields->indicator = CCDrawNode::create();
        if (m_fields->indicator) {
            m_fields->indicator->setPosition({15.f, win.height - 15.f});
            this->addChild(m_fields->indicator, 950);
        }

        // Clean HUD readout next to the circle
        m_fields->stats = CCLabelBMFont::create("", "chatFont.fnt");
        if (m_fields->stats) {
            m_fields->stats->setScale(0.68f);
            m_fields->stats->setAnchorPoint({0.f, 1.f});
            m_fields->stats->setPosition({29.f, win.height - 7.f});
            this->addChild(m_fields->stats, 950);
        }

        ensureMenuAttached();
        return true;
    }

    void resetLevel() {
        m_fields->delayPending = false;
        this->stopActionByTag(8801);
        PlayLayer::resetLevel();

        auto& s = S();
        s.frame = 0;
        s.replayIndex = 0;
        if (s.recording) s.macro.clear();
        m_fields->totalFrames = 0;
        m_fields->hitFrames = 0;
        m_fields->deaths = 0;
        m_fields->lastHit = -10;
        m_fields->recentRects.clear();
        m_fields->deathFrames.clear();
        if (m_fields->deathNode) {
            m_fields->deathNode->clear();
        }
        if (s.hitboxOnDeath) {
            m_isDebugDrawEnabled = false;
            this->updateDebugDraw();
        }
        ensureMenuAttached();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& s = S();
        if (s.noclip && !s.safe && object != m_anticheatSpike) {
            m_fields->hitFrames++;
            if (m_fields->totalFrames - m_fields->lastHit > 1) m_fields->deaths++;
            m_fields->lastHit = m_fields->totalFrames;
            return;
        }

        recordDeathSnapshot();
        PlayLayer::destroyPlayer(player, object);

        if (s.hitboxOnDeath) {
            m_isDebugDrawEnabled = true;
            this->updateDebugDraw();
        }
    }

    void delayedResetLevel() {
        float d = S().respawnDelay;
        if (d <= 0.01f) {
            PlayLayer::delayedResetLevel();
            return;
        }
        if (m_fields->delayPending) return;
        m_fields->delayPending = true;

        auto seq = CCSequence::create(
            CCDelayTime::create(d),
            CCCallFunc::create(this, callfunc_selector(PMPlay::doDelayedReset)),
            nullptr
        );
        if (seq) {
            seq->setTag(8801);
            this->runAction(seq);
        } else {
            m_fields->delayPending = false;
            PlayLayer::delayedResetLevel();
        }
    }

    void doDelayedReset() {
        m_fields->delayPending = false;
        this->resetLevel();
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

        if (m_player1) {
            m_fields->recentRects.push_back(m_player1->getObjectRect());
            while (static_cast<int>(m_fields->recentRects.size()) > 60) {
                m_fields->recentRects.pop_front();
            }
        }

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

        std::string text = fmt::format("FPS: {}  |  CPS: {}", m_fields->fps, s.clicks.size());
        if (noclipOn) {
            float acc = 100.f * (1.f - static_cast<float>(m_fields->hitFrames) /
                                       static_cast<float>(std::max(1, m_fields->totalFrames)));
            text += fmt::format("\nAcc: {:.2f}%  |  Deaths: {}", acc, m_fields->deaths);
        }
        if (speedOn) text += fmt::format("\nSpeed: {:.2f}x", s.speed);
        if (fpsOn) text += fmt::format("\nTarget FPS: {}", targetFps());
        if (s.respawnDelay > 0.01f) text += fmt::format("\nDelay: {:.2f}s", s.respawnDelay);
        if (s.recording) text += "\nMacro: Recording";
        if (replayOn) text += "\nMacro: Replaying";
        if (s.safe) text += "\nSafe Mode";

        if (m_fields->stats) {
            m_fields->stats->setString(text.c_str());
        }

        if (m_fields->indicator) {
            m_fields->indicator->clear();
            ccColor4F fill = cheating ? ccColor4F{0.96f, 0.22f, 0.22f, 0.95f}
                                       : ccColor4F{0.20f, 0.92f, 0.36f, 0.95f};
            m_fields->indicator->drawDot({0.f, 0.f}, 6.5f, fill);
            drawCircleOutline(
                m_fields->indicator,
                {0.f, 0.f},
                6.8f,
                24,
                1.4f,
                ccColor4F{0.05f, 0.05f, 0.08f, 0.95f}
            );
        }
    }

    void onQuit() {
        m_fields->delayPending = false;
        this->stopActionByTag(8801);
        if (auto sched = CCDirector::get()->getScheduler()) {
            sched->setTimeScale(1.f);
        }
        CCDirector::get()->setAnimationInterval(1.0 / 60.0);
        PlayLayer::onQuit();
    }
};
