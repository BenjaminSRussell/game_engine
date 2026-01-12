// Advanced AI Implementation
// Implements comprehensive artificial intelligence capabilities

#include "advanced_ai.h"
#include "src/engine/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Utility functions implementation
AIVector3 ai_vector3_make(float x, float y, float z) {
    AIVector3 v = {x, y, z};
    return v;
}

AIVector3 ai_vector3_add(const AIVector3* a, const AIVector3* b) {
    return ai_vector3_make(a->x + b->x, a->y + b->y, a->z + b->z);
}

AIVector3 ai_vector3_subtract(const AIVector3* a, const AIVector3* b) {
    return ai_vector3_make(a->x - b->x, a->y - b->y, a->z - b->z);
}

AIVector3 ai_vector3_multiply(const AIVector3* v, float scalar) {
    return ai_vector3_make(v->x * scalar, v->y * scalar, v->z * scalar);
}

float ai_vector3_distance(const AIVector3* a, const AIVector3* b) {
    AIVector3 diff = ai_vector3_subtract(a, b);
    return ai_vector3_length(&diff);
}

float ai_vector3_length(const AIVector3* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

AIVector3 ai_vector3_normalize(const AIVector3* v) {
    float length = ai_vector3_length(v);
    if (length > 0.0f) {
        return ai_vector3_multiply(v, 1.0f / length);
    }
    return ai_vector3_make(0.0f, 0.0f, 0.0f);
}

float ai_vector3_dot(const AIVector3* a, const AIVector3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

// AI world implementation
AIWorld* ai_world_create(uint32_t maxAgents) {
    if (maxAgents == 0) {
        LOG_ERROR("Invalid maxAgents for AI world creation");
        return NULL;
    }
    
    AIWorld* world = malloc(sizeof(AIWorld));
    if (!world) {
        LOG_ERROR("Failed to allocate memory for AI world");
        return NULL;
    }
    
    memset(world, 0, sizeof(AIWorld));
    world->maxAgents = maxAgents;
    world->agents = malloc(sizeof(AIAgent) * maxAgents);
    
    if (!world->agents) {
        LOG_ERROR("Failed to allocate memory for AI agents");
        free(world);
        return NULL;
    }
    
    world->globalTimeScale = 1.0f;
    world->enablePathfinding = true;
    world->enablePerception = true;
    world->enableBehaviorTrees = true;
    world->gridSize = 10.0f;
    
    LOG_INFO("AI world created with capacity for %u agents", maxAgents);
    return world;
}

void ai_world_destroy(AIWorld* world) {
    if (!world) return;
    
    // Destroy all agents
    for (uint32_t i = 0; i < world->agentCount; i++) {
        AIAgent* agent = &world->agents[i];
        if (agent->behaviorTree) {
            ai_behavior_destroy(agent->behaviorTree);
        }
        if (agent->perceptions) {
            free(agent->perceptions);
        }
        if (agent->memories) {
            free(agent->memories);
        }
        if (agent->currentPath) {
            free(agent->currentPath);
        }
    }
    
    if (world->agents) {
        free(world->agents);
    }
    
    if (world->navMesh) {
        ai_navmesh_destroy(world->navMesh);
    }
    
    free(world);
    LOG_INFO("AI world destroyed");
}

void ai_world_update(AIWorld* world, float deltaTime) {
    if (!world || deltaTime <= 0.0f) return;
    
    float scaledDeltaTime = deltaTime * world->globalTimeScale;
    
    for (uint32_t i = 0; i < world->agentCount; i++) {
        AIAgent* agent = &world->agents[i];
        if (!agent->isAlive || !agent->isActive) continue;
        
        // Update decision timer
        agent->decisionTimer += scaledDeltaTime;
        if (agent->decisionTimer >= agent->decisionInterval) {
            agent->decisionTimer = 0.0f;
            
            // Make AI decision
            AIStateType newState = ai_agent_decide_state(agent);
            if (newState != agent->currentState) {
                ai_agent_set_state(agent, newState);
            }
        }
        
        // Update perceptions
        if (world->enablePerception) {
            ai_agent_update_perceptions(agent, scaledDeltaTime);
        }
        
        // Update memories
        ai_agent_update_memories(agent, scaledDeltaTime);
        
        // Update current state behavior
        ai_agent_update_state(agent, scaledDeltaTime);
        
        // Follow path if exists
        if (world->enablePathfinding && agent->hasPath) {
            ai_agent_follow_path(agent, scaledDeltaTime);
        }
        
        // Execute behavior tree
        if (world->enableBehaviorTrees && agent->behaviorTree) {
            ai_behavior_execute(agent->behaviorTree, agent, scaledDeltaTime);
        }
    }
}

// Agent management
uint64_t ai_agent_create(AIWorld* world, AIAgentType type, const AIVector3* position) {
    if (!world || !position || world->agentCount >= world->maxAgents) {
        return 0;
    }
    
    uint64_t agentId = world->agentCount + 1; // 1-based indexing
    AIAgent* agent = &world->agents[world->agentCount];
    
    memset(agent, 0, sizeof(AIAgent));
    agent->id = agentId;
    agent->type = type;
    agent->position = *position;
    agent->currentState = AI_STATE_IDLE;
    agent->previousState = AI_STATE_IDLE;
    
    // Set default values
    agent->maxSpeed = 5.0f;
    agent->maxForce = 10.0f;
    agent->visualRange = 20.0f;
    agent->audioRange = 15.0f;
    agent->fieldOfView = 90.0f * (3.14159f / 180.0f); // Convert to radians
    agent->decisionInterval = 0.5f;
    agent->maxHealth = 100.0f;
    agent->health = agent->maxHealth;
    agent->isAlive = true;
    agent->isActive = true;
    
    // Allocate perception and memory arrays
    agent->maxPerceptions = 32;
    agent->perceptions = malloc(sizeof(AIPerception) * agent->maxPerceptions);
    agent->maxMemories = 64;
    agent->memories = malloc(sizeof(AIMemory) * agent->maxMemories);
    
    if (!agent->perceptions || !agent->memories) {
        LOG_ERROR("Failed to allocate memory for agent perceptions or memories");
        if (agent->perceptions) free(agent->perceptions);
        if (agent->memories) free(agent->memories);
        return 0;
    }
    
    world->agentCount++;
    LOG_DEBUG("Created AI agent %lu with type %d", agentId, type);
    return agentId;
}

void ai_agent_destroy(AIWorld* world, uint64_t agentId) {
    if (!world || agentId == 0 || agentId > world->agentCount) return;
    
    AIAgent* agent = &world->agents[agentId - 1];
    
    if (agent->behaviorTree) {
        ai_behavior_destroy(agent->behaviorTree);
        agent->behaviorTree = NULL;
    }
    
    if (agent->perceptions) {
        free(agent->perceptions);
        agent->perceptions = NULL;
    }
    
    if (agent->memories) {
        free(agent->memories);
        agent->memories = NULL;
    }
    
    if (agent->currentPath) {
        free(agent->currentPath);
        agent->currentPath = NULL;
    }
    
    agent->isAlive = false;
    agent->isActive = false;
    
    LOG_DEBUG("Destroyed AI agent %lu", agentId);
}

AIAgent* ai_agent_get(AIWorld* world, uint64_t agentId) {
    if (!world || agentId == 0 || agentId > world->agentCount) return NULL;
    return &world->agents[agentId - 1];
}

void ai_agent_set_position(AIWorld* world, uint64_t agentId, const AIVector3* position) {
    AIAgent* agent = ai_agent_get(world, agentId);
    if (!agent || !position) return;
    
    agent->position = *position;
    LOG_DEBUG("Set position for agent %lu", agentId);
}

void ai_agent_set_target(AIWorld* world, uint64_t agentId, const AIVector3* target) {
    AIAgent* agent = ai_agent_get(world, agentId);
    if (!agent || !target) return;
    
    agent->targetPosition = *target;
    LOG_DEBUG("Set target for agent %lu", agentId);
}

// State management
void ai_agent_set_state(AIAgent* agent, AIStateType newState) {
    if (!agent) return;
    
    agent->previousState = agent->currentState;
    agent->currentState = newState;
    agent->stateTimer = 0.0f;
    
    LOG_DEBUG("Agent %lu changed state from %d to %d", agent->id, agent->previousState, agent->currentState);
}

void ai_agent_update_state(AIAgent* agent, float deltaTime) {
    if (!agent || !agent->isAlive) return;
    
    agent->stateTimer += deltaTime;
    
    switch (agent->currentState) {
        case AI_STATE_IDLE:
            ai_behavior_idle(agent, deltaTime);
            break;
        case AI_STATE_PATROL:
            ai_behavior_patrol(agent, deltaTime);
            break;
        case AI_STATE_CHASE:
            ai_behavior_chase(agent, deltaTime);
            break;
        case AI_STATE_FLEE:
            ai_behavior_flee(agent, deltaTime);
            break;
        case AI_STATE_ATTACK:
            ai_behavior_attack(agent, deltaTime);
            break;
        case AI_STATE_SEARCH:
            ai_behavior_search(agent, deltaTime);
            break;
        default:
            break;
    }
}

// Perception system
void ai_agent_add_perception(AIAgent* agent, const AIPerception* perception) {
    if (!agent || !perception || agent->perceptionCount >= agent->maxPerceptions) return;
    
    agent->perceptions[agent->perceptionCount] = *perception;
    agent->perceptionCount++;
    
    LOG_DEBUG("Added perception to agent %lu", agent->id);
}

void ai_agent_update_perceptions(AIAgent* agent, float deltaTime) {
    if (!agent) return;
    
    // Remove old perceptions
    float currentTime = 0.0f; // In a real implementation, this would be the actual game time
    uint32_t writeIndex = 0;
    
    for (uint32_t i = 0; i < agent->perceptionCount; i++) {
        if (currentTime - agent->perceptions[i].timestamp < 5.0f) { // Keep perceptions for 5 seconds
            agent->perceptions[writeIndex] = agent->perceptions[i];
            writeIndex++;
        }
    }
    
    agent->perceptionCount = writeIndex;
}

bool ai_agent_can_see(AIAgent* agent, const AIVector3* target, float maxDistance) {
    if (!agent || !target) return false;
    
    float distance = ai_vector3_distance(&agent->position, target);
    if (distance > maxDistance || distance > agent->visualRange) return false;
    
    // Check field of view
    AIVector3 toTarget = ai_vector3_subtract(target, &agent->position);
    AIVector3 forward = ai_vector3_make(0.0f, 0.0f, 1.0f); // Assuming forward is Z axis
    
    float cosAngle = ai_vector3_dot(&forward, &toTarget) / (ai_vector3_length(&forward) * ai_vector3_length(&toTarget));
    float angle = acosf(cosAngle);
    
    return angle <= agent->fieldOfView;
}

bool ai_agent_can_hear(AIAgent* agent, const AIVector3* soundSource, float volume) {
    if (!agent || !soundSource) return false;
    
    float distance = ai_vector3_distance(&agent->position, soundSource);
    float effectiveRange = agent->audioRange * volume;
    
    return distance <= effectiveRange;
}

// Memory system
void ai_agent_add_memory(AIAgent* agent, const AIMemory* memory) {
    if (!agent || !memory || agent->memoryCount >= agent->maxMemories) return;
    
    agent->memories[agent->memoryCount] = *memory;
    agent->memoryCount++;
    
    LOG_DEBUG("Added memory to agent %lu", agent->id);
}

void ai_agent_update_memories(AIAgent* agent, float deltaTime) {
    if (!agent) return;
    
    // Decay and remove old memories
    uint32_t writeIndex = 0;
    
    for (uint32_t i = 0; i < agent->memoryCount; i++) {
        agent->memories[i].timestamp -= deltaTime;
        if (agent->memories[i].timestamp > 0.0f) {
            agent->memories[writeIndex] = agent->memories[i];
            writeIndex++;
        }
    }
    
    agent->memoryCount = writeIndex;
}

AIMemory* ai_agent_get_relevant_memory(AIAgent* agent, const AIVector3* position, float maxDistance) {
    if (!agent || !position) return NULL;
    
    AIMemory* bestMemory = NULL;
    float bestDistance = maxDistance;
    
    for (uint32_t i = 0; i < agent->memoryCount; i++) {
        float distance = ai_vector3_distance(&agent->memories[i].position, position);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestMemory = &agent->memories[i];
        }
    }
    
    return bestMemory;
}

// Built-in behaviors
bool ai_behavior_idle(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Simple idle behavior - just stand still
    agent->velocity = ai_vector3_make(0.0f, 0.0f, 0.0f);
    return true;
}

bool ai_behavior_patrol(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Simple patrol - move towards target if set, otherwise pick random point
    if (ai_agent_has_reached_target(agent, 1.0f)) {
        // Pick new random target
        float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        float distance = 5.0f + ((float)rand() / RAND_MAX) * 10.0f;
        
        agent->targetPosition.x = agent->position.x + cosf(angle) * distance;
        agent->targetPosition.z = agent->position.z + sinf(angle) * distance;
        agent->targetPosition.y = agent->position.y;
    }
    
    // Move towards target
    AIVector3 toTarget = ai_vector3_subtract(&agent->targetPosition, &agent->position);
    AIVector3 desired = ai_vector3_multiply(ai_vector3_normalize(&toTarget), agent->maxSpeed);
    AIVector3 steer = ai_vector3_subtract(&desired, &agent->velocity);
    
    // Limit steering force
    float steerLength = ai_vector3_length(&steer);
    if (steerLength > agent->maxForce) {
        steer = ai_vector3_multiply(ai_vector3_normalize(&steer), agent->maxForce);
    }
    
    agent->acceleration = steer;
    agent->velocity = ai_vector3_add(&agent->velocity, ai_vector3_multiply(&agent->acceleration, deltaTime));
    
    // Limit velocity
    float velocityLength = ai_vector3_length(&agent->velocity);
    if (velocityLength > agent->maxSpeed) {
        agent->velocity = ai_vector3_multiply(ai_vector3_normalize(&agent->velocity), agent->maxSpeed);
    }
    
    agent->position = ai_vector3_add(&agent->position, ai_vector3_multiply(&agent->velocity, deltaTime));
    
    return true;
}

bool ai_behavior_chase(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Move towards target at maximum speed
    AIVector3 toTarget = ai_vector3_subtract(&agent->targetPosition, &agent->position);
    AIVector3 desired = ai_vector3_multiply(ai_vector3_normalize(&toTarget), agent->maxSpeed);
    AIVector3 steer = ai_vector3_subtract(&desired, &agent->velocity);
    
    agent->acceleration = steer;
    agent->velocity = ai_vector3_add(&agent->velocity, ai_vector3_multiply(&agent->acceleration, deltaTime));
    
    // Limit velocity
    float velocityLength = ai_vector3_length(&agent->velocity);
    if (velocityLength > agent->maxSpeed) {
        agent->velocity = ai_vector3_multiply(ai_vector3_normalize(&agent->velocity), agent->maxSpeed);
    }
    
    agent->position = ai_vector3_add(&agent->position, ai_vector3_multiply(&agent->velocity, deltaTime));
    
    return true;
}

bool ai_behavior_flee(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Move away from target at maximum speed
    AIVector3 fromTarget = ai_vector3_subtract(&agent->position, &agent->targetPosition);
    AIVector3 desired = ai_vector3_multiply(ai_vector3_normalize(&fromTarget), agent->maxSpeed);
    AIVector3 steer = ai_vector3_subtract(&desired, &agent->velocity);
    
    agent->acceleration = steer;
    agent->velocity = ai_vector3_add(&agent->velocity, ai_vector3_multiply(&agent->acceleration, deltaTime));
    
    // Limit velocity
    float velocityLength = ai_vector3_length(&agent->velocity);
    if (velocityLength > agent->maxSpeed) {
        agent->velocity = ai_vector3_multiply(ai_vector3_normalize(&agent->velocity), agent->maxSpeed);
    }
    
    agent->position = ai_vector3_add(&agent->position, ai_vector3_multiply(&agent->velocity, deltaTime));
    
    return true;
}

bool ai_behavior_attack(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Move towards target and attack when close enough
    float distance = ai_vector3_distance(&agent->position, &agent->targetPosition);
    
    if (distance > 2.0f) {
        // Move closer
        return ai_behavior_chase(agent, deltaTime);
    } else {
        // Attack (for now, just stop)
        agent->velocity = ai_vector3_make(0.0f, 0.0f, 0.0f);
        LOG_DEBUG("Agent %lu is attacking!", agent->id);
        return true;
    }
}

bool ai_behavior_search(AIAgent* agent, float deltaTime) {
    if (!agent) return false;
    
    // Search behavior - move to last known position and look around
    return ai_behavior_patrol(agent, deltaTime);
}

// Decision making
AIStateType ai_agent_decide_state(AIAgent* agent) {
    if (!agent) return AI_STATE_IDLE;
    
    // Simple decision making based on perceptions and memories
    bool hasThreat = false;
    bool hasTarget = false;
    
    // Check perceptions for threats or targets
    for (uint32_t i = 0; i < agent->perceptionCount; i++) {
        AIPerception* perception = &agent->perceptions[i];
        
        if (perception->type == AI_PERCEPTION_VISUAL) {
            float distance = ai_vector3_distance(&agent->position, &perception->position);
            
            if (agent->type == AI_AGENT_TYPE_HOSTILE && distance < agent->visualRange) {
                hasTarget = true;
                agent->targetPosition = perception->position;
            } else if (agent->type == AI_AGENT_TYPE_PASSIVE && distance < agent->visualRange * 0.5f) {
                hasThreat = true;
                agent->targetPosition = perception->position;
            }
        }
    }
    
    // Make decision based on agent type and situation
    switch (agent->type) {
        case AI_AGENT_TYPE_HOSTILE:
            if (hasTarget) return AI_STATE_CHASE;
            break;
        case AI_AGENT_TYPE_PASSIVE:
            if (hasThreat) return AI_STATE_FLEE;
            break;
        case AI_AGENT_TYPE_NEUTRAL:
            if (hasThreat) return AI_STATE_FLEE;
            if (hasTarget) return AI_STATE_SEARCH;
            break;
        default:
            break;
    }
    
    return AI_STATE_PATROL;
}

bool ai_agent_has_reached_target(AIAgent* agent, float tolerance) {
    if (!agent) return false;
    
    float distance = ai_vector3_distance(&agent->position, &agent->targetPosition);
    return distance <= tolerance;
}

// Behavior tree implementation
AIBehaviorNode* ai_behavior_create_selector(AIBehaviorNode** children, uint32_t childCount) {
    AIBehaviorNode* node = malloc(sizeof(AIBehaviorNode));
    if (!node) return NULL;
    
    node->type = AI_NODE_SELECTOR;
    node->children = children;
    node->childCount = childCount;
    node->condition = NULL;
    node->action = NULL;
    node->isActive = true;
    
    return node;
}

AIBehaviorNode* ai_behavior_create_sequence(AIBehaviorNode** children, uint32_t childCount) {
    AIBehaviorNode* node = malloc(sizeof(AIBehaviorNode));
    if (!node) return NULL;
    
    node->type = AI_NODE_SEQUENCE;
    node->children = children;
    node->childCount = childCount;
    node->condition = NULL;
    node->action = NULL;
    node->isActive = true;
    
    return node;
}

AIBehaviorNode* ai_behavior_create_condition(bool (*condition)(AIAgent* agent)) {
    AIBehaviorNode* node = malloc(sizeof(AIBehaviorNode));
    if (!node) return NULL;
    
    node->type = AI_NODE_CONDITION;
    node->children = NULL;
    node->childCount = 0;
    node->condition = condition;
    node->action = NULL;
    node->isActive = true;
    
    return node;
}

AIBehaviorNode* ai_behavior_create_action(bool (*action)(AIAgent* agent, float deltaTime)) {
    AIBehaviorNode* node = malloc(sizeof(AIBehaviorNode));
    if (!node) return NULL;
    
    node->type = AI_NODE_ACTION;
    node->children = NULL;
    node->childCount = 0;
    node->condition = NULL;
    node->action = action;
    node->isActive = true;
    
    return node;
}

void ai_behavior_destroy(AIBehaviorNode* node) {
    if (!node) return;
    
    // Destroy children recursively
    if (node->children) {
        for (uint32_t i = 0; i < node->childCount; i++) {
            ai_behavior_destroy(node->children[i]);
        }
        free(node->children);
    }
    
    free(node);
}

bool ai_behavior_execute(AIBehaviorNode* node, AIAgent* agent, float deltaTime) {
    if (!node || !agent || !node->isActive) return false;
    
    switch (node->type) {
        case AI_NODE_SELECTOR:
            // Execute first successful child
            for (uint32_t i = 0; i < node->childCount; i++) {
                if (ai_behavior_execute(node->children[i], agent, deltaTime)) {
                    return true;
                }
            }
            return false;
            
        case AI_NODE_SEQUENCE:
            // Execute all children, fail if any fails
            for (uint32_t i = 0; i < node->childCount; i++) {
                if (!ai_behavior_execute(node->children[i], agent, deltaTime)) {
                    return false;
                }
            }
            return true;
            
        case AI_NODE_CONDITION:
            return node->condition ? node->condition(agent) : false;
            
        case AI_NODE_ACTION:
            return node->action ? node->action(agent, deltaTime) : false;
            
        default:
            return false;
    }
}

// Navigation mesh (simplified implementation)
AINavigationMesh* ai_navmesh_create(const AIVector3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount) {
    if (!vertices || vertexCount == 0) return NULL;
    
    AINavigationMesh* navMesh = malloc(sizeof(AINavigationMesh));
    if (!navMesh) return NULL;
    
    navMesh->vertices = malloc(sizeof(AIVector3) * vertexCount);
    navMesh->indices = malloc(sizeof(uint32_t) * indexCount);
    
    if (!navMesh->vertices || !navMesh->indices) {
        if (navMesh->vertices) free(navMesh->vertices);
        if (navMesh->indices) free(navMesh->indices);
        free(navMesh);
        return NULL;
    }
    
    memcpy(navMesh->vertices, vertices, sizeof(AIVector3) * vertexCount);
    memcpy(navMesh->indices, indices, sizeof(uint32_t) * indexCount);
    
    navMesh->vertexCount = vertexCount;
    navMesh->indexCount = indexCount;
    
    LOG_DEBUG("Created navigation mesh with %u vertices, %u indices", vertexCount, indexCount);
    return navMesh;
}

void ai_navmesh_destroy(AINavigationMesh* navMesh) {
    if (!navMesh) return;
    
    if (navMesh->vertices) free(navMesh->vertices);
    if (navMesh->indices) free(navMesh->indices);
    if (navMesh->normals) free(navMesh->normals);
    
    free(navMesh);
    LOG_DEBUG("Destroyed navigation mesh");
}
