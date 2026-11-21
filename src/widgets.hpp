#pragma once
#include "state.hpp"

// swallows touches inside the panel so taps don't reach the game
class TouchBlocker : public CCLayer {
public:
    CCSize area;

    static TouchBlocker* create(CCSize area) {
        auto r = new TouchBlocker();
        if (r && r->init()) {
            r->area = area;
            r->setContentSize(area);
            r->setTouchEnabled(true);
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -600, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!pmTouchAllowed(this)) return false;
        auto l = this->convertToNodeSpace(t->getLocation());
        return l.x >= 0 && l.y >= 0 && l.x <= area.width && l.y <= area.height;
    }

    void ccTouchMoved(CCTouch*, CCEvent*) override {}
    void ccTouchEnded(CCTouch*, CCEvent*) override {}
    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
};

// drag the panel by its title bar
class DragBar : public CCLayer {
public:
    CCNode* target = nullptr;
    CCSize barSize;
    CCSize panelSize;
    CCPoint lastPos;

    static DragBar* create(CCNode* target, CCSize barSize, CCSize panelSize) {
        auto r = new DragBar();
        if (r && r->init()) {
            r->target = target;
            r->barSize = barSize;
            r->panelSize = panelSize;
            r->setContentSize(barSize);
            r->setTouchEnabled(true);
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -601, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!pmTouchAllowed(this)) return false;
        auto local = this->convertToNodeSpace(t->getLocation());
        if (local.x < 0 || local.y < 0 || local.x > barSize.width || local.y > barSize.height)
            return false;
        lastPos = t->getLocation();
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = t->getLocation();
        auto pos = target->getPosition();
        pos.x += p.x - lastPos.x;
        pos.y += p.y - lastPos.y;
        lastPos = p;
        auto win = CCDirector::get()->getWinSize();
        pos.x = std::clamp(pos.x, 0.f, win.width - panelSize.width);
        pos.y = std::clamp(pos.y, 0.f, win.height - panelSize.height);
        target->setPosition(pos);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        auto pos = target->getPosition();
        Mod::get()->setSavedValue<float>("panel-x", pos.x);
        Mod::get()->setSavedValue<float>("panel-y", pos.y);
    }

    void ccTouchCancelled(CCTouch* t, CCEvent* e) override {
        ccTouchEnded(t, e);
    }
};

// simple rounded button / checkbox. No scale animation, so it never grows.
class PMBtn : public CCLayer {
public:
    std::function<void()> cb;
    CCSize size;
    CCScale9Sprite* body = nullptr;
    CCScale9Sprite* mark = nullptr;

    // a rounded box, used for the panel and all buttons
    static CCScale9Sprite* box(CCSize size, ccColor3B color, GLubyte opacity) {
        auto s = CCScale9Sprite::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
        s->setContentSize(size);
        s->setAnchorPoint({0.f, 0.f});
        s->setColor(color);
        s->setOpacity(opacity);
        return s;
    }

    static PMBtn* create(CCSize size, ccColor3B color, std::function<void()> cb) {
        auto r = new PMBtn();
        if (r && r->init()) {
            r->size = size;
            r->cb = cb;
            r->setContentSize(size);
            r->body = box(size, color, 235);
            r->addChild(r->body);
            r->setTouchEnabled(true);
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }

    static PMBtn* createText(CCSize size, ccColor3B color, const char* text,
                             const char* font, float scale, std::function<void()> cb) {
        auto r = create(size, color, cb);
        if (r) {
            auto l = CCLabelBMFont::create(text, font);
            l->setScale(scale);
            l->setPosition({size.width / 2.f, size.height / 2.f});
            r->addChild(l);
        }
        return r;
    }

    static PMBtn* createToggle(std::function<void()> cb) {
        auto r = create(CCSize{22.f, 22.f}, ccColor3B{55, 60, 90}, cb);
        if (r) {
            r->mark = box(CCSize{14.f, 14.f}, ccColor3B{70, 230, 110}, 255);
            r->mark->setPosition({4.f, 4.f});
            r->mark->setVisible(false);
            r->addChild(r->mark);
        }
        return r;
    }

    void setOn(bool on) {
        if (mark) mark->setVisible(on);
    }

    bool inside(CCPoint l) {
        return l.x >= 0 && l.y >= 0 && l.x <= size.width && l.y <= size.height;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -602, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!pmTouchAllowed(this)) return false;
        if (!inside(this->convertToNodeSpace(t->getLocation()))) return false;
        body->setOpacity(150);
        return true;
    }

    void ccTouchMoved(CCTouch*, CCEvent*) override {}

    void ccTouchEnded(CCTouch* t, CCEvent*) override {
        body->setOpacity(235);
        if (inside(this->convertToNodeSpace(t->getLocation())) && cb) cb();
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        body->setOpacity(235);
    }
};

// the round PM button: tap to open, drag to move
class PMButton : public CCLayer {
public:
    std::function<void()> onTap;
    float radius = 20.f;
    CCPoint lastPos;
    float moved = 0.f;

    static PMButton* create(CCNode* icon, float radius, std::function<void()> cb) {
        auto r = new PMButton();
        if (r && r->init()) {
            r->radius = radius;
            r->onTap = cb;
            r->setContentSize({radius * 2, radius * 2});
            icon->setPosition({radius, radius});
            r->addChild(icon);
            r->setTouchEnabled(true);
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -603, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!pmTouchAllowed(this)) return false;
        auto local = this->convertToNodeSpace(t->getLocation());
        float dx = local.x - radius;
        float dy = local.y - radius;
        if (dx * dx + dy * dy > radius * radius) return false;
        lastPos = t->getLocation();
        moved = 0.f;
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = t->getLocation();
        float dx = p.x - lastPos.x;
        float dy = p.y - lastPos.y;
        lastPos = p;
        moved += std::abs(dx) + std::abs(dy);
        if (moved > 6.f) {
            auto win = CCDirector::get()->getWinSize();
            auto pos = this->getPosition();
            pos.x = std::clamp(pos.x + dx, 0.f, win.width - radius * 2);
            pos.y = std::clamp(pos.y + dy, 0.f, win.height - radius * 2);
            this->setPosition(pos);
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (moved <= 6.f) {
            if (onTap) onTap();
        } else {
            auto pos = this->getPosition();
            Mod::get()->setSavedValue<float>("btn-x", pos.x);
            Mod::get()->setSavedValue<float>("btn-y", pos.y);
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
};
