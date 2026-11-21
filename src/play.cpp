#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <cmath>
#include <algorithm>
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

    // Draws overlapping hitbox frames like a dense ribbon trail (similar to reference screenshot)
    void drawRibbonHitboxFrame(CCDrawNode* node, const CCRect& rc, ccColor4F fillCol,
                               ccColor4F borderCol, float borderThick) {
        if (!node || rc.size.width <= 0.f || rc.size.height <= 0.f) return;

        CCPoint bl{rc.origin.x, rc.origin.y};
        CCPoint br{rc.origin.x + rc.size.width, rc.origin.y};
        CCPoint tr{rc.origin.x + rc.size.width, rc.origin.y + rc.size.height};
        CCPoint tl{rc.origin.x, rc.origin.y + rc.size.height};

        CCPoint v[4] = {bl, br, tr, tl};
        node->drawPolygon(v, 4, fillCol, 0.f, ccColor4F{0.f, 0.f, 0.f, 0.f});

        node->drawSegment(bl, br, borderThick, borderCol);
        node->drawSegment(br, tr, borderThick, borderCol);
        node->drawSegment(tr, tl, borderThick, borderCol);
        node->drawSegment(tl, bl, borderThick, borderCol);
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
        CCScale9Sprite* hudBg = nullptr;
        CCLabelBMFont* stats = nullptr;
        CCDrawNode* indicator = nullptr;
        CCDrawNode* deathNode = nullptr;
        std::vector<StartPosObject*> startPositions;
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

    void collectStartPositions() {
        m_fields->startPositions.clear();
        if (!m_objects) return;
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (auto sp = typeinfo_cast<StartPosObject*>(obj)) {
                m_fields->startPositions.push_back(sp);
            }
        }
        std::sort(m_fields->startPositions.begin(), m_fields->startPositions.end(),
                  [](StartPosObject* a, StartPosObject* b) {
                      return a->getPositionX() < b->getPositionX();
                  });
    }

    void applyStartPosSelection() {
        if (m_fields->startPositions.empty()) {
            collectStartPositions();
        }
        auto& s = S();
        auto& sps = m_fields->startPositions;
        if (sps.empty()) {
            s.startPosIndex = -1;
            return;
        }

        if (s.smartStartPos) {
            float px = m_player1 ? m_player1->getPositionX() : 0.f;
            int bestIdx = 0;
            for (int i = 0; i < static_cast<int>(sps.size()); ++i) {
                if (sps[i]->getPositionX() <= px + 120.f) {
                    bestIdx = i;
                }
            }
            s.startPosIndex = bestIdx;
            if (auto targetSp = sps[bestIdx]) {
                this->setStartPosObject(targetSp);
            }
            return;
        }

        if (s.startPosIndex >= 0) {
            int idx = std::min(s.startPosIndex, static_cast<int>(sps.size()) - 1);
            s.startPosIndex = idx;
            if (auto targetSp = sps[idx]) {
                this->setStartPosObject(targetSp);
            }
        } else {
            this->setStartPosObject(nullptr);
        }
    }

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        if (S().noBestOrbs) {
            return;
        }
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
    }

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

            float borderAlpha = isDeathFrame ? 0.98f : (0.28f + 0.58f * t);
            float fillAlpha = isDeathFrame ? 0.24f : (0.06f + 0.16f * t);
            float thick = isDeathFrame ? 1.25f : 0.72f;
            drawRibbonHitboxFrame(
                m_fields->deathNode,
                m_fields->deathFrames[i],
                ccColor4F{r, g, b, fillAlpha},
                ccColor4F{r, g, b, borderAlpha},
                thick
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
        int trailCount = (s.trail > 0) ? std::clamp(s.trail, 1, 90) : 0;
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
        collectStartPositions();

        auto dir = CCDirector::get();
        auto win = dir ? dir->getWinSize() : CCSize{480.f, 320.f};
        applyFps();

        if (m_objectLayer) {
            m_fields->deathNode = CCDrawNode::create();
            if (m_fields->deathNode) {
                m_objectLayer->addChild(m_fields->deathNode, 30);
            }
        }

        m_fields->hudBg = PMBtn::box({164.f, 22.f}, {12, 14, 24}, 165);
        if (m_fields->hudBg) {
            m_fields->hudBg->setAnchorPoint({0.f, 1.f});
            m_fields->hudBg->setPosition({8.f, win.height - 6.f});
            this->addChild(m_fields->hudBg, 945);
        }

        m_fields->indicator = CCDrawNode::create();
        if (m_fields->indicator) {
            m_fields->indicator->setPosition({17.f, win.height - 17.f});
            this->addChild(m_fields->indicator, 950);
        }

        m_fields->stats = CCLabelBMFont::create("", "chatFont.fnt");
        if (m_fields->stats) {
            m_fields->stats->setScale(0.54f);
            m_fields->stats->setColor({232, 238, 250});
            m_fields->stats->setAnchorPoint({0.f, 1.f});
            m_fields->stats->setPosition({28.f, win.height - 10.f});
            this->addChild(m_fields->stats, 950);
        }

        ensureMenuAttached();
        return true;
    }

    void resetLevel() {
        m_fields->delayPending = false;
        this->stopActionByTag(8801);
        applyStartPosSelection();
        PlayLayer::resetLevel();
        if (m_isPracticeMode) {
            this->togglePracticeMode(S().autoCheckpoints);
        }

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
        if (s.noDeathEffect && m_player1) {
            m_player1->m_isDead = true;
            this->delayedResetLevel();
            return;
        }
        PlayLayer::destroyPlayer(player, object);

        if (s.hitboxOnDeath) {
            m_isDebugDrawEnabled = true;
            this->updateDebugDraw();
        }
    }

    void delayedResetLevel() {
        auto& s = S();
        float d = s.instantRespawn ? 0.f : s.respawnDelay;
        if (d <= 0.01f) {
            if (s.instantRespawn) {
                this->resetLevel();
            } else {
                PlayLayer::delayedResetLevel();
            }
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
            while (static_cast<int>(m_fields->recentRects.size()) > 90) {
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

        std::vector<std::string> lines;
        lines.push_back(fmt::format("FPS {}  |  CPS {}", m_fields->fps, s.clicks.size()));
        if (noclipOn) {
            float acc = 100.f * (1.f - static_cast<float>(m_fields->hitFrames) /
                                       static_cast<float>(std::max(1, m_fields->totalFrames)));
            lines.push_back(fmt::format("Acc {:.2f}%  |  Deaths {}", acc, m_fields->deaths));
        }
        if (speedOn) lines.push_back(fmt::format("Speed {:.2f}x", s.speed));
        if (fpsOn) lines.push_back(fmt::format("FPS Cap {}", targetFps()));
        if (s.instantRespawn) lines.push_back("Instant Respawn");
        else if (s.respawnDelay > 0.01f) lines.push_back(fmt::format("Delay {:.2f}s", s.respawnDelay));
        if (s.noBestOrbs) lines.push_back("No Best Orbs");
        if (s.smartStartPos) lines.push_back("Smart StartPos");
        if (s.recording) lines.push_back("Macro: Rec");
        if (replayOn) lines.push_back("Macro: Play");
        if (s.safe) lines.push_back("Safe Mode");

        std::string text;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) text += "\n";
            text += lines[i];
        }

        if (m_fields->hudBg) {
            float cardH = 15.f + static_cast<float>(lines.size()) * 11.f;
            m_fields->hudBg->setContentSize({164.f, cardH});
            if (auto dir = CCDirector::get()) {
                auto win = dir->getWinSize();
                m_fields->hudBg->setPosition({8.f, win.height - 6.f});
            }
        }

        if (m_fields->stats) {
            m_fields->stats->setString(text.c_str());
        }

        if (m_fields->indicator) {
            m_fields->indicator->clear();
            ccColor4F fill = cheating ? ccColor4F{0.96f, 0.24f, 0.24f, 0.96f}
                                       : ccColor4F{0.22f, 0.92f, 0.40f, 0.96f};
            m_fields->indicator->drawDot({0.f, 0.f}, 5.f, fill);
            drawCircleOutline(
                m_fields->indicator,
                {0.f, 0.f},
                5.3f,
                24,
                1.15f,
                ccColor4F{0.05f, 0.06f, 0.10f, 0.96f}
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
