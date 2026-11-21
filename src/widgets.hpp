#pragma once
#include "state.hpp"

class TouchBlocker : public CCLayer {
public:
    CCSize area = {0.f, 0.f};

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
        if (!t || !pmTouchAllowed(this)) return false;
        auto l = this->convertToNodeSpace(t->getLocation());
        return l.x >= 0.f && l.y >= 0.f && l.x <= area.width && l.y <= area.height;
    }

    void ccTouchMoved(CCTouch*, CCEvent*) override {}
    void ccTouchEnded(CCTouch*, CCEvent*) override {}
    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
};

class DragBar : public CCLayer {
public:
    CCNode* target = nullptr;
    CCSize barSize = {0.f, 0.f};
    CCSize panelSize = {0.f, 0.f};
    CCPoint lastPos = {0.f, 0.f};

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
        if (!t || !target || !pmTouchAllowed(this)) return false;
        auto local = this->convertToNodeSpace(t->getLocation());
        if (local.x < 0.f || local.y < 0.f || local.x > barSize.width || local.y > barSize.height)
            return false;
        lastPos = t->getLocation();
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (!t || !target) return;
        auto p = t->getLocation();
        auto pos = target->getPosition();
        pos.x += p.x - lastPos.x;
        pos.y += p.y - lastPos.y;
        lastPos = p;
        auto win = CCDirector::get()->getWinSize();
        float maxX = std::max(0.f, win.width - panelSize.width);
        float maxY = std::max(0.f, win.height - panelSize.height);
        pos.x = std::clamp(pos.x, 0.f, maxX);
        pos.y = std::clamp(pos.y, 0.f, maxY);
        target->setPosition(pos);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!target) return;
        auto pos = target->getPosition();
        Mod::get()->setSavedValue<float>("panel-x", pos.x);
        Mod::get()->setSavedValue<float>("panel-y", pos.y);
    }

    void ccTouchCancelled(CCTouch* t, CCEvent* e) override {
        ccTouchEnded(t, e);
    }
};

class PMBtn : public CCLayer {
public:
    std::function<void()> cb;
    CCSize size = {12.f, 12.f};
    CCScale9Sprite* body = nullptr;
    CCScale9Sprite* mark = nullptr;
    CCLabelBMFont* labelNode = nullptr;

    static CCScale9Sprite* box(CCSize size, ccColor3B color, GLubyte opacity) {
        auto s = CCScale9Sprite::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
        if (!s) return nullptr;
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
            if (r->body) r->addChild(r->body);
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
            auto l = CCLabelBMFont::create(text ? text : "", font ? font : "chatFont.fnt");
            if (l) {
                l->setScale(scale);
                l->setPosition({size.width * 0.5f, size.height * 0.5f});
                r->labelNode = l;
                r->addChild(l);
            }
        }
        return r;
    }

    static PMBtn* createToggle(std::function<void()> cb) {
        auto r = create(CCSize{12.f, 12.f}, ccColor3B{34, 22, 24}, cb);
        if (r) {
            r->mark = box(CCSize{5.f, 5.f}, ccColor3B{255, 130, 35}, 255);
            if (r->mark) {
                r->mark->setPosition({3.5f, 3.5f});
                r->mark->setVisible(false);
                r->addChild(r->mark);
            }
        }
        return r;
    }

    void setOn(bool on) {
        if (mark) mark->setVisible(on);
    }

    void setActiveTab(bool active) {
        if (body) {
            body->setColor(active ? ccColor3B{214, 78, 28} : ccColor3B{30, 18, 22});
            body->setOpacity(active ? 255 : 210);
        }
        if (labelNode) {
            labelNode->setColor(active ? ccColor3B{255, 245, 225} : ccColor3B{185, 155, 145});
        }
    }

    bool inside(CCPoint l) {
        return l.x >= 0.f && l.y >= 0.f && l.x <= size.width && l.y <= size.height;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -602, true);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!t || !pmTouchAllowed(this)) return false;
        if (!inside(this->convertToNodeSpace(t->getLocation()))) return false;
        if (body) body->setOpacity(150);
        return true;
    }

    void ccTouchMoved(CCTouch*, CCEvent*) override {}

    void ccTouchEnded(CCTouch* t, CCEvent*) override {
        if (body) body->setOpacity(235);
        if (t && inside(this->convertToNodeSpace(t->getLocation())) && cb) cb();
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        if (body) body->setOpacity(235);
    }
};

class PMButton : public CCLayer {
public:
    std::function<void()> onTap;
    float radius = 11.5f;
    CCPoint lastPos = {0.f, 0.f};
    float moved = 0.f;

    static PMButton* create(CCNode* icon, float radius, std::function<void()> cb) {
        auto r = new PMButton();
        if (r && r->init()) {
            r->radius = radius;
            r->onTap = cb;
            r->setContentSize({radius * 2.f, radius * 2.f});
            if (icon) {
                icon->setPosition({radius, radius});
                r->addChild(icon);
            }
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
        if (!t || !pmTouchAllowed(this)) return false;
        auto local = this->convertToNodeSpace(t->getLocation());
        float dx = local.x - radius;
        float dy = local.y - radius;
        if (dx * dx + dy * dy > radius * radius) return false;
        lastPos = t->getLocation();
        moved = 0.f;
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (!t) return;
        auto p = t->getLocation();
        float dx = p.x - lastPos.x;
        float dy = p.y - lastPos.y;
        lastPos = p;
        moved += std::abs(dx) + std::abs(dy);
        if (moved > 6.f) {
            auto win = CCDirector::get()->getWinSize();
            float maxX = std::max(0.f, win.width - radius * 2.f);
            float maxY = std::max(0.f, win.height - radius * 2.f);
            auto pos = this->getPosition();
            pos.x = std::clamp(pos.x + dx, 0.f, maxX);
            pos.y = std::clamp(pos.y + dy, 0.f, maxY);
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
