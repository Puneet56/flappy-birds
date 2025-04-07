#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define SCREEN_HEIGHT 768
#define SCREEN_WIDTH 1300

#define SCALE 1.5f

#define MAX_VELOCITY 800
#define MIN_VELOCITY -400

#define BG_SPEED 40.0f
#define BG_POS_Y 0
#define BASE_SPEED BG_SPEED * 3
#define BASE_POS_Y 600

#define PIPE_GAP 300
#define PIPE_SPACING 250

typedef struct {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Texture2D *textures;
  int textureLength;

  int currentFrame;
  float frameTimer;
  float frameDuration;
  float angle;
} Bird;

Bird CreateBird(Texture2D *textures, size_t length) {
  assert(textures != NULL);

  Bird bird = {.position = {100, SCREEN_HEIGHT / 2.0f},
               .velocity = {0, 0},
               .radius = (float)textures[0].width / 2,
               .textures = textures,
               .textureLength = length,

               .currentFrame = 0,  // Start at the first frame
               .frameTimer = 0.0f, // Reset timer
               .frameDuration = 1.0f / 8.0f,
               .angle = 0};

  return bird;
}

void DestroyBird(Bird *bird) {
  assert(bird != NULL);

  for (int i = 0; i < bird->textureLength; i++) {
    UnloadTexture(bird->textures[i]);
  }
}

void DrawBird(Bird *bird, float dt) {
  assert(bird != NULL);

  bird->frameTimer += dt; // Accumulate time

  // Check if it's time to advance the frame
  if (bird->frameTimer >= bird->frameDuration) {
    bird->frameTimer -= bird->frameDuration; // Reset timer (subtracting is
                                             // often better than setting to 0)
    bird->currentFrame++;                    // Move to the next frame

    // Wrap the frame index back to 0 if it goes past the last frame
    if (bird->currentFrame >= bird->textureLength) {
      bird->currentFrame = 0;
    }
    // A more concise way using modulo:
    // bird->currentFrame = (bird->currentFrame + 1) % bird->textureLength;
  }

  Texture2D currentTexture = bird->textures[bird->currentFrame];

  Rectangle source = {0, 0, currentTexture.width, currentTexture.height};
  Rectangle dest = {bird->position.x, bird->position.y,
                    currentTexture.width * SCALE,
                    currentTexture.height * SCALE};

  Vector2 origin = (Vector2){dest.width / 2.0, dest.height / 2.0};

  // Change angle only if bird is moving else use old angle
  if (bird->velocity.y != 0) {
    bird->angle = Remap(bird->velocity.y, MIN_VELOCITY, MAX_VELOCITY, -30, 90);
  }

  DrawTexturePro(currentTexture, source, dest, origin, bird->angle, WHITE);
  DrawCircleV(bird->position, 2, WHITE);
}

typedef struct {
  Texture2D texture;
  float posX;
  float posY;
  float scrollSpeed;
} ScrollingBackground;

ScrollingBackground CreateScrollingBackground(Texture2D texture, float posY,
                                              float speed) {

  ScrollingBackground bg = {
      .texture = texture, .posX = 0, .posY = posY, .scrollSpeed = speed};
  return bg;
}

void DrawScrollingBackground(ScrollingBackground *bg, float dt) {
  bg->posX -= bg->scrollSpeed * dt;

  // Reset position when the first image is completely off-screen to the left
  if (bg->posX <= -bg->texture.width * 1.5f) {
    bg->posX = 0;
  }

  Rectangle source = {0, 0, bg->texture.width, bg->texture.height};

  int txH = bg->texture.height * SCALE;
  int txW = bg->texture.width * SCALE;
  int renderCount = 2 + SCREEN_WIDTH / txW;
  if (txW > SCREEN_WIDTH) {
    renderCount = 2 + txW / SCREEN_WIDTH;
  }

  for (int i = 0; i < renderCount; i++) {
    Rectangle dest = {bg->posX + (i * dest.width), bg->posY, txW, txH};

    DrawTexturePro(bg->texture, source, dest, (Vector2){0, 0}, 0, WHITE);
    DrawRectangleV((Vector2){bg->posX + (i * dest.width), bg->posY},
                   (Vector2){2, SCREEN_HEIGHT}, RED);
  }
}

void DestroyAnimation(ScrollingBackground *b) {
  assert(b != NULL);
  UnloadTexture(b->texture);
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy birds");
  SetTargetFPS(60);

  bool gameStarted = false;

  Texture2D bgTexture = LoadTexture("./assets/sprites/background-day.png");
  if (!IsTextureValid(bgTexture)) {
    TraceLog(LOG_ERROR, "Failed to load background texture");
    CloseWindow();
    return 1;
  } else {
    TraceLog(LOG_INFO, "Loaded background texture successfully.");
  }
  ScrollingBackground background =
      CreateScrollingBackground(bgTexture, BG_POS_Y, BG_SPEED);

  Texture2D baseTx = LoadTexture("./assets/sprites/base.png");
  if (!IsTextureValid(baseTx)) {
    TraceLog(LOG_ERROR, "Failed to load base texture");
    CloseWindow();
    return 1;
  } else {
    TraceLog(LOG_INFO, "Loaded background base successfully.");
  }
  ScrollingBackground base =
      CreateScrollingBackground(baseTx, BASE_POS_Y, BASE_SPEED);

  Texture2D pipeTexture = LoadTexture("./assets/sprites/pipe-green.png");
  if (!IsTextureValid(pipeTexture)) {
    TraceLog(LOG_ERROR, "Failed to pipe texture");
    CloseWindow();
    return 1;
  } else {
    TraceLog(LOG_INFO, "Loaded base successfully.");
  }

  Texture2D birdTextures[] = {
      LoadTexture("./assets/sprites/bluebird-upflap.png"),
      LoadTexture("./assets/sprites/bluebird-midflap.png"),
      LoadTexture("./assets/sprites/bluebird-downflap.png"),
  };

  size_t numTextures = sizeof(birdTextures) / sizeof(birdTextures[0]);
  for (size_t i = 0; i < numTextures; i++) {
    if (!IsTextureValid(birdTextures[i])) {
      TraceLog(LOG_ERROR, "Failed to load bird texture %d!", i);
      CloseWindow();
      return 1;
    } else {
      TraceLog(LOG_INFO, "Loaded bird texture %d successfully.", i);
    }
  }

  Bird bird = CreateBird(birdTextures, numTextures);

  // TODO: Tune these
  Vector2 gravity = {.x = 0, .y = 980.0f};
  Vector2 jumpForce = {.x = 0, .y = -400.0f};

  float dt; // important
  while (!WindowShouldClose()) {
    dt = GetFrameTime();

    if (IsKeyPressed(KEY_S) && !gameStarted) {
      gameStarted = true;
    }

    if (IsKeyPressed(KEY_SPACE) && gameStarted) {
      bird.velocity = jumpForce;
    }

    if (gameStarted) {
      bird.velocity = Vector2Add(bird.velocity, Vector2Scale(gravity, dt));
      bird.velocity.y = Clamp(bird.velocity.y, -1200, 1500);

      bird.position =
          Vector2Add(bird.position, Vector2Scale(bird.velocity, dt));

      bird.velocity.y *= 0.99f;

      if (bird.position.y > BASE_POS_Y - bird.radius * SCALE) {
        bird.position.y = BASE_POS_Y - bird.radius * SCALE;
        bird.velocity.y = 0;
      }

      if (bird.position.y < bird.radius * SCALE) {
        bird.position.y = bird.radius * SCALE;
        bird.velocity.y = 0;
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawScrollingBackground(&background, dt);

    DrawBird(&bird, dt);
    DrawScrollingBackground(&base, dt);

    if (!gameStarted) {
      DrawText("Press S to start!", 10, 10, 20, DARKGRAY);
    } else {
      DrawText("Press SPACE to jump!", 10, 10, 20, DARKGRAY);
    }

    EndDrawing();
  }

  DestroyBird(&bird);
  DestroyAnimation(&background);
  DestroyAnimation(&base);
  CloseWindow();
  return 0;
}
