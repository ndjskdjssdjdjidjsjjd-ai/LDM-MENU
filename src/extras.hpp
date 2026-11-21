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

inline void openCBF() {
    auto mod = Loader::get()->getLoadedMod("syzzi.click_between_frames");
    if (!mod) {
        pmNotify("Install Click Between Frames from Geode's mod list first");
        return;
    }
    openSettingsPopup(mod, false);
}
