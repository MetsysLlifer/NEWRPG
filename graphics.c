#include "game.h"
#include <math.h>

void DrawGame(Entity* entities, int count, Rectangle* walls, int wallCount) {
    float time = GetTime();

    for (int i = 0; i < wallCount; i++) {
        DrawRectangleRec(walls[i], GRAY);
        DrawRectangleLinesEx(walls[i], 2, DARKGRAY);
    }

    for (int i = 0; i < count; i++) {
        Entity* e = &entities[i];
        if (!e->isActive) continue;

        int chunks = GetSpellChunkCount(e);
        
        for(int k=0; k<chunks; k++) {
            Vector2 pos = GetSpellChunkPos(e, k, time);
            float size = GetSpellChunkSize(e, k);
            
            // --- NEW WATER ANIMATION ---
            if (e->spellData.core == ELEM_WATER) {
                // If it's a Puddle (Raw State)
                if (e->state == STATE_RAW) {
                    // Draw Metaball-like blob
                    float wobble = sinf(time * 5.0f + k) * 2.0f;
                    float wobble2 = cosf(time * 3.0f + k) * 2.0f;
                    
                    // Main body
                    DrawCircleV((Vector2){pos.x + wobble, pos.y + wobble2}, size * 1.2f, Fade(BLUE, 0.6f));
                    // Highlight
                    DrawCircleV((Vector2){pos.x + wobble - 2, pos.y + wobble2 - 2}, size * 0.4f, Fade(WHITE, 0.4f));
                } 
                else {
                    // Flowing projectile
                    float stretch = Vector2Length(e->velocity) / 200.0f;
                    if(stretch > 2.0f) stretch = 2.0f;
                    
                    // Rotate based on velocity
                    float rot = atan2f(e->velocity.y, e->velocity.x) * RAD2DEG;
                    
                    Rectangle drop = { pos.x, pos.y, size*2 + (size*stretch), size*1.5f };
                    DrawRectanglePro(drop, (Vector2){drop.width/2, drop.height/2}, rot, Fade(BLUE, 0.8f));
                    DrawCircleV(pos, size*0.8f, SKYBLUE); // Core
                }
            }
            // --- FIRE ANIMATION ---
            else if (e->spellData.core == ELEM_FIRE) {
                // Flickering embers
                float flicker = (sinf(time * 30.0f + k) + 1.0f) / 2.0f; // 0.0 to 1.0
                Color c = (k%2==0) ? RED : ORANGE;
                c = (flicker > 0.5f) ? c : YELLOW;
                
                DrawRectangle(pos.x - size, pos.y - size, size*2, size*2, c);
            }
            // --- VOID ANIMATION ---
            else if (e->spellData.behavior == SPELL_VOID) {
                float pulse = sinf(time * 10.0f) * 5.0f;
                DrawCircleV(pos, size + pulse, BLACK);
                DrawCircleLines(pos.x, pos.y, size + 10, PURPLE);
                DrawCircleLines(pos.x, pos.y, size - 5, DARKPURPLE);
            }
            else if (e->spellData.behavior == SPELL_CHAIN_LIGHTNING) {
                 DrawLineEx(pos, Vector2Add(pos, (Vector2){(float)GetRandomValue(-20,20), (float)GetRandomValue(-20,20)}), 2.0f, YELLOW);
                 DrawCircleV(pos, size, YELLOW);
            }
            else if (e->spellData.behavior == SPELL_PHANTOM) {
                 DrawRectangle(pos.x - size, pos.y - size, size*2, size*2, Fade(WHITE, 0.3f));
            }
            else if (e->spellData.core == ELEM_EARTH || e->state == STATE_STATIC_WALL) {
                DrawRectangle(pos.x - size, pos.y - size, size*2, size*2, (e->state==STATE_STATIC_WALL)?e->color:DARKBROWN);
                DrawRectangleLines(pos.x - size, pos.y - size, size*2, size*2, BLACK);
            }
            else if (e->spellData.core == ELEM_AIR) {
                 DrawCircleV(pos, size, Fade(SKYBLUE, 0.5f));
                 DrawCircleLines(pos.x, pos.y, size, WHITE);
            }
            else {
                 DrawRectangle(pos.x - size, pos.y - size, size*2, size*2, e->color);
            }
        }
        
        if (e->state == STATE_PROJECTILE) {
            for(int j=0; j < e->spellData.auxCount; j++) {
                float angle = (time * 3.0f) + (j * (PI * 2 / e->spellData.auxCount));
                Vector2 offset = { cosf(angle)*30.0f, sinf(angle)*30.0f };
                Vector2 orbPos = Vector2Add(e->position, offset);
                DrawLineEx(e->position, orbPos, 1.0f, Fade(BLACK, 0.5f));
                DrawRectangle(orbPos.x-4, orbPos.y-4, 8, 8, GetElementColor(e->spellData.aux[j]));
            }
        }
    }
}