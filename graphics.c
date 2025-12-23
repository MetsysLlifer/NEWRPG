#include "game.h"
#include <math.h>

// --- OPTIMIZATION GLOBALS (Grid System) ---
#define GRID_CELL_SIZE 80
#define GRID_COLS (SCREEN_WIDTH / GRID_CELL_SIZE + 1)
#define GRID_ROWS (SCREEN_HEIGHT / GRID_CELL_SIZE + 1)
#define MAX_ENTITIES_PER_CELL 64 

static int cellCounts[GRID_ROWS][GRID_COLS];
static int cellContents[GRID_ROWS][GRID_COLS][MAX_ENTITIES_PER_CELL];

// --- GRASS SYSTEM ---

void InitGrass(GrassSystem* gs) {
    for(int i=0; i<MAX_GRASS; i++) {
        gs->blades[i].position = (Vector2){ (float)GetRandomValue(0, SCREEN_WIDTH), (float)GetRandomValue(0, SCREEN_HEIGHT) };
        gs->blades[i].angle = 0.0f;
        gs->blades[i].targetAngle = 0.0f;
        
        // Randomize physics properties
        gs->blades[i].stiffness = (float)GetRandomValue(3, 6) * 0.01f; 
        gs->blades[i].height = (float)GetRandomValue(8, 15); // Taller grass
        
        // Base Color (Standard Green)
        int g = GetRandomValue(120, 180);
        gs->blades[i].color = (Color){ 20, g, 20, 255 }; 
    }
}

void UpdateGrass(GrassSystem* gs, Entity* entities, int count) {
    float time = GetTime();

    // 1. POPULATE GRID
    for(int y=0; y<GRID_ROWS; y++) {
        for(int x=0; x<GRID_COLS; x++) cellCounts[y][x] = 0;
    }

    for(int i=0; i<count; i++) {
        if(!entities[i].isActive) continue;

        int minX = (int)((entities[i].position.x - entities[i].size - 30) / GRID_CELL_SIZE);
        int maxX = (int)((entities[i].position.x + entities[i].size + 30) / GRID_CELL_SIZE);
        int minY = (int)((entities[i].position.y - entities[i].size - 30) / GRID_CELL_SIZE);
        int maxY = (int)((entities[i].position.y + entities[i].size + 30) / GRID_CELL_SIZE);

        if (minX < 0) minX = 0; if (maxX >= GRID_COLS) maxX = GRID_COLS - 1;
        if (minY < 0) minY = 0; if (maxY >= GRID_ROWS) maxY = GRID_ROWS - 1;

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                if (cellCounts[y][x] < MAX_ENTITIES_PER_CELL) {
                    cellContents[y][x][cellCounts[y][x]++] = i;
                }
            }
        }
    }

    // 2. UPDATE BLADES
    for(int i=0; i<MAX_GRASS; i++) {
        Grass* g = &gs->blades[i];
        
        // Ambient Wind
        float wind = sinf(time * 1.5f + g->position.x * 0.02f + g->position.y * 0.02f) * 0.2f;
        float pushOffset = 0.0f;
        bool isInteracting = false;
        
        // Grid Lookup
        int gx = (int)(g->position.x / GRID_CELL_SIZE);
        int gy = (int)(g->position.y / GRID_CELL_SIZE);

        if (gx >= 0 && gx < GRID_COLS && gy >= 0 && gy < GRID_ROWS) {
            int localCount = cellCounts[gy][gx];
            
            for(int k=0; k<localCount; k++) {
                int entityIdx = cellContents[gy][gx][k];
                Entity* e = &entities[entityIdx];

                float radius = e->size + 15.0f; 
                float dist = Vector2Distance(g->position, e->position);
                
                if(dist < radius) {
                    // --- FIXED PHYSICS LOGIC ---
                    
                    // 1. Calculate Strength based on Proximity (0.0 to 1.0)
                    // Closer = Stronger. This ensures max bend at the center.
                    float strength = (1.0f - (dist / radius));
                    
                    // 2. Determine Direction
                    float dx = g->position.x - e->position.x;
                    float dir = (dx >= 0) ? 1.0f : -1.0f;
                    
                    // 3. Trample Fix: If directly vertical (dx ~ 0), force a side bend
                    // This prevents grass "standing up" when you walk straight up/down
                    if (fabsf(dx) < 2.0f) {
                        dir = (i % 2 == 0) ? 1.0f : -1.0f; // Alternate left/right
                    }
                    
                    // 4. Apply Force
                    // Multiply strength by max_bend (e.g., 2.0 radians)
                    pushOffset += dir * strength * 2.0f;
                    
                    isInteracting = true;
                }
            }
        }
        
        g->targetAngle = wind + pushOffset;
        
        // Asymmetric Physics: 
        // Snap fast when hit (0.3), Recover slow (stiffness)
        float lerpSpeed = isInteracting ? 0.3f : g->stiffness; 
        g->angle += (g->targetAngle - g->angle) * lerpSpeed;
        
        // Clamp Angle (Prevent spinning)
        if(g->angle > 1.8f) g->angle = 1.8f; 
        if(g->angle < -1.8f) g->angle = -1.8f;
    }
}

void DrawGrass(GrassSystem* gs) {
    for(int i=0; i<MAX_GRASS; i++) {
        Grass* g = &gs->blades[i];
        
        Vector2 start = g->position;
        float height = g->height;
        
        // Calculate Tip
        float tipX = start.x + sinf(g->angle) * height;
        float tipY = start.y - cosf(g->angle) * height;
        Vector2 tip = { tipX, tipY };
        
        // Calculate Base (Rotates slightly with angle)
        float baseWidth = 2.5f;
        float baseCos = cosf(g->angle) * baseWidth;
        float baseSin = sinf(g->angle) * baseWidth;
        
        Vector2 baseLeft  = { start.x - baseCos, start.y - baseSin };
        Vector2 baseRight = { start.x + baseCos, start.y + baseSin };
        
        // Gradient Color
        Color tipColor = g->color;
        tipColor.g = (unsigned char)(g->color.g + 40 > 255 ? 255 : g->color.g + 40);
        Color baseColor = g->color;
        baseColor.g = (unsigned char)(g->color.g - 40 < 0 ? 0 : g->color.g - 40);
        
        DrawTriangle(baseLeft, baseRight, tip, baseColor); 
        DrawPixelV(start, (Color){0, 40, 0, 100}); // Root shadow
    }
}

// --- MAIN GAME DRAWING ---

void DrawGame(Entity* entities, int count, Rectangle* walls, int wallCount) {
    float time = GetTime();

    // Draw Walls
    for (int i = 0; i < wallCount; i++) {
        DrawRectangleRec(walls[i], GRAY);
        DrawRectangleLinesEx(walls[i], 2, DARKGRAY);
    }

    // Draw Entities
    for (int i = 0; i < count; i++) {
        Entity* e = &entities[i];
        if (!e->isActive) continue;

        int chunks = GetSpellChunkCount(e);
        SpellBehavior b = e->spellData.behavior;
        
        for(int k=0; k<chunks; k++) {
            Vector2 pos = GetSpellChunkPos(e, k, time);
            float size = GetSpellChunkSize(e, k);
            
            // --- UNIQUE SPELL ANIMATIONS ---
            
            if (b == SPELL_NECROMANCY) {
                if (k == 0) { // Skull Head
                    DrawCircleV(pos, size, LIGHTGRAY);
                    DrawRectangle(pos.x-size/2, pos.y+size/2, size, size/2, LIGHTGRAY);
                    DrawCircle(pos.x-4, pos.y, 3, BLACK); 
                    DrawCircle(pos.x+4, pos.y, 3, BLACK);
                } else { // Bones
                    DrawRectanglePro((Rectangle){pos.x, pos.y, 8, 3}, (Vector2){4,1.5}, time*50+k*30, LIGHTGRAY);
                }
            }
            else if (b == SPELL_VAMPIRISM) {
                Vector2 v1 = {pos.x, pos.y - 5}; Vector2 v2 = {pos.x - 5, pos.y + 5}; Vector2 v3 = {pos.x + 5, pos.y + 5};
                DrawTriangle(v1, v2, v3, RED);
            }
            else if (b == SPELL_SWARM) {
                DrawCircleV(pos, size, BLACK);
                DrawLine(pos.x-3, pos.y, pos.x+3, pos.y, Fade(BLACK, 0.5f)); 
            }
            else if (b == SPELL_POISON) {
                float pulse = sinf(time*5.0f + k)*2.0f;
                DrawCircleV(pos, size + pulse, Fade(LIME, 0.6f));
                DrawCircleLines(pos.x, pos.y, size + pulse, GREEN);
            }
            else if (b == SPELL_HEAL) {
                DrawRectangle(pos.x-2, pos.y-6, 4, 12, GREEN); DrawRectangle(pos.x-6, pos.y-2, 12, 4, GREEN);
            }
            else if (b == SPELL_TSUNAMI) {
                DrawCircleV(pos, size, BLUE); DrawCircleLines(pos.x, pos.y, size, WHITE);
            }
            else if (b == SPELL_SNIPER) {
                DrawCircleV(pos, size, RED);
                DrawLineEx(pos, Vector2Subtract(pos, Vector2Scale(Vector2Normalize(e->velocity), 20)), 2.0f, RED);
            }
            else if (b == SPELL_REWIND) {
                DrawCircleLines(pos.x, pos.y, size, GOLD);
            }
            else if (b == SPELL_MIDAS) {
                DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, GOLD);
                DrawRectangleLines(pos.x-size, pos.y-size, size*2, size*2, YELLOW);
            }
            else if (b == SPELL_VOID) {
                DrawCircleV(pos, size + sinf(time*10)*2, BLACK);
                DrawCircleLines(pos.x, pos.y, size + 5, PURPLE);
            }
            else if (b == SPELL_WALL) {
                DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, DARKBROWN);
                DrawRectangleLines(pos.x-size, pos.y-size, size*2, size*2, BLACK);
            }
            else if (b == SPELL_CHAIN_LIGHTNING) {
                 DrawLineEx(pos, Vector2Add(pos, (Vector2){(float)GetRandomValue(-20,20), (float)GetRandomValue(-20,20)}), 2.0f, YELLOW);
            }
            else if (b == SPELL_PHANTOM) {
                 DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, Fade(WHITE, 0.3f));
            }
            else if (b == SPELL_PETRIFY) {
                 DrawPoly(pos, 6, size, 0, GRAY);
            }
            else if (b == SPELL_FREEZE) {
                 DrawPoly(pos, 3, size, time*90, SKYBLUE); 
            }
            else if (b == SPELL_GROWTH) {
                 DrawCircleV(pos, size, DARKGREEN); DrawCircleLines(pos.x, pos.y, size+2, GREEN);
            }
            else if (b == SPELL_SHRINK) {
                 DrawCircleLines(pos.x, pos.y, size, PURPLE); DrawCircleV(pos, size/2, PURPLE);
            }
            else if (b == SPELL_MAGNET) {
                 DrawRing(pos, size-2, size, 0, 180, 10, GRAY);
            }
            else if (b == SPELL_BOUNCE) {
                 DrawCircleV(pos, size, ORANGE); DrawCircleV(pos, size*0.5f, WHITE); 
            }
            else if (b == SPELL_LANDMINE) {
                 Color c = (fmodf(time, 1.0f) > 0.5f) ? RED : GRAY; 
                 DrawCircleV(pos, size, GRAY); DrawCircleV(pos, 3, c);
            }
            else if (b == SPELL_CLUSTER) {
                 DrawCircleV(pos, size, ORANGE); DrawCircleLines(pos.x, pos.y, size, RED);
            }
            
            // --- BASIC ELEMENT ANIMATIONS ---
            else if (e->spellData.core == ELEM_FIRE) {
                float f = sinf(time*30+k);
                DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, (f>0)?RED:ORANGE);
            }
            else if (e->spellData.core == ELEM_WATER) {
                if (e->state == STATE_RAW) {
                    // Puddle Wobble
                    float wobble = sinf(time * 5.0f + k) * 2.0f;
                    float wobble2 = cosf(time * 3.0f + k) * 2.0f;
                    DrawCircleV((Vector2){pos.x + wobble, pos.y + wobble2}, size * 1.2f, Fade(BLUE, 0.6f));
                    DrawCircleV((Vector2){pos.x + wobble - 2, pos.y + wobble2 - 2}, size * 0.4f, Fade(WHITE, 0.4f));
                } else {
                    // Projectile Stretch
                    float stretch = Vector2Length(e->velocity) / 200.0f; if(stretch > 2.0f) stretch = 2.0f;
                    float rot = atan2f(e->velocity.y, e->velocity.x) * RAD2DEG;
                    Rectangle drop = { pos.x, pos.y, size*2 + (size*stretch), size*1.5f };
                    DrawRectanglePro(drop, (Vector2){drop.width/2, drop.height/2}, rot, Fade(BLUE, 0.8f));
                    DrawCircleV(pos, size*0.8f, SKYBLUE);
                }
            }
            else if (e->spellData.core == ELEM_AIR) {
                 DrawCircleV(pos, size, Fade(SKYBLUE, 0.5f));
            }
            else if (e->spellData.core == ELEM_EARTH) {
                 DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, DARKBROWN);
                 DrawRectangleLines(pos.x-size, pos.y-size, size*2, size*2, BLACK);
            }
            else {
                 DrawRectangle(pos.x-size, pos.y-size, size*2, size*2, e->color);
            }
        }
        
        // Draw Connecting Lines (unless specific spell forbids it)
        if (e->state == STATE_PROJECTILE && b != SPELL_SWARM && b != SPELL_SNIPER && b != SPELL_TSUNAMI) {
            for(int j=0; j < e->spellData.auxCount; j++) {
                float angle = (time * 3.0f) + (j * (PI * 2 / e->spellData.auxCount));
                Vector2 offset = { cosf(angle)*30.0f, sinf(angle)*30.0f };
                Vector2 orbPos = Vector2Add(e->position, offset);
                DrawLineEx(e->position, orbPos, 1.0f, Fade(BLACK, 0.2f));
                DrawRectangle(orbPos.x-3, orbPos.y-3, 6, 6, GetElementColor(e->spellData.aux[j]));
            }
        }
    }
}