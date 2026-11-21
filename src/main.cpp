#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "controller.hpp"

class $modify(PMGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (unlockAll()) return true;
        return GameManager::isIconUnlocked(id, type);
    }

    bool isColorUnlocked(int key, UnlockType type) {
        if (unlockAll()) return true;
        return GameManager::isColorUnlocked(key, type);
    }
};

class $modify(PMMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!menuHidden() && Mod::get()->getSettingValue<bool>("show-outside")) {
            if (auto pm = PMController::create(false)) {
                this->addChild(pm, 999);
            }
        }
        return true;
    }
};

class $modify(PMEditor, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        if (!menuHidden() && Mod::get()->getSettingValue<bool>("show-in-editor")) {
            if (auto pm = PMController::create(false)) {
                this->addChild(pm, 999);
            }
        }
        return true;
    }
};
