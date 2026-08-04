// =====================================================================
//  shell.cpp - Desktop shell implementation
// =====================================================================
#include "shell.h"
#include <algorithm>
#include <cctype>
#include <ctime>

namespace win11 {

// App content binding (defined in apps.cpp)
void attachAppContent(WindowManager&, int, int);

// ---- App registry ----
const AppDef g_apps[] = {
    {"Settings",       "S", Color::fromHex("#4CC2FF"), 0},
    {"File Explorer",  "F", Color::fromHex("#F3B13C"), 1},
    {"Calculator",     "C", Color::fromHex("#37B3E5"), 2},
    {"Notepad",        "N", Color::fromHex("#5C7CFA"), 3},
    {"Terminal",       "T", Color::fromHex("#1F1F1F"), 4},
    {"Task Manager",   "M", Color::fromHex("#2FA84F"), 5},
    {"About MiniOS",   "A", Color::fromHex("#7E57C2"), 6},
};
const int g_appsCount = (int)(sizeof(g_apps) / sizeof(g_apps[0]));

void Shell::init(int screenW, int screenH) {
    sw = screenW; sh = screenH;
    g_screen_w = sw; g_screen_h = sh;
    taskbarH = 56;
    buildDefaultIcons();
    wm.setThemePtr(m_theme);
    updateClock(0);
}

void Shell::buildDefaultIcons() {
    icons.clear();
    const char* names[] = {
        "File Explorer", "Calculator", "Notepad", "Terminal",
        "Task Manager",  "Settings",   "About MiniOS",
    };
    int appsIdx[] = {1, 2, 3, 4, 5, 0, 6};
    int n = 7;
    for (int i = 0; i < n; i++) {
        DeskIcon ic;
        ic.name = names[i];
        ic.app = appsIdx[i];
        ic.rc = Rect(24 + (i % 4) * 112, 40 + (i / 4) * 118, 96, 104);
        icons.push_back(ic);
    }
    // A folder icon
    DeskIcon f;
    f.name = "Apps";
    f.folder = true;
    f.app = -1;
    f.rc = Rect(24 + 3 * 112, 40 + 1 * 118, 96, 104);
    icons.push_back(f);
}

int Shell::openWindow(const std::string& title, int w, int h, const Color& accent) {
    return wm.create(title, w, h, accent);
}

void Shell::launchApp(int app) {
    switch (app) {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: {
            // Window content is attached by apps.cpp via wm.onPaint; here we
            // just open a placeholder window with the app name.  The real
            // content hook is installed by AppHost::attach (see apps.cpp).
            Color ac = g_apps[app].color;
            int w = 0, h = 0;
            switch (app) {
                case 0: w = 720; h = 520; break; // settings
                case 1: w = 820; h = 540; break; // explorer
                case 2: w = 320; h = 460; break; // calc
                case 3: w = 640; h = 420; break; // notepad
                case 4: w = 760; h = 480; break; // terminal
                case 5: w = 560; h = 420; break; // taskmgr
                default: w = 480; h = 360; break;
            }
            int wid = openWindow(g_apps[app].name, w, h, ac);
            attachApp(wid, app);
            break;
        }
        default: break;
    }
}

void Shell::attachApp(int wndId, int app) {
    // Implemented in apps.cpp: sets wm window onPaint/onMouse/onKey
    attachAppContent(wm, wndId, app);
}

void Shell::toggleTheme() {
    dark = !dark;
    // Main loop picks up shell.dark next frame
}

void Shell::updateClock(uint64_t nowMs) {
    (void)nowMs;
    time_t raw = time(nullptr);
    struct tm* ti = localtime(&raw);
    if (!ti) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti->tm_hour, ti->tm_min);
    clockText = buf;
    m_lastClockMin = ti->tm_min;
}

// ---- Taskbar ----
void Shell::taskbarHitTest(int x, int y) {
    Rect tb = taskbarRect();
    if (!tb.contains(x, y)) { m_hoverStart = -1; m_hoverTray = -1; return; }
    int center = sw / 2;
    int iconY = tb.y + 8;
    // Center icons: start, search, taskview, widgets
    int pos[4] = { center - 92, center - 46, center, center + 46 };
    m_hoverStart = -1;
    for (int i = 0; i < 4; i++) {
        if (x >= pos[i] && x < pos[i] + 40 && y >= iconY && y < iconY + 40) {
            m_hoverStart = i;
            break;
        }
    }
    // Taskbar items (running apps) - next to center icons
    m_hoverTray = -1;
    int ix = center + 92;
    for (int i = 0; i < (int)taskbarItems.size(); i++) {
        if (x >= ix && x < ix + 40 && y >= iconY && y < iconY + 40) {
            m_hoverTray = i;
            break;
        }
        ix += 44;
    }
}

int Shell::taskbarItemAt(int x, int y) const {
    Rect tb = taskbarRect();
    if (!tb.contains(x, y)) return -1;
    int ix = sw / 2 + 92;
    for (int i = 0; i < (int)taskbarItems.size(); i++) {
        if (x >= ix && x < ix + 40 && y >= tb.y + 8 && y < tb.y + 48) return i;
        ix += 44;
    }
    return -1;
}

void Shell::renderTaskbar(Gfx& g, const Theme& t) {
    Rect tb = taskbarRect();
    // Taskbar surface (acrylic feel)
    g.fillRect(tb, t.taskbar);
    g.drawLineH(tb.x, tb.y, tb.w, t.border);
    // Center icons
    int center = sw / 2;
    int iconY = tb.y + 8;
    struct { int x; const char* letter; Color c; bool start; } btns[4] = {
        { center - 92, "W", Color::fromHex("#0078D7"), true },  // start (Win logo)
        { center - 46, "?", t.accent, false },                  // search
        { center,      "=", t.accent, false },                  // task view
        { center + 46, "W", t.accent, false },                  // widgets
    };
    for (int i = 0; i < 4; i++) {
        Rect r(btns[i].x, iconY, 40, 40);
        bool hover = (m_hoverStart == i);
        if (hover) g.fillRounded(r, 8, t.taskbarHover);
        if (btns[i].start) {
            g.winLogo(r.x + r.w / 2, r.y + r.h / 2, 18, btns[i].c);
        } else if (btns[i].letter[0] == '=') {
            // task view: two overlapping rects
            g.drawRect(Rect(r.x + 12, r.y + 12, 16, 16), btns[i].c, 1);
            g.drawRect(Rect(r.x + 18, r.y + 18, 16, 16), btns[i].c, 1);
        } else if (btns[i].letter[0] == 'W') {
            // widgets: weather-ish square
            g.fillRounded(Rect(r.x + 10, r.y + 12, 20, 16), 3, btns[i].c.withAlpha(180));
        } else {
            g.textCentered(r, btns[i].letter, btns[i].c, 1);
        }
    }
    // Running apps (taskbar items)
    int ix = center + 92;
    for (int i = 0; i < (int)taskbarItems.size(); i++) {
        Rect r(ix, iconY, 40, 40);
        bool hover = (m_hoverTray == i);
        if (hover) g.fillRounded(r, 8, t.taskbarHover);
        const AppDef* ad = nullptr;
        if (taskbarItems[i].app >= 0 && taskbarItems[i].app < g_appsCount)
            ad = &g_apps[taskbarItems[i].app];
        Color c = ad ? ad->color : t.accent;
        std::string letter = ad ? ad->letter : "?";
        g.appTile(r.x + 8, r.y + 8, 24, c, letter);
        // Running indicator underline
        g.fillRounded(Rect(r.x + 6, r.y + r.h - 5, 28, 3), 2, t.accent);
        ix += 44;
    }
    // System tray (right)
    int trayX = sw - 12;
    Rect clockR(trayX - 60, tb.y + 14, 56, 28);
    renderClock(g, t, clockR);
    // Tray icons: wifi, volume, battery, chevron
    int ti = trayX - 82;
    g.fillRounded(Rect(ti - 26, tb.y + 18, 20, 20), 6, t.taskbarHover); // action center
    g.textCentered(Rect(ti - 26, tb.y + 18, 20, 20), "^", t.foreground, 1);
    ti -= 30;
    g.textCentered(Rect(ti, tb.y + 18, 22, 20), "W", t.foreground, 1); // wifi
    ti -= 26;
    g.textCentered(Rect(ti, tb.y + 18, 22, 20), "V", t.foreground, 1); // volume
    // Battery
    Rect bat(ti - 30, tb.y + 22, 24, 12);
    g.drawRect(bat, t.foreground, 1);
    g.fillRect(Rect(bat.x + 2, bat.y + 2, 12, bat.h - 4), t.foreground);
    g.fillRect(Rect(bat.x + bat.w, bat.y + 3, 2, bat.h - 6), t.foreground);
}

void Shell::renderClock(Gfx& g, const Theme& t, const Rect& r) {
    g.textCentered(Rect(r.x, r.y, r.w, r.h), clockText, t.foreground, 1);
    g.textCentered(Rect(r.x, r.y + 12, r.w, 12), "8/4/2026", t.foreground2, 1);
}

void Shell::renderDesktopIcons(Gfx& g, const Theme& t) {
    for (auto& ic : icons) {
        if (ic.selected)
            g.fillRounded(Rect(ic.rc.x - 6, ic.rc.y - 6, ic.rc.w + 12, ic.rc.h + 12), 12, t.hover.withAlpha(120));
        if (ic.folder) {
            // Folder: tab + body
            g.fillRounded(Rect(ic.rc.x + 18, ic.rc.y, 60, 10), 2, Color::fromHex("#E8B84B").withAlpha(230));
            g.fillRounded(Rect(ic.rc.x + 8, ic.rc.y + 10, 80, 62), 8, Color::fromHex("#E8B84B").withAlpha(235));
            g.drawRounded(Rect(ic.rc.x + 8, ic.rc.y + 10, 80, 62), 8, Color(0, 0, 0, 40), 1);
            g.textCentered(Rect(ic.rc.x, ic.rc.y + 76, ic.rc.w, 26), ic.name, Color(255, 255, 255), 1);
        } else if (ic.app >= 0 && ic.app < g_appsCount) {
            const AppDef& ad = g_apps[ic.app];
            g.appTile(ic.rc.x + 12, ic.rc.y, 72, ad.color, ad.letter);
            // Label with shadow
            g.textCentered(Rect(ic.rc.x, ic.rc.y + 78, ic.rc.w, 26), ic.name, Color(255, 255, 255, 230), 1);
        }
    }
}

// ---- Context menu ----
void Shell::openContextMenu(int x, int y, const std::vector<MenuItem>& items, int target) {
    menu.items = items;
    menu.x = std::min(x, sw - menu.w - 8);
    menu.y = std::min(y, sh - (int)items.size() * 34 - 12);
    menu.h = (int)items.size() * 34 + 8;
    menu.open = true;
    menu.hover = -1;
    menu.targetApp = target;
}

void Shell::renderContextMenu(Gfx& g, const Theme& t) {
    if (!menu.open) return;
    Rect r(menu.x, menu.y, menu.w, menu.h);
    g.shadowBox(r, 8, Color(0, 0, 0, 90), 6, 3);
    g.fillRounded(r, 8, t.menuSurface);
    g.drawRounded(r, 8, Color(0, 0, 0, 30), 1);
    int y = menu.y + 4;
    int idx = 0;
    for (auto& it : menu.items) {
        if (it.separator) {
            g.fillRect(Rect(menu.x + 14, y + 6, menu.w - 28, 1), Color(0, 0, 0, 26));
            y += 14;
            idx++;
            continue;
        }
        Rect item(menu.x + 4, y, menu.w - 8, 30);
        if (idx == menu.hover)
            g.fillRounded(item, 6, t.hover);
        g.text(item.x + 14, y + 7, it.label, t.foreground, 1);
        y += 34;
        idx++;
    }
}

void Shell::handleMenuClick(int itemIdx) {
    if (itemIdx < 0 || itemIdx >= (int)menu.items.size()) return;
    const MenuItem& it = menu.items[itemIdx];
    // Desktop context menu actions
    if (menu.targetApp == -2) { // desktop menu
        if (it.action == 10) { /* refresh - nothing to rebuild yet */ }
        else if (it.action == 11) { /* new folder */ }
        else if (it.action == 12) { toggleTheme(); }
        else if (it.action == 13) { openWindow("Display Settings", 640, 420, Color::fromHex("#0067C0")); }
    } else if (menu.targetApp == -3) { // taskbar menu
        if (it.action == 20) toggleStartMenu();
    } else if (menu.targetApp >= 0) { // icon menu
        int app = menu.targetApp;
        if (it.action == 30) { launchApp(app); }
        else if (it.action == 31) { /* pin */ }
        else if (it.action == 32) { /* delete icon */ }
        else if (it.action == 33) { toggleTheme(); }
    }
    closeMenu();
}

// ---- Start menu ----
void Shell::openStartMenu(bool open) {
    startOpen = open;
    if (open) { notificationsOpen = false; widgetsOpen = false; }
}

void Shell::toggleStartMenu() { openStartMenu(!startOpen); }

void Shell::renderStartMenu(Gfx& g, const Theme& t) {
    if (!startOpen) return;
    // Dim background
    g.fillRect(Rect(0, 0, sw, sh - taskbarH), Color(0, 0, 0, 90));
    // Panel (Win11: centered, rounded, acrylic)
    int mw = 620, mh = sh - taskbarH - 40;
    int mx = (sw - mw) / 2, my = 20;
    Rect panel(mx, my, mw, mh);
    g.shadowBox(panel, 14, Color(0, 0, 0, 100), 12, 6);
    g.fillRounded(panel, 14, t.startMenu);
    g.drawRounded(panel, 14, Color(255, 255, 255, 26), 1);
    // Blur feel: overlay translucent tint (approx mica)
    g.fillRounded(panel, 14, t.micaTint.withAlpha((uint8_t)((1.0f - t.micaOpacity) * 255)));
    // Search box (top)
    Rect search(mx + 24, my + 20, mw - 48, 36);
    g.fillRounded(search, 18, t.inputBg);
    g.drawRounded(search, 18, t.border, 1);
    g.text(search.x + 14, search.y + 10, searchText.empty() ? "Search apps, files..." : searchText,
           searchText.empty() ? t.foreground2 : t.foreground, 1);
    // User avatar row (top-left)
    g.fillCircle(mx + 40, my + 34, 18, t.accent);
    g.textCentered(Rect(mx + 22, my + 16, 36, 36), "U", Color(255, 255, 255), 1);
    g.text(mx + 68, my + 24, "User", t.foreground, 1);
    // Pinned apps grid (3 cols x 4 rows)
    int gridX = mx + 24, gridY = my + 76;
    int col = 0, row = 0;
    int shown = 0;
    for (int i = 0; i < g_appsCount && shown < 12; i++) {
        if (!searchText.empty()) {
            std::string lower = g_apps[i].name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            std::string q = searchText;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            if (lower.find(q) == std::string::npos) continue;
        }
        int x = gridX + col * 170, y = gridY + row * 100;
        g.appTile(x, y, 64, g_apps[i].color, g_apps[i].letter);
        g.textCentered(Rect(x, y + 68, 64, 26), g_apps[i].name.substr(0, 9), t.foreground, 1);
        col++;
        if (col >= 3) { col = 0; row++; }
        shown++;
    }
    // All apps link
    int ay = my + mh - 84;
    g.text(mx + 24, ay, "All apps  >", t.accent, 1);
    // Power + user at bottom
    g.fillRounded(Rect(mx + mw - 96, my + mh - 40, 72, 32), 8, t.taskbarHover);
    g.textCentered(Rect(mx + mw - 96, my + mh - 40, 72, 32), "Power", t.foreground, 1);
    g.fillRounded(Rect(mx + 24, my + mh - 40, 96, 32), 8, t.taskbarHover);
    g.textCentered(Rect(mx + 24, my + mh - 40, 96, 32), "User", t.foreground, 1);
}

// ---- Tray flyouts (notifications / widgets) ----
void Shell::renderTrayFlyouts(Gfx& g, const Theme& t) {
    // Notification center (right side slide-in)
    if (notificationsOpen) {
        int w = 360, h = sh - taskbarH - 20;
        Rect r(sw - w - 8, 8, w, h);
        g.shadowBox(r, 12, Color(0, 0, 0, 90), 8, 4);
        g.fillRounded(r, 12, t.windowCard);
        g.text(r.x + 20, r.y + 16, "Notifications", t.foreground, 2);
        g.drawLineH(r.x + 16, r.y + 44, w - 32, t.border);
        if (notifications.empty()) {
            g.text(r.x + 20, r.y + 70, "No new notifications", t.foreground2, 1);
        } else {
            int y = r.y + 60;
            for (auto& n : notifications) {
                g.fillRounded(Rect(r.x + 14, y, w - 28, 56), 10, t.hover);
                g.text(r.x + 24, y + 8, n.app, t.foreground, 1);
                g.text(r.x + 24, y + 26, n.text, t.foreground2, 1);
                y += 66;
            }
        }
    }
    // Widgets panel (left side)
    if (widgetsOpen) {
        int w = 380, h = sh - taskbarH - 20;
        Rect r(8, 8, w, h);
        g.shadowBox(r, 12, Color(0, 0, 0, 90), 8, 4);
        g.fillRounded(r, 12, t.windowCard);
        g.text(r.x + 20, r.y + 16, "Widgets", t.foreground, 2);
        g.drawLineH(r.x + 16, r.y + 44, w - 32, t.border);
        // Weather card
        g.fillRounded(Rect(r.x + 16, r.y + 54, w - 32, 90), 10, t.hover);
        g.text(r.x + 28, r.y + 66, "Beijing, CN", t.foreground2, 1);
        g.text(r.x + 28, r.y + 90, "28°C  Sunny", t.foreground, 2);
        // Calendar card
        g.fillRounded(Rect(r.x + 16, r.y + 158, w - 32, 120), 10, t.hover);
        g.text(r.x + 28, r.y + 170, "August 2026", t.foreground, 2);
        g.text(r.x + 28, r.y + 200, "Mon Tue Wed Thu Fri Sat Sun", t.foreground2, 1);
        g.text(r.x + 28, r.y + 226, "                      1  2", t.foreground, 1);
        g.text(r.x + 28, r.y + 244, " 3  4  5  6  7  8  9", t.foreground, 1);
        // News card
        g.fillRounded(Rect(r.x + 16, r.y + 292, w - 32, 70), 10, t.hover);
        g.text(r.x + 28, r.y + 304, "Top stories", t.foreground2, 1);
        g.text(r.x + 28, r.y + 328, "MiniOS reaches 60fps", t.foreground, 1);
    }
}

// ---- Full render ----
void Shell::render(Gfx& g, const Theme& t, uint64_t nowMs) {
    // Wallpaper (Win11 Bloom gradient)
    g.fillGradientV(Rect(0, 0, sw, sh),
                    Color::fromHex("#2E4372"), Color::fromHex("#5A8BCA"));
    // Bloom blob (approx): radial-ish glow via layered circles
    Color bloom(0x4A, 0x7A, 0xC8, 160);
    g.fillCircle(sw / 2 + 160, sh / 2 - 80, 220, bloom.withAlpha(30));
    g.fillCircle(sw / 2 + 160, sh / 2 - 80, 150, bloom.withAlpha(40));
    g.fillCircle(sw / 2 + 160, sh / 2 - 80, 90,  bloom.withAlpha(60));

    // Desktop icons
    renderDesktopIcons(g, t);

    // Windows
    wm.renderAll(g, t);

    // Context menu (above windows)
    if (menu.open) renderContextMenu(g, t);

    // Start menu / flyouts
    renderStartMenu(g, t);
    renderTrayFlyouts(g, t);

    // Taskbar (topmost)
    renderTaskbar(g, t);

    // Clock update
    if (m_lastClockMin != (uint64_t)(nowMs / 60000)) updateClock(nowMs);
}

// ---- Input dispatch ----
void Shell::onMouseMove(int x, int y) {
    g_mouse = true; g_mouseX = x; g_mouseY = y;
    if (menu.open) {
        // hover detection
        int idx = (y - menu.y - 4) / 34;
        menu.hover = (y >= menu.y + 4 && idx >= 0 && idx < (int)menu.items.size()) ? idx : -1;
        return;
    }
    // Drag desktop icon
    if (m_dragIcon && m_dragIconIdx >= 0) {
        icons[m_dragIconIdx].rc.x = x - m_dragIconDx;
        icons[m_dragIconIdx].rc.y = y - m_dragIconDy;
        return;
    }
    taskbarHitTest(x, y);
    wm.onMouseMove(x, y);
}

void Shell::onMouseDown(int x, int y, int btn) {
    g_mouse = true; g_mouseX = x; g_mouseY = y;
    if (btn == 2) { // right button
        // Desktop right-click
        if (y < sh - taskbarH && wm.hitWindow(x, y) < 0 && !startOpen) {
            std::vector<MenuItem> items = {
                {"Refresh", false, 10}, {"New folder", false, 11},
                {"Separator", true, 0}, {"Dark mode", false, 12},
                {"Display settings", false, 13},
            };
            openContextMenu(x, y, items, -2);
        } else if (taskbarRect().contains(x, y)) {
            std::vector<MenuItem> items = {{"Start menu", false, 20}};
            openContextMenu(x, y, items, -3);
        }
        return;
    }
    // Left
    if (menu.open) { handleMenuClick(menu.hover); return; }
    if (startOpen) {
        // Click outside panel closes it
        Rect panel((sw - 620) / 2, 20, 620, sh - taskbarH - 40);
        if (!panel.contains(x, y)) { openStartMenu(false); }
        return;
    }
    if (notificationsOpen || widgetsOpen) {
        notificationsOpen = false; widgetsOpen = false;
        return;
    }
    // Start button / search / taskview / widgets in taskbar
    if (taskbarRect().contains(x, y)) {
        int center = sw / 2;
        if (x >= center - 92 && x < center - 52) { toggleStartMenu(); return; }
        if (x >= center - 46 && x < center - 6) { toggleStartMenu(); return; } // search -> start
        if (x >= center && x < center + 40) { /* task view */ return; }
        if (x >= center + 46 && x < center + 86) { widgetsOpen = !widgetsOpen; return; }
        // Tray notification area
        if (x >= sw - 120) { notificationsOpen = !notificationsOpen; return; }
        // Running app icon
        int idx = taskbarItemAt(x, y);
        if (idx >= 0 && idx < (int)taskbarItems.size()) {
            int wid = taskbarItems[idx].wndId;
            wm.focus(wid);
            Window* w = wm.get(wid);
            if (w) w->minimized = false;
            return;
        }
        return;
    }
    // Desktop icon click / drag
    for (size_t i = 0; i < icons.size(); i++) {
        if (icons[i].rc.contains(x, y)) {
            // select
            for (auto& ic : icons) ic.selected = false;
            icons[i].selected = true;
            m_dragIcon = true;
            m_dragIconIdx = (int)i;
            m_dragIconDx = x - icons[i].rc.x;
            m_dragIconDy = y - icons[i].rc.y;
            return;
        }
    }
    // Otherwise window interaction
    wm.onMouseDown(x, y, btn);
}

void Shell::onMouseUp(int x, int y, int btn) {
    g_mouse = true; g_mouseX = x; g_mouseY = y;
    if (btn == 2) return;
    if (m_dragIcon) {
        // Double-click detection on icons is handled via click count in main loop;
        // here: if no move happened, treat as single click (select only).
        m_dragIcon = false;
        m_dragIconIdx = -1;
        return;
    }
    wm.onMouseUp(x, y, btn);
}

void Shell::onScroll(int x, int y, int dy) {
    wm.onScroll(x, y, dy);
}

void Shell::onKey(char ch) {
    wm.onKey(ch);
}

} // namespace win11
