#include "game.h"

// The World Grid (Double Buffered)
static Cell grid[GRID_H][GRID_W];
static Cell nextGrid[GRID_H][GRID_W]; 

// Generate noise colors for texture
Color GetBlockColor(BlockType t) {
    // Makes block not look too dull by tweaking the element's color
    int v = GetRandomValue(-15, 15);
    switch(t) {
        case BLOCK_STONE: return (Color){100+v, 100+v, 100+v, 255};
        case BLOCK_DIRT:  return (Color){120+v, 90+v, 40+v, 255};
        case BLOCK_SAND:  return (Color){230+v, 210+v, 100+v, 255};
        case BLOCK_WATER: return (Color){0, 150+v, 250, 200}; // Transparent Blue
        case BLOCK_LAVA:  return (Color){255, 100+v, 0, 255};
        case BLOCK_WOOD:  return (Color){139+v, 69+v, 19+v, 255};
        case BLOCK_FIRE:  return (Color){255, GetRandomValue(150,250), 0, 255}; // Flicker
        case BLOCK_SMOKE: return (Color){50+v, 50+v, 50+v, 150};
        default: return BLANK;
    }
}

// --- PHYSICS HELPER: DENSITY ---
int GetDensity(BlockType t) {
    switch(t) {
        case BLOCK_STONE: 
        case BLOCK_DIRT:
        case BLOCK_WOOD: return 1000; // Immovable Solids

        case BLOCK_SAND: return 500;
        
        case BLOCK_LAVA: return 100;  // Heavy Liquid
        case BLOCK_WATER: return 50;  // Light Liquid
        
        case BLOCK_SMOKE: return 5;   // Gas
        case BLOCK_FIRE: return 1;    // Plasma/Gas
        case BLOCK_AIR: return 0;     // Vacuum
        default: return 0;
    }
}

void InitWorld() {
    // 1. Generate a Perlin noise map using Raylib
    // Parameters: Width, Height, OffsetX, OffsetY, Scale
    // We use GetRandomValue for offsets so the map is different every time you press 'R'
    float freq = 0.2f; // frequency (noise scale)
        // Lower Scale (e.g., 1.0f): Zooms in. The blobs of Sand and Stone become huge.
        // Higher Scale (e.g., 10.0f): Zooms out. The terrain looks like scattered noise or static.

        // 8.0f+ = Tiny Features: Scattered static, small dots of stone/sand.
        // 4.0f = Medium Features: Distinct patches (Current setting).
        // 1.0f = Large Features: Massive continents of stone or sand.
        // 0.2f = Huge Features: The entire screen might be just one material.
    Image noiseMap = GenImagePerlinNoise(GRID_W, GRID_H, GetRandomValue(0, 1000), GetRandomValue(0, 1000), freq);
    
    // 2. Load the pixel data so we can read values
    Color* pixels = LoadImageColors(noiseMap);

    for(int y=0; y < GRID_H; y++){
        for(int x=0; x < GRID_W; x++){
            // 3. Read the noise value (0.0 to 1.0)
            // Since it's grayscale, R, G, and B are the same. We normalize to 0.0-1.0.
            float noiseVal = pixels[y * GRID_W + x].r / 255.0f;

            // SETUP FLOOR (Background)
            // The floor is always DIRT (or you can add noise for Stone floors)
            grid[y][x].floor = BLOCK_DIRT;
            // Generate the color ONCE and save it.
            grid[y][x].floorColor = GetBlockColor(BLOCK_DIRT);

            // SETUP OBJECTS (Foreground)
            // By default, the foreground is AIR (Empty, so we see the floor)
            BlockType fgType = BLOCK_AIR;

            // 4. Thresholding: Decide block based on noise height
            if (noiseVal < 0.30f) fgType = BLOCK_STONE;      // Hard patches
            else if (noiseVal < 0.35f) fgType = BLOCK_SAND;       // Transition border
            else if (noiseVal > 0.65f) fgType = BLOCK_WATER;       // Sandy patches
            
            // 5. Apply to Grid
            grid[y][x].type = fgType;
            grid[y][x].active = true;
            grid[y][x].color = GetBlockColor(fgType);
            // grid[y][x].life = 0;

            if (fgType == BLOCK_WATER) grid[y][x].life = 5;
            else grid[y][x].life = 0;
        }
    }

    // 6. Cleanup memory
    UnloadImageColors(pixels);
    UnloadImage(noiseMap);
}

bool IsSolid(BlockType t) {
    return (t == BLOCK_STONE || t == BLOCK_WOOD || t == BLOCK_SAND);
    // Dirst is not included because it's considered as the floor
    // These are ON TOP of the floor, which is dirt, that block player
}

bool IsValid(int x, int y) {
    return (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H);
}

// Brush Tool
void EditWorld(int x, int y, BlockType type, int radius) {
    for(int j = -radius; j <= radius; j++) {
        for(int i = -radius; i <= radius; i++) {
            int nx = x + i;
            int ny = y + j;
            if (IsValid(nx, ny)) {

                // If the user selects DIRT, they are "Cleaning" the foreground to reveal the floor
                BlockType placeType = (type == BLOCK_DIRT) ? BLOCK_AIR : type;

                // Don't overwrite Solids if we are placing fluid (unless clearing with Air)
                if (type == BLOCK_AIR || !IsSolid(grid[ny][nx].type)) {
                    grid[ny][nx].type = placeType;
                    // Add color based on what type of element on the screen
                    grid[ny][nx].color = GetBlockColor(placeType);
                    
                    // Initialize spread life
                    if (placeType == BLOCK_WATER) grid[ny][nx].life = 5; 
                    else if (type == BLOCK_LAVA) grid[ny][nx].life = 20;
                    else if (type == BLOCK_FIRE) grid[ny][nx].life = 100;
                    else grid[ny][nx].life = 0;
                    
                    grid[ny][nx].active = true; 
                }
            }
        }
    }
}

// --- CELLULAR AUTOMATA ENGINE ---
void UpdateWorld() {
    // 1. Copy State
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            nextGrid[y][x] = grid[y][x];
        }
    }

    // 2. Physics Pass
    for(int y = 0; y < GRID_H; y++) {
        for(int x = 0; x < GRID_W; x++) {
            Cell c = grid[y][x];

            // Skip Empty Air (Optimization)
            if (c.type == BLOCK_AIR || c.type == BLOCK_DIRT || IsSolid(c.type)) continue;

            // Coordinates (x, y)
            int dx = 0, dy = 0; 

            // --- FLUIDS (Water, Lava) ---
            if (c.type == BLOCK_WATER || c.type == BLOCK_LAVA) {
                if (c.life <= 0) continue; // Settled
                // Viscosity check
                // int skipChance = (c.type == BLOCK_LAVA) ? 10 : 2;
                // if (GetRandomValue(0, skipChance) != 0) continue;
                if (c.type == BLOCK_LAVA && GetRandomValue(0, 10) != 0) continue; // Viscosity
                if (c.type == BLOCK_WATER && GetRandomValue(0, 2) != 0) continue;

                

                // Random Direction
                int dir = GetRandomValue(0, 3);
                switch(dir){
                    case 0:
                        dy = -1;
                        break;
                    case 1:
                        dx = 1;
                        break;
                    case 2:
                        dy = 1;
                        break;
                    case 3:
                        dx = -1;
                        break;
                    default:
                        dx = 1;
                }

                if (IsValid(x+dx, y+dy)) {
                    BlockType target = grid[y+dy][x+dx].type;
                    int myDen = GetDensity(c.type);
                    int targetDen = GetDensity(target);

                    // Move if target is AIR or Lighter Fluid
                    // We check 'target != BLOCK_DIRT' just in case, to protect the floor.
                    if (target != BLOCK_DIRT && !IsSolid(target) && myDen > targetDen) {
                        // Check NextGrid to avoid race conditions
                        if (nextGrid[y+dy][x+dx].type == target) {
                            
                            // 1. Move Fluid to New Spot
                            nextGrid[y+dy][x+dx].type = c.type;
                            nextGrid[y+dy][x+dx].life = c.life - 1;
                            nextGrid[y+dy][x+dx].color = c.color;

                            // 2. Leave AIR behind at Old Spot (Revealing the Dirt Floor)
                            nextGrid[y][x].type = BLOCK_AIR; 
                            // Note: We do NOT touch .floor, so the dirt stays!
                        }
                    }
                }
            }

            // --- FIRE ---
            else if (c.type == BLOCK_FIRE) {
                // Spread
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
                // Decay
                nextGrid[y][x].life--;
                nextGrid[y][x].color = GetBlockColor(BLOCK_FIRE); 
                if(nextGrid[y][x].life <= 0) {
                    nextGrid[y][x].type = BLOCK_SMOKE;
                    nextGrid[y][x].life = 60;
                    nextGrid[y][x].color = GetBlockColor(BLOCK_SMOKE);
                }
            }

            // --- SMOKE ---
            else if (c.type == BLOCK_SMOKE) {
                if (GetRandomValue(0, 3) == 0) {
                    int dir = GetRandomValue(0, 3);
                    switch(dir){
                        case 0:
                            dy = -1;
                            break;
                        case 1:
                            dx = 1;
                            break;
                        case 2:
                            dy = 1;
                            break;
                        case 3:
                            dx = -1;
                            break;
                        default:
                            dx = 1;
                    }
                    
                    if (IsValid(x+dx, y+dy) && grid[y+dy][x+dx].type == BLOCK_DIRT) {
                         if (nextGrid[y+dy][x+dx].type == BLOCK_DIRT) {
                            nextGrid[y+dy][x+dx] = c;
                            nextGrid[y][x].type = BLOCK_DIRT;
                         }
                    }
                }
                nextGrid[y][x].life--;
                if(nextGrid[y][x].life <= 0) nextGrid[y][x].type = BLOCK_DIRT;
            }
        }
    }

    // 3. Swap (Apply)
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x] = nextGrid[y][x];
        }
    }
}

// --- RENDER GRID ---
void DrawWorld() {
    // Thickness of the border lines (1 or 2 looks best)
    int lineThickness = 2; 

    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            Cell c = grid[y][x];
            // if (c.type == BLOCK_AIR) continue;

            int px = x * CELL_SIZE;
            int py = y * CELL_SIZE;

            // ALWAYS Draw Floor First (Dirt)
            // Even if there is water, we draw dirt first so it shows through transparent water
            DrawRectangle(px, py, CELL_SIZE, CELL_SIZE, c.floorColor);

            // Draw Foreground (If not Air)
            if (c.type != BLOCK_AIR) {
                DrawRectangle(px, py, CELL_SIZE, CELL_SIZE, c.color);
            }

            // 2. Draw Outlines ONLY for Solid blocks (Walls)
            if (IsSolid(c.type)) {
                Color outlineColor = Fade(BLACK, 0.5f);
                
                // --- BORDER LOGIC ---
                // We draw a border if the neighbor is a DIFFERENT TYPE.
                // This separates Stone from Dirt, but also Stone from Wood.
                
                // Check UP
                if (IsValid(x, y-1) && grid[y-1][x].type != c.type) 
                    DrawRectangle(px, py, CELL_SIZE, lineThickness, outlineColor);
                
                // Check DOWN
                if (IsValid(x, y+1) && grid[y+1][x].type != c.type) 
                    DrawRectangle(px, py + CELL_SIZE - lineThickness, CELL_SIZE, lineThickness, outlineColor);

                // Check LEFT
                if (IsValid(x-1, y) && grid[y][x-1].type != c.type) 
                    DrawRectangle(px, py, lineThickness, CELL_SIZE, outlineColor);

                // Check RIGHT
                if (IsValid(x+1, y) && grid[y][x+1].type != c.type) 
                    DrawRectangle(px + CELL_SIZE - lineThickness, py, lineThickness, CELL_SIZE, outlineColor);
            }
        }
    }
}

// --- PLAYER PHYSICS (MOVE AND SLIDE) ---

void InitPlayer(Player* p) {
    p->position = (Vector2){400, 300};
    p->velocity = (Vector2){0,0};
    p->size = 12.0f;
    p->color = MAROON;
}

// Helper: Checks if a rectangle overlaps any SOLID blocks in the grid
bool CheckCollision(Vector2 pos, float radius) {
    int minX = (int)((pos.x - radius) / CELL_SIZE);
    int maxX = (int)((pos.x + radius) / CELL_SIZE);
    int minY = (int)((pos.y - radius) / CELL_SIZE);
    int maxY = (int)((pos.y + radius) / CELL_SIZE);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (IsValid(x, y) && IsSolid(grid[y][x].type)) {
                return true;
            }
        }
    }
    return false;
}

void UpdatePlayer(Player* p, float dt) {
    Vector2 input = {0,0};
    if(IsKeyDown(KEY_W) || IsKeyDown(KEY_S)) input.y = IsKeyDown(KEY_W)? -1: 1;
    if(IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) input.x = IsKeyDown(KEY_A)? -1: 1;
    
    if (Vector2Length(input) > 0) input = Vector2Normalize(input);
    
    // This holds the time consistency without using GetFrameTime
    p->velocity = Vector2Scale(input, 200.0f); // 200 pixels/sec speed
    
    // --- MOVE AND SLIDE LOGIC ---
    
    // 1. Try Moving X
    Vector2 nextPosX = { p->position.x + p->velocity.x * dt, p->position.y };
    if (!CheckCollision(nextPosX, p->size)) {
        p->position.x = nextPosX.x;
    }
    
    // 2. Try Moving Y (Independent of X)
    Vector2 nextPosY = { p->position.x, p->position.y + p->velocity.y * dt };
    if (!CheckCollision(nextPosY, p->size)) {
        p->position.y = nextPosY.y;
    }
    
    // 3. Screen Bounds Check
    if (p->position.x < p->size) p->position.x = p->size;
    if (p->position.x > SCREEN_WIDTH - p->size) p->position.x = SCREEN_WIDTH - p->size;
    if (p->position.y < p->size) p->position.y = p->size;
    if (p->position.y > SCREEN_HEIGHT - p->size) p->position.y = SCREEN_HEIGHT - p->size;
}

void DrawPlayer(Player* p) {
    DrawCircle(p->position.x, p->position.y, p->size*2, p->color);
    DrawCircleLines(p->position.x, p->position.y, p->size*2, BLACK);
    // DrawRectangle(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, p->color);
    // DrawRectangleLines(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, BLACK);
}

void Trail(Player* p, Vector2 *trailPositions){
        // Draw trail
    // Draw the trail by looping through the history array
    for (int i = 0; i < MAX_TRAIL_LENGTH; i++)
    {
        // Ensure we skip drawing if the array hasn't been fully filled on startup
        if ((trailPositions[i].x != 0.0f) || (trailPositions[i].y != 0.0f))
        {
            // Calculate relative trail strength (ratio is near 1.0 for new, near 0.0 for old)
            float ratio = (float)(MAX_TRAIL_LENGTH - i)/MAX_TRAIL_LENGTH - 0.1;

            // Fade effect: oldest positions are more transparent
            // Fade (color, alpha) - alpha is 0.5 to 1.0 based on ratio
            Color trailColor = Fade(LIGHTGRAY, ratio*0.5f + 0.5f);

            // Size effect: oldest positions are smaller
            float trailRadius = 15.0f*ratio;

            DrawCircleV(trailPositions[i], trailRadius, trailColor);
        }
    }
}


void UpdateTrail(Vector2* trailPositions, Player p){
    // Shift all existing positions backward by one slot in the array
    // The last element (the oldest position) is dropped
    for (int i = MAX_TRAIL_LENGTH - 1; i > 0; i--)
    {
        trailPositions[i] = trailPositions[i - 1];
    }

    // Store the new, current mouse position at the start of the array (Index 0)
    trailPositions[0] = p.position;
}