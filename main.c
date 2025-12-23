#include "game.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Mine-Noita-Craft: Physics Sandbox");
    SetTargetFPS(60);

    InitWorld();

    Player player;
    InitPlayer(&player);

    // Initial loadout
    Inventory inv = { 
        .slots = { BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_WATER, BLOCK_LAVA, BLOCK_WOOD, BLOCK_FIRE, BLOCK_AIR }, 
        .selected = 0 
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        
        // --- INPUTS ---
        
        // Select Material (1-8)
        for(int i=0; i<8; i++) {
            if(IsKeyPressed(KEY_ONE + i)) inv.selected = i;
        }

        // MINE (Left Click) -> Set to Air
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            int gx = mouse.x / CELL_SIZE;
            int gy = mouse.y / CELL_SIZE;
            EditWorld(gx, gy, BLOCK_AIR, 2); // 2-block radius brush
        }

        // BUILD (Right Click) -> Set to Material
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            int gx = mouse.x / CELL_SIZE;
            int gy = mouse.y / CELL_SIZE;
            
            // Collision check: Don't place blocks inside the player
            Vector2 pGrid = { player.position.x/CELL_SIZE, player.position.y/CELL_SIZE };
            if (Vector2Distance((Vector2){(float)gx, (float)gy}, pGrid) > 3.0f) {
                EditWorld(gx, gy, inv.slots[inv.selected], 2);
            }
        }

        // Reset
        if (IsKeyPressed(KEY_R)) InitWorld();

        // --- PHYSICS ---
        UpdatePlayer(&player, dt);
        UpdateWorld(); // Cellular Automata Step

        // --- RENDER ---
        BeginDrawing();
            ClearBackground((Color){20, 20, 30, 255}); // Dark Void
            
            DrawWorld();
            DrawPlayer(&player);
            DrawHUD(&player, &inv);
            
            // Cursor Highlight
            int gx = mouse.x / CELL_SIZE;
            int gy = mouse.y / CELL_SIZE;
            DrawRectangleLines(gx*CELL_SIZE, gy*CELL_SIZE, CELL_SIZE*4, CELL_SIZE*4, Fade(WHITE, 0.5f));
            
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}