#pragma once

// "Dev Mode" tab of the ESC menu: a warning to accept first, then game functions (Warp).
// See docs/specs/Tooie/window-ui-and-config.md (Dev Mode).
namespace tooie_dev_mode_ui {
// Adds the tab; call between the other tabs and recompui::config::finalize().
void create_tab();
}
