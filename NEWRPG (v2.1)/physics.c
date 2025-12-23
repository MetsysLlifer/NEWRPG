#include "game.h"

// Returns true if drawn as a Square
bool IsShapeRect(Entity* e) {
    if (e->state == STATE_STATIC_WALL) return true;
    if (e->state == STATE_POOL) return true; // Pools are rects
    if (!e->isSpell) return true; 
    ElementType core = e->spellData.core;
    // Water is now drawn as rects
    if (core == ELEM_WATER || core == ELEM_EARTH || core == ELEM_FIRE) return true;
    SpellBehavior b = e->spellData.behavior;
    if (b == SPELL_WALL || b == SPELL_MIDAS || b == SPELL_PHANTOM || b == SPELL_NECROMANCY) return true;
    return false;
}

// --- FLUID PHYSICS: SPAWN ---
void SpawnWaterSpread(Entity* parent, Entity* entities, int* count) {
    int drops = 12; 
    for(int i=0; i<drops; i++) {
        if (*count >= MAX_ENTITIES - 1) return; 
        
        Entity drop = CreateRawElement(ELEM_WATER, parent->position);
        drop.size = 6.0f; 
        drop.mass = 1.0f; 
        drop.friction = 0.5f; 
        
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 150);
        
        drop.velocity = (Vector2){ cosf(angle)*speed, sinf(angle)*speed };
        drop.lifeTime = 0; 
        drop.spellData.dryness = 0.0f; 
        
        entities[(*count)++] = drop;
    }
}

void UpdateEntityAI(Entity* e, Entity* entities, int count, Vector2 targetPos) {
    if (!e->isActive || !e->isSpell) return;
    if (e->state == STATE_POOL) return; // Pools have no AI

    if (e->color.r == 255 && e->color.g == 105 && e->color.b == 180) { 
        e->velocity = (Vector2){ -e->velocity.y, e->velocity.x }; return;
    }

    if (e->spellData.aiType == AI_HOMING) {
        Vector2 toTarget = Vector2Subtract(targetPos, e->position);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(toTarget), 30.0f));
    }
}

void UpdateEntityPhysics(Entity* e, Vector2 inputDirection, Rectangle* walls, int wallCount) {
    if (e->isHeld || e->state == STATE_STATIC_WALL) return; 
    
    // --- POOL LOGIC ---
    if (e->state == STATE_POOL) {
        e->velocity = (Vector2){0,0}; // Freeze position
        e->lifeTime += GetFrameTime();
        if (e->lifeTime > 20.0f) { // Evaporate
            e->size -= 0.01f; 
            if(e->size <= 0) e->isActive = false;
        }
        return; 
    }

    float dt = GetFrameTime(); 
    e->lifeTime += dt * 5.0f;

    // Transition: Moving Water -> Static Pool
    if (e->state == STATE_RAW && e->spellData.core == ELEM_WATER) {
        if (Vector2Length(e->velocity) < 5.0f && e->lifeTime > 1.0f) {
            e->state = STATE_POOL;
            e->velocity = (Vector2){0,0};
            e->position.x = roundf(e->position.x);
            e->position.y = roundf(e->position.y);
            return;
        }
    }

    // Movement
    if (Vector2Length(inputDirection) > 0) {
        inputDirection = Vector2Normalize(inputDirection);
        Vector2 desiredVelocity = Vector2Scale(inputDirection, e->maxSpeed);
        Vector2 steering = Vector2Subtract(desiredVelocity, e->velocity);
        if (Vector2Length(steering) > e->moveForce) { steering = Vector2Normalize(steering); steering = Vector2Scale(steering, e->moveForce); }
        e->velocity = Vector2Add(e->velocity, Vector2Scale(steering, 1.0f/e->mass * dt));
    } else {
        float fric = e->friction;
        Vector2 frictionDir = Vector2Scale(Vector2Normalize(e->velocity), -1.0f);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(frictionDir, fric * e->mass * dt));
    }

    // --- WALL COLLISION ---
    e->position.x += e->velocity.x * dt;
    Rectangle hitBox = { e->position.x - e->size, e->position.y - e->size, e->size*2, e->size*2 };
    for (int w = 0; w < wallCount; w++) {
        if (CheckCollisionRecs(hitBox, walls[w])) {
            e->position.x -= e->velocity.x * dt;
            e->velocity.x *= -0.5f; 
        }
    }

    e->position.y += e->velocity.y * dt;
    hitBox.y = e->position.y - e->size; 
    for (int w = 0; w < wallCount; w++) {
        if (CheckCollisionRecs(hitBox, walls[w])) {
            e->position.y -= e->velocity.y * dt;
            e->velocity.y *= -0.5f;
        }
    }
    
    // Bounds
    if (e->position.x < 0) e->position.x = 0;
    if (e->position.x > SCREEN_WIDTH) e->position.x = SCREEN_WIDTH;
    if (e->position.y < 0) e->position.y = 0; 
    if (e->position.y > SCREEN_HEIGHT) e->position.y = SCREEN_HEIGHT; 
}

void ApplySpellFieldEffects(Entity* entities, int count, ParticleSystem* ps) {}

void ResolveEntityCollisions(Entity* entities, int* countPtr, Entity* player, ParticleSystem* ps) {
    int count = *countPtr; 
    
    for (int i = 0; i < count; i++) {
        if (!entities[i].isActive) continue;
        
        if (entities[i].state == STATE_PROJECTILE && entities[i].spellData.core == ELEM_WATER) {
             if (Vector2Length(entities[i].velocity) < 10.0f || entities[i].lifeTime > 10.0f) {
                 SpawnWaterSpread(&entities[i], entities, countPtr); 
                 entities[i].isActive = false; continue;
             }
        }

        for (int j = i + 1; j < count; j++) {
            if (!entities[j].isActive) continue;
            
            // Interaction: Entity walking on Pool
            Entity* pool = NULL;
            Entity* walker = NULL;
            if(entities[i].state == STATE_POOL && entities[j].state != STATE_POOL) { pool = &entities[i]; walker = &entities[j]; }
            else if(entities[j].state == STATE_POOL && entities[i].state != STATE_POOL) { pool = &entities[j]; walker = &entities[i]; }
            
            if (pool && walker) {
                if (fabsf(pool->position.x - walker->position.x) < (pool->size + walker->size) &&
                    fabsf(pool->position.y - walker->position.y) < (pool->size + walker->size)) 
                {
                    walker->velocity = Vector2Scale(walker->velocity, 0.95f); // Drag
                }
                continue; 
            }
            if(entities[i].state == STATE_POOL && entities[j].state == STATE_POOL) continue;

            // Standard Collision
            float rI = entities[i].size; 
            float rJ = entities[j].size;
            bool collision = (fabsf(entities[i].position.x - entities[j].position.x) * 2 < (rI + rJ) * 2) &&
                             (fabsf(entities[i].position.y - entities[j].position.y) * 2 < (rI + rJ) * 2);

            if (collision) {
                Entity* proj = (entities[i].state == STATE_PROJECTILE) ? &entities[i] : (entities[j].state == STATE_PROJECTILE ? &entities[j] : NULL);
                Entity* target = (proj == &entities[i]) ? &entities[j] : &entities[i];
                
                if (proj && target->state != STATE_PROJECTILE) {
                    if (proj->spellData.core == ELEM_FIRE) { target->color = DARKGRAY; target->health -= 25.0f; SpawnExplosion(ps, target->position, ORANGE); }
                    if (proj->spellData.core == ELEM_WATER) SpawnWaterSpread(proj, entities, countPtr);
                    if (proj->spellData.behavior != SPELL_BOUNCE && proj->spellData.behavior != SPELL_CHAIN_LIGHTNING) {
                        proj->isActive = false; 
                    }
                    continue;
                }

                Vector2 diff = Vector2Subtract(entities[i].position, entities[j].position);
                if (Vector2Length(diff) == 0) diff = (Vector2){0, -1};
                Vector2 push = Vector2Scale(Vector2Normalize(diff), 1.0f);
                entities[i].position = Vector2Add(entities[i].position, push);
                entities[j].position = Vector2Subtract(entities[j].position, push);
            }
        }
    }
}

void EnforceWallConstraints(Entity* entities, int count, Rectangle* walls, int wallCount) {}