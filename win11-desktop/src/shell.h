// =====================================================================
//  shell.h - Win11 Desktop Shell
//  Desktop wallpaper + icons, centered taskbar, full start menu,
//  system tray, notification center, widgets panel, context menus.
// =====================================================================
#pragma once
#include "gfx.h"
#include "theme.h"
#include "window.h"
#include <string>
#include <vector>

namespace win11 {

// ---- App registry ----
struct AppDef {
    std::string name;
    std::string letter;    // tile letter
    Color color;
    int kind;              // 0=settings,1=explorer,2=calc,3=notepad,4=terminal,5=taskmgr,6=about
};
extern const AppDef g_apps[];
extern const int g_appsCount;

// ---- Desktop icon ----
struct DeskIcon {
    std::string name;
    int app = -1;          // app index, or -1 for folder
    bool folder = false;
    Rect rc;
    bool selected = false;
};

// ---- Taskbar running item ----
struct TaskbarItem {
    int wndId = -1;
    int app = -1;          // associated app (for icons)
    std::string title;
};

// ---- Context menu ----
struct MenuItem {
    std::string label;
    bool separator = false;
    int action = 0;        // app-defined action id
};
struct ContextMenu {
    bool open = false;
    int x = 0, y = 0, w = 190, h = 0;
    std::vector<MenuItem> items;
    int hover = -1;
    int targetApp = -1;    // for icon menus
};

// ---- Notification ----
struct Notification {
    std::string app;
    std::string text;
    uint64_t timeMs = 0;
};

// ---- Shell ----
class Shell {
public:
    void init(int screenW, int screenH);
    void updateClock(uint64_t nowMs);
    void render(Gfx& g, const Theme& t, uint64_t nowMs);
    void renderTaskbar(Gfx& g, const Theme& t);
    void renderStartMenu(Gfx& g, const Theme& t);
    void renderTrayFlyouts(Gfx& g, const Theme& t);

    // Input (called from main loop)
    void onMouseMove(int x, int y);
    void onMouseDown(int x, int y, int btn);
    void onMouseUp(int x, int y, int btn);
    void onScroll(int x, int y, int dy);
    void onKey(char ch);

    // Actions
    void openStartMenu(bool open);
    void toggleStartMenu();
    void launchApp(int app);
    void toggleNotifications();
    void toggleWidgets();
    void setTheme(const Theme& t) { m_theme = &t; }
    void toggleTheme();   // defined in shell.cpp
    bool isDark() const { return dark; }
    void setDark(bool d) { dark = d; }

    WindowManager wm;                 // window manager (shell owns it)
    std::vector<DeskIcon> icons;
    std::vector<TaskbarItem> taskbarItems;
    std::vector<Notification> notifications;
    ContextMenu menu;
    std::string clockText;
    bool startOpen = false;
    bool notificationsOpen = false;
    bool widgetsOpen = false;
    int searchTextLen = 0;
    std::string searchText;
    bool dark = false;
    int lastClickIcon = -1;           // for double-click detection
    uint64_t lastClickTime = 0;

    // for apps to create windows
    int openWindow(const std::string& title, int w, int h, const Color& accent);
    // Attach real app content to a window (implemented in apps.cpp)
    void attachApp(int wndId, int app);

    // public geometry access
    int screenW() const { return sw; }
    int screenH() const { return sh; }
    int taskbarHeight() const { return taskbarH; }
    Rect taskbarRectPublic() const { return Rect(0, sh - taskbarH, sw, taskbarH); }

private:
    const Theme* m_theme = nullptr;
    int sw = 1280, sh = 720;
    int taskbarH = 56;
    int m_hoverStart = -1;
    int m_hoverTray = -1;
    uint64_t m_lastClockMin = 0;
    bool m_dragIcon = false;
    int m_dragIconIdx = -1;
    int m_dragIconDx = 0, m_dragIconDy = 0;

    void buildDefaultIcons();
    void renderDesktopIcons(Gfx& g, const Theme& t);
    void renderContextMenu(Gfx& g, const Theme& t);
    void openContextMenu(int x, int y, const std::vector<MenuItem>& items, int target);
    void closeMenu() { menu.open = false; }
    void taskbarHitTest(int x, int y);
    void handleMenuClick(int itemIdx);
    void renderClock(Gfx& g, const Theme& t, const Rect& r);
    int taskbarItemAt(int x, int y) const;
    Rect taskbarRect() const { return Rect(0, sh - taskbarH, sw, taskbarH); }
};

} // namespace win11
