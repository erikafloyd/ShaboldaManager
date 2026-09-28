#include "gui_renderer.hpp"
#include <cstddef>

static Font currentFont = { 0 };
static bool hasCustomFont = false;

void GuiSetCustomFont(Font font) {
    currentFont = font;
    hasCustomFont = true;
}

// Convert Raylib colors from microui mu_Color
static inline Color ToRaylibColor(mu_Color c) {
    return Color{ c.r, c.g, c.b, c.a };
}

void GuiProcessInput(mu_Context *ctx) {
    // 1. Mouse coordinates
    Vector2 mouse = GetMousePosition();
    mu_input_mousemove(ctx, (int)mouse.x, (int)mouse.y);

    // 2. Mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        mu_input_scroll(ctx, 0, (int)(wheel * -30.0f));
    }

    // 3. Mouse buttons
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        mu_input_mousedown(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_LEFT);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        mu_input_mouseup(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_LEFT);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        mu_input_mousedown(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_RIGHT);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        mu_input_mouseup(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_RIGHT);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        mu_input_mousedown(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_MIDDLE);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
        mu_input_mouseup(ctx, (int)mouse.x, (int)mouse.y, MU_MOUSE_MIDDLE);
    }

    // 4. Text typing
    int keyChar = GetCharPressed();
    while (keyChar > 0) {
        if (keyChar >= 32 && keyChar <= 126) {
            char str[2] = { (char)keyChar, '\0' };
            mu_input_text(ctx, str);
        }
        keyChar = GetCharPressed();
    }

    // 5. Special modifier and navigation keys
    if (IsKeyDown(KEY_BACKSPACE)) mu_input_keydown(ctx, MU_KEY_BACKSPACE);
else mu_input_keyup(ctx, MU_KEY_BACKSPACE);

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) mu_input_keydown(ctx, MU_KEY_RETURN);
    if (IsKeyReleased(KEY_ENTER) || IsKeyReleased(KEY_KP_ENTER)) mu_input_keyup(ctx, MU_KEY_RETURN);

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        mu_input_keydown(ctx, MU_KEY_SHIFT);
    } else {
        mu_input_keyup(ctx, MU_KEY_SHIFT);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        mu_input_keydown(ctx, MU_KEY_CTRL);
    } else {
        mu_input_keyup(ctx, MU_KEY_CTRL);
    }

    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
        mu_input_keydown(ctx, MU_KEY_ALT);
    } else {
        mu_input_keyup(ctx, MU_KEY_ALT);
    }
}

void GuiRenderCommands(mu_Context *ctx) {
    mu_Command *cmd = nullptr;
    bool scissorActive = false;

    while (mu_next_command(ctx, &cmd)) {
        switch (cmd->type) {
            case MU_COMMAND_TEXT: {
                if (hasCustomFont) {
                    DrawTextEx(currentFont, cmd->text.str, Vector2{ (float)cmd->text.pos.x, (float)cmd->text.pos.y }, (float)currentFont.baseSize, 1.0f, ToRaylibColor(cmd->text.color));
                } else {
                    DrawText(cmd->text.str, cmd->text.pos.x, cmd->text.pos.y, 14, ToRaylibColor(cmd->text.color));
                }
                break;
            }
            case MU_COMMAND_RECT: {
                DrawRectangle(
                    cmd->rect.rect.x, 
                    cmd->rect.rect.y, 
                    cmd->rect.rect.w, 
                    cmd->rect.rect.h, 
                    ToRaylibColor(cmd->rect.color)
                );
                break;
            }
            case MU_COMMAND_ICON: {
                // Render microui icons (checkmarks, close X, collapse arrows)
                mu_Rect r = cmd->icon.rect;
                Color col = ToRaylibColor(cmd->icon.color);
                switch (cmd->icon.id) {
                    case MU_ICON_CLOSE:
                        DrawLine(r.x, r.y, r.x + r.w, r.y + r.h, col);
                        DrawLine(r.x + r.w, r.y, r.x, r.y + r.h, col);
                        break;
                    case MU_ICON_CHECK:
                        DrawLine(r.x + 2, r.y + r.h / 2, r.x + r.w / 2, r.y + r.h - 2, col);
                        DrawLine(r.x + r.w / 2, r.y + r.h - 2, r.x + r.w - 2, r.y + 2, col);
                        break;
                    case MU_ICON_COLLAPSED:
                        DrawTriangle(
                            Vector2{ (float)r.x, (float)r.y },
                            Vector2{ (float)r.x, (float)(r.y + r.h) },
                            Vector2{ (float)(r.x + r.w), (float)(r.y + r.h / 2) },
                            col
                        );
                        break;
                    case MU_ICON_EXPANDED:
                        DrawTriangle(
                            Vector2{ (float)r.x, (float)r.y },
                            Vector2{ (float)(r.x + r.w), (float)r.y },
                            Vector2{ (float)(r.x + r.w / 2), (float)(r.y + r.h) },
                            col
                        );
                        break;
                    default:
                        break;
                }
                break;
            }
            case MU_COMMAND_CLIP: {
                if (cmd->clip.rect.w > 0 && cmd->clip.rect.h > 0) {
                    BeginScissorMode(
                        cmd->clip.rect.x, 
                        cmd->clip.rect.y, 
                        cmd->clip.rect.w, 
                        cmd->clip.rect.h
                    );
                    scissorActive = true;
                } else if (scissorActive) {
                    EndScissorMode();
                    scissorActive = false;
                }
                break;
            }
            default:
                break;
        }
    }

    if (scissorActive) {
        EndScissorMode();
    }
}
