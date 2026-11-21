#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

inline void pmNotify(const char* text) {
    if (auto n = Notification::create(text, NotificationIcon::Info)) {
        n->show();
    }
}

// save the icons and colors you are using right now
inline void saveIcons() {
    auto gm = GameManager::get();
    auto m = Mod::get();
    m->setSavedValue<int>("ic-cube", gm->getPlayerFrame());
    m->setSavedValue<int>("ic-ship", gm->getPlayerShip());
    m->setSavedValue<int>("ic-ball", gm->getPlayerBall());
    m->setSavedValue<int>("ic-ufo", gm->getPlayerBird());
    m->setSavedValue<int>("ic-wave", gm->getPlayerDart());
    m->setSavedValue<int>("ic-robot", gm->getPlayerRobot());
    m->setSavedValue<int>("ic-spider", gm->getPlayerSpider());
    m->setSavedValue<int>("ic-swing", gm->getPlayerSwing());
    m->setSavedValue<int>("ic-jetpack", gm->getPlayerJetpack());
    m->setSavedValue<int>("ic-col1", gm->getPlayerColor());
    m->setSavedValue<int>("ic-col2", gm->getPlayerColor2());
    m->setSavedValue<bool>("ic-saved", true);
    pmNotify("Icons saved");
}

inline void loadIcons() {
    auto m = Mod::get();
    if (!m->getSavedValue<bool>("ic-saved", false)) {
        pmNotify("No saved icons yet");
        return;
    }
    auto gm = GameManager::get();
    gm->setPlayerFrame(m->getSavedValue<int>("ic-cube", 1));
    gm->setPlayerShip(m->getSavedValue<int>("ic-ship", 1));
    gm->setPlayerBall(m->getSavedValue<int>("ic-ball", 1));
    gm->setPlayerBird(m->getSavedValue<int>("ic-ufo", 1));
    gm->setPlayerDart(m->getSavedValue<int>("ic-wave", 1));
    gm->setPlayerRobot(m->getSavedValue<int>("ic-robot", 1));
    gm->setPlayerSpider(m->getSavedValue<int>("ic-spider", 1));
    gm->setPlayerSwing(m->getSavedValue<int>("ic-swing", 1));
    gm->setPlayerJetpack(m->getSavedValue<int>("ic-jetpack", 1));
    gm->setPlayerColor(m->getSavedValue<int>("ic-col1", 0));
    gm->setPlayerColor2(m->getSavedValue<int>("ic-col2", 3));
    pmNotify("Icons loaded. Restart the game to see them");
}

// Click Between Frames is a separate mod, this just opens its settings
inline void openCBF() {
    auto mod = Loader::get()->getLoadedMod("syzzi.click_between_frames");
    if (!mod) {
        pmNotify("Install Click Between Frames from Geode's mod list first");
        return;
    }
    openSettingsPopup(mod, false);
}
