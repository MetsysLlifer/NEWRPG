#include "game.h"

Color GetElementColor(ElementType type) {
    switch (type) {
        case ELEM_EARTH: return ORANGE;      
        case ELEM_WATER: return SKYBLUE;     
        case ELEM_FIRE:  return RED;         
        case ELEM_AIR:   return PURPLE;      
        default: return LIGHTGRAY;
    }
}

Entity CreateRawElement(ElementType type, Vector2 pos) {
    Entity e = {0};
    e.position = pos;
    e.state = STATE_RAW;
    e.isActive = true;
    e.mass = 20.0f; e.friction = 5.0f; e.size = 15.0f;
    e.color = GetElementColor(type);
    e.maxSpeed = 0.0f; e.moveForce = 0.0f;
    e.spellData.core = type;
    e.spellData.auxCount = 0;
    if(type == ELEM_FIRE) { e.spellData.temperature = 100.0; e.spellData.dryness = 1.0; }
    else if(type == ELEM_WATER) { e.spellData.temperature = 20.0; e.spellData.dryness = 0.0; }
    else if(type == ELEM_EARTH) { e.spellData.temperature = 50.0; e.spellData.dryness = 1.0; }
    else if(type == ELEM_AIR) { e.spellData.temperature = 40.0; e.spellData.dryness = 0.5; }
    e.spellData.intensity = 0.5;
    return e;
}

int GetSpellChunkCount(Entity* e) {
    if (e->state == STATE_RAW) return 3;
    if (e->state == STATE_PROJECTILE) {
        SpellBehavior b = e->spellData.behavior;
        if (b == SPELL_SWARM || b == SPELL_CLUSTER || b == SPELL_POISON) return 20;
        if (b == SPELL_TSUNAMI || b == SPELL_WALL) return 15;
        if (b == SPELL_SNIPER || b == SPELL_VOID || b == SPELL_BOUNCE) return 1;
        if (b == SPELL_MIRROR) return 2;
        if (b == SPELL_NECROMANCY) return 5;
        if (e->spellData.core == ELEM_FIRE) return 12;
        if (e->spellData.core == ELEM_EARTH || b == SPELL_MIDAS) return 8;
        if (e->spellData.core == ELEM_WATER) return 8;
        if (e->spellData.core == ELEM_AIR) return 6;
        return 6; 
    }
    return 1; 
}

Vector2 GetSpellChunkPos(Entity* e, int index, float time) {
    Vector2 center = e->position;
    SpellBehavior b = e->spellData.behavior;
    if (e->state == STATE_RAW) {
        float offX = sinf(index * 99.0f + time * 2.0f) * 8.0f;
        float offY = cosf(index * 13.0f + time * 2.0f) * 8.0f;
        return Vector2Add(center, (Vector2){offX, offY});
    }
    if (e->state == STATE_PROJECTILE) {
        if (b == SPELL_TSUNAMI || b == SPELL_WALL) {
            Vector2 forward = Vector2Normalize(e->velocity);
            if(Vector2Length(e->velocity) < 1) forward = (Vector2){0,1};
            Vector2 right = { -forward.y, forward.x };
            float width = e->size * 3.0f; float step = width / 15.0f; float offset = (index - 7.5f) * step;
            float curve = (offset*offset) / width; 
            Vector2 pos = Vector2Add(center, Vector2Scale(right, offset));
            pos = Vector2Subtract(pos, Vector2Scale(forward, curve * 2.0f)); 
            return pos;
        }
        if (b == SPELL_SNIPER || b == SPELL_CHAIN_LIGHTNING) {
            Vector2 trail = Vector2Scale(Vector2Normalize(e->velocity), -1.0f);
            return Vector2Add(center, Vector2Scale(trail, index * 5.0f));
        }
        if (b == SPELL_SWARM || b == SPELL_POISON) {
            float angle = index * 137.5f; float dist = index * 2.0f + sinf(time * 10.0f + index) * 5.0f;
            return Vector2Add(center, (Vector2){ cosf(angle)*dist, sinf(angle)*dist });
        }
        if (b == SPELL_MIRROR) {
            Vector2 off = { 20.0f, 0.0f }; if (index == 1) off.x = -20.0f;
            float rot = atan2f(e->velocity.y, e->velocity.x);
            Vector2 rotated = { off.x*cosf(rot) - off.y*sinf(rot), off.x*sinf(rot) + off.y*cosf(rot) };
            return Vector2Add(center, rotated);
        }
        if (b == SPELL_REWIND) {
            float t = fmodf(time + index * 0.1f, 1.0f); float dist = (1.0f - t) * 50.0f; 
            float angle = index * (PI*2/6);
            return Vector2Add(center, (Vector2){ cosf(angle)*dist, sinf(angle)*dist });
        }
        float angle = (time * (b == SPELL_WHIRLWIND ? 15.0f : 2.0f)) + (index * (PI*2 / GetSpellChunkCount(e)));
        float dist = e->size + sinf(time*5.0f + index)*5.0f;
        if (e->spellData.core == ELEM_FIRE && b == SPELL_PROJECTILE) { 
             float jitterX = sinf(time * 20.0f + index) * 10.0f;
             return Vector2Add(center, (Vector2){jitterX, -index * 2.0f});
        }
        return Vector2Add(center, (Vector2){ cosf(angle)*dist, sinf(angle)*dist });
    }
    return center;
}

float GetSpellChunkSize(Entity* e, int index) {
    if (e->state == STATE_RAW) return e->size * 0.6f;
    SpellBehavior b = e->spellData.behavior;
    if (b == SPELL_VOID) return e->size * 1.5f;
    if (b == SPELL_SNIPER) return 4.0f;
    if (b == SPELL_SWARM) return 3.0f;
    if (b == SPELL_POISON) return 5.0f;
    if (b == SPELL_MIDAS) return 6.0f;
    if (b == SPELL_NECROMANCY && index == 0) return 12.0f; 
    return e->size * 0.5f;
}

void ApplyElementPhysics(Entity* e) {
    e->moveForce = 2000.0f;
    switch (e->spellData.core) {
        case ELEM_FIRE: e->mass = 1.0f; e->maxSpeed = 1000.0f; e->friction = 1.0f; break;
        case ELEM_EARTH: e->mass = 50.0f; e->maxSpeed = 500.0f; e->friction = 10.0f; break;
        case ELEM_WATER: e->mass = 10.0f; e->maxSpeed = 700.0f; e->friction = 2.0f; break;
        case ELEM_AIR: e->mass = 5.0f; e->maxSpeed = 900.0f; e->friction = 0.5f; break;
        default: e->mass = 20.0f; e->maxSpeed = 600.0f; e->friction = 5.0f; break;
    }
}

void RecalculateStats(Spell* s) {
    s->power = 10.0f; s->aiType = AI_LINEAR; s->behavior = SPELL_PROJECTILE;
    s->isTeleport = false; s->hasHoming = false; s->hasGravity = false;
    
    int air = 0, earth = 0, water = 0, fire = 0;
    for(int i=0; i<s->auxCount; i++) {
        if(s->aux[i] == ELEM_FIRE) fire++;
        if(s->aux[i] == ELEM_AIR) air++;
        if(s->aux[i] == ELEM_EARTH) earth++;
        if(s->aux[i] == ELEM_WATER) water++;
    }
    
    if (air >= 2) { s->aiType = AI_HOMING; s->hasHoming = true; }
    if (air >= 1 && fire >= 1) s->aiType = AI_PREDICT; 
    if (water >= 2) s->aiType = AI_ORBIT;       
    if (earth >= 2 && fire >= 1) s->aiType = AI_ERRATIC; 
    if (water >= 1 && air >= 1 && earth >= 1) s->aiType = AI_SWARM;
    
    if (s->core == ELEM_EARTH) {
        if (air >= 2) s->isTeleport = true; 
        else if (earth >= 2) s->behavior = SPELL_PETRIFY;
        else if (earth >= 1) s->behavior = SPELL_WALL;
        else if (water >= 2) s->behavior = SPELL_GROWTH;
        else if (fire >= 1 && air == 0) s->behavior = SPELL_LANDMINE;
        else if (water >= 1 && air >= 1) s->behavior = SPELL_REWIND;
        else if (air >= 1 && fire >= 1) s->behavior = SPELL_MAGNET;
    }
    else if (s->core == ELEM_FIRE) {
        if (earth >= 1 && fire == 0) s->behavior = SPELL_MIDAS;
        else if (fire >= 2) s->behavior = SPELL_BERSERK;
        else if (fire >= 1 && earth >= 1) s->behavior = SPELL_CLUSTER;
        else if (air >= 2) s->behavior = SPELL_VOID;
        else if (air >= 1 && earth == 0) s->behavior = SPELL_CHAIN_LIGHTNING;
        else if (water >= 1 && air >= 1) s->behavior = SPELL_CONFUSE;
        else if (air >= 1) s->behavior = SPELL_SHRINK;
    }
    else if (s->core == ELEM_WATER) {
        if (water >= 2) s->behavior = SPELL_TSUNAMI;
        else if (air >= 1 && earth == 0 && fire == 0) s->behavior = SPELL_FREEZE;
        else if (fire >= 1 && earth >= 1) s->behavior = SPELL_VAMPIRISM;
        else if (earth >= 1 && fire == 0) s->behavior = SPELL_BOUNCE;
        else if (air >= 1 && earth >= 1) s->behavior = SPELL_SWARM;
        else if (earth >= 1 && fire >= 1) s->behavior = SPELL_POISON;
        else if (air >= 1 && fire >= 1) s->behavior = SPELL_MIRROR;
        else if (s->auxCount > 0) s->behavior = SPELL_HEAL;
    }
    else if (s->core == ELEM_AIR) {
        if (air >= 2) s->behavior = SPELL_WHIRLWIND;
        else if (air >= 1 && fire == 0) { s->behavior = SPELL_TELEKINESIS; s->hasGravity = true; }
        else if (water >= 1 && earth == 0) s->behavior = SPELL_PHANTOM; 
        else if (fire >= 1 && earth >= 1) s->behavior = SPELL_SNIPER;
    }
}

void PerformSpatialFusion(Entity* entities, int count, int coreIndex, ParticleSystem* ps, Player* player) {
    Entity* core = &entities[coreIndex];
    Spell* s = &core->spellData;
    double totalTemp = s->temperature * s->intensity;
    double totalDry = s->dryness * s->intensity;
    double totalInt = s->intensity;

    for(int i = 1; i < count; i++) {
        if (i == coreIndex) continue; 
        if (!entities[i].isActive || entities[i].state != STATE_RAW) continue;
        float dist = Vector2Distance(core->position, entities[i].position);
        if (dist < FUSION_RADIUS) {
            if (s->auxCount < MAX_AUX) s->aux[s->auxCount++] = entities[i].spellData.core;
            totalTemp += entities[i].spellData.temperature * entities[i].spellData.intensity;
            totalDry += entities[i].spellData.dryness * entities[i].spellData.intensity;
            totalInt += entities[i].spellData.intensity;
            SpawnExplosion(ps, entities[i].position, entities[i].color);
            entities[i].isActive = false; 
        }
    }
    if(totalInt > 0) { s->temperature = totalTemp/totalInt; s->dryness = totalDry/totalInt; s->intensity = totalInt; }
    RecalculateStats(s);
    core->state = STATE_PROJECTILE; core->size = 25.0f; core->isSpell = true; 
    ApplyElementPhysics(core);
    sprintf(s->name, "Fused Spell");
    if (s->behavior != SPELL_PROJECTILE) {
        if (!player->book.discovered[s->behavior]) {
            player->book.discovered[s->behavior] = true;
            player->book.notificationTimer = 3.0f; 
            sprintf(player->book.notificationText, "NEW DISCOVERY!");
        }
    }
}

Spell FuseSpellData(Spell A, Spell B) {
    Spell C = {0};
    C.core = (A.intensity > B.intensity) ? A.core : B.core;
    for(int i=0; i<A.auxCount && C.auxCount < MAX_AUX; i++) C.aux[C.auxCount++] = A.aux[i];
    for(int i=0; i<B.auxCount && C.auxCount < MAX_AUX; i++) C.aux[C.auxCount++] = B.aux[i];
    double totalInt = A.intensity + B.intensity; if(totalInt==0) totalInt=1.0;
    C.temperature = ((A.temperature * A.intensity) + (B.temperature * B.intensity)) / totalInt;
    C.dryness = ((A.dryness * A.intensity) + (B.dryness * B.intensity)) / totalInt;
    C.intensity = totalInt;
    RecalculateStats(&C);
    sprintf(C.name, "Impact Fusion");
    return C;
}