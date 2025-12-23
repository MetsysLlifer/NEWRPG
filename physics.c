#include "game.h"

static Cell grid[GRID_H][GRID_W];
static Cell nextGrid[GRID_H][GRID_W]; 

Color GetBlockColor(BlockType t) {
    int v = GetRandomValue(-15, 15);
    switch(t) {
        case BLOCK_STONE: return (Color){100+v, 100+v, 100+v, 255};
        case BLOCK_DIRT:  return (Color){120+v, 90+v, 40+v, 255};
        case BLOCK_SAND:  return (Color){230+v, 210+v, 100+v, 255};
        case BLOCK_WATER: return (Color){0, 150+v, 250+v, 200}; 
        case BLOCK_LAVA:  return (Color){255, 100+v, 0, 255};
        case BLOCK_WOOD:  return (Color){139+v, 69+v, 19+v, 255};
        case BLOCK_FIRE:  return (Color){255, GetRandomValue(150,250), 0, 255};
        case BLOCK_SMOKE: return (Color){50+v, 50+v, 50+v, 150};
        default: return BLANK;
    }
}

// --- PHYSICS PROPERTIES ---

bool IsSolid(BlockType t) {
    return (t == BLOCK_STONE || t == BLOCK_WOOD || t == BLOCK_DIRT || t == BLOCK_SAND);
}

// Higher number = Heavier/Denser
// Heavier things displace lighter things
int GetDensity(BlockType t) {
    switch(t) {
        case BLOCK_STONE: 
        case BLOCK_DIRT:
        case BLOCK_SAND:
        case BLOCK_WOOD: return 1000; // Immovable Solids
        
        case BLOCK_LAVA: return 100;  // Heavy Liquid
        case BLOCK_WATER: return 50;  // Light Liquid
        
        case BLOCK_SMOKE: return 5;   // Gas
        case BLOCK_FIRE: return 1;    // Plasma/Gas
        case BLOCK_AIR: return 0;     // Vacuum
        default: return 0;
    }
}

bool IsValid(int x, int y) {
    return (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H);
}

void InitWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x].type = BLOCK_AIR;
            grid[y][x].active = false;
        }
    }
}

void EditWorld(int x, int y, BlockType type, int radius) {
    for(int j = -radius; j <= radius; j++) {
        for(int i = -radius; i <= radius; i++) {
            int nx = x + i; int ny = y + j;
            if (IsValid(nx, ny)) {
                if (type == BLOCK_AIR || !IsSolid(grid[ny][nx].type)) {
                    grid[ny][nx].type = type;
                    grid[ny][nx].color = GetBlockColor(type);
                    
                    // --- SPREAD/LIFE INITIALIZATION ---
                    // Water spreads moderately (50 steps)
                    // Lava spreads shortly (20 steps)
                    // Fire burns briefly (100 ticks)
                    if (type == BLOCK_WATER) grid[ny][nx].life = 50; 
                    else if (type == BLOCK_LAVA) grid[ny][nx].life = 20;
                    else if (type == BLOCK_FIRE) grid[ny][nx].life = 100;
                    else grid[ny][nx].life = 0;
                    
                    grid[ny][nx].active = true; 
                }
            }
        }
    }
}

// --- TOP-DOWN PHYSICS ---
void UpdateWorld() {
    // 1. Snapshot
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            nextGrid[y][x] = grid[y][x];
        }
    }

    // 2. Logic Pass
    for(int y = 0; y < GRID_H; y++) {
        for(int x = 0; x < GRID_W; x++) {
            Cell c = grid[y][x];
            if (c.type == BLOCK_AIR) continue;

            int dx = 0, dy = 0; 

            // --- FLUIDS (Water/Lava) ---
            if (c.type == BLOCK_WATER || c.type == BLOCK_LAVA) {
                
                // 1. SPREAD LIMIT (Stamina)
                // If life runs out, it settles (becomes a static puddle)
                if (c.life <= 0) continue; 

                // 2. VISCOSITY (Speed)
                // Water moves often (1 in 4 skip)
                // Lava moves rarely (10 in 11 skip) -> Viscous/Thick
                int skipChance = (c.type == BLOCK_LAVA) ? 10 : 2;
                if (GetRandomValue(0, skipChance) != 0) continue;

                // 3. DIRECTION
                int dir = GetRandomValue(0, 3);
                if (dir == 0) dy = -1;
                else if (dir == 1) dx = 1;
                else if (dir == 2) dy = 1;
                else if (dir == 3) dx = -1;

                if (IsValid(x+dx, y+dy)) {
                    BlockType targetType = grid[y+dy][x+dx].type;
                    int myDensity = GetDensity(c.type);
                    int targetDensity = GetDensity(targetType);

                    // MOVE: If target is Air
                    // SWAP: If target is lighter fluid (Density Check)
                    // e.g., Lava (100) will push Water (50)
                    if (targetType == BLOCK_AIR || (targetType != BLOCK_AIR && !IsSolid(targetType) && myDensity > targetDensity)) {
                        
                        // Check NextGrid to avoid race conditions
                        if (nextGrid[y+dy][x+dx].type == targetType) {
                            
                            // Move Self
                            nextGrid[y+dy][x+dx] = c;
                            nextGrid[y+dy][x+dx].life--; // Reduce spread stamina
                            
                            // Displace Target (Swap)
                            nextGrid[y][x] = grid[y+dy][x+dx]; 
                        }
                    }
                }
            }

            // --- FIRE (Spreading) ---
            else if (c.type == BLOCK_FIRE) {
                // Spread to Wood
                for(int i=-1; i<=1; i++) {
                    for(int j=-1; j<=1; j++) {
                        if(IsValid(x+i, y+j) && grid[y+j][x+i].type == BLOCK_WOOD) {
                            if(GetRandomValue(0, 20) == 0) {
                                nextGrid[y+j][x+i].type = BLOCK_FIRE;
                                nextGrid[y+j][x+i].life = 150;
                                nextGrid[y+j][x+i].color = GetBlockColor(BLOCK_FIRE);
                            }
                        }
                    }
                }
                nextGrid[y][x].life--;
                nextGrid[y][x].color = GetBlockColor(BLOCK_FIRE); // Flicker
                if(nextGrid[y][x].life <= 0) {
                    nextGrid[y][x].type = BLOCK_SMOKE;
                    nextGrid[y][x].life = 60;
                    nextGrid[y][x].color = GetBlockColor(BLOCK_SMOKE);
                }
            }

            // --- SMOKE ---
            else if (c.type == BLOCK_SMOKE) {
                if (GetRandomValue(0, 3) == 0) { // Waft randomly
                    int dir = GetRandomValue(0, 3);
                    if (dir == 0) dy = -1; else if (dir == 1) dx = 1; 
                    else if (dir == 2) dy = 1; else if (dir == 3) dx = -1;
                    
                    if (IsValid(x+dx, y+dy) && grid[y+dy][x+dx].type == BLOCK_AIR) {
                         if (nextGrid[y+dy][x+dx].type == BLOCK_AIR) {
                            nextGrid[y+dy][x+dx] = c;
                            nextGrid[y][x].type = BLOCK_AIR;
                         }
                    }
                }
                nextGrid[y][x].life--;
                if(nextGrid[y][x].life <= 0) nextGrid[y][x].type = BLOCK_AIR;
            }
        }
    }

    // 3. Swap
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x] = nextGrid[y][x];
        }
    }
}

void DrawWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            if (grid[y][x].type != BLOCK_AIR) {
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, grid[y][x].color);
            }
        }
    }
}

// --- PLAYER ---
void InitPlayer(Player* p) {
    p->position = (Vector2){400, 300};
    p->velocity = (Vector2){0,0};
    p->size = 12.0f;
    p->color = MAROON;
}

void UpdatePlayer(Player* p, float dt) {
    Vector2 input = {0,0};
    if (IsKeyDown(KEY_W)) input.y -= 1;
    if (IsKeyDown(KEY_S)) input.y += 1;
    if (IsKeyDown(KEY_A)) input.x -= 1;
    if (IsKeyDown(KEY_D)) input.x += 1;
    
    if (Vector2Length(input) > 0) input = Vector2Normalize(input);
    
    p->velocity = Vector2Scale(input, 200.0f);
    Vector2 nextPos = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
    
    // Collision checking (4 corners)
    int cornersX[4] = { -1, 1, -1, 1 };
    int cornersY[4] = { -1, -1, 1, 1 };
    bool collision = false;
    
    for(int i=0; i<4; i++) {
        int gx = (nextPos.x + cornersX[i]*p->size) / CELL_SIZE;
        int gy = (nextPos.y + cornersY[i]*p->size) / CELL_SIZE;
        
        // Player only collides with Solids (Stone/Sand/Wood)
        // Player WALKS THROUGH Water/Lava (but maybe we can add drag later)
        if (IsValid(gx, gy) && IsSolid(grid[gy][gx].type)) collision = true;
    }
    
    if (!collision) p->position = nextPos;
}

void DrawPlayer(Player* p) {
    DrawRectangle(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, p->color);
    DrawRectangleLines(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, BLACK);
}