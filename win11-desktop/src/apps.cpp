// =====================================================================
//  apps.cpp - Built-in app content (attached to windows via onPaint)
//  Settings, File Explorer, Calculator, Notepad, Terminal, Task Manager
// =====================================================================
#include "window.h"
#include "gfx.h"
#include "theme.h"
#include "shell.h"
#include <string>
#include <vector>

namespace win11 {

// Theme switch callback provided by main (shell.dark)
extern Shell* g_shell;

// Simple nav row: [x, y, w, h] label list helper
static void drawNavList(Gfx& g, const Theme& t, const Rect& r,
                        const std::vector<std::string>& items, int selected, int scroll) {
    // Left nav pane background
    g.fillRect(r, t.background);
    int y = r.y + 8 - scroll;
    for (int i = 0; i < (int)items.size(); i++) {
        if (y + 28 < r.y || y > r.y + r.h) { y += 36; continue; }
        Rect item(r.x + 4, y, r.w - 8, 32);
        if (i == selected)
            g.fillRounded(item, 6, t.hover);
        g.text(item.x + 12, y + 8, items[i], t.foreground, 1);
        y += 36;
    }
}

// =====================================================================
//  Settings
// =====================================================================
static void paintSettings(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    // Left nav
    static const std::vector<std::string> nav = {
        "System", "Bluetooth & devices", "Network & internet",
        "Personalization", "Apps", "Accounts", "Time & language",
        "Gaming", "Accessibility", "Privacy & security", "Windows Update",
    };
    drawNavList(g, t, Rect(c.x, c.y, 200, c.h), nav, 0, 0);
    g.drawLineV(c.x + 200, c.y, c.h, t.border);
    // Content
    int cx = c.x + 220;
    g.text(cx, c.y + 12, "System", t.foreground, 3);
    g.drawLineH(cx, c.y + 44, c.w - 240, t.border);
    int y = c.y + 64;
    struct Row { const char* name; const char* val; };
    Row rows[] = {
        {"Display", "1920 x 1080 (Recommended)"},
        {"Sound", "Speakers (Realtek)"},
        {"Notifications", "On"},
        {"Focus assist", "Off"},
        {"Power & battery", "Battery saver off"},
        {"Storage", "SSD 256 GB - 62% used"},
        {"Multitasking", "Snap layouts on"},
        {"About", "MiniOS 11 Home"},
    };
    for (auto& r : rows) {
        Rect row(cx, y, c.w - 260, 44);
        if (g_mouse && row.contains(g_mouseX, g_mouseY))
            g.fillRounded(row, 6, t.hover);
        g.text(row.x + 10, y + 12, r.name, t.foreground, 1);
        g.textRight(row.x + row.w - 10, y + 12, r.val, t.foreground2, 1);
        y += 52;
    }
    // Theme card
    Rect themeCard(cx, y + 8, c.w - 260, 96);
    g.fillRounded(themeCard, 10, t.windowCard);
    g.drawRounded(themeCard, 10, t.border, 1);
    g.text(themeCard.x + 14, themeCard.y + 12, "Appearance", t.foreground, 2);
    g.text(themeCard.x + 14, themeCard.y + 40,
           g_shell && g_shell->isDark() ? "Dark mode" : "Light mode", t.foreground2, 1);
    // Theme toggle button
    Rect btn(themeCard.x + themeCard.w - 120, themeCard.y + 34, 100, 32);
    g.fillRounded(btn, 8, t.accent);
    g.textCentered(btn, "Switch", Color(255, 255, 255), 1);
}

// =====================================================================
//  File Explorer
// =====================================================================
struct FakeFile { std::string name; bool folder; int size; };
static const std::vector<FakeFile>& fakeFiles() {
    static std::vector<FakeFile> files = {
        {"Documents", true, 0}, {"Downloads", true, 0}, {"Pictures", true, 0},
        {"Music", true, 0}, {"Videos", true, 0}, {"Desktop", true, 0},
        {"report.txt", false, 1240}, {"photo.png", false, 8842},
        {"notes.md", false, 512}, {"iso.img", false, 4096},
    };
    return files;
}

static void paintExplorer(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    // Command bar
    Rect cmd(c.x, c.y, c.w, 44);
    g.fillRect(cmd, t.titleBar);
    g.fillRounded(Rect(cmd.x + 12, cmd.y + 8, 90, 28), 6, t.hover);
    g.text(cmd.x + 24, cmd.y + 14, "<  New", t.foreground, 1);
    g.fillRounded(Rect(cmd.x + 108, cmd.y + 8, 40, 28), 6, t.hover);
    g.textCentered(Rect(cmd.x + 108, cmd.y + 8, 40, 28), "Cut", t.foreground, 1);
    g.fillRounded(Rect(cmd.x + 152, cmd.y + 8, 46, 28), 6, t.hover);
    g.textCentered(Rect(cmd.x + 152, cmd.y + 8, 46, 28), "Copy", t.foreground, 1);
    // Address bar (breadcrumb)
    Rect addr(c.x + 12, c.y + 48, c.w - 24, 34);
    g.fillRounded(addr, 8, t.inputBg);
    g.drawRounded(addr, 8, t.border, 1);
    g.text(addr.x + 12, addr.y + 9, "This PC > MiniOS (C:) > Desktop", t.foreground, 1);
    // Nav pane
    static const std::vector<std::string> nav = {
        "Quick access", "OneDrive", "This PC", "Network", "Libraries",
    };
    drawNavList(g, t, Rect(c.x, c.y + 92, 180, c.h - 92), nav, 2, 0);
    g.drawLineV(c.x + 180, c.y + 92, c.h - 92, t.border);
    // File grid
    int gx = c.x + 196, gy = c.y + 100;
    int col = 0, row = 0;
    for (auto& f : fakeFiles()) {
        int x = gx + col * 100, y = gy + row * 96;
        if (f.folder) {
            g.fillRounded(Rect(x + 20, y, 56, 8), 2, Color::fromHex("#E8B84B"));
            g.fillRounded(Rect(x + 8, y + 8, 80, 58), 8, Color::fromHex("#E8B84B"));
        } else {
            g.fillRounded(Rect(x + 8, y, 80, 66), 8, Color::fromHex("#8AB4F8"));
            g.textCentered(Rect(x + 8, y, 80, 66), f.name.substr(0, 1), Color(255, 255, 255), 3);
        }
        g.textCentered(Rect(x, y + 70, 96, 24), f.name.substr(0, 10), t.foreground, 1);
        col++;
        if (col >= 5) { col = 0; row++; }
    }
    // Status bar
    g.fillRect(Rect(c.x, c.y + c.h - 28, c.w, 28), t.titleBar);
    g.text(c.x + 12, c.y + c.h - 22, "10 items", t.foreground2, 1);
    g.textRight(c.x + c.w - 12, c.y + c.h - 22, "Free space: 96.2 GB of 256 GB", t.foreground2, 1);
}

// =====================================================================
//  Calculator
// =====================================================================
struct CalcState {
    double acc = 0.0;
    double cur = 0.0;
    char op = 0;
    bool fresh = true;
    std::string disp = "0";
};
static CalcState g_calc;
static int g_calcWnd = -1;

static void calcPress(char key) {
    if (g_calcWnd < 0) return;
    switch (key) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            if (g_calc.fresh) { g_calc.cur = 0; g_calc.fresh = false; }
            g_calc.cur = g_calc.cur * 10 + (key - '0');
            break;
        }
        case '.': {
            if (g_calc.fresh) { g_calc.cur = 0; g_calc.fresh = false; }
            g_calc.cur += 0.5; // simple decimal hint
            break;
        }
        case '+': case '-': case '*': case '/': {
            if (g_calc.op && !g_calc.fresh) {
                double a = g_calc.acc, b = g_calc.cur;
                if (g_calc.op == '+') g_calc.acc = a + b;
                else if (g_calc.op == '-') g_calc.acc = a - b;
                else if (g_calc.op == '*') g_calc.acc = a * b;
                else if (g_calc.op == '/' && b != 0) g_calc.acc = a / b;
            } else {
                g_calc.acc = g_calc.cur;
            }
            g_calc.op = key;
            g_calc.fresh = true;
            g_calc.disp = std::to_string((long long)g_calc.acc);
            break;
        }
        case '=': {
            double a = g_calc.acc, b = g_calc.cur;
            if (g_calc.op == '+') g_calc.acc = a + b;
            else if (g_calc.op == '-') g_calc.acc = a - b;
            else if (g_calc.op == '*') g_calc.acc = a * b;
            else if (g_calc.op == '/' && b != 0) g_calc.acc = a / b;
            g_calc.op = 0;
            g_calc.fresh = true;
            g_calc.disp = std::to_string((long long)g_calc.acc);
            g_calc.cur = g_calc.acc;
            break;
        }
        case 'C':
            g_calc.acc = 0; g_calc.cur = 0; g_calc.op = 0;
            g_calc.fresh = true; g_calc.disp = "0";
            break;
        default: break;
    }
    // Update display from state if needed
    if (key >= '0' && key <= '9')
        g_calc.disp = std::to_string((long long)g_calc.cur);
}

static void paintCalculator(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    g_calcWnd = w.id;
    // Display
    Rect disp(c.x + 12, c.y + 12, c.w - 24, 64);
    g.fillRounded(disp, 10, t.inputBg);
    g.textRight(disp.x + disp.w - 16, disp.y + 20, g_calc.disp, t.foreground, 3);
    // Buttons grid: 4 cols
    const char* keys[5][4] = {
        {"C", "(", ")", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "=", "="},
    };
    int bw = (c.w - 36) / 4, bh = (c.h - 110) / 5;
    for (int r = 0; r < 5; r++) {
        for (int cc = 0; cc < 4; cc++) {
            Rect btn(c.x + 12 + cc * (bw + 4), c.y + 88 + r * (bh + 4), bw, bh);
            bool hover = g_mouse && btn.contains(g_mouseX, g_mouseY);
            bool op = (keys[r][cc][0] == '+' || keys[r][cc][0] == '-' ||
                       keys[r][cc][0] == '*' || keys[r][cc][0] == '/' ||
                       keys[r][cc][0] == '=');
            g.fillRounded(btn, 10, op ? t.accent : (hover ? t.hover : t.windowCard));
            g.drawRounded(btn, 10, op ? t.accent : t.border, 1);
            g.textCentered(btn, keys[r][cc], op ? Color(255, 255, 255) : t.foreground, 2);
        }
    }
    // Click handling: track mouse down in region (simplified - handled by onMouse below)
    (void)0;
}

static void calcOnMouse(int x, int y, int btn, int state) {
    if (state != 0 || btn != 0) return;
    Window* w = g_shell ? g_shell->wm.get(g_calcWnd) : nullptr;
    if (!w) return;
    Rect c = w->client();
    // Find which button was pressed
    int bw = (c.w - 36) / 4, bh = (c.h - 110) / 5;
    for (int r = 0; r < 5; r++) {
        for (int cc = 0; cc < 4; cc++) {
            Rect btn(c.x + 12 + cc * (bw + 4), c.y + 88 + r * (bh + 4), bw, bh);
            if (btn.contains(x, y)) {
                const char* keys[5][4] = {
                    {"C", "(", ")", "/"},
                    {"7", "8", "9", "*"},
                    {"4", "5", "6", "-"},
                    {"1", "2", "3", "+"},
                    {"0", ".", "=", "="},
                };
                calcPress(keys[r][cc][0]);
                return;
            }
        }
    }
}

// =====================================================================
//  Notepad
// =====================================================================
static std::string g_noteText = "Welcome to MiniOS Notepad!\n\nType here...\n";
static void paintNotepad(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    g.fillRect(c, Color(255, 255, 255));
    // Line numbers
    int ln = 1;
    for (int i = 0; i < (int)g_noteText.size(); i++) if (g_noteText[i] == '\n') ln++;
    g.fillRect(Rect(c.x, c.y, 34, c.h), t.hover.withAlpha(90));
    int y = c.y + 8;
    for (int i = 1; i <= ln; i++) {
        g.textRight(c.x + 30, y, std::to_string(i), t.foreground2, 1);
        y += 18;
    }
    // Text
    int tx = c.x + 44, ty = c.y + 6;
    std::string cur;
    for (char ch : g_noteText) {
        if (ch == '\n') { g.text(tx, ty, cur, Color(20, 20, 20), 1); cur.clear(); ty += 18; }
        else cur += ch;
    }
    if (!cur.empty()) g.text(tx, ty, cur, Color(20, 20, 20), 1);
}

// =====================================================================
//  Terminal
// =====================================================================
static std::vector<std::string> g_termLines = {
    "MiniOS Terminal - Windows PowerShell 7",
    "Copyright (C) MiniOS. All rights reserved.",
    "",
    "PS C:\\Users\\User> ",
};
static void paintTerminal(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    (void)t;
    g.fillRect(c, Color(12, 12, 12));
    int y = c.y + 10;
    for (auto& ln : g_termLines) {
        g.text(c.x + 14, y, ln, Color(204, 204, 204), 1);
        y += 20;
    }
}

// =====================================================================
//  Task Manager
// =====================================================================
struct Proc { const char* name; int cpu; int mem; };
static const Proc g_procs[] = {
    {"System", 3, 128}, {"explorer.exe", 2, 512}, {"win11desktop", 8, 96},
    {"chrome.exe", 12, 640}, {"python.exe", 5, 220}, {"code.exe", 9, 512},
    {"dwm.exe", 2, 90}, {"svchost.exe", 1, 60},
};
static const int g_procsCount = (int)(sizeof(g_procs) / sizeof(g_procs[0]));

static void paintTaskMgr(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    // Tabs
    const char* tabs[] = {"Processes", "Performance", "App history", "Startup", "Details"};
    int tx = c.x + 10;
    for (int i = 0; i < 5; i++) {
        Rect tab(tx, c.y + 6, 92, 30);
        if (i == 0) { g.fillRounded(tab, 6, t.accent); g.textCentered(tab, tabs[i], Color(255,255,255), 1); }
        else g.textCentered(tab, tabs[i], t.foreground2, 1);
        tx += 96;
    }
    g.drawLineH(c.x, c.y + 40, c.w, t.border);
    // Header
    g.text(c.x + 12, c.y + 48, "Name", t.foreground2, 1);
    g.text(c.x + c.w / 2 - 40, c.y + 48, "Status", t.foreground2, 1);
    g.text(c.x + c.w / 2 + 80, c.y + 48, "CPU", t.foreground2, 1);
    g.text(c.x + c.w - 100, c.y + 48, "Memory", t.foreground2, 1);
    g.drawLineH(c.x, c.y + 64, c.w, t.border);
    int y = c.y + 72;
    for (int i = 0; i < g_procsCount && y < c.y + c.h - 30; i++) {
        Rect row(c.x + 4, y, c.w - 8, 28);
        if (g_mouse && row.contains(g_mouseX, g_mouseY)) g.fillRect(row, t.hover);
        g.text(c.x + 12, y + 6, g_procs[i].name, t.foreground, 1);
        g.text(c.x + c.w / 2 - 40, y + 6, "Running", t.foreground2, 1);
        g.text(c.x + c.w / 2 + 80, y + 6, std::to_string(g_procs[i].cpu) + "%", t.foreground, 1);
        g.text(c.x + c.w - 100, y + 6, std::to_string(g_procs[i].mem) + " MB", t.foreground, 1);
        y += 32;
    }
}

// =====================================================================
//  About
// =====================================================================
static void paintAbout(Gfx& g, const Theme& t, const Window& w, const Rect& c) {
    (void)w;
    g.appTile(c.x + c.w / 2 - 32, c.y + 24, 64, Color::fromHex("#0078D7"), "M");
    g.textCentered(Rect(c.x, c.y + 100, c.w, 30), "MiniOS 11", t.foreground, 3);
    g.textCentered(Rect(c.x, c.y + 140, c.w, 20), "Version 11 Home (Build 22631)", t.foreground2, 1);
    g.textCentered(Rect(c.x, c.y + 170, c.w, 20), "Windows 11 style desktop simulator", t.foreground2, 1);
    g.textCentered(Rect(c.x, c.y + 200, c.w, 20), "(C) 2026 MiniOS", t.foreground2, 1);
}

// =====================================================================
//  attachAppContent - bind app painters to a window
// =====================================================================
void attachAppContent(WindowManager& wm, int wndId, int app) {
    Window* w = wm.get(wndId);
    if (!w) return;
    switch (app) {
        case 0: w->onPaint = paintSettings; break;
        case 1: w->onPaint = paintExplorer; break;
        case 2: w->onPaint = paintCalculator; w->onMouse = calcOnMouse; break;
        case 3: w->onPaint = paintNotepad; break;
        case 4: w->onPaint = paintTerminal; break;
        case 5: w->onPaint = paintTaskMgr; break;
        case 6: w->onPaint = paintAbout; break;
        default: break;
    }
}

} // namespace win11
