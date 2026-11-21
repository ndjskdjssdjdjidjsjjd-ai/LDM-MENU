#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <deque>
#include <chrono>

using namespace geode::prelude;

namespace ldm {

struct ModState {
    bool lowGraphics = false;
    bool cheatActive = false;
    int targetFps = 240;
    std::deque<double> clickTimes;
    double lastFpsUpdate = 0.0;
    int frameCounter = 0;
    int currentFps = 0;
};

inline ModState& state() {
    static ModState s;
    return s;
}

inline double nowSec() {
    using clock = std::chrono::steady_clock;
    static auto start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}

inline int clampFps(int fps) {
    return std::clamp(fps, 30, 240);
}

inline void applyFpsCap(int fps) {
    int clamped = clampFps(fps);
    state().targetFps = clamped;
    if (auto* app = CCDirector::sharedDirector()->getOpenGLView()) {
        // Cocos2d interval = 1.0 / FPS
        CCApplication::sharedApplication()->setAnimationInterval(1.0 / static_cast<double>(clamped));
    }
}

inline void applyLowGraphics(bool enabled) {
    state().lowGraphics = enabled;
    Mod::get()->setSettingValue<bool>("low-graphics", enabled);

    if (auto* gm = GameManager::sharedState()) {
        // 0060 is Low Detail Mode in GD's game variables
        gm->setGameVariable("0060", enabled);
    }
}

inline int computeCps() {
    double t = nowSec();
    auto& q = state().clickTimes;
    while (!q.empty() && (t - q.front()) > 1.0) {
        q.pop_front();
    }
    return static_cast<int>(q.size());
}

inline void recordClick() {
    state().clickTimes.push_back(nowSec());
}

inline void drawIndicatorCircle(CCDrawNode* dot, bool cheated) {
    if (!dot) return;
    dot->clear();
    ccColor4F fill = cheated
        ? ccc4f(1.0f, 0.22f, 0.22f, 1.0f)
        : ccc4f(0.10f, 0.90f, 0.25f, 1.0f);
    ccColor4F border = ccc4f(1.0f, 1.0f, 1.0f, 0.85f);
    dot->drawDot(CCPointZero, 6.0f, fill, 1.5f, border);
}

} // namespace ldm

class $modify(LDMMenuLayer, MenuLayer) {
    struct Fields {
        CCLabelBMFont* m_statusLabel = nullptr;
        CCDrawNode* m_indicator = nullptr;
    };

    bool init() {
        if (!MenuLayer::init()) return false;

        auto* mod = Mod::get();
        ldm::state().lowGraphics = mod->getSettingValue<bool>("low-graphics");
        ldm::state().targetFps = ldm::clampFps(mod->getSettingValue<int64_t>("fps-limit"));

        ldm::applyLowGraphics(ldm::state().lowGraphics);
        ldm::applyFpsCap(ldm::state().targetFps);

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Low Graphics toggle button on the right menu
        auto* rightMenu = this->getChildByID("right-side-menu");
        if (!rightMenu) {
            rightMenu = CCMenu::create();
            rightMenu->setPosition({winSize.width - 36.f, winSize.height / 2.f});
            rightMenu->setLayout(ColumnLayout::create()->setGap(8.f));
            this->addChild(rightMenu);
        }

        auto* btnSprite = ButtonSprite::create(
            ldm::state().lowGraphics ? "LDM: ON" : "LDM: OFF",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.75f
        );
        auto* ldmBtn = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(LDMMenuLayer::onToggleLowGraphics)
        );
        ldmBtn->setID("ldm-menu-low-graphics-btn"_spr);
        rightMenu->addChild(ldmBtn);
        rightMenu->updateLayout();

        // Top-left HUD: Green/Red circle indicator + FPS/CPS label
        auto* dot = CCDrawNode::create();
        dot->setPosition({18.f, winSize.height - 16.f});
        ldm::drawIndicatorCircle(dot, ldm::state().cheatActive);
        this->addChild(dot, 100);
        m_fields->m_indicator = dot;

        auto* label = CCLabelBMFont::create("FPS: 0 | CPS: 0", "chatFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});
        label->setScale(0.65f);
        label->setPosition({32.f, winSize.height - 16.f});
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        this->schedule(schedule_selector(LDMMenuLayer::onTickHud), 0.0f);
        return true;
    }

    void onToggleLowGraphics(CCObject* sender) {
        bool nextState = !ldm::state().lowGraphics;
        ldm::applyLowGraphics(nextState);

        if (auto* btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) {
            if (auto* spr = typeinfo_cast<ButtonSprite*>(btn->getNormalImage())) {
                spr->setString(nextState ? "LDM: ON" : "LDM: OFF");
            }
        }
        Notification::create(
            nextState ? "Low Graphics / LDM Enabled" : "Low Graphics / LDM Disabled",
            NotificationIcon::Info
        )->show();
    }

    void onTickHud(float dt) {
        auto& st = ldm::state();
        st.frameCounter++;
        double now = ldm::nowSec();
        if (st.lastFpsUpdate == 0.0) st.lastFpsUpdate = now;

        if ((now - st.lastFpsUpdate) >= 0.25) {
            st.currentFps = static_cast<int>(st.frameCounter / (now - st.lastFpsUpdate));
            st.frameCounter = 0;
            st.lastFpsUpdate = now;
        }

        bool showHud = Mod::get()->getSettingValue<bool>("show-hud");
        if (m_fields->m_statusLabel) {
            m_fields->m_statusLabel->setVisible(showHud);
            if (showHud) {
                std::string text = fmt::format("FPS: {} | CPS: {}", st.currentFps, ldm::computeCps());
                m_fields->m_statusLabel->setString(text.c_str());
            }
        }
        if (m_fields->m_indicator) {
            m_fields->m_indicator->setVisible(showHud);
            ldm::drawIndicatorCircle(m_fields->m_indicator, st.cheatActive);
        }
    }
};

class $modify(LDMPlayLayer, PlayLayer) {
    void ccTouchBegan(CCTouch* touch, CCEvent* event) {
        ldm::recordClick();
        PlayLayer::ccTouchBegan(touch, event);
    }
};
