#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "controller.hpp"

// PM menu on the pause screen
class $modify(PMPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!menuHidden()) {
            this->addChild(PMController::create(true), 200);
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
        PMController* pm = nullptr;

        // hitbox trail
        CCNode* liveHolder = nullptr;   // trail of the current attempt
        CCNode* ghostHolder = nullptr;  // trail of the attempt you just died in
        std::deque<CCDrawNode*> chunks;
        CCDrawNode* curChunk = nullptr;
        int curCount = 0;
        bool hasPrev = false;
        CCRect prevRect;
        bool dead = false;
        float deathX = 0.f;
        bool ghostActive = false;
        float ghostX = 0.f;

        bool boxes = false;
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

        m_fields->ghostHolder = CCNode::create();
        m_objectLayer->addChild(m_fields->ghostHolder, 49);
        m_fields->liveHolder = CCNode::create();
        m_objectLayer->addChild(m_fields->liveHolder, 50);

        m_fields->dot = CCDrawNode::create();
        m_fields->dot->setPosition({14.f, win.height - 14.f});
        this->addChild(m_fields->dot, 100);

        m_fields->stats = CCLabelBMFont::create("", "bigFont.fnt");
        m_fields->stats->setScale(0.33f);
        m_fields->stats->setOpacity(190);
        m_fields->stats->setAnchorPoint({0.f, 1.f});
        m_fields->stats->setPosition({28.f, win.height - 4.f});
        this->addChild(m_fields->stats, 100);

        m_fields->pm = PMController::create(true);
        if (menuHidden()) m_fields->pm->setVisible(false);
        this->addChild(m_fields->pm, 200);
        return true;
    }

    void clearLive() {
        for (auto node : m_fields->chunks) node->removeFromParent();
        m_fields->chunks.clear();
        m_fields->curChunk = nullptr;
        m_fields->curCount = 0;
        m_fields->hasPrev = false;
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

        // resetLevel can run before init has finished, so check first
        if (m_fields->liveHolder && m_fields->ghostHolder) {
            if (m_fields->dead && !m_fields->chunks.empty()) {
                // keep the trail of the attempt you died in until you pass that spot
                m_fields->ghostHolder->removeAllChildrenWithCleanup(true);
                for (auto node : m_fields->chunks) {
                    node->retain();
                    node->removeFromParentAndCleanup(false);
                    m_fields->ghostHolder->addChild(node);
                    node->release();
                }
                m_fields->chunks.clear();
                m_fields->curChunk = nullptr;
                m_fields->curCount = 0;
                m_fields->ghostX = m_fields->deathX;
                m_fields->ghostActive = true;
            } else {
                this->clearLive();
                m_fields->ghostHolder->removeAllChildrenWithCleanup(true);
                m_fields->ghostActive = false;
            }
        }
        m_fields->dead = false;
        m_fields->hasPrev = false;

        if (s.hitboxOnDeath || m_fields->boxes) {
            m_isDebugDrawEnabled = m_fields->boxes;
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
        if (!m_fields->dead) {
            m_fields->dead = true;
            m_fields->deathX = player->getPositionX();
        }
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

    // adds one hitbox outline to the trail
    void addTrailBox(const CCRect& rc, float r, float g, float b) {
        if (!m_fields->curChunk || m_fields->curCount >= 60) {
            auto node = CCDrawNode::create();
            m_fields->liveHolder->addChild(node);
            m_fields->chunks.push_back(node);
            m_fields->curChunk = node;
            m_fields->curCount = 0;
            int maxChunks = std::max(1, (S().trail * 4 + 59) / 60);
            while (static_cast<int>(m_fields->chunks.size()) > maxChunks) {
                m_fields->chunks.front()->removeFromParent();
                m_fields->chunks.pop_front();
            }
        }
        CCPoint v[4] = {
            {rc.origin.x, rc.origin.y},
            {rc.origin.x + rc.size.width, rc.origin.y},
            {rc.origin.x + rc.size.width, rc.origin.y + rc.size.height},
            {rc.origin.x, rc.origin.y + rc.size.height}
        };
        // thin bright outline + very faint fill: many of these merge into a ribbon
        m_fields->curChunk->drawPolygon(v, 4, ccColor4F{r, g, b, 0.07f}, 0.5f,
                                        ccColor4F{r, g, b, 0.85f});
        m_fields->curCount++;
    }

    // hitbox trail. Only draws when BOTH "Show hitboxes" and "Hitbox trail" are on.
    void updateTrail() {
        auto& s = S();
        if (!m_fields->liveHolder || !m_fields->ghostHolder) return;

        bool active = trailOn() && showHitboxes() && s.trail > 0;
        if (!active) {
            if (!m_fields->chunks.empty()) this->clearLive();
            if (m_fields->ghostActive) {
                m_fields->ghostHolder->removeAllChildrenWithCleanup(true);
                m_fields->ghostActive = false;
            }
            return;
        }

        // the old trail goes away once you pass the spot where you died
        if (m_fields->ghostActive && m_player1->getPositionX() > m_fields->ghostX) {
            m_fields->ghostHolder->removeAllChildrenWithCleanup(true);
            m_fields->ghostActive = false;
        }

        // frozen while dead, so you can see where the attempt ended
        if (m_fields->dead) return;

        auto c = TRAIL_COLORS[s.trailColor].color;
        float r = c.r / 255.f;
        float g = c.g / 255.f;
        float b = c.b / 255.f;

        CCRect now = m_player1->getObjectRect();
        int fpsNow = m_fields->fps > 0 ? m_fields->fps : 60;
        int steps = std::clamp(240 / fpsNow, 1, 4);

        bool jump = false;
        if (m_fields->hasPrev) {
            float dx = now.origin.x - m_fields->prevRect.origin.x;
            float dy = now.origin.y - m_fields->prevRect.origin.y;
            jump = (dx * dx + dy * dy) > 10000.f; // portal or teleport
        }

        if (m_fields->hasPrev && !jump) {
            auto& p = m_fields->prevRect;
            for (int k = 1; k <= steps; ++k) {
                float t = static_cast<float>(k) / static_cast<float>(steps);
                CCRect rc(
                    p.origin.x + (now.origin.x - p.origin.x) * t,
                    p.origin.y + (now.origin.y - p.origin.y) * t,
                    p.size.width + (now.size.width - p.size.width) * t,
                    p.size.height + (now.size.height - p.size.height) * t);
                this->addTrailBox(rc, r, g, b);
            }
        } else {
            this->addTrailBox(now, r, g, b);
        }
        m_fields->prevRect = now;
        m_fields->hasPrev = true;
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

        // always-on hitbox view, applied when you flip the toggle
        bool wantBoxes = showHitboxes();
        if (wantBoxes != m_fields->boxes) {
            m_fields->boxes = wantBoxes;
            m_isDebugDrawEnabled = wantBoxes;
            this->updateDebugDraw();
        }

        CCDirector::get()->getScheduler()->setTimeScale(s.safe ? 1.f : s.speed);

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
        m_fields->stats->setString(text.c_str());
        m_fields->stats->setVisible(showStats());

        // honest indicator: red = cheats on, green = clean (always visible)
        m_fields->dot->clear();
        ccColor4F color = cheating ? ccColor4F{1.f, 0.15f, 0.15f, 1.f}
                                   : ccColor4F{0.1f, 1.f, 0.2f, 1.f};
        m_fields->dot->drawDot({0.f, 0.f}, 10.f, ccColor4F{0.f, 0.f, 0.f, 1.f});
        m_fields->dot->drawDot({0.f, 0.f}, 8.f, color);
    }

    void onQuit() {
        CCDirector::get()->getScheduler()->setTimeScale(1.f);
        CCDirector::get()->setAnimationInterval(1.0 / 60.0);
        PlayLayer::onQuit();
    }
};
