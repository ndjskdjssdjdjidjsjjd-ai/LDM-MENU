#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <iterator>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ldm {

struct MacroFrame {
    uint32_t frame = 0;
    bool down = false;
};

struct LdmState {
    // Menu
    bool menuHidden = false;
    CCPoint menuPos = { 120.f, 80.f };

    // Cheats
    bool noclip = false;
    float speed = 1.0f;
    int fpsLimit = 240;

    // Hitbox trail (max 1000 frames)
    bool hitboxTrail = true;
    static constexpr size_t MAX_HITBOX_FRAMES = 1000;
    std::deque<CCPoint> hitboxPoints;

    // Macro
    bool macroRecord = false;
    bool macroPlay = false;
    bool injecting = false;
    uint32_t currentFrame = 0;
    size_t macroIdx = 0;
    std::vector<MacroFrame> macroFrames;

    // Stats
    int deaths = 0;
    int totalClicks = 0;
    int validClicks = 0;
    int currentFps = 0;
    int frameCounter = 0;
    double lastFpsSample = 0.0;
    std::deque<double> clickTimestamps;
};

inline LdmState& state() {
    static LdmState s;
    return s;
}

inline double nowSec() {
    using clock = std::chrono::steady_clock;
    static auto start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}

inline bool isCheating() {
    auto& st = state();
    return st.noclip || st.macroPlay || (std::abs(st.speed - 1.0f) > 0.01f);
}

// Uses Geode's built-in saved values (no manual file IO needed)
inline void saveState() {
    auto* mod = Mod::get();
    auto& st = state();
    mod->setSavedValue<bool>("menuHidden", st.menuHidden);
    mod->setSavedValue<float>("menuX", st.menuPos.x);
    mod->setSavedValue<float>("menuY", st.menuPos.y);
    mod->setSavedValue<int>("fpsLimit", st.fpsLimit);
    mod->setSavedValue<bool>("hitboxTrail", st.hitboxTrail);
    mod->setSavedValue<int>("deaths", st.deaths);
    mod->setSavedValue<int>("totalClicks", st.totalClicks);
    mod->setSavedValue<int>("validClicks", st.validClicks);
}

inline void loadState() {
    auto* mod = Mod::get();
    auto& st = state();
    st.menuHidden = mod->getSavedValue<bool>("menuHidden", false);
    st.menuPos.x = mod->getSavedValue<float>("menuX", 120.f);
    st.menuPos.y = mod->getSavedValue<float>("menuY", 80.f);
    st.fpsLimit = std::clamp(mod->getSavedValue<int>("fpsLimit", 240), 30, 360);
    st.hitboxTrail = mod->getSavedValue<bool>("hitboxTrail", true);
    st.deaths = std::max(0, mod->getSavedValue<int>("deaths", 0));
    st.totalClicks = std::max(0, mod->getSavedValue<int>("totalClicks", 0));
    st.validClicks = std::max(0, mod->getSavedValue<int>("validClicks", 0));
}

inline void applyFps(int fps) {
    auto& st = state();
    st.fpsLimit = std::clamp(fps, 30, 360);
    CCApplication::sharedApplication()->setAnimationInterval(1.0 / static_cast<double>(st.fpsLimit));
    saveState();
}

inline void applySpeed(float spd) {
    auto& st = state();
    st.speed = std::clamp(spd, 0.25f, 3.0f);
    CCDirector::sharedDirector()->getScheduler()->setTimeScale(st.speed);
}

inline int computeCps() {
    double t = nowSec();
    auto& q = state().clickTimestamps;
    while (!q.empty() && (t - q.front()) > 1.0) {
        q.pop_front();
    }
    return static_cast<int>(q.size());
}

inline float computeAccuracy() {
    auto& st = state();
    if (st.totalClicks <= 0) return 100.0f;
    float acc = (static_cast<float>(st.validClicks) / static_cast<float>(st.totalClicks)) * 100.0f;
    return std::clamp(acc, 0.0f, 100.0f);
}

inline void recordClick(bool valid = true) {
    auto& st = state();
    st.clickTimestamps.push_back(nowSec());
    st.totalClicks++;
    if (valid) st.validClicks++;
}

inline void drawCheatCircle(CCDrawNode* node) {
    if (!node) return;
    node->clear();
    ccColor4F fill = isCheating()
        ? ccc4f(0.95f, 0.20f, 0.20f, 1.0f)
        : ccc4f(0.12f, 0.92f, 0.30f, 1.0f);
    ccColor4F outline = ccc4f(1.0f, 1.0f, 1.0f, 0.9f);
    // drawDot takes (pos, radius, color): draw a white dot behind for an outline
    node->drawDot(CCPointZero, 8.5f, outline);
    node->drawDot(CCPointZero, 7.0f, fill);
}

inline ButtonSprite* makeBtn(char const* text, char const* texture, float scale = 0.6f) {
    return ButtonSprite::create(text, 50, true, "goldFont.fnt", texture, 30.f, scale);
}

class LdmOverlay : public CCLayer {
public:
    CCLayerColor* m_panel = nullptr;
    CCMenu* m_showBtnMenu = nullptr;
    CCDrawNode* m_cheatDot = nullptr;
    CCLabelBMFont* m_statsLabel = nullptr;
    CCLabelBMFont* m_noclipLbl = nullptr;
    CCLabelBMFont* m_speedLbl = nullptr;
    CCLabelBMFont* m_fpsLbl = nullptr;
    CCLabelBMFont* m_macroLbl = nullptr;
    CCLabelBMFont* m_hitboxLbl = nullptr;

    bool m_dragging = false;
    CCPoint m_dragOffset = CCPointZero;

    static LdmOverlay* create() {
        auto* ret = new LdmOverlay();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true);
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        this->setTouchEnabled(true);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& st = state();

        // Cheat dot + stats label
        m_cheatDot = CCDrawNode::create();
        m_cheatDot->setPosition({ 18.f, winSize.height - 18.f });
        drawCheatCircle(m_cheatDot);
        this->addChild(m_cheatDot, 200);

        m_statsLabel = CCLabelBMFont::create("CPS:0 FPS:0 Noclip:OFF Acc:100% Deaths:0", "chatFont.fnt");
        m_statsLabel->setAnchorPoint({ 0.f, 0.5f });
        m_statsLabel->setScale(0.55f);
        m_statsLabel->setPosition({ 32.f, winSize.height - 18.f });
        this->addChild(m_statsLabel, 200);

        // Show button (visible only when the panel is hidden)
        m_showBtnMenu = CCMenu::create();
        m_showBtnMenu->setPosition({ winSize.width - 42.f, winSize.height - 24.f });
        auto* showItem = CCMenuItemSpriteExtra::create(
            makeBtn("LDM", "GJ_button_01.png", 0.5f), this, menu_selector(LdmOverlay::onToggleHide)
        );
        m_showBtnMenu->addChild(showItem);
        this->addChild(m_showBtnMenu, 210);

        // Panel
        const float w = 230.f;
        const float h = 175.f;
        m_panel = CCLayerColor::create({ 18, 18, 24, 225 }, w, h);
        m_panel->setPosition(st.menuPos);
        this->addChild(m_panel, 205);

        auto* title = CCLabelBMFont::create("LDM MENU (DRAG)", "bigFont.fnt");
        title->setScale(0.38f);
        title->setPosition({ 80.f, h - 14.f });
        m_panel->addChild(title);

        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_panel->addChild(menu);

        auto* hideBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Hide", "GJ_button_06.png", 0.4f), this, menu_selector(LdmOverlay::onToggleHide)
        );
        hideBtn->setPosition({ w - 32.f, h - 14.f });
        menu->addChild(hideBtn);

        // Row 1: Noclip / Speed
        auto* noclipBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Noclip", "GJ_button_01.png", 0.4f), this, menu_selector(LdmOverlay::onToggleNoclip)
        );
        noclipBtn->setPosition({ 40.f, h - 46.f });
        menu->addChild(noclipBtn);

        m_noclipLbl = CCLabelBMFont::create("OFF", "chatFont.fnt");
        m_noclipLbl->setScale(0.55f);
        m_noclipLbl->setPosition({ 90.f, h - 46.f });
        m_panel->addChild(m_noclipLbl);

        auto* spdBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Speed", "GJ_button_01.png", 0.4f), this, menu_selector(LdmOverlay::onCycleSpeed)
        );
        spdBtn->setPosition({ 140.f, h - 46.f });
        menu->addChild(spdBtn);

        m_speedLbl = CCLabelBMFont::create("1.00x", "chatFont.fnt");
        m_speedLbl->setScale(0.55f);
        m_speedLbl->setPosition({ 195.f, h - 46.f });
        m_panel->addChild(m_speedLbl);

        // Row 2: FPS
        auto* fpsBtn = CCMenuItemSpriteExtra::create(
            makeBtn("FPS", "GJ_button_01.png", 0.4f), this, menu_selector(LdmOverlay::onCycleFps)
        );
        fpsBtn->setPosition({ 40.f, h - 82.f });
        menu->addChild(fpsBtn);

        m_fpsLbl = CCLabelBMFont::create("240", "chatFont.fnt");
        m_fpsLbl->setScale(0.55f);
        m_fpsLbl->setPosition({ 90.f, h - 82.f });
        m_panel->addChild(m_fpsLbl);

        // Row 3: Macro / Hitbox
        auto* macroBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Macro", "GJ_button_01.png", 0.4f), this, menu_selector(LdmOverlay::onCycleMacro)
        );
        macroBtn->setPosition({ 40.f, h - 118.f });
        menu->addChild(macroBtn);

        m_macroLbl = CCLabelBMFont::create("IDLE", "chatFont.fnt");
        m_macroLbl->setScale(0.52f);
        m_macroLbl->setPosition({ 92.f, h - 118.f });
        m_panel->addChild(m_macroLbl);

        auto* hbBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Hitbox", "GJ_button_01.png", 0.4f), this, menu_selector(LdmOverlay::onToggleHitbox)
        );
        hbBtn->setPosition({ 145.f, h - 118.f });
        menu->addChild(hbBtn);

        m_hitboxLbl = CCLabelBMFont::create("0f", "chatFont.fnt");
        m_hitboxLbl->setScale(0.52f);
        m_hitboxLbl->setPosition({ 200.f, h - 118.f });
        m_panel->addChild(m_hitboxLbl);

        // Row 4: Reset stats
        auto* rstBtn = CCMenuItemSpriteExtra::create(
            makeBtn("Reset Stats", "GJ_button_06.png", 0.4f), this, menu_selector(LdmOverlay::onResetStats)
        );
        rstBtn->setPosition({ w / 2.f, 24.f });
        menu->addChild(rstBtn);

        syncVisibility();
        refreshLabels();
        this->schedule(schedule_selector(LdmOverlay::onTick), 0.0f);
        return true;
    }

    void syncVisibility() {
        auto& st = state();
        if (m_panel) m_panel->setVisible(!st.menuHidden);
        if (m_showBtnMenu) m_showBtnMenu->setVisible(st.menuHidden);
    }

    void refreshLabels() {
        auto& st = state();
        if (m_noclipLbl) m_noclipLbl->setString(st.noclip ? "ON" : "OFF");
        if (m_speedLbl) m_speedLbl->setString(fmt::format("{:.2f}x", st.speed).c_str());
        if (m_fpsLbl) m_fpsLbl->setString(fmt::format("{}", st.fpsLimit).c_str());
        if (m_macroLbl) {
            if (st.macroRecord) m_macroLbl->setString("REC");
            else if (st.macroPlay) m_macroLbl->setString("PLAY");
            else m_macroLbl->setString("IDLE");
        }
        if (m_hitboxLbl) {
            m_hitboxLbl->setString(
                st.hitboxTrail ? fmt::format("{}f", st.hitboxPoints.size()).c_str() : "OFF"
            );
        }
        drawCheatCircle(m_cheatDot);
    }

    void onToggleHide(CCObject*) {
        auto& st = state();
        st.menuHidden = !st.menuHidden;
        syncVisibility();
        saveState();
    }

    void onToggleNoclip(CCObject*) {
        auto& st = state();
        st.noclip = !st.noclip;
        refreshLabels();
    }

    void onCycleSpeed(CCObject*) {
        const float presets[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
        auto& st = state();
        float next = presets[0];
        for (size_t i = 0; i < std::size(presets); ++i) {
            if (std::abs(st.speed - presets[i]) < 0.02f) {
                next = presets[(i + 1) % std::size(presets)];
                break;
            }
        }
        applySpeed(next);
        refreshLabels();
    }

    void onCycleFps(CCObject*) {
        const int presets[] = { 60, 120, 144, 240, 360 };
        auto& st = state();
        int next = presets[0];
        for (size_t i = 0; i < std::size(presets); ++i) {
            if (st.fpsLimit == presets[i]) {
                next = presets[(i + 1) % std::size(presets)];
                break;
            }
        }
        applyFps(next);
        refreshLabels();
    }

    void onCycleMacro(CCObject*) {
        auto& st = state();
        if (!st.macroRecord && !st.macroPlay) {
            st.macroFrames.clear();
            st.currentFrame = 0;
            st.macroIdx = 0;
            st.macroRecord = true;
            Notification::create("LDM Macro: Recording (restart level to begin)", NotificationIcon::Info)->show();
        } else if (st.macroRecord) {
            st.macroRecord = false;
            st.macroPlay = !st.macroFrames.empty();
            st.currentFrame = 0;
            st.macroIdx = 0;
            Notification::create(
                st.macroPlay ? fmt::format("LDM Macro: Playing ({} inputs)", st.macroFrames.size())
                             : std::string("LDM Macro: Empty"),
                NotificationIcon::Info
            )->show();
        } else {
            st.macroPlay = false;
            st.macroRecord = false;
            Notification::create("LDM Macro: Stopped", NotificationIcon::Info)->show();
        }
        refreshLabels();
    }

    void onToggleHitbox(CCObject*) {
        auto& st = state();
        st.hitboxTrail = !st.hitboxTrail;
        if (!st.hitboxTrail) st.hitboxPoints.clear();
        refreshLabels();
        saveState();
    }

    void onResetStats(CCObject*) {
        auto& st = state();
        st.deaths = 0;
        st.totalClicks = 0;
        st.validClicks = 0;
        st.hitboxPoints.clear();
        refreshLabels();
        saveState();
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        auto& st = state();
        if (st.menuHidden || !m_panel || !m_panel->isVisible()) return false;

        CCPoint local = m_panel->convertToNodeSpace(touch->getLocation());
        auto sz = m_panel->getContentSize();
        // Drag from the header, leaving room on the right for the Hide button
        if (local.x >= 0.f && local.x <= sz.width - 64.f &&
            local.y >= sz.height - 32.f && local.y <= sz.height) {
            m_dragging = true;
            m_dragOffset = ccpSub(m_panel->getPosition(), touch->getLocation());
            return true;
        }
        return false;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging || !m_panel) return;
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto sz = m_panel->getContentSize();
        CCPoint next = ccpAdd(touch->getLocation(), m_dragOffset);
        next.x = std::clamp(next.x, 0.f, std::max(0.f, winSize.width - sz.width));
        next.y = std::clamp(next.y, 0.f, std::max(0.f, winSize.height - sz.height));
        m_panel->setPosition(next);
        state().menuPos = next;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (m_dragging) {
            m_dragging = false;
            saveState();
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
    }

    void onTick(float) {
        auto& st = state();
        st.frameCounter++;
        double now = nowSec();
        if (st.lastFpsSample == 0.0) st.lastFpsSample = now;
        if ((now - st.lastFpsSample) >= 0.25) {
            st.currentFps = static_cast<int>(st.frameCounter / (now - st.lastFpsSample));
            st.frameCounter = 0;
            st.lastFpsSample = now;
        }

        if (m_statsLabel) {
            auto line = fmt::format(
                "CPS:{} FPS:{} Noclip:{} Acc:{:.1f}% Deaths:{}",
                computeCps(), st.currentFps, st.noclip ? "ON" : "OFF",
                computeAccuracy(), st.deaths
            );
            m_statsLabel->setString(line.c_str());
        }
        drawCheatCircle(m_cheatDot);
        if (m_hitboxLbl) {
            m_hitboxLbl->setString(
                st.hitboxTrail ? fmt::format("{}f", st.hitboxPoints.size()).c_str() : "OFF"
            );
        }
    }

    ~LdmOverlay() {
        saveState();
    }
};

} // namespace ldm

class $modify(LDMMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        ldm::loadState();
        ldm::applyFps(ldm::state().fpsLimit);
        ldm::applySpeed(1.0f);

        if (auto* overlay = ldm::LdmOverlay::create()) {
            this->addChild(overlay, 999);
        }
        return true;
    }
};

class $modify(LDMPlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* m_trailNode = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if (auto* overlay = ldm::LdmOverlay::create()) {
            this->addChild(overlay, 1000);
        }

        m_fields->m_trailNode = CCDrawNode::create();
        m_objectLayer->addChild(m_fields->m_trailNode, 500);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto& st = ldm::state();
        st.currentFrame = 0;
        st.macroIdx = 0;
        st.hitboxPoints.clear();
        if (st.macroRecord) st.macroFrames.clear();
        if (m_fields->m_trailNode) m_fields->m_trailNode->clear();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& st = ldm::state();
        // Noclip: ignore hazards, but keep the anti-cheat spike working
        if (st.noclip && object != m_anticheatSpike) return;
        st.deaths++;
        PlayLayer::destroyPlayer(player, object);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& st = ldm::state();
        st.currentFrame++;

        // Macro playback
        if (st.macroPlay) {
            while (st.macroIdx < st.macroFrames.size() &&
                   st.macroFrames[st.macroIdx].frame <= st.currentFrame) {
                st.injecting = true;
                this->handleButton(st.macroFrames[st.macroIdx].down, 1, true);
                st.injecting = false;
                st.macroIdx++;
            }
        }

        // Hitbox trail
        auto* node = m_fields->m_trailNode;
        if (node) {
            node->clear();
            if (st.hitboxTrail && m_player1) {
                st.hitboxPoints.push_back(m_player1->getPosition());
                while (st.hitboxPoints.size() > ldm::LdmState::MAX_HITBOX_FRAMES) {
                    st.hitboxPoints.pop_front();
                }
                for (size_t i = 1; i < st.hitboxPoints.size(); ++i) {
                    node->drawSegment(
                        st.hitboxPoints[i - 1], st.hitboxPoints[i], 1.2f, ccc4f(0.1f, 0.9f, 1.f, 0.8f)
                    );
                }
            }
        }
    }
};

class $modify(LDMBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& st = ldm::state();

        if (!st.injecting) {
            // Block manual input while a macro is playing
            if (st.macroPlay) return;

            if (down) ldm::recordClick();
            if (st.macroRecord && button == 1 && isPlayer1) {
                st.macroFrames.push_back({ st.currentFrame, down });
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};
