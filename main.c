#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define SCREEN_HEIGHT 768
#define SCREEN_WIDTH 432

#define MAX_VELOCITY 800
#define MIN_VELOCITY -400

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
               .radius = (float)textures[0].height / 2,
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
  Rectangle dest = {bird->position.x, bird->position.y, currentTexture.width,
                    currentTexture.height};

  Vector2 origin =
      (Vector2){currentTexture.width / 2.0, currentTexture.height / 2.0};

  bird->angle = bird->velocity.y != 0 ? Remap(bird->velocity.y, MIN_VELOCITY,
                                              MAX_VELOCITY, -30, 90)
                                      : bird->angle;

  DrawTexturePro(currentTexture, source, dest, origin, bird->angle, WHITE);
  DrawCircleV(bird->position, 2, WHITE);
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy birds");
  SetTargetFPS(60);

  bool gameStarted = false;

  Texture2D bgTexture = LoadTexture("./assets/sprites/background-day.png");

  TraceLog(LOG_INFO, "background %d %d", bgTexture.height, bgTexture.width);

  Texture2D birdTextures[] = {
      LoadTexture("./assets/sprites/bluebird-upflap.png"),
      LoadTexture("./assets/sprites/bluebird-midflap.png"),
      LoadTexture("./assets/sprites/bluebird-downflap.png"),
  };

  size_t numTextures = sizeof(birdTextures) / sizeof(birdTextures[0]);

  for (int i = 0; i < numTextures; i++) {
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

      if (bird.position.y > SCREEN_HEIGHT - bird.radius) {
        bird.position.y = SCREEN_HEIGHT - bird.radius;
        bird.velocity.y = 0;
      }

      if (bird.position.y < bird.radius) {
        bird.position.y = bird.radius;
        bird.velocity.y = 0;
      }
    }

    TraceLog(LOG_INFO, "velocity %f", bird.velocity.y);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextureEx(bgTexture, (Vector2){0, 0}, 0, 1.5, WHITE);

    DrawBird(&bird, dt);

    if (!gameStarted) {
      DrawText("Press S to start!", 10, 10, 20, DARKGRAY);
    } else {
      DrawText("Press SPACE to jump!", 10, 10, 20, DARKGRAY);
    }

    EndDrawing();
  }

  DestroyBird(&bird);
  CloseWindow();
  return 0;
}
