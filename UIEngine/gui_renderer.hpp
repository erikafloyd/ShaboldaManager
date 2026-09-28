#pragma once

extern "C" {
#include "microui.h"   // found via ${CMAKE_CURRENT_SOURCE_DIR}/microui
}
#include "raylib.h"    // found via raylib/include

// Translates Raylib mouse and keyboard inputs into microui input events.
// Call this once per frame BEFORE mu_begin().
void GuiProcessInput(mu_Context *ctx);

// Drains and executes microui draw commands via Raylib graphics primitives.
// Call this once per frame inside BeginDrawing() / EndDrawing() AFTER mu_end().
void GuiRenderCommands(mu_Context *ctx);

// Set custom font for smooth rendering
void GuiSetCustomFont(Font font);
