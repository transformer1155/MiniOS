// =====================================================================
//  window.h - Win11 Window & WindowManager
//  Rounded corners, centered title bar, hamburger menu, Snap Layouts,
//  Alt+Tab switcher, drag / 8-way resize, open/close animations.
// =====================================================================
#pragma once
#include "gfx.h"
#include "theme.h"
#include <string>
#include <vector>
#include <functional>

namespace win11 {

enum class WinState : int { Normal, Minimized, Maximized };

struct Window {
    int id = -1;
    std::string title;
    Rect rc;                     // current geometry (client area)
    Rect restoreRc;              // pre-maximize geometry
    WinState state = WinState::Normal;
    bool focused = false;
    bool maximized = false;
    bool minimized = false;
    Color accent;                // window accent (from app)
    // Animation
    float openT = 1.0f;          // 0 -> 1 open progress
    float closeT = 0.0f;         // 0 -> 1 close progress
    bool closing = false;
    bool opening = false;
    // Content
    std::function<void(Gfx&, const Theme&, const Window&, const Rect&)> onPaint;
    std::function<void(int, int, int, int)> onMouse;  // (x,y,button,state)
    std::function<void(char)> onKey;
    // Drag / resize
    bool dragging = false;
    int dragDx = 0, dragDy = 0;
    int resizeMode = 0;          // 0 none, 1..8 directions
    int resizeSX = 0, resizeSY = 0;
    Rect resizeOrig;
    // Snap animation
    Rect snapFrom, snapTo;
    bool snapping = false;
    float snapT = 0.0f;

    bool hitTitle(int x, int y) const {
        return x >= rc.x && x < rc.x + rc.w && y >= rc.y && y < rc.y + titleBarH();
    }
    int titleBarH() const { return 40; }
    Rect client() const {
        return Rect(rc.x, rc.y + titleBarH(), rc.w, rc.h - titleBarH());
    }
};

// ---- WindowManager ----
class WindowManager {
public:
    void setThemePtr(const Theme* t) { m_theme = t; }

    int create(const std::string& title, int w, int h, const Color& accent);
    void close(int id);
    void focus(int id);
    void minimize(int id);
    void toggleMaximize(int id);
    void toggleSnap(int id, SnapZone z);
    void moveTo(int id, int x, int y);
    void resizeTo(int id, int w, int h);
    Window* get(int id);
    Window* focusedWindow();
    int focusedId() const { return m_focused; }
    const std::vector<Window>& windows() const { return m_windows; }

    // Input
    void onMouseDown(int x, int y, int btn);
    void onMouseUp(int x, int y, int btn);
    void onMouseMove(int x, int y);
    void onScroll(int x, int y, int dy);
    void onKey(char ch);

    // Alt+Tab switcher
    void startAltTab();
    void altTabStep(int dir);
    void endAltTab(bool commit);
    bool altTabActive() const { return m_altTabActive; }
    std::vector<int> altTabOrder;

    // Update animations, returns true if redraw needed
    bool tick(uint64_t nowMs);
    void renderAll(Gfx& g, const Theme& t);

    // Hit test helpers
    int hitWindow(int x, int y) const;
    int hitWindowTitle(int x, int y) const;

    // Snap preview (while dragging to edge)
    SnapZone snapZoneAt(int x, int y) const;
    Rect snapRectFor(int id, SnapZone z) const;
    SnapZone activeSnapPreview = SnapZone::None;
    int snapPreviewWnd = -1;

private:
    const Theme* m_theme = nullptr;
    std::vector<Window> m_windows;
    int m_nextId = 1;
    int m_focused = -1;
    int m_dragWnd = -1;
    int m_resizeWnd = -1;
    uint64_t m_lastNow = 0;
    bool m_altTabActive = false;
    int m_altTabIndex = 0;

    void raiseWindow(int id);
    void drawTitleBar(Gfx& g, const Theme& t, const Window& w);
    void drawWindowFrame(Gfx& g, const Theme& t, const Window& w);
};

} // namespace win11
