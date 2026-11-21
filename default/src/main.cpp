#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ldm {

struct MacroFrame {
    uint32_t frame = 0;
    bool holding = false;
    double playerX = 0.0;
    double playerY = 0.0;
};

struct HitboxPoint {
    CCPoint center;
    float radius = 8.f;
};

struct LdmState {
    // Menu & visibility
    bool menuOpen = true;
    bool menuHidden = false;
    CCPoint menuPos = { 120.f, 160.f };

    // Cheats & physics
    bool noclip = false;
    float speed = 1.0f;
    int fpsLimit = 240;
    int tpsLimit = 240;

    // Hitbox trail (max 1000 frames)
    bool hitboxTrail = true;
    static constexpr size_t MAX_HITBOX_FRAMES = 1000;
    std::deque<HitboxPoint> hitboxPoints;

    // Bot macro (realistic frames)
    bool macroRecord = false;
    bool macroPlay = false;
    bool isHolding = false;
    uint32_t currentFrame = 0;
    std::vector<MacroFrame> macroFrames;

    // Stats & labels
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

inline std::filesystem::path saveFilePath() {
    return Mod::get()->getSaveDir() / "ldm_state.json";
}

inline void saveState() {
    auto& st = state();
    matjson::Value root = matjson::Value::object();
    root["menuOpen"] = st.menuOpen;
    root["menuHidden"] = st.menuHidden;
    root["menuX"] = st.menuPos.x;
    root["menuY"] = st.menuPos.y;
    root["noclip"] = st.noclip;
    root["speed"] = st.speed;
    root["fpsLimit"] = st.fpsLimit;
    root["tpsLimit"] = st.tpsLimit;
    root["hitboxTrail"] = st.hitboxTrail;
    root["deaths"] = st.deaths;
    root["totalClicks"] = st.totalClicks;
    root["validClicks"] = st.validClicks;

    std::error_code ec;
    std::filesystem::create_directories(Mod::get()->getSaveDir(), ec);
    std::ofstream out(saveFilePath(), std::ios::out | std::ios::trunc);
    if (out.is_open()) {
        out << root.dump(2);
    }
}

inline void loadState() {
    auto path = saveFilePath();
    if (!std::filesystem::exists(path)) return;

    std::ifstream in(path);
    if (!in.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto parsed = matjson::parse(content);
    if (!parsed.isOk()) return;

    auto root = parsed.unwrap();
    auto& st = state();

    if (root.contains("menuOpen") && root["menuOpen"].isBool())
        st.menuOpen = root["menuOpen"].asBool().unwrapOr(true);
    if (root.contains("menuHidden") && root["menuHidden"].isBool())
        st.menuHidden = root["menuHidden"].asBool().unwrapOr(false);
    if (root.contains("menuX") && root["menuX"].isNumber() && root.contains("menuY") && root["menuY"].isNumber()) {
        st.menuPos.x = static_cast<float>(root["menuX"].asDouble().unwrapOr(120.0));
        st.menuPos.y = static_cast<float>(root["menuY"].asDouble().unwrapOr(160.0));
    }
    if (root.contains("noclip") && root["noclip"].isBool())
        st.noclip = root["noclip"].asBool().unwrapOr(false);
    if (root.contains("speed") && root["speed"].isNumber())
        st.speed = std::clamp(static_cast<float>(root["speed"].asDouble().unwrapOr(1.0)), 0.25f, 3.0f);
    if (root.contains("fpsLimit") && root["fpsLimit"].isNumber())
        st.fpsLimit = std::clamp(static_cast<int>(root["fpsLimit"].asInt().unwrapOr(240)), 30, 360);
    if (root.contains("tpsLimit") && root["tpsLimit"].isNumber())
        st.tpsLimit = std::clamp(static_cast<int>(root["tpsLimit"].asInt().unwrapOr(240)), 60, 480);
    if (root.contains("hitboxTrail") && root["hitboxTrail"].isBool())
        st.hitboxTrail = root["hitboxTrail"].asBool().unwrapOr(true);
    if (root.contains("deaths") && root["deaths"].isNumber())
        st.deaths = std::max(0, static_cast<int>(root["deaths"].asInt().unwrapOr(0)));
    if (root.contains("totalClicks") && root["totalClicks"].isNumber())
        st.totalClicks = std::max(0, static_cast<int>(root["totalClicks"].asInt().unwrapOr(0)));
    if (root.contains("validClicks") && root["validClicks"].isNumber())
        st.validClicks = std::max(0, static_cast<int>(root["validClicks"].asInt().unwrapOr(0)));
}

inline void applyFps(int fps) {
    auto& st = state();
    st.fpsLimit = std::clamp(fps, 30, 360);
    CCApplication::sharedApplication()->setAnimationInterval(1.0 / static_cast<double>(st.fpsLimit));
    saveState();
}

inline void applyTps(int tps) {
    auto& st = state();
    st.tpsLimit = std::clamp(tps, 60, 480);
    if (auto* fmod = FMODAudioEngine::sharedEngine()) {
        // Keep audio pitch/time synced cleanly when speed changes
    }
    saveState();
}

inline void applySpeed(float spd) {
    auto& st = state();
    st.speed = std::clamp(spd, 0.25f, 3.0f);
    if (auto* scheduler = CCDirector::sharedDirector()->getScheduler()) {
        scheduler->setTimeScale(st.speed);
    }
    saveState();
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
    bool cheat = isCheating();
    ccColor4F fill = cheat
        ? ccc4f(0.95f, 0.20f, 0.20f, 1.0f)
        : ccc4f(0.12f, 0.92f, 0.30f, 1.0f);
    ccColor4F outline = ccc4f(1.0f, 1.0f, 1.0f, 0.9f);
    node->drawDot(CCPointZero, 7.0f, fill, 1.5f, outline);
}

// Movable LDM overlay widget shared across MenuLayer and PlayLayer
class LdmOverlay : public CCLayer {
public:
    CCLayerColor* m_panel = nullptr;
    CCMenu* m_showBtnMenu = nullptr;
    CCDrawNode* m_cheatDot = nullptr;
    CCLabelBMFont* m_statsLabel = nullptr;

    CCLabelBMFont* m_noclipLbl = nullptr;
    CCLabelBMFont* m_speedLbl = nullptr;
    CCLabelBMFont* m_fpsLbl = nullptr;
    CCLabelBMFont* m_tpsLbl = nullptr;
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

    bool init() override {
        if (!CCLayer::init()) return false;
        this->setTouchEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-500);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& st = state();

        // Cheat circle + live labels at top-left
        m_cheatDot = CCDrawNode::create();
        m_cheatDot->setPosition({ 18.f, winSize.height - 18.f });
        drawCheatCircle(m_cheatDot);
        this->addChild(m_cheatDot, 200);

        m_statsLabel = CCLabelBMFont::create("CPS:0 FPS:0 Noclip:OFF Acc:100% Deaths:0", "chatFont.fnt");
        m_statsLabel->setAnchorPoint({ 0.f, 0.5f });
        m_statsLabel->setScale(0.55f);
        m_statsLabel->setPosition({ 32.f, winSize.height - 18.f });
        this->addChild(m_statsLabel, 200);

        // Re-open button when menu is hidden
        m_showBtnMenu = CCMenu::create();
        m_showBtnMenu->setPosition({ winSize.width - 42.f, winSize.height - 24.f });
        auto* showSpr = ButtonSprite::create("LDM", "goldFont.fnt", "GJ_button_01.png", 0.65f);
        auto* showItem = CCMenuItemSpriteExtra::create(showSpr, this, menu_selector(LdmOverlay::onToggleHide));
        m_showBtnMenu->addChild(showItem);
        this->addChild(m_showBtnMenu, 210);

        // Movable LDM panel
        const float w = 230.f;
        const float h = 175.f;
        m_panel = CCLayerColor::create({ 18, 18, 24, 225 }, w, h);
        m_panel->setPosition(st.menuPos);
        this->addChild(m_panel, 205);

        auto* title = CCLabelBMFont::create("LDM MENU (DRAG)", "bigFont.fnt");
        title->setScale(0.38f);
        title->setPosition({ 75.f, h - 14.f });
        m_panel->addChild(title);

        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_panel->addChild(menu);

        // Hide button inside LDM panel
        auto* hideSpr = ButtonSprite::create("Hide", "goldFont.fnt", "GJ_button_06.png", 0.55f);
        auto* hideBtn = CCMenuItemSpriteExtra::create(hideSpr, this, menu_selector(LdmOverlay::onToggleHide));
        hideBtn->setPosition({ w - 28.f, h - 14.f });
        menu->addChild(hideBtn);

        // Row 1: Noclip & Speed
        auto* noclipSpr = ButtonSprite::create("Noclip", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* noclipBtn = CCMenuItemSpriteExtra::create(noclipSpr, this, menu_selector(LdmOverlay::onToggleNoclip));
        noclipBtn->setPosition({ 38.f, h - 42.f });
        menu->addChild(noclipBtn);

        m_noclipLbl = CCLabelBMFont::create("OFF", "chatFont.fnt");
        m_noclipLbl->setScale(0.55f);
        m_noclipLbl->setPosition({ 78.f, h - 42.f });
        m_panel->addChild(m_noclipLbl);

        auto* spdSpr = ButtonSprite::create("Spd+", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* spdBtn = CCMenuItemSpriteExtra::create(spdSpr, this, menu_selector(LdmOverlay::onCycleSpeed));
        spdBtn->setPosition({ 135.f, h - 42.f });
        menu->addChild(spdBtn);

        m_speedLbl = CCLabelBMFont::create("1.00x", "chatFont.fnt");
        m_speedLbl->setScale(0.55f);
        m_speedLbl->setPosition({ 185.f, h - 42.f });
        m_panel->addChild(m_speedLbl);

        // Row 2: FPS & TPS changers
        auto* fpsSpr = ButtonSprite::create("FPS+", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* fpsBtn = CCMenuItemSpriteExtra::create(fpsSpr, this, menu_selector(LdmOverlay::onCycleFps));
        fpsBtn->setPosition({ 38.f, h - 74.f });
        menu->addChild(fpsBtn);

        m_fpsLbl = CCLabelBMFont::create("240", "chatFont.fnt");
        m_fpsLbl->setScale(0.55f);
        m_fpsLbl->setPosition({ 78.f, h - 74.f });
        m_panel->addChild(m_fpsLbl);

        auto* tpsSpr = ButtonSprite::create("TPS+", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* tpsBtn = CCMenuItemSpriteExtra::create(tpsSpr, this, menu_selector(LdmOverlay::onCycleTps));
        tpsBtn->setPosition({ 135.f, h - 74.f });
        menu->addChild(tpsBtn);

        m_tpsLbl = CCLabelBMFont::create("240", "chatFont.fnt");
        m_tpsLbl->setScale(0.55f);
        m_tpsLbl->setPosition({ 185.f, h - 74.f });
        m_panel->addChild(m_tpsLbl);

        // Row 3: Bot Macro & Hitbox Trail (max 1000 frames)
        auto* macroSpr = ButtonSprite::create("Macro", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* macroBtn = CCMenuItemSpriteExtra::create(macroSpr, this, menu_selector(LdmOverlay::onCycleMacro));
        macroBtn->setPosition({ 42.f, h - 108.f });
        menu->addChild(macroBtn);

        m_macroLbl = CCLabelBMFont::create("IDLE", "chatFont.fnt");
        m_macroLbl->setScale(0.52f);
        m_macroLbl->setPosition({ 92.f, h - 108.f });
        m_panel->addChild(m_macroLbl);

        auto* hbSpr = ButtonSprite::create("Hitbox", "goldFont.fnt", "GJ_button_01.png", 0.52f);
        auto* hbBtn = CCMenuItemSpriteExtra::create(hbSpr, this, menu_selector(LdmOverlay::onToggleHitbox));
        hbBtn->setPosition({ 152.f, h - 108.f });
        menu->addChild(hbBtn);

        m_hitboxLbl = CCLabelBMFont::create("1000f", "chatFont.fnt");
        m_hitboxLbl->setScale(0.52f);
        m_hitboxLbl->setPosition({ 200.f, h - 108.f });
        m_panel->addChild(m_hitboxLbl);

        // Row 4: Reset Stats
        auto* rstSpr = ButtonSprite::create("Reset Stats", "goldFont.fnt", "GJ_button_06.png", 0.52f);
        auto* rstBtn = CCMenuItemSpriteExtra::create(rstSpr, this, menu_selector(LdmOverlay::onResetStats));
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
        if (m_tpsLbl) m_tpsLbl->setString(fmt::format("{}", st.tpsLimit).c_str());

        if (m_macroLbl) {
            if (st.macroRecord) m_macroLbl->setString("REC");
            else if (st.macroPlay) m_macroLbl->setString("PLAY");
            else m_macroLbl->setString("IDLE");
        }
        if (m_hitboxLbl) {
            m_hitboxLbl->setString(st.hitboxTrail ? fmt::format("{}f", st.hitboxPoints.size()).c_str() : "OFF");
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
        saveState();
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

    void onCycleTps(CCObject*) {
        const int presets[] = { 60, 120, 240, 360, 480 };
        auto& st = state();
        int next = presets[0];
        for (size_t i = 0; i < std::size(presets); ++i) {
            if (st.tpsLimit == presets[i]) {
                next = presets[(i + 1) % std::size(presets)];
                break;
            }
        }
        applyTps(next);
        refreshLabels();
    }

    void onCycleMacro(CCObject*) {
        auto& st = state();
        if (!st.macroRecord && !st.macroPlay) {
            // Start recording realistic frames
            st.macroFrames.clear();
            st.currentFrame = 0;
            st.macroRecord = true;
            st.macroPlay = false;
            Notification::create("LDM Macro: Recording frames", NotificationIcon::Info)->show();
        } else if (st.macroRecord) {
            // Switch to playback
            st.macroRecord = false;
            st.macroPlay = !st.macroFrames.empty();
            st.currentFrame = 0;
            Notification::create(
                st.macroPlay ? fmt::format("LDM Macro: Playing ({}f)", st.macroFrames.size()) : "LDM Macro: Empty",
                NotificationIcon::Info
            )->show();
        } else {
            // Stop macro
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

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        auto& st = state();
        if (!st.menuHidden && m_panel && m_panel->isVisible()) {
            CCPoint local = m_panel->convertToNodeSpace(touch->getLocation());
            auto sz = m_panel->getContentSize();
            // Drag from top header bar area
            if (local.x >= 0.f && local.x <= sz.width && local.y >= sz.height - 32.f && local.y <= sz.height) {
                m_dragging = true;
                m_dragOffset = ccpSub(m_panel->getPosition(), touch->getLocation());
                return true;
            }
        }
        return false;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (!m_dragging || !m_panel) return;
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto sz = m_panel->getContentSize();
        CCPoint next = ccpAdd(touch->getLocation(), m_dragOffset);
        next.x = std::clamp(next.x, 0.f, std::max(0.f, winSize.width - sz.width));
        next.y = std::clamp(next.y, 0.f, std::max(0.f, winSize.height - sz.height));
        m_panel->setPosition(next);
        state().menuPos = next;
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        if (m_dragging) {
            m_dragging = false;
            saveState();
        }
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
                computeCps(),
                st.currentFps,
                st.noclip ? "ON" : "OFF",
                computeAccuracy(),
                st.deaths
            );
            m_statsLabel->setString(line.c_str());
        }
        drawCheatCircle(m_cheatDot);
        if (m_hitboxLbl) {
            m_hitboxLbl->setString(st.hitboxTrail ? fmt::format("{}f", st.hitboxPoints.size()).c_str() : "OFF");
        }
    }

    ~LdmOverlay() override {
        saveState();
    }
};

} // namespace ldm

class $modify(LDMMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        ldm::loadState();
        ldm::applyFps(ldm::state().fpsLimit);
        ldm::applySpeed(ldm::state().speed);

        if (auto* overlay = ldm::LdmOverlay::create()) {
            this->addChild(overlay, 999);
        }
        return true;
    }
};

class $modify(LDMPlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* m_trailNode = nullptr;
        double m_tpsAccumulator = 0.0;
    };

    bool init(GJGameLevel* leve
