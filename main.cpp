#include <array>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <future>
#include <raylib.h>
#include <stdlib.h>

using namespace std;

const vector<int> SCREEN_DIMS = {1366 / 2, 768 / 2};
const vector<float> START_POS = {371, 500};
vector<float> offset = {0, 0};
float dt;

class Collision
{

public:
    float x, y, w, h;

public:
    Collision(float x, float y, float w, float h)
    {
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;
    }

public:
    void draw()
    {
        DrawRectangle(x + offset[0], y + offset[1], w, h, DARKGRAY);
    }
};

vector<Collision>
    collisions = {
        Collision(0.0, 560.0, 800, 50),
        Collision(0.0, 510.0, 50, 50),
        Collision(2000.0, 500.0, 50, 50),
        Collision(200.0, 450.0, 400, 40),
        Collision(600.0, 390.0, 50, 100),
        Collision(310.0, 350.0, 200, 40),
        Collision(1000.0, 400.0, 310, 40),
        Collision(1200.0, 350.0, 50, 50),
        Collision(1400.0, 310.0, 200, 50),
        Collision(1600.0, 250.0, 50, 200)};

class Player
{
public:
    float x, y, hsp = 0, vsp = 0;

public:
    array<int, 4> hitbox = {16, 0, 31, 54};
    const float WALK_SPD = 5.0;
    const float WALK_ACC = 1.0;
    const float FRICTION = 0.25;
    const float GRAVITY_ACC = 0.5;
    const float GRAVITY = 10.0;
    const float JUMP_SPD = 10.0;
    const float JUMP_ACC = 2;
    const float DASH_SPD = 13.0;
    const int DASH_COOLDOWN = 120;
    const int ATTACK_COOLDOWN = 15;
    const int FRAME_DURATION = 7;
    const int JUMPS = 2;

    int jumps = 2;
    float attack_timer = 0;
    float dash_timer = 0;
    float can_dash = true;
    bool can_jump = true;
    float jump_used = 0.0;
    int facing = 1;
    int current_frame = 0;
    float animation_clock = 0;

    enum State
    {
        IDLE,
        DASHING,
        JUMPING,
        RISING,
        MID_AIR,
        FALLING,
        WALKING
    };
    State current_state = IDLE;
    array<Image, 4> IDLE_ANIMATION = {LoadImage("assets/knight/idle1.png"),
                                      LoadImage("assets/knight/idle2.png"),
                                      LoadImage("assets/knight/idle3.png"),
                                      LoadImage("assets/knight/idle4.png")};
    array<Image, 4> DASH_ANIMATION = {LoadImage("assets/knight/dash1.png"),
                                      LoadImage("assets/knight/dash2.png"),
                                      LoadImage("assets/knight/dash3.png"),
                                      LoadImage("assets/knight/dash4.png")};
    array<Image, 5> JUMP_ANIMATION = {LoadImage("assets/knight/jump1.png"),
                                      LoadImage("assets/knight/jump2.png"),
                                      LoadImage("assets/knight/jump3.png"),
                                      LoadImage("assets/knight/jump4.png"),
                                      LoadImage("assets/knight/jump5.png")};
    array<Image, 6> WALK_ANIMATION = {LoadImage("assets/knight/walk1.png"),
                                      LoadImage("assets/knight/walk2.png"),
                                      LoadImage("assets/knight/walk3.png"),
                                      LoadImage("assets/knight/walk4.png"),
                                      LoadImage("assets/knight/walk5.png"),
                                      LoadImage("assets/knight/walk6.png")};
    vector<Texture2D> IDLE_ANIMATION_TEXTURES = {};
    vector<Texture2D> JUMP_ANIMATION_TEXTURES = {};
    vector<Texture2D> WALK_ANIMATION_TEXTURES = {};
    vector<Texture2D> DASH_ANIMATION_TEXTURES = {};

public:
    Player(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

public:
    void load_textures()
    {
        for (Image &i : IDLE_ANIMATION)
        {
            ImageResizeNN(&i, 54, 54);
            Texture2D texture = LoadTextureFromImage(i);
            IDLE_ANIMATION_TEXTURES.push_back(texture);
            UnloadImage(i);
        }
        for (Image &i : JUMP_ANIMATION)
        {
            ImageResizeNN(&i, 54, 54);
            Texture2D texture = LoadTextureFromImage(i);
            JUMP_ANIMATION_TEXTURES.push_back(texture);
            UnloadImage(i);
        }
        for (Image &i : WALK_ANIMATION)
        {
            ImageResizeNN(&i, 54, 54);
            Texture2D texture = LoadTextureFromImage(i);
            WALK_ANIMATION_TEXTURES.push_back(texture);
            UnloadImage(i);
        }
        for (Image &i : DASH_ANIMATION)
        {
            ImageResizeNN(&i, 70, 54);
            Texture2D texture = LoadTextureFromImage(i);
            DASH_ANIMATION_TEXTURES.push_back(texture);
            UnloadImage(i);
        }
    }

public:
    void set_state(State new_state)
    {
        if (new_state != current_state)
        {
            current_frame = 0;
            animation_clock = 0;
        }
        current_state = new_state;
    }

public:
    void loop()
    {
        // Andar
        bool goingRight = IsKeyDown(KEY_RIGHT);
        bool goingLeft = IsKeyDown(KEY_LEFT);

        int dir = (goingRight - goingLeft);
        if (dir != 0 && current_state != DASHING)
        {
            facing = dir;
        }
        if (dash_timer <= 100)
        {
            if (abs(hsp) <= WALK_SPD)
            {
                hsp += WALK_ACC * dir * dt;
            }
            else
            {
                hsp = WALK_SPD * dir * dt;
            }

            if (dir == 0)
            {
                set_state(IDLE);
                hsp += ((hsp > 0) - (hsp < 0)) * FRICTION * dt;
            }
            else
            {
                set_state(WALKING);
            }
        }

        // Gravidade
        vsp += GRAVITY_ACC;
        if (vsp > GRAVITY)
        {
            vsp = GRAVITY;
        }

        // Ataque
        bool attack_pressed = IsKeyDown(KEY_X);
        if (attack_pressed && attack_timer == 0)
        {
            attack_timer = ATTACK_COOLDOWN;
        }
        if (attack_timer > 0)
        {
            attack_timer--;
        }

        // Dash
        bool dash_pressed = IsKeyDown(KEY_C);
        if (dash_pressed && dash_timer == 0)
        {
            hsp = 0;
            dash_timer = DASH_COOLDOWN;
        }
        if (dash_timer > 100 && dash_timer < 120)
        {
            hsp += DASH_SPD / 2 * facing * dt;
            if (abs(hsp) > DASH_SPD)
            {
                hsp = DASH_SPD * facing;
            }
            vsp = 0;
            set_state(DASHING);
        }
        if (dash_timer > 0)
        {
            dash_timer--;
        }

        // Pulo
        bool jump_pressed = IsKeyDown(KEY_Z);
        if (!jump_pressed && jump_used != 0)
        {
            jumps--;
            if (jumps == 0)
            {
                can_jump = false;
            }
            else
            {
                jump_used = 0.0;
                can_jump = true;
            }
        }
        if (jump_pressed && can_jump)
        {
            if (jump_used == 0 && vsp > 0)
            {
                vsp = -3;
            }
            vsp += -JUMP_ACC;
            jump_used += JUMP_ACC;
            if (vsp < -JUMP_SPD)
            {
                vsp = -JUMP_SPD;
            }
            if (jump_used >= JUMP_SPD)
            {
                can_jump = false;
            }
        }

        // Colisão
        for (Collision &c : collisions)
        {
            bool col_h = CheckCollisionRecs(Rectangle{x + hsp + hitbox[0], y + hitbox[1], hitbox[2], hitbox[3]}, Rectangle{c.x, c.y, c.w, c.h});
            bool col_v = CheckCollisionRecs(Rectangle{x + hitbox[0], y + vsp + hitbox[1], hitbox[2], hitbox[3]}, Rectangle{c.x, c.y, c.w, c.h});

            int h_dir = (hsp > 0) - (hsp < 0);
            int v_dir = (vsp > 0) - (vsp < 0);
            if (v_dir > 0 && vsp > 0 && col_v)
            {
                can_jump = true;
                jump_used = 0.0;
            }

            if (col_h)
            {
                for (float new_hsp = 0; new_hsp <= abs(hsp); new_hsp += 0.25)
                {
                    bool new_col_h = CheckCollisionRecs(Rectangle{x + new_hsp * h_dir + hitbox[0], y + hitbox[1], hitbox[2], hitbox[3]}, Rectangle{c.x, c.y, c.w, c.h});
                    if (new_col_h)
                    {
                        if (new_hsp > 0)
                        {
                            hsp = (new_hsp - 0.25) * h_dir;
                        }
                        else
                        {
                            hsp = 0;
                        }
                        break;
                    }
                }
            }
            if (col_v)
            {
                for (float new_vsp = 0; new_vsp <= abs(vsp); new_vsp += 0.25)
                {
                    bool new_col_v = CheckCollisionRecs(Rectangle{x + hitbox[0], y + new_vsp * v_dir + hitbox[1], hitbox[2], hitbox[3]}, Rectangle{c.x, c.y, c.w, c.h});
                    if (new_col_v)
                    {
                        if (new_vsp > 0)
                        {
                            vsp = (new_vsp - 0.25) * v_dir;
                        }
                        else
                        {
                            vsp = 0;
                        }
                        break;
                    }
                }
            }
            if (CheckCollisionRecs(Rectangle{x + hsp + hitbox[0], y + hitbox[1], hitbox[2], hitbox[3]}, Rectangle{c.x, c.y, c.w, c.h}))
            {
                vsp -= 1 * v_dir;
                hsp -= 1 * h_dir;
            }
        }

        // Processamento das velocidades
        x += hsp;
        y += vsp;
        offset = {START_POS[0] + (START_POS[0] - 371) / 2 - x + (SCREEN_DIMS[0] - 800) / 2, START_POS[1] + (START_POS[1] - 500) / 2 - y - 225 + (SCREEN_DIMS[1] - 600) / 2};

        // Animação
        animation_clock += dt;
        if (animation_clock >= FRAME_DURATION)
        {
            animation_clock = 0;
            current_frame += 1;
            int animation_length = current_state <= 2 ? IDLE_ANIMATION.size() - 1 : WALK_ANIMATION.size() - 1;
            if (current_frame >= animation_length)
            {
                current_frame = 0;
            }
        }

        if (vsp > 0)
        {
            set_state(FALLING);
        }
        if (vsp < 0)
        {
            if (vsp < -5)
            {
                set_state(RISING);
            }
            else
            {
                set_state(JUMPING);
            }
        }
        if (abs(vsp) < 1 && can_jump == false)
        {
            set_state(MID_AIR);
        }
        if (dash_timer > 95 && dash_timer < 120)
        {
            set_state(DASHING);
        }
    }

public:
    void draw()
    {
        Texture2D player_sprite;
        switch (current_state)
        {
        case IDLE:
            player_sprite = IDLE_ANIMATION_TEXTURES[current_frame];
            break;
        case WALKING:
            player_sprite = WALK_ANIMATION_TEXTURES[current_frame];
            break;
        case DASHING:
            current_frame = 4 - (dash_timer > 115) * 1 - (dash_timer > 100) * 2 - (dash_timer >= 95);
            player_sprite = DASH_ANIMATION_TEXTURES[current_frame];
            break;
        case JUMPING:
            player_sprite = JUMP_ANIMATION_TEXTURES[1];
            break;
        case RISING:
            player_sprite = JUMP_ANIMATION_TEXTURES[2];
            break;
        case MID_AIR:
            player_sprite = JUMP_ANIMATION_TEXTURES[3];
            break;
        case FALLING:
            player_sprite = JUMP_ANIMATION_TEXTURES[4];
            break;
        }
        // DrawRectangleLines(SCREEN_DIMS[0] / 2 - 28 + hitbox[0], SCREEN_DIMS[1] / 2 - 25 + hitbox[1], hitbox[2], hitbox[3], BLUE);
        DrawTextureRec(player_sprite, Rectangle{0, 0, (current_state == DASHING ? 70 : 54) * facing, 54}, Vector2{SCREEN_DIMS[0] / 2 - (current_state == DASHING ? 32 : 24), SCREEN_DIMS[1] / 2 - 25}, WHITE);
        if (attack_timer > 10 && attack_timer < 15)
        {
            DrawRectangle(SCREEN_DIMS[0] / 2 - 25 + (facing == 1 ? 50 : -25), SCREEN_DIMS[1] / 2 - 25 + 20, 25, 7, WHITE);
        }
    }
};
Player p(START_POS[0], START_POS[1]);

void loop()
{
    dt = GetFrameTime() * 60;
    p.loop();

    for (Collision c : collisions)
    {
        c.draw();
    }
}

void draw(RenderTexture2D render_texture)
{
    BeginTextureMode(render_texture);
    ClearBackground(GRAY);
    p.draw();
    for (Collision c : collisions)
    {
        c.draw();
    }
    EndTextureMode();

    BeginDrawing();
    DrawTexturePro(
        render_texture.texture,
        Rectangle{0, 0, static_cast<float>(render_texture.texture.width), static_cast<float>(-render_texture.texture.height)},
        Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
        Vector2{0, 0},
        0,
        WHITE);
    EndDrawing();
}

int main()
{

    InitWindow(SCREEN_DIMS[0], SCREEN_DIMS[1], "Hollow Knight - C++");
    RenderTexture2D render_texture = LoadRenderTexture(SCREEN_DIMS[0], SCREEN_DIMS[1]);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    p.load_textures();
    SetTargetFPS(60);

    while (WindowShouldClose() == false)
    {
        loop();
        draw(render_texture);
    }

    UnloadRenderTexture(render_texture);
    CloseWindow();
    return 0;
}