// =====================================================================
//  theme.h - Win11 Design Tokens
//  Light/Dark themes, accent colors, typography scale, animation timing.
//  Follows the Win11 spec: Light accent #0067C0, Dark accent #4CC2FF.
// =====================================================================
#pragma once
#include <cstdint>
#include <string>

namespace win11 {

// ---- Color ----
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color() = default;
    constexpr Color(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa = 255)
        : r(rr), g(gg), b(bb), a(aa) {}
    // Parse "#RRGGBB" / "RRGGBB"
    static Color fromHex(const std::string& h) {
        Color c;
        size_t off = (!h.empty() && h[0] == '#') ? 1 : 0;
        auto hex = [&](size_t i) -> uint8_t {
            auto v = [](char ch) -> int {
                if (ch >= '0' && ch <= '9') return ch - '0';
                if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                return 0;
            };
            return (uint8_t)((v(h[off + i]) << 4) | v(h[off + i + 1]));
        };
        if (h.size() - off >= 6) { c.r = hex(0); c.g = hex(2); c.b = hex(4); }
        return c;
    }
    Color withAlpha(uint8_t aa) const { Color c = *this; c.a = aa; return c; }
};

// ---- Theme ----
struct Theme {
    // Basics
    Color background;      // desktop / app background
    Color foreground;      // primary text
    Color foreground2;     // secondary text
    Color accent;          // system accent
    Color accentHover;
    Color accentPressed;
    Color hover;           // hover highlight surface
    Color pressed;         // pressed surface
    Color disabled;
    Color window;          // window body
    Color windowCard;      // card / menu surface
    Color titleBar;        // window title bar
    Color border;          // window border
    Color taskbar;         // taskbar surface
    Color taskbarHover;
    Color shadow;          // shadow color (with alpha)
    Color menuSurface;     // context menu / flyout surface
    Color startMenu;       // start menu surface
    Color inputBg;
    Color selection;       // text selection / highlight
    Color scrollTrack;
    Color scrollThumb;
    // Mica / Acrylic feel (opacity applied at draw time)
    float micaOpacity = 0.85f;
    float acrylicOpacity = 0.80f;
    Color micaTint;        // e.g. #C0C0C0
    Color acrylicTint;     // e.g. #F0F0F0
    Color acrylicBorder;   // 0.5px white border feel

    bool dark = false;

    // ---- Font sizes (px, based on 11pt body at 96dpi) ----
    int fCaption = 12;     // 9pt
    int fBody   = 15;      // 11pt
    int fSubtitle = 18;    // 13pt
    int fTitle  = 20;      // 15pt
    int fHeading = 27;     // 20pt
    int fDisplay = 37;     // 28pt
    // Font family name (used by host font loader; bitmap fallback in gfx)
    std::string fontFamily = "Segoe UI";
};

// ---- Predefined themes (Win11 spec) ----
inline Theme makeLightTheme() {
    Theme t;
    t.dark = false;
    t.background  = Color::fromHex("#FFFFFF");
    t.foreground  = Color::fromHex("#000000");
    t.foreground2 = Color::fromHex("#5B5B5B");
    t.accent      = Color::fromHex("#0067C0");
    t.accentHover = Color::fromHex("#0B74CC");
    t.accentPressed = Color::fromHex("#00509E");
    t.hover       = Color::fromHex("#E5F3FF");
    t.pressed     = Color::fromHex("#CCE3FF");
    t.disabled    = Color::fromHex("#9E9E9E");
    t.window      = Color::fromHex("#F3F3F3");
    t.windowCard  = Color::fromHex("#FFFFFF");
    t.titleBar    = Color::fromHex("#FFFFFF");
    t.border      = Color::fromHex("#E5E5E5");
    t.taskbar     = Color(243, 243, 243, 230);
    t.taskbarHover= Color::fromHex("#E5E5E5");
    t.shadow      = Color(0, 0, 0, 26);   // rgba(0,0,0,0.1)
    t.menuSurface = Color::fromHex("#FBFBFB");
    t.startMenu   = Color(243, 243, 243, 245);
    t.inputBg     = Color::fromHex("#FFFFFF");
    t.selection   = Color::fromHex("#CCE4FF");
    t.scrollTrack = Color(0, 0, 0, 10);
    t.scrollThumb = Color(0, 0, 0, 60);
    t.micaTint    = Color::fromHex("#C0C0C0");
    t.acrylicTint = Color::fromHex("#F0F0F0");
    t.acrylicBorder = Color(255, 255, 255, 128);
    return t;
}

inline Theme makeDarkTheme() {
    Theme t;
    t.dark = true;
    t.background  = Color::fromHex("#202020");
    t.foreground  = Color::fromHex("#FFFFFF");
    t.foreground2 = Color::fromHex("#A0A0A0");
    t.accent      = Color::fromHex("#4CC2FF");
    t.accentHover = Color::fromHex("#66CCFF");
    t.accentPressed = Color::fromHex("#2FA8E0");
    t.hover       = Color::fromHex("#383838");
    t.pressed     = Color::fromHex("#505050");
    t.disabled    = Color::fromHex("#6E6E6E");
    t.window      = Color::fromHex("#2B2B2B");
    t.windowCard  = Color::fromHex("#2B2B2B");
    t.titleBar    = Color::fromHex("#202020");
    t.border      = Color::fromHex("#444444");
    t.taskbar     = Color(32, 32, 32, 235);
    t.taskbarHover= Color::fromHex("#383838");
    t.shadow      = Color(0, 0, 0, 77);   // rgba(0,0,0,0.3)
    t.menuSurface = Color::fromHex("#2B2B2B");
    t.startMenu   = Color(32, 32, 32, 240);
    t.inputBg     = Color::fromHex("#3C3C3C");
    t.selection   = Color::fromHex("#1F4E79");
    t.scrollTrack = Color(255, 255, 255, 12);
    t.scrollThumb = Color(255, 255, 255, 70);
    t.micaTint    = Color::fromHex("#404040");
    t.acrylicTint = Color::fromHex("#303030");
    t.acrylicBorder = Color(255, 255, 255, 40);
    return t;
}

// ---- Animation timings (ms) ----
struct Anim {
    static constexpr int windowOpen   = 200;
    static constexpr int windowClose  = 150;
    static constexpr int minimize     = 300;
    static constexpr int maximize     = 200;
    static constexpr int snap         = 150;
    static constexpr int menuOpen     = 100;
    static constexpr int menuClose    = 75;
    static constexpr int subMenu      = 150;
    static constexpr int hover        = 100;
    static constexpr int focus        = 150;
    static constexpr int click        = 50;
    static constexpr int scroll       = 300;
    static constexpr int flyout       = 150;  // notification center / widgets
};

// ---- Snap layout regions ----
enum class SnapZone : int { None = 0, Left, Right, Top, TopLeft, TopRight, BottomLeft, BottomRight, Maximize };

} // namespace win11
