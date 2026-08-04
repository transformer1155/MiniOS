// =====================================================================
//  window.cpp - WindowManager implementation
// =====================================================================
#include "window.h"
#include <algorithm>

namespace win11 {

int g_screen_w = 1280;
int g_screen_h = 720;

int WindowManager::create(const std::string& title, int w, int h, const Color& accent) {
    Window wn;
    wn.id = m_nextId++;
    wn.title = title;
    wn.accent = accent;
    // Centered with cascade
    int baseX = (g_screen_w - w) / 2 + (m_windows.size() % 6) * 24;
    int baseY = (g_screen_h - h) / 2 + (m_windows.size() % 6) * 18;
    baseX = std::max(8, baseX);
    baseY = std::max(8, baseY);
    wn.rc = Rect(baseX, baseY, w, h);
    wn.restoreRc = wn.rc;
    wn.opening = true;
    wn.openT = 0.0f;
    m_windows.push_back(wn);
    int id = wn.id;
    raiseWindow(id);
    focus(id);
    return id;
}

void WindowManager::close(int id) {
    for (auto& w : m_windows) {
        if (w.id == id) {
            if (!w.closing) { w.closing = true; w.closeT = 0.0f; }
            return;
        }
    }
}

void WindowManager::focus(int id) {
    Window* w = get(id);
    if (!w) return;
    m_focused = id;
    for (auto& x : m_windows) x.focused = (x.id == id);
    raiseWindow(id);
}

void WindowManager::raiseWindow(int id) {
    int idx = -1;
    for (size_t i = 0; i < m_windows.size(); i++)
        if (m_windows[i].id == id) { idx = (int)i; break; }
    if (idx < 0) return;
    Window w = m_windows[idx];
    m_windows.erase(m_windows.begin() + idx);
    m_windows.push_back(w);
}

Window* WindowManager::get(int id) {
    for (auto& w : m_windows)
        if (w.id == id) return &w;
    return nullptr;
}

Window* WindowManager::focusedWindow() { return get(m_focused); }

void WindowManager::minimize(int id) {
    Window* w = get(id);
    if (!w) return;
    w->minimized = true;
    w->state = WinState::Minimized;
}

void WindowManager::toggleMaximize(int id) {
    Window* w = get(id);
    if (!w) return;
    if (w->maximized) {
        w->rc = w->restoreRc;
        w->maximized = false;
        w->state = WinState::Normal;
    } else {
        w->restoreRc = w->rc;
        w->rc = Rect(0, 0, g_screen_w, g_screen_h);
        w->maximized = true;
        w->state = WinState::Maximized;
    }
}

Rect WindowManager::snapRectFor(int id, SnapZone z) const {
    (void)id;
    int w = g_screen_w, h = g_screen_h;
    int tw = w / 2, th = h / 2;
    switch (z) {
        case SnapZone::Left:        return Rect(0, 0, tw, h);
        case SnapZone::Right:       return Rect(w - tw, 0, tw, h);
        case SnapZone::Top:         return Rect(0, 0, w, th);
        case SnapZone::TopLeft:     return Rect(0, 0, tw, th);
        case SnapZone::TopRight:    return Rect(w - tw, 0, tw, th);
        case SnapZone::BottomLeft:  return Rect(0, h - th, tw, th);
        case SnapZone::BottomRight: return Rect(w - tw, h - th, tw, th);
        case SnapZone::Maximize:    return Rect(0, 0, w, h);
        default: return Rect(0, 0, w / 2, h);
    }
}

void WindowManager::toggleSnap(int id, SnapZone z) {
    Window* w = get(id);
    if (!w) return;
    if (!w->maximized) w->restoreRc = w->rc;
    w->snapFrom = w->rc;
    w->snapTo = snapRectFor(id, z);
    w->snapping = true;
    w->snapT = 0.0f;
    w->maximized = false;
    w->state = WinState::Normal;
}

void WindowManager::moveTo(int id, int x, int y) {
    Window* w = get(id);
    if (!w) return;
    w->rc.x = x; w->rc.y = y;
}

void WindowManager::resizeTo(int id, int w, int h) {
    Window* wn = get(id);
    if (!wn) return;
    if (w > 40) wn->rc.w = w;
    if (h > 40) wn->rc.h = h;
}

int WindowManager::hitWindow(int x, int y) const {
    for (int i = (int)m_windows.size() - 1; i >= 0; i--) {
        const Window& w = m_windows[i];
        if (w.minimized || w.closing) continue;
        if (w.rc.contains(x, y)) return w.id;
    }
    return -1;
}

int WindowManager::hitWindowTitle(int x, int y) const {
    for (int i = (int)m_windows.size() - 1; i >= 0; i--) {
        const Window& w = m_windows[i];
        if (w.minimized || w.closing) continue;
        if (w.hitTitle(x, y)) return w.id;
    }
    return -1;
}

SnapZone WindowManager::snapZoneAt(int x, int y) const {
    int w = g_screen_w, h = g_screen_h;
    if (x < 0 && y < 0) return SnapZone::None;
    int edge = 8, corner = 80;
    bool left = x <= edge, right = x >= w - edge;
    bool top = y <= edge, bot = y >= h - edge;
    if (x <= corner && y <= corner) return SnapZone::TopLeft;
    if (x >= w - corner && y <= corner) return SnapZone::TopRight;
    if (x <= corner && y >= h - corner) return SnapZone::BottomLeft;
    if (x >= w - corner && y >= h - corner) return SnapZone::BottomRight;
    if (left) return SnapZone::Left;
    if (right) return SnapZone::Right;
    if (top) return SnapZone::Top;
    if (bot) return SnapZone::Maximize;
    return SnapZone::None;
}

// ---- Input ----
void WindowManager::onMouseDown(int x, int y, int btn) {
    if (m_altTabActive) { endAltTab(true); return; }
    if (btn == 1) return; // right handled by shell
    int wid = hitWindow(x, y);
    if (wid < 0) { m_focused = -1; for (auto& w : m_windows) w.focused = false; return; }
    focus(wid);
    Window* w = get(wid);
    if (!w) return;
    // Resize zones (edges/corners, only when not maximized)
    if (!w->maximized) {
        const Rect& r = w->rc;
        int e = 8;
        int mode = 0;
        if (x >= r.x - e && x < r.x + e && y >= r.y - e && y < r.y + e) mode = 1;       // TL
        else if (x >= r.x + r.w - e && x < r.x + r.w + e && y >= r.y - e && y < r.y + e) mode = 3; // TR
        else if (x >= r.x - e && x < r.x + e && y >= r.y + r.h - e && y < r.y + r.h + e) mode = 6; // BL
        else if (x >= r.x + r.w - e && x < r.x + r.w + e && y >= r.y + r.h - e && y < r.y + r.h + e) mode = 8; // BR
        else if (y >= r.y - e && y < r.y + e) mode = 2;   // T
        else if (y >= r.y + r.h - e && y < r.y + r.h + e) mode = 7; // B
        else if (x >= r.x - e && x < r.x + e) mode = 4;   // L
        else if (x >= r.x + r.w - e && x < r.x + r.w + e) mode = 5; // R
        if (mode) {
            m_resizeWnd = wid;
            w->resizeMode = mode;
            w->resizeSX = x; w->resizeSY = y;
            w->resizeOrig = w->rc;
            return;
        }
    }
    // Title bar -> drag or buttons
    if (w->hitTitle(x, y)) {
        // Buttons region (right side: min / max / close)
        int bx = w->rc.x + w->rc.w - 46 * 3;
        if (x >= bx) return; // button click handled on mouseup
        // Hamburger menu (left)
        if (x <= w->rc.x + 48) return;
        w->dragging = true;
        w->dragDx = x - w->rc.x;
        w->dragDy = y - w->rc.y;
    }
    if (w->onMouse) w->onMouse(x, y, btn, 0);
}

void WindowManager::onMouseUp(int x, int y, int btn) {
    if (btn == 1) return;
    // Title buttons
    bool handled = false;
    if (m_focused >= 0) {
        Window* w = get(m_focused);
        if (w && !w->dragging && !w->resizeMode) {
            int bx = w->rc.x + w->rc.w - 46 * 3;
            int by = w->rc.y;
            if (x >= bx && x < bx + 46 && y >= by && y < by + w->titleBarH()) { minimize(w->id); handled = true; }
            else if (x >= bx + 46 && x < bx + 92 && y >= by && y < by + w->titleBarH()) { toggleMaximize(w->id); handled = true; }
            else if (x >= bx + 92 && x < bx + 138 && y >= by && y < by + w->titleBarH()) { close(w->id); handled = true; }
        }
    }
    if (!handled && m_dragWnd >= 0) {
        Window* w = get(m_dragWnd);
        if (w) {
            // Snap on release near edge
            SnapZone z = snapZoneAt(x, y);
            if (z != SnapZone::None && !w->maximized) toggleSnap(w->id, z);
            w->dragging = false;
        }
        m_dragWnd = -1;
    }
    if (m_resizeWnd >= 0) {
        Window* w = get(m_resizeWnd);
        if (w) w->resizeMode = 0;
        m_resizeWnd = -1;
    }
    if (m_focused >= 0) {
        Window* w = get(m_focused);
        if (w && w->onMouse) w->onMouse(x, y, btn, 1);
    }
}

void WindowManager::onMouseMove(int x, int y) {
    if (m_dragWnd >= 0) {
        Window* w = get(m_dragWnd);
        if (w) {
            w->rc.x = x - w->dragDx;
            w->rc.y = y - w->dragDy;
        }
        // snap preview
        if (w) {
            activeSnapPreview = snapZoneAt(x, y);
            snapPreviewWnd = w->id;
        }
        return;
    }
    if (m_resizeWnd >= 0) {
        Window* w = get(m_resizeWnd);
        if (w) {
            int dx = x - w->resizeSX, dy = y - w->resizeSY;
            Rect o = w->resizeOrig;
            switch (w->resizeMode) {
                case 1: w->rc = Rect(o.x + dx, o.y + dy, o.w - dx, o.h - dy); break;
                case 2: w->rc = Rect(o.x, o.y + dy, o.w, o.h - dy); break;
                case 3: w->rc = Rect(o.x, o.y + dy, o.w + dx, o.h - dy); break;
                case 4: w->rc = Rect(o.x + dx, o.y, o.w - dx, o.h); break;
                case 5: w->rc = Rect(o.x, o.y, o.w + dx, o.h); break;
                case 6: w->rc = Rect(o.x + dx, o.y, o.w - dx, o.h + dy); break;
                case 7: w->rc = Rect(o.x, o.y, o.w, o.h + dy); break;
                case 8: w->rc = Rect(o.x, o.y, o.w + dx, o.h + dy); break;
            }
            if (w->rc.w < 80) w->rc.w = 80;
            if (w->rc.h < 60) w->rc.h = 60;
        }
        return;
    }
    if (m_focused >= 0) {
        Window* w = get(m_focused);
        if (w && w->onMouse) w->onMouse(x, y, 0, 2); // hover move
    }
}

void WindowManager::onScroll(int x, int y, int dy) {
    int wid = hitWindow(x, y);
    if (wid < 0) return;
    Window* w = get(wid);
    if (w && w->onMouse) w->onMouse(x, y, dy, 3);
}

void WindowManager::onKey(char ch) {
    if (m_focused >= 0) {
        Window* w = get(m_focused);
        if (w && w->onKey) w->onKey(ch);
    }
}

// ---- Alt+Tab ----
void WindowManager::startAltTab() {
    m_altTabActive = true;
    m_altTabIndex = 0;
    altTabOrder.clear();
    for (int i = (int)m_windows.size() - 1; i >= 0; i--) {
        if (!m_windows[i].minimized && !m_windows[i].closing)
            altTabOrder.push_back(m_windows[i].id);
    }
    if (altTabOrder.empty() && !m_windows.empty()) {
        for (auto& w : m_windows)
            if (!w.closing) altTabOrder.push_back(w.id);
    }
}

void WindowManager::altTabStep(int dir) {
    if (altTabOrder.empty()) return;
    m_altTabIndex = (m_altTabIndex + dir + (int)altTabOrder.size()) % (int)altTabOrder.size();
}

void WindowManager::endAltTab(bool commit) {
    m_altTabActive = false;
    if (commit && !altTabOrder.empty()) {
        int id = altTabOrder[m_altTabIndex % altTabOrder.size()];
        focus(id);
        Window* w = get(id);
        if (w) w->minimized = false;
    }
    altTabOrder.clear();
}

// ---- Animations ----
bool WindowManager::tick(uint64_t nowMs) {
    if (m_lastNow == 0) m_lastNow = nowMs;
    float dt = (float)(nowMs - m_lastNow) / 1000.0f;
    m_lastNow = nowMs;
    if (dt <= 0.0f) dt = 0.016f;
    bool dirty = false;
    for (auto& w : m_windows) {
        if (w.opening) {
            w.openT += dt * (1000.0f / Anim::windowOpen);
            if (w.openT >= 1.0f) { w.openT = 1.0f; w.opening = false; }
            dirty = true;
        }
        if (w.closing) {
            w.closeT += dt * (1000.0f / Anim::windowClose);
            if (w.closeT >= 1.0f) { dirty = true; }
        }
        if (w.snapping) {
            w.snapT += dt * (1000.0f / Anim::snap);
            if (w.snapT >= 1.0f) { w.snapT = 1.0f; w.snapping = false; }
            // Lerp
            float t = w.snapT;
            w.rc.x = (int)(w.snapFrom.x + (w.snapTo.x - w.snapFrom.x) * t);
            w.rc.y = (int)(w.snapFrom.y + (w.snapTo.y - w.snapFrom.y) * t);
            w.rc.w = (int)(w.snapFrom.w + (w.snapTo.w - w.snapFrom.w) * t);
            w.rc.h = (int)(w.snapFrom.h + (w.snapTo.h - w.snapFrom.h) * t);
            dirty = true;
        }
    }
    // Remove fully closed
    for (size_t i = 0; i < m_windows.size();) {
        if (m_windows[i].closing && m_windows[i].closeT >= 1.0f) {
            int erasedId = m_windows[i].id;
            m_windows.erase(m_windows.begin() + i);
            if (m_focused == erasedId) m_focused = -1;
            dirty = true;
        } else i++;
    }
    return dirty;
}

// ---- Render ----
void WindowManager::drawTitleBar(Gfx& g, const Theme& t, const Window& w) {
    int h = w.titleBarH();
    Rect tb(w.rc.x, w.rc.y, w.rc.w, h);
    // Centered title
    g.textCentered(Rect(tb.x, tb.y, tb.w - 140, h), w.title, t.foreground, 2);
    // Hamburger menu button (left)
    Rect ham(tb.x + 12, tb.y + (h - 16) / 2, 16, 16);
    if (g_mouse && ham.contains(g_mouseX, g_mouseY))
        g.fillRounded(Rect(tb.x + 8, tb.y + 6, 26, h - 12), 6, t.hover);
    g.drawLineH(ham.x, ham.y + 3, 16, t.foreground);
    g.drawLineH(ham.x, ham.y + 7, 16, t.foreground);
    g.drawLineH(ham.x, ham.y + 11, 16, t.foreground);
    // Window buttons (right)
    int bx = tb.x + tb.w - 46 * 3;
    int by = tb.y;
    bool hoverMin = g_mouse && g_mouseX >= bx && g_mouseX < bx + 46 && g_mouseY >= by && g_mouseY < by + h;
    bool hoverMax = g_mouse && g_mouseX >= bx + 46 && g_mouseX < bx + 92 && g_mouseY >= by && g_mouseY < by + h;
    bool hoverClose = g_mouse && g_mouseX >= bx + 92 && g_mouseX < bx + 138 && g_mouseY >= by && g_mouseY < by + h;
    if (hoverMin) g.fillRect(Rect(bx, by, 46, h), t.hover);
    if (hoverMax) g.fillRect(Rect(bx + 46, by, 46, h), t.hover);
    if (hoverClose) g.fillRect(Rect(bx + 92, by, 46, h), Color(0xE8, 0x11, 0x23, 255));
    int iy = by + h / 2;
    // minimize (3 lines)
    g.drawLineH(bx + 15, iy - 5, 16, t.foreground);
    g.drawLineH(bx + 15, iy, 16, t.foreground);
    g.drawLineH(bx + 15, iy + 5, 16, t.foreground);
    // maximize (square)
    g.drawRect(Rect(bx + 46 + 14, iy - 7, 15, 13), t.foreground, 1);
    g.drawLineH(bx + 46 + 14, iy - 7, 15, t.foreground);
    // close X (two diagonals)
    Color xc(255, 255, 255);
    for (int d = 0; d < 12; d++) {
        g.drawPixel(bx + 92 + 16 + d, iy - 6 + d, xc);
        g.drawPixel(bx + 92 + 27 - d, iy - 6 + d, xc);
    }
}

void WindowManager::drawWindowFrame(Gfx& g, const Theme& t, const Window& w) {
    int rad = w.maximized ? 0 : 12;
    Rect rc = w.rc;
    // Shadow
    g.shadowBox(rc, rad, t.shadow, 10, 5);
    // Body
    g.fillRounded(rc, rad, t.window);
    // Title bar background (rounded top)
    if (!w.maximized) {
        g.fillRounded(Rect(rc.x, rc.y, rc.w, w.titleBarH()), rad, t.titleBar);
        // square off the bottom of the title bar strip
        g.fillRect(Rect(rc.x, rc.y + w.titleBarH() - rad, rad, rad), t.titleBar);
        g.fillRect(Rect(rc.x + rc.w - rad, rc.y + w.titleBarH() - rad, rad, rad), t.titleBar);
    } else {
        g.fillRect(Rect(rc.x, rc.y, rc.w, w.titleBarH()), t.titleBar);
    }
    // Border
    g.drawRounded(rc, rad, w.focused ? t.accent.withAlpha(200) : t.border, 1);
    // Content
    Rect content = w.client();
    if (w.onPaint) w.onPaint(g, t, w, content);
    // Title bar UI (buttons + title + hamburger) drawn last (on top)
    drawTitleBar(g, t, w);
    // Border line under title
    g.drawLineH(rc.x, rc.y + w.titleBarH() - 1, rc.w, t.border);
    // Active accent underline on title (Win11 style)
    if (w.focused)
        g.fillRect(Rect(rc.x + 4, rc.y + w.titleBarH() - 2, rc.w - 8, 2), t.accent);
}

void WindowManager::renderAll(Gfx& g, const Theme& t) {
    for (auto& w : m_windows) {
        if (w.minimized) continue;
        if (w.closing && w.closeT >= 1.0f) continue;
        drawWindowFrame(g, t, w);
    }
    // Snap preview overlay
    if (activeSnapPreview != SnapZone::None && snapPreviewWnd >= 0) {
        Window* w = get(snapPreviewWnd);
        if (w && !w->maximized) {
            Rect pr = snapRectFor(w->id, activeSnapPreview);
            g.fillRect(pr, t.accent.withAlpha(36));
            g.drawRect(pr, t.accent.withAlpha(220), 3);
        }
    }
    // Alt+Tab overlay
    if (m_altTabActive) {
        g.fillRect(Rect(0, 0, g_screen_w, g_screen_h), Color(0, 0, 0, 120));
        int n = (int)altTabOrder.size();
        int iw = 140, ih = 100, gap = 16;
        int total = n * (iw + gap) - gap;
        int sx = (g_screen_w - total) / 2;
        int sy = g_screen_h / 2 - ih / 2 - 40;
        for (int i = 0; i < n; i++) {
            Window* w = get(altTabOrder[i]);
            if (!w) continue;
            int x = sx + i * (iw + gap);
            bool sel = (i == m_altTabIndex);
            g.shadowBox(Rect(x, sy, iw, ih), 10, Color(0, 0, 0, 120), 6, 3);
            g.fillRounded(Rect(x, sy, iw, ih), 10, sel ? t.accent.withAlpha(200) : t.windowCard);
            g.drawRounded(Rect(x, sy, iw, ih), 10, sel ? t.accent : t.border, 2);
            g.appTile(x + iw / 2 - 20, sy + 14, 40, w->accent, w->title.substr(0, 1));
            g.textCentered(Rect(x, sy + ih - 30, iw, 22), w->title.substr(0, 14), t.foreground, 1);
        }
    }
}

// Global mouse state (set by shell for hover effects)
bool g_mouse = false;
int g_mouseX = 0;
int g_mouseY = 0;

} // namespace win11
