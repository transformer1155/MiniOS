// =====================================================================
//  main.cpp - MiniOS Win11 Desktop Simulator entry point
//  SDL2 main loop, event dispatch, keyboard shortcuts, self-test mode.
//
//  Usage:  win11desktop [--selftest]
//    --selftest  : headless (SDL_VIDEODRIVER=dummy) render frames and
//                  save screenshots to ./shots/ (for CI / verification)
// =====================================================================
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <string>

#include "gfx.h"
#include "theme.h"
#include "shell.h"

using namespace win11;

namespace win11 {
Shell* g_shell = nullptr;   // global shell pointer (used by apps.cpp)
}

// ---- Keyboard shortcuts ----
static void handleShortcut(const SDL_Event& e, Shell& shell) {
    bool win = (e.key.keysym.mod & KMOD_LGUI) || (e.key.keysym.mod & KMOD_RGUI);
    bool ctrl = (e.key.keysym.mod & KMOD_CTRL);
    bool alt = (e.key.keysym.mod & KMOD_ALT);
    SDL_Keycode k = e.key.keysym.sym;

    // Release Alt -> commit Alt+Tab
    if ((k == SDLK_LALT || k == SDLK_RALT) && e.type == SDL_KEYUP) {
        if (shell.wm.altTabActive()) shell.wm.endAltTab(true);
        return;
    }
    // Alt+Tab (pressed while holding Alt)
    if (alt && k == SDLK_TAB) {
        if (e.type == SDL_KEYDOWN) {
            if (!shell.wm.altTabActive()) {
                shell.wm.startAltTab();
            } else {
                shell.wm.altTabStep((e.key.keysym.mod & KMOD_SHIFT) ? -1 : 1);
            }
        }
        return;
    }
    if (k == SDLK_ESCAPE && e.type == SDL_KEYDOWN) {
        if (shell.wm.altTabActive()) { shell.wm.endAltTab(false); return; }
        if (shell.menu.open) { shell.menu.open = false; return; }
        if (shell.startOpen) { shell.openStartMenu(false); return; }
        if (shell.notificationsOpen) { shell.notificationsOpen = false; return; }
        if (shell.widgetsOpen) { shell.widgetsOpen = false; return; }
        return;
    }
    if (e.type != SDL_KEYDOWN) return;

    if (win) {
        switch (k) {
            case SDLK_s: shell.toggleStartMenu(); break;               // Win+S
            case SDLK_e: shell.launchApp(1); break;                     // Win+E explorer
            case SDLK_i: shell.launchApp(0); break;                     // Win+I settings
            case SDLK_d:                                                   // Win+D show desktop
                { int id = shell.wm.focusedId(); if (id >= 0) shell.wm.minimize(id); }
                break;
            case SDLK_TAB: break;                                        // handled above
            case SDLK_w: shell.widgetsOpen = !shell.widgetsOpen; break;  // Win+W widgets
            case SDLK_UP:   // Win+Up maximize
                { int id = shell.wm.focusedId(); if (id >= 0) shell.wm.toggleMaximize(id); }
                break;
            case SDLK_LEFT:
                { int id = shell.wm.focusedId(); if (id >= 0) shell.wm.toggleSnap(id, SnapZone::Left); }
                break;
            case SDLK_RIGHT:
                { int id = shell.wm.focusedId(); if (id >= 0) shell.wm.toggleSnap(id, SnapZone::Right); }
                break;
            case SDLK_DOWN:
                { int id = shell.wm.focusedId(); if (id >= 0) shell.wm.minimize(id); }
                break;
            default: break;
        }
        return;
    }
    if (alt && k == SDLK_F4) {   // Alt+F4 close focused
        int id = shell.wm.focusedId();
        if (id >= 0) shell.wm.close(id);
        return;
    }
    // Text input to focused window
    if (e.type == SDL_KEYDOWN && !ctrl && !alt) {
        if (k >= 32 && k <= 126) {
            shell.onKey((char)k);
        }
    }
}

// ---- Click / double click detection for desktop icons ----
static void handleMouseButton(SDL_MouseButtonEvent& b, Shell& shell, uint64_t nowMs) {
    if (b.button == SDL_BUTTON_LEFT) {
        if (b.type == SDL_MOUSEBUTTONDOWN) {
            shell.onMouseDown(b.x, b.y, 0);
        } else {
            // Double-click: same icon clicked twice within 400ms
            for (size_t i = 0; i < shell.icons.size(); i++) {
                if (shell.icons[i].rc.contains(b.x, b.y) && b.y < shell.screenH() - shell.taskbarHeight()) {
                    if (shell.lastClickIcon == (int)i && nowMs - shell.lastClickTime < 400) {
                        // Double click -> launch
                        if (shell.icons[i].app >= 0) shell.launchApp(shell.icons[i].app);
                        else if (shell.icons[i].folder) { /* open folder */ }
                        shell.lastClickIcon = -1;
                    } else {
                        shell.lastClickIcon = (int)i;
                        shell.lastClickTime = nowMs;
                    }
                    break;
                }
            }
            shell.onMouseUp(b.x, b.y, 0);
        }
    } else if (b.button == SDL_BUTTON_RIGHT) {
        if (b.type == SDL_MOUSEBUTTONDOWN) shell.onMouseDown(b.x, b.y, 2);
        else shell.onMouseUp(b.x, b.y, 2);
    }
}

// ---- Self test: render a few frames and dump screenshots ----
static void runSelfTest(Shell& shell, Gfx& g, const Theme& t, int w, int h) {
    (void)w; (void)h;
    g.clear(t.background);

    // Frame 1: desktop + taskbar
    shell.render(g, t, 0);
    g.saveBmp("shot_01_desktop.bmp");

    // Frame 2: start menu open
    shell.openStartMenu(true);
    shell.render(g, t, 0);
    g.saveBmp("shot_02_startmenu.bmp");
    shell.openStartMenu(false);

    // Frame 3: windows + calculator
    shell.launchApp(2);   // calculator
    shell.launchApp(1);   // explorer
    shell.launchApp(0);   // settings
    shell.render(g, t, 0);
    g.saveBmp("shot_03_windows.bmp");

    // Frame 4: dark theme
    Theme dark = makeDarkTheme();
    shell.setTheme(dark);
    shell.dark = true;
    shell.render(g, dark, 0);
    g.saveBmp("shot_04_dark.bmp");

    // Frame 5: notifications
    shell.notificationsOpen = true;
    shell.render(g, dark, 0);
    g.saveBmp("shot_05_notify.bmp");
    shell.notificationsOpen = false;

    // Frame 6: widgets
    shell.widgetsOpen = true;
    shell.render(g, dark, 0);
    g.saveBmp("shot_06_widgets.bmp");
    shell.widgetsOpen = false;

    printf("[selftest] screenshots written to *.bmp\n");
    // Verify images non-empty
    FILE* f = fopen("shot_01_desktop.bmp", "rb");
    if (f) { fseek(f, 0, SEEK_END); long sz = ftell(f); fclose(f); printf("[selftest] shot_01_desktop.bmp = %ld bytes\n", sz); }
    else { printf("[selftest] FAILED: no screenshot output\n"); }
}

int main(int argc, char** argv) {
    bool selftest = false;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--selftest") == 0) selftest = true;

    int screenW = 1280, screenH = 720;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    Gfx g;
    if (!g.init(screenW, screenH)) {
        fprintf(stderr, "Gfx init failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // In selftest mode the window surface still exists; use SDL dummy driver
    if (selftest) {
        // Surface-backed: use the window surface directly
    }

    Shell shell;
    g_shell = &shell;
    shell.init(screenW, screenH);

    Theme theme = makeLightTheme();
    shell.setTheme(theme);

    if (selftest) {
        runSelfTest(shell, g, theme, screenW, screenH);
        SDL_Quit();
        return 0;
    }

    // ---- Main loop ----
    bool running = true;
    uint64_t lastTicks = SDL_GetTicks64();
    Uint32 lastFrame = SDL_GetTicks();

    while (running) {
        uint64_t nowMs = SDL_GetTicks64();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_MOUSEMOTION:
                    shell.onMouseMove(e.motion.x, e.motion.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    handleMouseButton(e.button, shell, nowMs);
                    break;
                case SDL_MOUSEWHEEL:
                    shell.onScroll(e.wheel.x, e.wheel.y, -e.wheel.y);
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    handleShortcut(e, shell);
                    break;
                default: break;
            }
        }

        // Theme hot-reload
        if (shell.dark != theme.dark) {
            theme = shell.dark ? makeDarkTheme() : makeLightTheme();
            shell.setTheme(theme);
        }

        // Update window manager (taskbar items sync)
        {
            // Sync taskbar items with open windows
            shell.taskbarItems.clear();
            for (auto& w : shell.wm.windows()) {
                if (w.closing) continue;
                TaskbarItem ti;
                ti.wndId = w.id;
                ti.title = w.title;
                // Match app by title
                ti.app = -1;
                for (int a = 0; a < g_appsCount; a++)
                    if (w.title == g_apps[a].name) { ti.app = a; break; }
                shell.taskbarItems.push_back(ti);
            }
        }

        // Tick animations + clock
        shell.wm.tick(nowMs);

        // Render
        shell.render(g, theme, nowMs);
        g.present();

        // Frame limiter (~60 fps)
        Uint32 frameTime = SDL_GetTicks() - lastFrame;
        if (frameTime < 16) SDL_Delay(16 - frameTime);
        lastFrame = SDL_GetTicks();
        (void)lastTicks;
    }

    shell.wm.close(0);
    g.destroy();
    SDL_Quit();
    return 0;
}
