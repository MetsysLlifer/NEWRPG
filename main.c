#include "game.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Mine-Noita-Craft: Physics Sandbox");
    SetTargetFPS(60);

    InitWorld();

    Player player;
    InitPlayer(&player);

    Inventory inv = { 
        .slots = { BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_WATER, BLOCK_LAVA, BLOCK_WOOD, BLOCK_FIRE, BLOCK_AIR }, 
        .selected = 0 
    };

    // 1. Camera Setup
    Camera2D camera = { 0 }; 
    camera.zoom = 1.0f; 
    camera.offset = (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
    camera.target = player.position; // Start centered on player

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // --- INPUTS ---
        
        // Get mouse relative to the world (using current camera position)
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

        for(int i=0; i<8; i++) {
            if(IsKeyPressed(KEY_ONE + i)) inv.selected = i;
        }

        int gx = (int)(mouseWorld.x / CELL_SIZE);
        int gy = (int)(mouseWorld.y / CELL_SIZE);
        int brushRadius = 2; 

        // MINE
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            EditWorld(gx, gy, BLOCK_AIR, brushRadius); 
        }

        // BUILD
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 pGrid = { player.position.x/CELL_SIZE, player.position.y/CELL_SIZE };
            if (Vector2Distance((Vector2){(float)gx, (float)gy}, pGrid) > 3.0f) {
                EditWorld(gx, gy, inv.slots[inv.selected], brushRadius);
            }
        }

        if (IsKeyPressed(KEY_R)) InitWorld();
        
        // --- PHYSICS ---
        UpdatePlayer(&player, dt);
        UpdateWorld(); 
        
        // --- SMOOTH CAMERA LOGIC ---
        // Instead of snapping, we slide the camera towards the player.
        // "speed" determines how tight the camera follows. 
        // 5.0f is smooth/loose. 10.0f is tight.
        float camSpeed = 5.0f; 
        
        camera.target.x += (player.position.x - camera.target.x) * camSpeed * dt;
        camera.target.y += (player.position.y - camera.target.y) * camSpeed * dt;

        // --- RENDER ---
        BeginDrawing();
            ClearBackground((Color){20, 20, 30, 255}); 

            BeginMode2D(camera);
                DrawWorld();
                DrawPlayer(&player);
                
                // Draw Cursor (Centered on the brush area)
                int drawX = (gx - brushRadius) * CELL_SIZE;
                int drawY = (gy - brushRadius) * CELL_SIZE;
                int diameter = (brushRadius * 2 + 1) * CELL_SIZE;
                DrawRectangleLines(drawX, drawY, diameter, diameter, Fade(WHITE, 0.5f));
            EndMode2D();

            DrawHUD(&player, &inv);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}