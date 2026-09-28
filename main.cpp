extern "C" {
#include "UIEngine/microui/microui.h"
#include "sodium.h"
}

#include "gui_manager.hpp"
#include "gui_renderer.hpp"
#include "raylib.h"
#include "vault_backend.h"

#include <cstring>

const int WIDTH = 850;
const int HEIGHT = 620;
const int FPS = 180;

static Font appFont = { 0 };

// Text measurement callbacks for microui supporting custom font or default fallback
int text_width(mu_Font font, const char *str, int len) {
  int l = (len == -1) ? (int)strlen(str) : len;
  char buf[512] = {0};
  if (l >= (int)sizeof(buf)) l = (int)sizeof(buf) - 1;
  memcpy(buf, str, l);
  buf[l] = '\0';

  if (appFont.texture.id > 0) {
    Vector2 size = MeasureTextEx(appFont, buf, (float)appFont.baseSize, 1.0f);
    return (int)size.x;
  }
  return MeasureText(buf, 14);
}

int text_height(mu_Font font) { 
  if (appFont.texture.id > 0) {
    return appFont.baseSize + 4;
  }
  return 18; 
}

int main() {
  // 1. Initialize libsodium cryptographic library
  if (sodium_init() < 0) {
    TraceLog(LOG_ERROR, "Failed to initialize libsodium!");
    return 1;
  }

  // 2. Initialize Raylib window
  InitWindow(WIDTH, HEIGHT, "Password Papochka [Secure Vault]");
  SetTargetFPS(FPS);

  // Try loading a modern custom TTF font if available (e.g., assets/font.ttf or font.ttf)
  if (FileExists("assets/font.ttf")) {
    appFont = LoadFontEx("assets/font.ttf", 16, 0, 250);
    GuiSetCustomFont(appFont);
  } else if (FileExists("font.ttf")) {
    appFont = LoadFontEx("font.ttf", 16, 0, 250);
    GuiSetCustomFont(appFont);
  }

  // 3. Initialize microui context and sleek modern dark theme
  mu_Context *ctx = new mu_Context();
  mu_init(ctx);
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  // Modern Dark Theme Palette (Bitwarden / 1Password inspired)
  ctx->style->colors[MU_COLOR_TEXT]          = mu_color(243, 244, 246, 255); // #f3f4f6
  ctx->style->colors[MU_COLOR_BORDER]        = mu_color(55, 65, 81, 255);    // #374151
  ctx->style->colors[MU_COLOR_WINDOWBG]      = mu_color(30, 34, 42, 255);    // #1e222a (Deep Slate)
  ctx->style->colors[MU_COLOR_TITLEBG]       = mu_color(17, 20, 26, 255);    // #11141a
  ctx->style->colors[MU_COLOR_TITLETEXT]     = mu_color(255, 255, 255, 255);
  ctx->style->colors[MU_COLOR_PANELBG]       = mu_color(30, 34, 42, 255);
  ctx->style->colors[MU_COLOR_BUTTON]        = mu_color(79, 70, 229, 255);   // #4f46e5 (Vibrant Indigo)
  ctx->style->colors[MU_COLOR_BUTTONHOVER]   = mu_color(99, 102, 241, 255);  // #6366f1
  ctx->style->colors[MU_COLOR_BUTTONFOCUS]   = mu_color(67, 56, 202, 255);   // #4338ca
  ctx->style->colors[MU_COLOR_BASE]          = mu_color(22, 25, 31, 255);    // Input fields bg
  ctx->style->colors[MU_COLOR_BASEHOVER]     = mu_color(38, 43, 54, 255);
  ctx->style->colors[MU_COLOR_BASEFOCUS]     = mu_color(45, 51, 64, 255);
  ctx->style->colors[MU_COLOR_SCROLLBASE]    = mu_color(17, 20, 26, 255);
  ctx->style->colors[MU_COLOR_SCROLLTHUMB]   = mu_color(79, 70, 229, 255);

  ctx->style->padding = 10;
  ctx->style->spacing = 8;
  ctx->style->title_height = 32;

  // 4. Initialize GUI Manager (Handles state, forms, password generator)
  GuiManager gui;

  // --- CONNECT YOUR BACKEND HERE ---
  gui.SetUnlockCallback(UnlockVault);
  gui.SetInitVaultCallback(initVault);
  gui.SetSaveEntryCallback(SaveEntry);
  gui.SetActiveVaultCallback(SetActiveVault);
  // ---------------------------------

  // 5. Main application loop
  while (!WindowShouldClose()) {
    // Step A: Feed Raylib mouse, wheel, keyboard inputs to microui
    GuiProcessInput(ctx);

    // Step B: Begin microui frame
    mu_begin(ctx);

    // Step C: Process UI widgets, windows, and callbacks
    gui.ProcessGui(ctx, GetScreenWidth(), GetScreenHeight());

    // Step D: Finish microui frame calculations
    mu_end(ctx);

    // Step E: Render the frame to screen
    BeginDrawing();
    ClearBackground(Color{22, 25, 31, 255}); // Sleek dark canvas background

    // Execute microui draw commands via Raylib primitives
    GuiRenderCommands(ctx);

    EndDrawing();
  }

  // Cleanup
  if (appFont.texture.id > 0) {
    UnloadFont(appFont);
  }
  gui.Lock();
  delete ctx;
  CloseWindow();

  return 0;
}
