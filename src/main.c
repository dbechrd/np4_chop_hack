#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "raylib/raylib.h"

enum FontId {
    Font_20,
    Font_24,
    Font_36,
    Font_Count,
};
Font fonts[Font_Count];

enum SoundId {
    Sound_Timer,
    Sound_Key,
    Sound_Success,
    Sound_Fail,
    Sound_Count
};
Sound sounds[Sound_Count];

enum TextureId {
    Texture_Keyboard,
    Texture_Count
};
Texture textures[Texture_Count];

typedef enum ChopChallengeState {
    Chop_Active,
    Chop_Success,
    Chop_Fail
} ChopChallengeState;

typedef struct ChopChallenge {
    double now;
    char letters[15];
    int correct_count;
    ChopChallengeState state;
    double started_at;
    double duration;
    double last_timer_tick_at;  // audio cue
} ChopChallenge;

char gen_chop_challenge_letter(void)
{
    int val = GetRandomValue(1, 7);
    switch (val) {
        case 1: return 'Q';
        case 2: return 'W';
        case 3: return 'E';
        case 4: return 'R';
        case 5: return 'A';
        case 6: return 'S';
        case 7: return 'D';
    }
    assert(!"shouldn't get here");
    return '?';
}

void gen_chop_challenge(ChopChallenge *chopChallenge)
{
    memset(chopChallenge, 0, sizeof(*chopChallenge));
    for (int i = 0; i < sizeof(chopChallenge->letters); i++) {
        chopChallenge->letters[i] = gen_chop_challenge_letter();
    }
    chopChallenge->now = GetTime();
    chopChallenge->started_at = chopChallenge->now;
    chopChallenge->duration = 5.0;
    chopChallenge->last_timer_tick_at = chopChallenge->started_at - 1.0;  // NOTE(dlb): Make it beep immediately when it starts
}

void update_chop_challenge(ChopChallenge *chopChallenge)
{
    chopChallenge->now = GetTime();

    switch (chopChallenge->state) {
        case Chop_Active: {
            if (chopChallenge->now - chopChallenge->started_at >= chopChallenge->duration) {
                PlaySound(sounds[Sound_Fail]);
                chopChallenge->state = Chop_Fail;
                break;
            }

            if (chopChallenge->now - chopChallenge->last_timer_tick_at >= 1.0) {
                PlaySound(sounds[Sound_Timer]);
                chopChallenge->last_timer_tick_at += 1.0;
            }

            char next = chopChallenge->letters[chopChallenge->correct_count];
            char pressed = GetKeyPressed();
            if (!pressed) break;

            PlaySound(sounds[Sound_Key]);

            char pressed2 = GetKeyPressed();
            if (pressed != next || pressed2) {
                PlaySound(sounds[Sound_Fail]);
                chopChallenge->state = Chop_Fail;
            } else {
                chopChallenge->correct_count++;
                if (chopChallenge->correct_count == sizeof(chopChallenge->letters)) {
                    PlaySound(sounds[Sound_Success]);
                    chopChallenge->state = Chop_Success;
                }
            }
            break;
        }
        case Chop_Success:
        case Chop_Fail: {
            if (IsKeyPressed(KEY_ENTER)) {
                gen_chop_challenge(chopChallenge);
                chopChallenge->state = Chop_Active;
            }
            break;
        }
    }
}

void draw_chop_challenge(ChopChallenge *chopChallenge)
{
    const int items_per_row = 6;
    const float button_size = 74.0f;
    const float button_pad = 8.0f;

    const float roundness = 0.15f;
    const int segments = 4;

    int rows = sizeof(chopChallenge->letters) / items_per_row;
    if (sizeof(chopChallenge->letters) % items_per_row) rows++;

    Vector2 maxCursor = { 0 };

    const float fullWidth = button_pad + items_per_row * button_size + items_per_row * button_pad;
    float xReset = (float)GetScreenWidth() / 2 - fullWidth / 2;
    Vector2 cursor = { xReset, button_pad };

    DrawTexture(textures[Texture_Keyboard], (int)cursor.x, (int)cursor.y + 2, GRAY);
    cursor.x += textures[Texture_Keyboard].width + button_pad;

    DrawTextEx(fonts[Font_24], "Alphabet", cursor, (float)fonts[Font_24].baseSize, 1.0f, GREEN);
    cursor.x += 100;
    cursor.y += 2;

    DrawTextEx(fonts[Font_20], "Tap the letters in order", cursor, (float)fonts[Font_20].baseSize, 1.0f, GRAY);
    cursor.y += fonts[Font_24].baseSize + button_pad;

    cursor.x = xReset;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < items_per_row; x++) {
            int i = y * items_per_row + x;
            if (i >= sizeof(chopChallenge->letters)) {
                break;
            }

            Rectangle buttonRec = {
                cursor.x, cursor.y, (float)button_size, (float)button_size
            };

            Color color = DARKBLUE;
            if (i < chopChallenge->correct_count) {
                color = DARKGREEN;
            } else if (i == chopChallenge->correct_count && chopChallenge->state == Chop_Fail) {
                color = MAROON;
            }

            DrawRectangleRounded(buttonRec, roundness, segments, ColorBrightness(color, -0.6f));
            DrawRectangleRoundedLines(buttonRec, roundness, segments, 2.0f, color);

            const char *letter = TextFormat("%c", chopChallenge->letters[i]);

            Vector2 letterSize = MeasureTextEx(fonts[Font_36], letter, (float)fonts[Font_36].baseSize, 1.0f);
            Vector2 letterPos = {
                cursor.x + (button_size / 2) - letterSize.x / 2,
                cursor.y + (button_size / 2) - letterSize.y / 2,
            };

            DrawTextEx(fonts[Font_36], letter, letterPos, (float)fonts[Font_36].baseSize, 1.0f, ColorBrightness(color, 0.6f));

            cursor.x += button_size + button_pad;
            if (cursor.x > maxCursor.x) maxCursor.x = cursor.x;
        }

        cursor.x = xReset;
        cursor.y += button_size + button_pad;
        if (cursor.y > maxCursor.y) maxCursor.y = cursor.y;
    }

    Rectangle statusBarRect = {
        (float)xReset,
        (float)cursor.y,
        (float)maxCursor.x - button_pad * 2 - xReset,
        (float)fonts[Font_24].baseSize + 2
    };

    if (chopChallenge->state == Chop_Active) {
        DrawRectangleRounded(statusBarRect, roundness + 0.15f, segments, ColorBrightness(DARKGRAY, -0.3f));

        Rectangle timerRect = statusBarRect;
        timerRect.x += 2;
        timerRect.y += 2;
        timerRect.width -= 4;
        timerRect.height -= 4;

        timerRect.width *= (float)(1.0 - (GetTime() - chopChallenge->started_at) / chopChallenge->duration);
        DrawRectangleRounded(timerRect, roundness + 0.15f, segments, ColorBrightness(ORANGE, -0.3f));
    } else {
        Color color = WHITE;
        const char *text = 0;

        switch (chopChallenge->state) {
            case Chop_Success: {
                color = DARKGREEN;
                text = "Success!";
                break;
            }
            case Chop_Fail: {
                color = MAROON;
                text = "Failed.";
                break;
            }
        }

        DrawRectangleRounded(statusBarRect, roundness + 0.15f, segments, ColorBrightness(color, -0.6f));
        Vector2 textSize = MeasureTextEx(fonts[Font_24], text, (float)fonts[Font_24].baseSize, 1.0f);
        Vector2 textPos = {
            xReset + (float)button_pad + (statusBarRect.width / 2 - textSize.x / 2),
            (float)cursor.y + 2
        };
        DrawTextEx(fonts[Font_24], text, textPos, (float)fonts[Font_24].baseSize, 1.0f, ColorBrightness(color, 0.3f));
    }
}

int main(int argc, char *argv[])
{
    InitAudioDevice();
    InitWindow(800, 600, "NoPixel Hax");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_VSYNC_HINT);

    fonts[Font_20] = LoadFontEx("KarminaBold.otf", 20, 0, 0);
    fonts[Font_24] = LoadFontEx("KarminaBold.otf", 24, 0, 0);
    fonts[Font_36] = LoadFontEx("KarminaBold.otf", 36, 0, 0);

    sounds[Sound_Timer  ] = LoadSound("chop_timer.ogg");
    sounds[Sound_Key    ] = LoadSound("chop_key.ogg");
    sounds[Sound_Success] = LoadSound("chop_success.ogg");
    sounds[Sound_Fail   ] = LoadSound("chop_fail.ogg");

    textures[Texture_Keyboard] = LoadTexture("keyboard.png");

    // QWERASD
    // https://www.youtube.com/watch?v=YUz5_3ogZzs&t=165s
    ChopChallenge chopChallenge = { 0 };
    gen_chop_challenge(&chopChallenge);

    // https://i.imgur.com/GWQ0Nbu.png
    // https://i.imgur.com/7iZCZL6.png
    // 10 - 13 random alphanum, all uppercase

    while (!WindowShouldClose()) {
        update_chop_challenge(&chopChallenge);

        ClearBackground(ColorBrightness(DARKBLUE, -0.75f));
        BeginDrawing();
        {
            draw_chop_challenge(&chopChallenge);
        }
        EndDrawing();
    }

    for (int i = 0; i < Font_Count; i++) {
        UnloadFont(fonts[i]);
    }
    for (int i = 0; i < Sound_Count; i++) {
        UnloadSound(sounds[i]);
    }
    for (int i = 0; i < Texture_Count; i++) {
        UnloadTexture(textures[i]);
    }
    CloseWindow();
    return 0;
}