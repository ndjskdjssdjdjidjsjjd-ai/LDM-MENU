#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ldm {

struct MacroEvent {
    double timeOffset = 0.0;
    bool down = false;
};

struct State {
    bool menuVisible = true;
    CCPoint panelPos = { 90.f, 140.f };

    bool noclip = false;
    float speed = 1.0f;
    int fpsLimit = 240;

    bool recording = false;
    bool playing = false;
    double macroStartTime = 0.0;
    size_t macroIndex = 0;
    std::vector<MacroEvent> macroEvents;

    int deaths = 0;
    int totalClicks = 0;
    int validClicks = 0;

    int fps = 0;
    int frames = 0;
    double lastFpsTime = 0.0;
    std::deque<double> clickTimes;
};

inline State& state() {
    static State s;
    return s;
}

inline double now() {
    using clock = std::chrono::steady_clock;
    static auto t0 = clock::now();
    return std::chrono::duration<double>(clock::now() - t0).count();
}

inline bool isCheatActive() {
    auto& s = state();
    return s.noclip || s.playing || (std::abs(s.speed - 1.0f) > 0.01f);
}

inline void applyFps(int fps) {
    auto& s = state();
    s.fpsLimit = std::clamp(fps, 30, 360);
    CCApplication::sharedApplication()->setAnimationInterval(1.0 / static_cast<double>(s.fpsLimit));
}

inline void applySpeed(float spd) {
    auto& s = state();
    s.speed = std::clamp(spd, 0.25f, 3.0f);
    if (auto* sched = CCDirector::sharedDirector()->getScheduler()) {
        sched->setTimeScale(s.speed);
    }
}

inline int getCps() {
    double t = now();
    auto& q = state().clickTimes;
    while (!q.empty() && (t - q.front()) > 1.0) {
        q.pop_front();
    }
    return static_cast<int>(q.size());
}

inline float getAccuracy() {
    auto& s = state();
    if (s.totalClicks <= 0) return 100.0f;
    float pct = (static_cast<float>(s.validClicks) / static_cast<float>(s.totalClicks)) * 100.0f;
    return std::clamp(pct, 0.0f, 100.0f);
}

inline void recordClick(bool valid = true) {
    auto& s = state();
    double t = now();
    s.clickTimes.push_back(t);
    s.totalClicks++;
    if (valid) s.validClicks++;

    if (s.recording) {
        s.macroEvents.push_back(MacroEvent{ t - s.macroStartTime, valid });
    }
}

inline void drawIndicator(CCDrawNode* dot) {
    if (!dot) return;
    dot->clear();
    ccColor4F fill = isCheatActive()
        ? ccc4f(0.95f, 0.20f, 0.20f, 1.0f)
        : ccc4f(0.10f, 0.90f, 0.25f, 1.0f);
    ccColor4F border = ccc4f(1.0f, 1.0f, 1.0f, 0.9f);
    dot->drawDot(CCPointZero, 7.f, fill, 1.5f, border);
}

class LdmMenuV1 : public CCLayer {
public:
    CCLayerColor* m_box = nullptr;
    CCDrawNode* m_dot = nullptr;
    CCLabelBMFont* m_hudLabel = nullptr;

    CCLabelBMFont* m_fpsVal = nullptr;
    CCLabelBMFont* m_speedVal = nullptr;
    CCLabelBMFont* m_noclipVal = nullptr;
    CCLabelBMFont* m_macroVal = nullptr;

    bool m_dragging = false;
    CCPoint m_dragDelta = CCPointZero;

    static LdmMenuV1* create() {
        auto* p = new LdmMenuV1();
        if (p && p->init()) {
            p->autorelease();
            return p;
        }
        CC_SAFE_DELETE(p);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        this->setTouchEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-400);

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto& s = state();

        // Cheat indicator green circle + labels
        m_dot = CCDrawNode::create();
        m_dot->setPosition({ 18.f, win.height - 18.f });
        drawIndicator(m_dot);
        this->addChild(m_dot, 200);

        m_hudLabel = CCLabelBMFont::create("CPS:0 FPS:0 Noclip:OFF Acc:100.0% Deaths:0", "chatFont.fnt");
        m_hudLabel->setAnchorPoint({ 0.f, 0.5f });
        m_hudLabel->setScale(0.55f);
        m_hudLabel->setPosition({ 32.f, win.height - 18.f });
        this->addChild(m_hudLabel, 200);

        // Toggle button for LDM-MENU v1
        auto* topMenu = CCMenu::create();
        topMenu->setPosition({ win.width - 48.f, win.height - 22.f });
        auto* toggleSpr = ButtonSprite::create("LDM v1", "goldFont.fnt", "GJ_button_01.png", 0.6f);
        auto* toggleBtn = CCMenuItemSpriteExtra::create(toggleSpr, this, menu_selector(LdmMenuV1::onToggleMenu));
        topMenu->addChild(toggleBtn);
        this->addChild(topMenu, 210);

        // Movable panel
        const float w = 215.f;
        const float h = 145.f;
        m_box = CCLayerColor::create({ 20, 20, 26, 225 }, w, h);
        m_box->setPosition(s.panelPos);
        m_box->setVisible(s.menuVisible);
        this->addChild(m_box, 205);

        auto* title = CCLabelBMFont::create("LDM-MENU v1", "bigFont.fnt");
        title->setScale(0.38f);
        title->setPosition({ w / 2.f, h - 14.f });
        m_box->addChild(title);

        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_box->addChild(menu);

        // FPS changer
        auto* fpsSpr = ButtonSprite::create("FPS", "goldFont.fnt", "GJ_button_01.png", 0.5f);
        auto* fpsBtn = CCMenuItemSpriteExtra::create(fpsSpr, this, menu_selector(LdmMenuV1::onFps));
        fpsBtn->setPosition({ 36.f, h - 42.f });
        menu->addChild(fpsBtn);

        m_fpsVal = CCLabelBMFont::create("240", "chatFont.fnt");
        m_fpsVal->setScale(0.55f);
        m_fpsVal->setPosition({ 76.f, h - 42.f });
        m_box->addChild(m_fpsVal);

        // Speed changer
        auto* spdSpr = ButtonSprite::create("Speed", "goldFont.fnt", "GJ_button_01.png", 0.5f);
        auto* spdBtn = CCMenuItemSpriteExtra::create(spdSpr, this, menu_selector(LdmMenuV1::onSpeed));
        spdBtn->setPosition({ 132.f, h - 42.f });
        menu->addChild(spdBtn);

        m_speedVal = CCLabelBMFont::create("1.00x", "chatFont.fnt");
        m_speedVal->setScale(0.55f);
        m_speedVal->setPosition({ 182.f, h - 42.f });
        m_box->addChild(m_speedVal);

        // Noclip toggle
        auto* ncSpr = ButtonSprite::create("Noclip", "goldFont.fnt", "GJ_button_01.png", 0.5f);
        auto* ncBtn = CCMenuItemSpriteExtra::create(ncSpr, this, menu_selector(LdmMenuV1::onNoclip));
        ncBtn->setPosition({ 42.f, h - 76.f });
        menu->addChild(ncBtn);

        m_noclipVal = CCLabelBMFont::create("OFF", "chatFont.fnt");
        m_noclipVal->setScale(0.55f);
        m_noclipVal->setPosition({ 88.f, h - 76.f });
        m_box->addChild(m_noclipVal);

        // Macro recorder / player
        auto* macSpr = ButtonSprite::create("Macro", "goldFont.fnt", "GJ_button_01.png", 0.5f);
        auto* macBtn = CCMenuItemSpriteExtra::create(macSpr, this, menu_selector(LdmMenuV1::onMacro));
        macBtn->setPosition({ 138.f, h - 76.f });
        menu->addChild(macBtn);

        m_macroVal = CCLabelBMFont::create("OFF", "chatFont.fnt");
        m_macroVal->setScale(0.52f);
        m_macroVal->setPosition({ 184.f, h - 76.f });
        m_box->addChild(m_macroVal);

        // Reset stats
        auto* rstSpr = ButtonSprite::create("Reset", "goldFont.fnt", "GJ_button_06.png", 0.5f);
        auto* rstBtn = CCMenuItemSpriteExtra::create(rstSpr, this, menu_selector(LdmMenuV1::onReset));
        rstBtn->setPosition({ w / 2.f, 24.f });
        menu->addChild(rstBtn);

        refreshUi();
        this->schedule(schedule_selector(LdmMenuV1::onUpdate), 0.0f);
        return true;
    }

    void refreshUi() {
        auto& s = state();
        if (m_fpsVal) m_fpsVal->setString(fmt::format("{}", s.fpsLimit).c_str());
        if (m_speedVal) m_speedVal->setString(fmt::format("{:.2f}x", s.speed).c_str());
        if (m_noclipVal) m_noclipVal->setString(s.noclip ? "ON" : "OFF");
        if (m_macroVal) {
            if (s.recording) m_macroVal->setString("REC");
            else if (s.playing) m_macroVal->setString("PLAY");
            else m_macroVal->setString("OFF");
        }
        drawIndicator(m_dot);
    }

    void onToggleMenu(CCObject*) {
        auto& s = state();
        s.menuVisible = !s.menuVisible;
        if (m_box) m_box->setVisible(s.menuVisible);
    }

    void onFps(CCObject*) {
        const int list[] = { 60, 120, 144, 240, 360 };
        auto& s = state();
        int next = list[0];
        for (size_t i = 0; i < std::size(list); ++i) {
            if (s.fpsLimit == list[i]) {
                next = list[(i + 1) % std::size(list)];
                break;
            }
        }
        applyFps(next);
        refreshUi();
    }

    void onSpeed(CCObject*) {
        const float list[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
        auto& s = state();
        float next = list[0];
        for (size_t i = 0; i < std::size(list); ++i) {
            if (std::abs(s.speed - list[i]) < 0.02f) {
                next = list[(i + 1) % std::size(list)];
                break;
            }
        }
        applySpeed(next);
        refreshUi();
    }

    void onNoclip(CCObject*) {
        auto& s = state();
        s.noclip = !s.noclip;
        refreshUi();
    }

    void onMacro(CCObject*) {
        auto& s = state();
        if (!s.recording && !s.playing) {
            s.macroEvents.clear();
            s.macroStartTime = now();
            s.recording = true;
            s.playing = false;
            Notification::create("Macro: Recording", NotificationIcon::Info)->show();
        } else if (s.recording) {
            s.recording = false;
            s.playing = !s.macroEvents.empty();
            s.macroStartTime = now();
            s.macroIndex = 0;
            Notification::create(
                s.playing ? fmt::format("Macro: Playing ({})", s.macroEvents.size()) : "Macro: No events",
                NotificationIcon::Info
            )->show();
        } else {
            s.playing = false;
            s.recording = false;
            Notification::create("Macro: Stopped", NotificationIcon::Info)->show();
        }
        refreshUi();
    }

    void onReset(CCObject*) {
        auto& s = state();
        s.deaths = 0;
        s.totalClicks = 0;
        s.validClicks = 0;
        refreshUi();
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        auto& s = state();
        if (s.menuVisible && m_box && m_box->isVisible()) {
            CCPoint local = m_box->convertToNodeSpace(touch->getLocation());
            auto sz = m_box->getContentSize();
            if (local.x >= 0.f && local.x <= sz.width && local.y >= sz.height - 28.f && local.y <= sz.height) {
                m_dragging = true;
                m_dragDelta = ccpSub(m_box->getPosition(), touch->getLocation());
                return true;
            }
        }
        return false;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (!m_dragging || !m_box) return;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto sz = m_box->getContentSize();
        CCPoint pos = ccpAdd(touch->getLocation(), m_dragDelta);
        pos.x = std::clamp(pos.x, 0.f, std::max(0.f, win.width - sz.width));
        pos.y = std::clamp(pos.y, 0.f, std::max(0.f, win.height - sz.height));
        m_box->setPosition(pos);
        state().panelPos = pos;
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        m_dragging = false;
    }

    void onUpdate(float) {
        auto& s = state();
        s.frames++;
        double t = now();
        if (s.lastFpsTime == 0.0) s.lastFpsTime = t;
        if ((t - s.lastFpsTime) >= 0.25) {
            s.fps = static_cast<int>(s.frames / (t - s.lastFpsTime));
            s.frames = 0;
            s.lastFpsTime = t;
        }

        if (m_hudLabel) {
            auto txt = fmt::format(
                "CPS:{} FPS:{} Noclip:{} Acc:{:.1f}% Deaths:{}",
                getCps(),
                s.fps,
                s.noclip ? "ON" : "OFF",
                getAccuracy(),
                s.deaths
            );
            m_hudLabel->setString(txt.c_str());
        }
        drawIndicator(m_dot);
    }
};

} // namespace ldm

class $modify(LDMMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        ldm::applyFps(ldm::state().fpsLimit);
        ldm::applySpeed(ldm::state().speed);

        if (auto* ui = ldm::LdmMenuV1::create()) {
            this->addChild(ui, 999);
        }
        return true;
    }
};

class $modify(LDMPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        auto& s = ldm::state();
        s.macroIndex = 0;
        s.macroStartTime = ldm::now();

        if (auto* ui = ldm::LdmMenuV1::create()) {
            this->addChild(ui, 999);
        }
        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);
        auto& s = ldm::state();
        if (s.playing && !s.macroEvents.empty()) {
            double elapsed = ldm::now() - s.macroStartTime;
            while (s.macroIndex < s.macroEvents.size() && s.macroEvents[s.macroIndex].timeOffset <= elapsed) {
                bool press = s.macroEvents[s.macroIndex].down;
                if (press) this->pushButton(1, true);
                else this->releaseButton(1, true);
                s.macroIndex++;
            }
            if (s.macroIndex >= s.macroEvents.size()) {
                s.macroIndex = 0;
                s.macroStartTime = ldm::now();
            }
        }
    }

    void ccTouchBegan(CCTouch* touch, CCEvent* event) {
        ldm::recordClick(true);
        PlayLayer::ccTouchBegan(touch, event);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) {
        if (ldm::state().recording) {
            double t = ldm::now();
            ldm::state().macroEvents.push_back(ldm::MacroEvent{ t - ldm::state().macroStartTime, false });
        }
        PlayLayer::ccTouchEnded(touch, event);
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        auto& s = ldm::state();
        s.deaths++;
        if (s.noclip) return;
        PlayLayer::destroyPlayer(player, obj);
    }
};
