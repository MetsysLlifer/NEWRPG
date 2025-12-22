#include "game.h"

// Returns true if drawn as a Square
bool IsShapeRect(Entity* e) {
    if (e->state == STATE_STATIC_WALL) return true;
    if (!e->isSpell) return true; // Player
    ElementType core = e->spellData.core;
    SpellBehavior b = e->spellData.behavior;
    if (core == ELEM_EARTH || core == ELEM_FIRE) return true;
    if (b == SPELL_WALL || b == SPELL_MIDAS || b == SPELL_PHANTOM || b == SPELL_NECROMANCY) return true;
    return false;
}

void SpawnWaterSpread(Entity* parent, Entity* entities, int* count) {
    int drops = 5;
    for(int i=0; i<drops; i++) {
        if (*count >= INVENTORY_CAPACITY * 30) return;
        Entity drop = CreateRawElement(ELEM_WATER, parent->position);
        drop.size = parent->size * 0.5f; 
        drop.mass = 8.0f; drop.friction = 1.5f; 
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 150);
        drop.velocity = (Vector2){ cosf(angle)*speed, sinf(angle)*speed };
        drop.lifeTime = 0; drop.spellData.dryness = 0.0f; 
        entities[(*count)++] = drop;
    }
}

void UpdateEntityAI(Entity* e, Entity* entities, int count, Vector2 targetPos) {
    if (!e->isActive || !e->isSpell) return;
    
    // Confusion
    if (e->color.r == 255 && e->color.g == 105 && e->color.b == 180) { 
        e->velocity = (Vector2){ -e->velocity.y, e->velocity.x }; return;
    }

    int bestTarget = -1; float closest = 10000.0f;
    if (e->spellData.aiType != AI_LINEAR && e->spellData.aiType != AI_NONE) {
        for(int i=0; i<count; i++) {
            if(&entities[i] == e || !entities[i].isActive) continue;
            if(entities[i].state == STATE_STATIC_WALL || entities[i].state == STATE_RAW) {
                float d = Vector2Distance(e->position, entities[i].position);
                if(d < closest) { closest = d; bestTarget = i; }
            }
        }
        if(bestTarget != -1) targetPos = entities[bestTarget].position;
    }

    if (e->spellData.aiType == AI_HOMING) {
        Vector2 toTarget = Vector2Subtract(targetPos, e->position);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(toTarget), 30.0f));
    }
    else if (e->spellData.aiType == AI_PREDICT && bestTarget != -1) {
        Vector2 predicted = Vector2Add(targetPos, Vector2Scale(entities[bestTarget].velocity, 0.8f));
        Vector2 toTarget = Vector2Subtract(predicted, e->position);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(toTarget), 45.0f));
    }
    else if (e->spellData.aiType == AI_ORBIT) {
        float time = GetTime();
        Vector2 orbitPoint = { targetPos.x + cosf(time*4.0f)*80.0f, targetPos.y + sinf(time*4.0f)*80.0f };
        Vector2 toOrbit = Vector2Subtract(orbitPoint, e->position);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(toOrbit), 60.0f));
    }
    else if (e->spellData.aiType == AI_SWARM) {
        Vector2 swarmCenter = targetPos;
        Vector2 toCenter = Vector2Subtract(swarmCenter, e->position);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(toCenter), 20.0f));
        e->velocity.x += GetRandomValue(-5, 5); e->velocity.y += GetRandomValue(-5, 5);
    }
}

void UpdateEntityPhysics(Entity* e, Vector2 inputDirection, Rectangle* walls, int wallCount) {
    if (e->isHeld || e->state == STATE_STATIC_WALL) return; 
    float dt = GetFrameTime(); e->lifeTime += dt * 5.0f;

    if (e->state == STATE_RAW && e->spellData.core == ELEM_WATER && e->lifeTime > 600.0f) {
        e->size -= 0.05f; if(e->size <= 0) e->isActive = false;
    }

    if (Vector2Length(inputDirection) > 0) {
        inputDirection = Vector2Normalize(inputDirection);
        Vector2 desiredVelocity = Vector2Scale(inputDirection, e->maxSpeed);
        Vector2 steering = Vector2Subtract(desiredVelocity, e->velocity);
        if (Vector2Length(steering) > e->moveForce) { steering = Vector2Normalize(steering); steering = Vector2Scale(steering, e->moveForce); }
        Vector2 acceleration = Vector2Scale(steering, 1.0f / e->mass);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(acceleration, dt));
    } else {
        Vector2 frictionDir = Vector2Scale(Vector2Normalize(e->velocity), -1.0f);
        Vector2 frictionForce = Vector2Scale(frictionDir, e->friction * e->mass);
        if (Vector2Length(e->velocity) < 10.0f) e->velocity = (Vector2){0,0};
        else e->velocity = Vector2Add(e->velocity, Vector2Scale(frictionForce, dt));
    }

    int chunkCount = GetSpellChunkCount(e);
    bool isRect = IsShapeRect(e);
    
    // X Axis
    e->position.x += e->velocity.x * dt;
    for(int k=0; k<chunkCount; k++) {
        Vector2 cPos = GetSpellChunkPos(e, k, GetTime());
        float cSize = GetSpellChunkSize(e, k);
        Rectangle hitBox = { cPos.x - cSize, cPos.y - cSize, cSize*2, cSize*2 };
        
        if (e->spellData.behavior != SPELL_PHANTOM) {
            for (int w = 0; w < wallCount; w++) {
                bool hit = false;
                if (isRect) hit = CheckCollisionRecs(hitBox, walls[w]);
                else hit = CheckCollisionCircleRec(cPos, cSize, walls[w]);
                
                if (hit) { 
                    // Bounce
                    if(e->spellData.behavior == SPELL_BOUNCE) { e->position.x -= e->velocity.x * dt; e->velocity.x *= -1.5f; goto checkedX; }
                    e->position.x -= e->velocity.x * dt; e->velocity.x = 0; goto checkedX; 
                }
            }
        }
    }
    checkedX:;

    // Y Axis
    e->position.y += e->velocity.y * dt;
    for(int k=0; k<chunkCount; k++) {
        Vector2 cPos = GetSpellChunkPos(e, k, GetTime());
        float cSize = GetSpellChunkSize(e, k);
        Rectangle hitBox = { cPos.x - cSize, cPos.y - cSize, cSize*2, cSize*2 };
        
        if (e->spellData.behavior != SPELL_PHANTOM) {
            for (int w = 0; w < wallCount; w++) {
                bool hit = false;
                if (isRect) hit = CheckCollisionRecs(hitBox, walls[w]);
                else hit = CheckCollisionCircleRec(cPos, cSize, walls[w]);
                
                if (hit) { 
                    if(e->spellData.behavior == SPELL_BOUNCE) { e->position.y -= e->velocity.y * dt; e->velocity.y *= -1.5f; goto checkedY; }
                    e->position.y -= e->velocity.y * dt; e->velocity.y = 0; goto checkedY; 
                }
            }
        }
    }
    checkedY:;
    
    if (e->position.x < 0) { e->position.x = 0; e->velocity.x *= -1; }
    if (e->position.x > SCREEN_WIDTH) { e->position.x = SCREEN_WIDTH; e->velocity.x *= -1; }
    if (e->position.y < 0) { e->position.y = 0; e->velocity.y *= -1; }
    if (e->position.y > SCREEN_HEIGHT) { e->position.y = SCREEN_HEIGHT; e->velocity.y *= -1; }
}

void ApplySpellFieldEffects(Entity* entities, int count, ParticleSystem* ps) {
    for(int i=0; i<count; i++) {
        if(!entities[i].isActive || entities[i].state != STATE_PROJECTILE) continue;
        
        if(entities[i].spellData.behavior == SPELL_MAGNET) {
            for(int j=0; j<count; j++) {
                if(i==j || !entities[j].isActive) continue;
                float dist = Vector2Distance(entities[i].position, entities[j].position);
                if(dist < 400.0f) {
                    Vector2 pull = Vector2Subtract(entities[i].position, entities[j].position);
                    float force = (entities[j].spellData.core == ELEM_EARTH) ? 60.0f : 20.0f;
                    entities[j].velocity = Vector2Add(entities[j].velocity, Vector2Scale(Vector2Normalize(pull), force));
                }
            }
        }
        if(entities[i].spellData.behavior == SPELL_WHIRLWIND) {
             for(int j=0; j<count; j++) {
                if(i==j || !entities[j].isActive) continue;
                float dist = Vector2Distance(entities[i].position, entities[j].position);
                if(dist < 200.0f) {
                    Vector2 push = Vector2Subtract(entities[j].position, entities[i].position);
                    entities[j].velocity = Vector2Add(entities[j].velocity, Vector2Scale(Vector2Normalize(push), 80.0f));
                }
            }
        }
        if(entities[i].spellData.behavior == SPELL_VOID) {
            for(int j=0; j<count; j++) {
                if(i==j || !entities[j].isActive || entities[j].state == STATE_STATIC_WALL) continue;
                float dist = Vector2Distance(entities[i].position, entities[j].position);
                if(dist < 300.0f) {
                    Vector2 pull = Vector2Subtract(entities[i].position, entities[j].position);
                    entities[j].velocity = Vector2Add(entities[j].velocity, Vector2Scale(Vector2Normalize(pull), 20.0f));
                }
            }
        }
        if(entities[i].spellData.behavior == SPELL_TELEKINESIS) {
             for(int j=0; j<count; j++) {
                if(i==j || !entities[j].isActive) continue;
                float dist = Vector2Distance(entities[i].position, entities[j].position);
                if(dist < 200.0f) entities[j].velocity = Vector2Scale(entities[j].velocity, 0.1f);
            }
        }
    }
}

void ResolveEntityCollisions(Entity* entities, int* countPtr, Entity* player, ParticleSystem* ps) {
    int count = *countPtr; 
    
    for (int i = 0; i < count; i++) {
        if (!entities[i].isActive) continue;
        
        if (entities[i].state == STATE_PROJECTILE && entities[i].spellData.behavior == SPELL_WALL) {
             if (Vector2Length(entities[i].velocity) < 20.0f) { entities[i].state = STATE_STATIC_WALL; entities[i].mass = 5000.0f; entities[i].color = BROWN; }
        }
        if (entities[i].state == STATE_PROJECTILE && entities[i].spellData.core == ELEM_WATER) {
             if (Vector2Length(entities[i].velocity) < 10.0f) {
                 SpawnWaterSpread(&entities[i], entities, countPtr); entities[i].isActive = false; continue;
             }
        }

        for (int j = i + 1; j < count; j++) {
            if (!entities[j].isActive) continue;
            
            bool collision = false;
            int chunksI = GetSpellChunkCount(&entities[i]);
            int chunksJ = GetSpellChunkCount(&entities[j]);
            
            bool iRect = IsShapeRect(&entities[i]);
            bool jRect = IsShapeRect(&entities[j]);
            
            for(int ki=0; ki<chunksI; ki++) {
                for(int kj=0; kj<chunksJ; kj++) {
                    Vector2 pI = GetSpellChunkPos(&entities[i], ki, GetTime());
                    float sI = GetSpellChunkSize(&entities[i], ki);
                    Vector2 pJ = GetSpellChunkPos(&entities[j], kj, GetTime());
                    float sJ = GetSpellChunkSize(&entities[j], kj);
                    
                    bool hit = false;
                    if (iRect && jRect) {
                        Rectangle r1 = {pI.x-sI, pI.y-sI, sI*2, sI*2};
                        Rectangle r2 = {pJ.x-sJ, pJ.y-sJ, sJ*2, sJ*2};
                        hit = CheckCollisionRecs(r1, r2);
                    }
                    else if (!iRect && !jRect) {
                        hit = CheckCollisionCircles(pI, sI, pJ, sJ);
                    }
                    else {
                        Vector2 circPos = iRect ? pJ : pI;
                        float circRad = iRect ? sJ : sI;
                        Vector2 recCenter = iRect ? pI : pJ;
                        float recHalf = iRect ? sI : sJ;
                        Rectangle rec = {recCenter.x-recHalf, recCenter.y-recHalf, recHalf*2, recHalf*2};
                        hit = CheckCollisionCircleRec(circPos, circRad, rec);
                    }
                    
                    if (hit) { collision = true; goto hit; }
                }
            }
            hit:;

            if (collision) {
                Entity* proj = (entities[i].state == STATE_PROJECTILE) ? &entities[i] : (entities[j].state == STATE_PROJECTILE ? &entities[j] : NULL);
                Entity* target = (proj == &entities[i]) ? &entities[j] : &entities[i];
                
                if (proj && target->state != STATE_PROJECTILE) {
                    
                    if (proj->spellData.core == ELEM_FIRE) { target->color = DARKGRAY; target->health -= 25.0f; SpawnExplosion(ps, target->position, ORANGE); }
                    if (proj->spellData.core == ELEM_WATER) SpawnWaterSpread(proj, entities, countPtr);

                    if (proj->spellData.behavior == SPELL_MIDAS) {
                        target->state = STATE_STATIC_WALL; target->color = GOLD; target->mass = 5000.0f; target->velocity = (Vector2){0,0};
                    }
                    else if (proj->spellData.behavior == SPELL_PETRIFY) {
                        target->state = STATE_STATIC_WALL; target->color = GRAY; target->mass = 5000.0f; target->velocity = (Vector2){0,0};
                    }
                    else if (proj->spellData.behavior == SPELL_FREEZE) {
                        target->velocity = (Vector2){0,0}; target->color = SKYBLUE;
                    }
                    else if (proj->spellData.behavior == SPELL_SHRINK) {
                        target->size *= 0.5f; SpawnExplosion(ps, target->position, PURPLE);
                    }
                    else if (proj->spellData.behavior == SPELL_GROWTH) {
                        target->size *= 1.5f; SpawnExplosion(ps, target->position, GREEN);
                    }
                    else if (proj->spellData.behavior == SPELL_CLUSTER) {
                        SpawnExplosion(ps, proj->position, ORANGE); SpawnExplosion(ps, Vector2Add(proj->position, (Vector2){30,0}), RED);
                    }
                    else if (proj->spellData.behavior == SPELL_CHAIN_LIGHTNING) {
                         SpawnExplosion(ps, target->position, YELLOW);
                    }
                    else if (proj->spellData.behavior == SPELL_SLOW) {
                        target->velocity = Vector2Scale(target->velocity, 0.1f); target->color = BROWN;
                    }
                    else if (proj->spellData.behavior == SPELL_VAMPIRISM) {
                        player->health += 5.0f; if(player->health > player->maxHealth) player->health = player->maxHealth;
                        SpawnExplosion(ps, player->position, RED); 
                    }
                    else if (proj->spellData.behavior == SPELL_HEAL) {
                        player->health += 20.0f; if(player->health > player->maxHealth) player->health = player->maxHealth;
                        SpawnExplosion(ps, player->position, GREEN);
                    }
                    else if (proj->spellData.behavior == SPELL_NECROMANCY) {
                        if(target->state == STATE_STATIC_WALL) { target->state = STATE_RAW; target->color = DARKGRAY; }
                    }
                    else if (proj->spellData.behavior == SPELL_REWIND) {
                        target->velocity = Vector2Scale(target->velocity, -1.0f); target->position = (Vector2){400,300}; 
                    }
                    else if (proj->spellData.behavior == SPELL_CONFUSE) {
                        target->velocity = Vector2Scale(target->velocity, -1.5f); target->color = PINK; 
                    }
                    else if (proj->spellData.behavior == SPELL_POISON) {
                        target->color = LIME; target->health -= 5.0f; 
                    }
                    else if (proj->spellData.behavior == SPELL_REFLECT) {
                        target->velocity = Vector2Scale(target->velocity, -1.0f); 
                    }
                    else if (proj->spellData.behavior == SPELL_BOUNCE) {
                         Vector2 n = Vector2Normalize(Vector2Subtract(proj->position, target->position));
                         proj->velocity = Vector2Reflect(proj->velocity, n);
                         proj->velocity = Vector2Scale(proj->velocity, 1.2f);
                    }
                    
                    if(target->health <= 0) target->isActive = false;
                    
                    if (proj->spellData.behavior != SPELL_BOUNCE && proj->spellData.behavior != SPELL_CHAIN_LIGHTNING) {
                        proj->isActive = false; 
                    }
                    continue;
                }

                if (entities[i].state == STATE_PROJECTILE && entities[j].state == STATE_PROJECTILE) {
                    Spell fused = FuseSpellData(entities[i].spellData, entities[j].spellData);
                    SpawnExplosion(ps, entities[i].position, GOLD);
                    entities[i].spellData = fused; entities[i].size += 5.0f; entities[j].isActive = false;
                    continue; 
                }

                Vector2 diff = Vector2Subtract(entities[i].position, entities[j].position);
                if (Vector2Length(diff) == 0) diff = (Vector2){1,0};
                Vector2 push = Vector2Scale(Vector2Normalize(diff), 1.0f);
                entities[i].position = Vector2Add(entities[i].position, push);
                entities[j].position = Vector2Subtract(entities[j].position, push);
                Vector2 tempVel = entities[i].velocity; entities[i].velocity = entities[j].velocity; entities[j].velocity = tempVel;
            }
        }
    }
}

void EnforceWallConstraints(Entity* entities, int count, Rectangle* walls, int wallCount) {}