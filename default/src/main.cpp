#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "controller.hpp"

// unlock all icons and colors (only changes what your game shows)
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

// menu on the main menu
class $modify(PMMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!menuHidden() && Mod::get()->getSettingValue<bool>("show-outside")) {
            this->addChild(PMController::create(false), 200);
        }
        return true;
    }
};

// menu in the editor (off by default)
class $modify(PMEditor, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        if (!menuHidden() && Mod::get()->getSettingValue<bool>("show-in-editor")) {
            this->addChild(PMController::create(false), 200);
        }
        return true;
    }
};
