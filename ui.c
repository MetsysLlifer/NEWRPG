#include "game.h"

const char* BlockNames[] = { "Air", "Stone", "Dirt", "Sand", "Water", "Lava", "Wood", "Fire", "Smoke" };
Color BlockColors[] = { BLACK, GRAY, DARKBROWN, GOLD, BLUE, ORANGE, BROWN, RED, DARKGRAY };

void DrawHUD(Player* p, Inventory* inv) {
    int startX = SCREEN_WIDTH/2 - (9 * 45)/2;
    int y = SCREEN_HEIGHT - 60;
    
    for(int i=0; i<8; i++) {
        Rectangle slot = { startX + i*45, y, 40, 40 };
        
        if (inv->selected == i) {
            DrawRectangleRec(slot, WHITE);
            DrawRectangle(slot.x+2, slot.y+2, 36, 36, Fade(BlockColors[inv->slots[i]], 0.8f));
        } else {
            DrawRectangleRec(slot, Fade(BLACK, 0.5f));
            DrawRectangle(slot.x+2, slot.y+2, 36, 36, BlockColors[inv->slots[i]]);
        }
        
        DrawRectangleLinesEx(slot, 1, DARKGRAY);
        DrawText(TextFormat("%d", i+1), slot.x+2, slot.y+2, 10, WHITE);
    }
    
    DrawText(TextFormat("Selected: %s", BlockNames[inv->slots[inv->selected]]), 20, 20, 20, WHITE);
    DrawText("L-Click: Mine | R-Click: Place | R: Reset", 20, 50, 10, LIGHTGRAY);
}