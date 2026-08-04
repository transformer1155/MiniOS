// =====================================================================
//  gfx.cpp - SDL2 software renderer implementation
// =====================================================================
#include "gfx.h"
#include <algorithm>
#include <cmath>

namespace win11 {

bool Gfx::init(int w, int h) {
    m_w = w; m_h = h;
    m_win = SDL_CreateWindow("MiniOS Win11 Desktop",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                             SDL_WINDOW_SHOWN);
    if (!m_win) return false;
    m_surf = SDL_GetWindowSurface(m_win);
    if (!m_surf) return false;
    // Standard desktop window surfaces are 32-bit; guard against 24-bit anyway.
    if (m_surf->format->BytesPerPixel != 4) {
        SDL_Surface* conv = SDL_ConvertSurfaceFormat(m_surf, SDL_PIXELFORMAT_ARGB8888, 0);
        if (conv) { m_surf = conv; }
    }
    return true;
}

void Gfx::destroy() {
    if (m_win) { SDL_DestroyWindow(m_win); m_win = nullptr; }
}

void Gfx::clear(const Color& c) {
    SDL_FillRect(m_surf, nullptr, SDL_MapRGBA(m_surf->format, c.r, c.g, c.b, c.a));
}

void Gfx::present() {
    if (m_win && m_surf) SDL_UpdateWindowSurface(m_win);
}

inline uint32_t packPixel(const SDL_PixelFormat* f, const Color& c) {
    return SDL_MapRGBA(f, c.r, c.g, c.b, c.a);
}

void Gfx::blendPixel(uint8_t* px, const Color& c, float alpha) {
    // px points to the 4-byte pixel (BGRA on little endian)
    uint8_t dr = px[2], dg = px[1], db = px[0];
    float sa = c.a / 255.0f * alpha;
    float ia = 1.0f - sa;
    px[0] = (uint8_t)(c.b * sa + db * ia);
    px[1] = (uint8_t)(c.g * sa + dg * ia);
    px[2] = (uint8_t)(c.r * sa + dr * ia);
    px[3] = 255;
}

void Gfx::fillRect(const Rect& r, const Color& c) {
    if (r.w <= 0 || r.h <= 0) return;
    Rect clip(r.x, r.y, r.w, r.h);
    if (clip.x < 0) { clip.w += clip.x; clip.x = 0; }
    if (clip.y < 0) { clip.h += clip.y; clip.y = 0; }
    if (clip.x + clip.w > m_w) clip.w = m_w - clip.x;
    if (clip.y + clip.h > m_h) clip.h = m_h - clip.y;
    if (clip.w <= 0 || clip.h <= 0) return;

    SDL_Rect sdl{clip.x, clip.y, clip.w, clip.h};
    if (c.a >= 254) {
        SDL_FillRect(m_surf, &sdl, packPixel(m_surf->format, c));
    } else {
        // Manual alpha blend
        uint8_t* base = (uint8_t*)m_surf->pixels;
        int pitch = m_surf->pitch;
        int bpp = m_surf->format->BytesPerPixel;
        for (int yy = clip.y; yy < clip.y + clip.h; yy++) {
            uint8_t* row = base + yy * pitch + clip.x * bpp;
            for (int xx = 0; xx < clip.w; xx++) {
                blendPixel(row + xx * bpp, c, 1.0f);
            }
        }
    }
}

void Gfx::drawPixel(int x, int y, const Color& c) {
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
    uint8_t* px = (uint8_t*)m_surf->pixels + y * m_surf->pitch + x * m_surf->format->BytesPerPixel;
    if (c.a >= 254) {
        px[0] = c.b; px[1] = c.g; px[2] = c.r; px[3] = 255;
    } else {
        blendPixel(px, c, 1.0f);
    }
}

void Gfx::drawRect(const Rect& r, const Color& c, int thickness) {
    for (int t = 0; t < thickness; t++) {
        drawLineH(r.x + t, r.y + t, r.w - 2 * t, c);
        drawLineH(r.x + t, r.y + r.h - 1 - t, r.w - 2 * t, c);
        drawLineV(r.x + t, r.y + t, r.h - 2 * t, c);
        drawLineV(r.x + r.w - 1 - t, r.y + t, r.h - 2 * t, c);
    }
}

void Gfx::drawLineH(int x, int y, int w, const Color& c) {
    if (y < 0 || y >= m_h) return;
    int x0 = std::max(x, 0);
    int x1 = std::min(x + w, m_w);
    uint8_t* row = (uint8_t*)m_surf->pixels + y * m_surf->pitch;
    int bpp = m_surf->format->BytesPerPixel;
    for (int i = x0; i < x1; i++) {
        uint8_t* px = row + i * bpp;
        if (c.a >= 254) { px[0] = c.b; px[1] = c.g; px[2] = c.r; px[3] = 255; }
        else blendPixel(px, c, 1.0f);
    }
}

void Gfx::drawLineV(int x, int y, int h, const Color& c) {
    if (x < 0 || x >= m_w) return;
    int y0 = std::max(y, 0);
    int y1 = std::min(y + h, m_h);
    uint8_t* base = (uint8_t*)m_surf->pixels;
    int pitch = m_surf->pitch;
    int bpp = m_surf->format->BytesPerPixel;
    for (int i = y0; i < y1; i++) {
        uint8_t* px = base + i * pitch + x * bpp;
        if (c.a >= 254) { px[0] = c.b; px[1] = c.g; px[2] = c.r; px[3] = 255; }
        else blendPixel(px, c, 1.0f);
    }
}

void Gfx::fillCircle(int cx, int cy, int radius, const Color& c) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
        drawLineH(cx - dx, cy + dy, dx * 2, c);
    }
}

void Gfx::fillRounded(const Rect& r, int radius, const Color& c) {
    if (r.w <= 0 || r.h <= 0) return;
    int rad = radius;
    if (rad > r.h / 2) rad = r.h / 2;
    if (rad > r.w / 2) rad = r.w / 2;
    if (rad <= 0) { fillRect(r, c); return; }
    // Center
    fillRect(Rect(r.x + rad, r.y, r.w - 2 * rad, r.h), c);
    // Side strips
    fillRect(Rect(r.x, r.y + rad, rad, r.h - 2 * rad), c);
    fillRect(Rect(r.x + r.w - rad, r.y + rad, rad, r.h - 2 * rad), c);
    // Corner quarter circles
    for (int dy = 0; dy < rad; dy++) {
        int dx = (int)std::sqrt((double)(rad * rad - (rad - 1 - dy) * (rad - 1 - dy)));
        if (dx > rad) dx = rad;
        // TL
        for (int i = 0; i <= dx; i++)
            drawPixel(r.x + rad - 1 - i, r.y + rad - 1 - dy, c);
        // TR
        for (int i = 0; i <= dx; i++)
            drawPixel(r.x + r.w - rad + i, r.y + rad - 1 - dy, c);
        // BL
        for (int i = 0; i <= dx; i++)
            drawPixel(r.x + rad - 1 - i, r.y + r.h - rad + dy, c);
        // BR
        for (int i = 0; i <= dx; i++)
            drawPixel(r.x + r.w - rad + i, r.y + r.h - rad + dy, c);
    }
}

void Gfx::drawRounded(const Rect& r, int radius, const Color& c, int thickness) {
    // Simplify: draw 4 straight edges + 4 corner pixels
    int rad = radius;
    if (rad > r.h / 2) rad = r.h / 2;
    if (rad > r.w / 2) rad = r.w / 2;
    for (int t = 0; t < thickness; t++) {
        drawLineH(r.x + rad, r.y + t, r.w - 2 * rad, c);
        drawLineH(r.x + rad, r.y + r.h - 1 - t, r.w - 2 * rad, c);
        drawLineV(r.x + t, r.y + rad, r.h - 2 * rad, c);
        drawLineV(r.x + r.w - 1 - t, r.y + rad, r.h - 2 * rad, c);
        // corner dots (approx)
        drawPixel(r.x + rad, r.y + rad, c);
        drawPixel(r.x + r.w - rad - 1, r.y + rad, c);
        drawPixel(r.x + rad, r.y + r.h - rad - 1, c);
        drawPixel(r.x + r.w - rad - 1, r.y + r.h - rad - 1, c);
    }
}

void Gfx::fillGradientV(const Rect& r, const Color& top, const Color& bot) {
    if (r.h <= 0) return;
    for (int i = 0; i < r.h; i++) {
        float t = (r.h <= 1) ? 0.5f : (float)i / (float)(r.h - 1);
        Color c(
            (uint8_t)(top.r + (bot.r - top.r) * t),
            (uint8_t)(top.g + (bot.g - top.g) * t),
            (uint8_t)(top.b + (bot.b - top.b) * t));
        drawLineH(r.x, r.y + i, r.w, c);
    }
}

void Gfx::fillGradientH(const Rect& r, const Color& left, const Color& right) {
    if (r.w <= 0) return;
    for (int i = 0; i < r.w; i++) {
        float t = (r.w <= 1) ? 0.5f : (float)i / (float)(r.w - 1);
        Color c(
            (uint8_t)(left.r + (right.r - left.r) * t),
            (uint8_t)(left.g + (right.g - left.g) * t),
            (uint8_t)(left.b + (right.b - left.b) * t));
        drawLineV(r.x + i, r.y, r.h, c);
    }
}

void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& c) {
    int minY = std::min({y0, y1, y2}), maxY = std::max({y0, y1, y2});
    auto edgeX = [&](int y, int ax, int ay, int bx, int by) -> int {
        if (by == ay) return ax;
        float t = (float)(y - ay) / (float)(by - ay);
        return (int)(ax + (bx - ax) * t);
    };
    for (int y = minY; y <= maxY; y++) {
        int xs[3] = { edgeX(y, x0, y0, x1, y1), edgeX(y, x1, y1, x2, y2), edgeX(y, x2, y2, x0, y0) };
        int lo = std::min({xs[0], xs[1], xs[2]});
        int hi = std::max({xs[0], xs[1], xs[2]});
        drawLineH(lo, y, hi - lo, c);
    }
}

void Gfx::shadowBox(const Rect& r, int radius, const Color& c, int blur, int offY) {
    // Multi-pass offset fill approximating soft shadow
    Rect sh(r.x + blur / 2, r.y + offY + blur / 4, r.w, r.h);
    for (int i = blur; i >= 1; i -= 2) {
        Color layer = c.withAlpha((uint8_t)(c.a * (blur - i + 1) / (float)blur * 0.5f));
        fillRounded(Rect(sh.x - i / 2, sh.y - i / 2, sh.w + i, sh.h + i), radius + i / 2, layer);
    }
    fillRounded(sh, radius, c.withAlpha((uint8_t)(c.a * 0.8f)));
}

void Gfx::text(int x, int y, const std::string& s, const Color& c, int scale) {
    if (scale < 1) scale = 1;
    int cx = x;
    for (char ch : s) {
        if (ch == '\n') { cx = x; y += 8 * scale; continue; }
        const uint8_t* g = glyph8x8(ch);
        for (int row = 0; row < 8; row++) {
            uint8_t bits = g[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) {
                    if (scale == 1) drawPixel(cx + col, y + row, c);
                    else fillRect(Rect(cx + col * scale, y + row * scale, scale, scale), c);
                }
            }
        }
        cx += 8 * scale;
    }
}

int Gfx::textWidth(const std::string& s, int scale) const {
    if (scale < 1) scale = 1;
    int maxW = 0, cur = 0;
    for (char ch : s) {
        if (ch == '\n') { maxW = std::max(maxW, cur); cur = 0; continue; }
        cur += 8 * scale;
    }
    return std::max(maxW, cur);
}

void Gfx::textCentered(const Rect& r, const std::string& s, const Color& c, int scale) {
    int tw = textWidth(s, scale);
    int th = textHeight(scale);
    text(r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2, s, c, scale);
}

void Gfx::textRight(int x, int y, const std::string& s, const Color& c, int scale) {
    text(x - textWidth(s, scale), y, s, c, scale);
}

void Gfx::blurRegion(const Rect& r, int radius) {
    if (radius <= 0) return;
    Rect c(r); // clipped
    if (c.x < 0) c.x = 0;
    if (c.y < 0) c.y = 0;
    if (c.x + c.w > m_w) c.w = m_w - c.x;
    if (c.y + c.h > m_h) c.h = m_h - c.y;
    if (c.w <= 0 || c.h <= 0) return;
    std::vector<uint32_t> tmp((size_t)c.w * c.h);
    uint8_t* base = (uint8_t*)m_surf->pixels;
    int pitch = m_surf->pitch;
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++) {
            uint8_t* px = base + (c.y + y) * pitch + (c.x + x) * 4;
            tmp[(size_t)y * c.w + x] = (uint32_t)px[2] | ((uint32_t)px[1] << 8) | ((uint32_t)px[0] << 16);
        }
    int rad = radius;
    std::vector<uint32_t> out(tmp.size());
    for (int y = 0; y < c.h; y++) {
        for (int x = 0; x < c.w; x++) {
            int rSum = 0, gSum = 0, bSum = 0, cnt = 0;
            for (int dy = -rad; dy <= rad; dy++) {
                for (int dx = -rad; dx <= rad; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= c.w || ny >= c.h) continue;
                    uint32_t p = tmp[(size_t)ny * c.w + nx];
                    rSum += (p & 0xFF); gSum += ((p >> 8) & 0xFF); bSum += ((p >> 16) & 0xFF);
                    cnt++;
                }
            }
            if (cnt == 0) cnt = 1;
            out[(size_t)y * c.w + x] = (uint32_t)(rSum / cnt) | ((uint32_t)(gSum / cnt) << 8) | ((uint32_t)(bSum / cnt) << 16);
        }
    }
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++) {
            uint8_t* px = base + (c.y + y) * pitch + (c.x + x) * 4;
            uint32_t p = out[(size_t)y * c.w + x];
            px[2] = p & 0xFF; px[1] = (p >> 8) & 0xFF; px[0] = (p >> 16) & 0xFF;
        }
}

void Gfx::appTile(int x, int y, int size, const Color& bg, const std::string& letter) {
    fillRounded(Rect(x, y, size, size), size / 4, bg);
    if (!letter.empty())
        textCentered(Rect(x, y, size, size), letter, Color(255, 255, 255), size / 14 > 0 ? size / 14 : 1);
}

void Gfx::winLogo(int cx, int cy, int size, const Color& c) {
    // Four-pane window logo
    int q = size / 2;
    fillRect(Rect(cx - size / 2, cy - size / 2, q - 1, q - 1), c);
    fillRect(Rect(cx, cy - size / 2, q - 1, q - 1), c);
    fillRect(Rect(cx - size / 2, cy, q - 1, q - 1), c);
    fillRect(Rect(cx, cy, q - 1, q - 1), c);
}

bool Gfx::saveBmp(const std::string& path) {
    if (!m_surf) return false;
    return SDL_SaveBMP(m_surf, path.c_str()) == 0;
}

bool Gfx::savePng(const std::string& path) {
    // SDL_image may not be linked; fall back to BMP
    return saveBmp(path);
}

} // namespace win11
