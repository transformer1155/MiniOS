// =====================================================================
//  gfx.h - SDL2 software renderer wrapper (Win11 visual primitives)
//  Rounded rects, soft shadows, gradients, bitmap text, app icons.
// =====================================================================
#pragma once
#include "theme.h"
#include "font8x8.h"
#include <SDL.h>
#include <string>
#include <vector>

namespace win11 {

// ---- Global screen state (set by shell) ----
extern int g_screen_w;
extern int g_screen_h;
// Mouse hover state (used for hover effects)
extern bool g_mouse;
extern int g_mouseX;
extern int g_mouseY;

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) {}
    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    bool intersects(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y;
    }
};

// ---- Gfx: all drawing goes to an offscreen RGBA surface, then presented ----
class Gfx {
public:
    bool init(int w, int h);
    void destroy();
    void clear(const Color& c);
    void present();   // blit surface to window

    int width() const { return m_w; }
    int height() const { return m_h; }

    // Primitives
    void fillRect(const Rect& r, const Color& c);
    void fillRect(int x, int y, int w, int h, const Color& c) { fillRect(Rect(x, y, w, h), c); }
    void drawRect(const Rect& r, const Color& c, int thickness = 1);
    void fillRounded(const Rect& r, int radius, const Color& c);
    void drawRounded(const Rect& r, int radius, const Color& c, int thickness = 1);
    void fillCircle(int cx, int cy, int radius, const Color& c);
    void fillGradientV(const Rect& r, const Color& top, const Color& bot);
    void fillGradientH(const Rect& r, const Color& left, const Color& right);
    void drawLineH(int x, int y, int w, const Color& c);
    void drawLineV(int x, int y, int h, const Color& c);
    void drawPixel(int x, int y, const Color& c);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& c);

    // Soft shadow: draw blurred-feel shadow box behind a rounded rect
    void shadowBox(const Rect& r, int radius, const Color& c, int blur = 8, int offY = 4);

    // Text (bitmap 8x8 font, scaled)
    void text(int x, int y, const std::string& s, const Color& c, int scale = 1);
    int textWidth(const std::string& s, int scale = 1) const;
    int textHeight(int scale = 1) const { return 8 * scale; }
    void textCentered(const Rect& r, const std::string& s, const Color& c, int scale = 1);
    void textRight(int x, int y, const std::string& s, const Color& c, int scale = 1);

    // Blur a region (for Mica/Acrylic feel) - simple box blur on surface
    void blurRegion(const Rect& r, int radius);

    // Icon glyphs: simple colored square + letter (app tile)
    void appTile(int x, int y, int size, const Color& bg, const std::string& letter);
    void winLogo(int cx, int cy, int size, const Color& c);

    // Save current surface to PNG (selftest). Returns false if SDL_image missing -> try BMP.
    bool savePng(const std::string& path);
    bool saveBmp(const std::string& path);

    SDL_Surface* surface() { return m_surf; }

private:
    void blendPixel(uint8_t* px, const Color& c, float alpha);
    SDL_Window* m_win = nullptr;
    SDL_Surface* m_surf = nullptr;
    int m_w = 0, m_h = 0;
};

} // namespace win11
