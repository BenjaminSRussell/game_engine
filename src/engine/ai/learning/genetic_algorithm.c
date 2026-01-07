#include "ai/ai_types.h"
#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_POPULATION 100
#define GENE_COUNT 8

typedef struct {
    float genes[GENE_COUNT];
    float fitness;
} Chromosome;

typedef struct {
    Chromosome population[MAX_POPULATION];
    int size;
    float mutation_rate;
    float crossover_rate;
} GeneticAlgorithm;

static GeneticAlgorithm g_ga = {.size = 50, .mutation_rate = 0.1f, .crossover_rate = 0.7f};

void genetic_algorithm_init(int population_size) {
    g_ga.size = population_size;
    for (int i = 0; i < g_ga.size; i++) {
        for (int j = 0; j < GENE_COUNT; j++) {
            g_ga.population[i].genes[j] = (float)rand() / RAND_MAX;
        }
        g_ga.population[i].fitness = 0.0f;
    }
}

int tournament_selection() {
    int a = rand() % g_ga.size;
    int b = rand() % g_ga.size;
    return (g_ga.population[a].fitness > g_ga.population[b].fitness) ? a : b;
}

void crossover(Chromosome *parent1, Chromosome *parent2, Chromosome *child) {
    int point = rand() % GENE_COUNT;
    for (int i = 0; i < GENE_COUNT; i++) {
        child->genes[i] = (i < point) ? parent1->genes[i] : parent2->genes[i];
    }
}

void mutate(Chromosome *chromosome) {
    for (int i = 0; i < GENE_COUNT; i++) {
        if ((float)rand() / RAND_MAX < g_ga.mutation_rate) {
            chromosome->genes[i] += ((float)rand() / RAND_MAX - 0.5f) * 0.2f;
            chromosome->genes[i] = fmaxf(0.0f, fminf(1.0f, chromosome->genes[i]));
        }
    }
}

void genetic_algorithm_evolve(float (*fitness_func)(float*)) {
    // Evaluate fitness
    for (int i = 0; i < g_ga.size; i++) {
        g_ga.population[i].fitness = fitness_func(g_ga.population[i].genes);
    }
    
    // Create next generation
    Chromosome next_gen[MAX_POPULATION];
    for (int i = 0; i < g_ga.size; i++) {
        int parent1_idx = tournament_selection();
        int parent2_idx = tournament_selection();
        
        if ((float)rand() / RAND_MAX < g_ga.crossover_rate) {
            crossover(&g_ga.population[parent1_idx], &g_ga.population[parent2_idx], &next_gen[i]);
        } else {
            next_gen[i] = g_ga.population[parent1_idx];
        }
        
        mutate(&next_gen[i]);
    }
    
    memcpy(g_ga.population, next_gen, sizeof(Chromosome) * g_ga.size);
}

float* genetic_algorithm_get_best() {
    int best_idx = 0;
    for (int i = 1; i < g_ga.size; i++) {
        if (g_ga.population[i].fitness > g_ga.population[best_idx].fitness) {
            best_idx = i;
        }
    }
    return g_ga.population[best_idx].genes;
}
