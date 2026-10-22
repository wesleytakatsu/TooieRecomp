#pragma once

#include <string>

namespace tooie {
    // Pure helper: picks the XDG Pictures dir from ~/.config/user-dirs.dirs text (no shell eval; only a leading
    // $HOME is expanded). Falls back to <home>/Pictures. Returns "" when home is empty and nothing absolute is found.
    inline std::string pictures_dir_from(const std::string &user_dirs_text, const std::string &home) {
        size_t pos = 0;
        while (pos < user_dirs_text.size()) {
            size_t end = user_dirs_text.find('\n', pos);
            if (end == std::string::npos) end = user_dirs_text.size();
            std::string line = user_dirs_text.substr(pos, end - pos);
            pos = end + 1;
            const std::string key = "XDG_PICTURES_DIR=\"";
            if (line.compare(0, key.size(), key) != 0) continue;
            const size_t close = line.find('"', key.size());
            if (close == std::string::npos) continue;
            std::string v = line.substr(key.size(), close - key.size());
            if (v.compare(0, 5, "$HOME") == 0) v = home + v.substr(5);
            if (!v.empty() && v[0] == '/' && v.find('$') == std::string::npos) return v;
        }
        return home.empty() ? std::string() : home + "/Pictures";
    }

    // The F8 hotkey that used to call RT64::interpBurst::request() is removed (user request); no key triggers a burst.
}
