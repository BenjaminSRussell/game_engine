CC = gcc
CFLAGS = -I../include -Wall -Wextra -O2
LDFLAGS = -lm

.PHONY: all clean

all: test_gltf test_water test_mesh_wave test_bucket test_game test_recipe_cache test_processing_queue test_experience_level test_damage_calc test_falling_blocks test_game_state test_enchanting test_brewing test_inventory test_lighting test_vfx test_chunk_brewing_vfx test_phase1 test_phase2a test_phase2b

test_mesh_wave: test_mesh_water_wave.c test_stubs.c
	$(CC) $(CFLAGS) -o test_mesh_wave test_mesh_water_wave.c test_stubs.c \
		../src/mesh/mesh.c ../src/mesh/chunk_mesh_generator.c ../src/chunk/chunk.c ../src/block/block.c ../src/rendering/texture_atlas.c ../src/block/water_physics.c $(LDFLAGS)

test_gltf: test_gltf_load.c
	$(CC) $(CFLAGS) -o test_gltf test_gltf_load.c $(LDFLAGS)

test_water: test_water_physics.c test_stubs.c
	$(CC) $(CFLAGS) -o test_water test_water_physics.c test_stubs.c \
		../src/chunk/chunk.c ../src/block/water_physics.c ../src/block/block.c ../src/block/block_states.c $(LDFLAGS)

test_bucket: test_bucket.c test_stubs.c
	$(CC) $(CFLAGS) -o test_bucket test_bucket.c test_stubs.c \
		../src/chunk/chunk.c ../src/block/water_physics.c ../src/block/block.c ../src/block/water_bucket.c $(LDFLAGS)

test_game: test_game_logic.c
	$(CC) $(CFLAGS) -o test_game test_game_logic.c ../src/game/game.c $(LDFLAGS)

test_recipe_cache: test_recipe_cache.c
	$(CC) $(CFLAGS) -o test_recipe_cache test_recipe_cache.c test_stubs.c ../src/game/crafting/recipe_manager.c ../src/game/crafting/recipe.c ../src/game/inventory/inventory.c $(LDFLAGS)

test_processing_queue: test_processing_queue.c
	$(CC) $(CFLAGS) -o test_processing_queue test_processing_queue.c test_stubs.c ../src/game/crafting/resource_processing.c ../src/game/inventory/inventory.c $(LDFLAGS)

test_experience_level: test_experience_level.c
	$(CC) $(CFLAGS) -o test_experience_level test_experience_level.c test_stubs.c ../src/player/experience_test.c $(LDFLAGS)

test_damage_calc: test_damage_calc.c
	$(CC) $(CFLAGS) -o test_damage_calc test_damage_calc.c ../src/combat/damage_calc.c -lm

test_falling_blocks: test_falling_blocks.c
	$(CC) $(CFLAGS) -o test_falling_blocks test_falling_blocks.c $(LDFLAGS)

test_game_state: test_game_state.c
	$(CC) $(CFLAGS) -o test_game_state test_game_state.c $(LDFLAGS)

test_enchanting: test_enchanting.c
	$(CC) $(CFLAGS) -o test_enchanting test_enchanting.c $(LDFLAGS)

test_brewing: test_brewing.c
	$(CC) $(CFLAGS) -o test_brewing test_brewing.c $(LDFLAGS)

test_inventory: test_inventory.c
	$(CC) $(CFLAGS) -o test_inventory test_inventory.c $(LDFLAGS)

test_lighting: test_lighting.c
	$(CC) $(CFLAGS) -o test_lighting test_lighting.c $(LDFLAGS)

# VFX test (particles/sounds emitters)
test_vfx: test_vfx.c test_vfx_stub.c
	$(CC) $(CFLAGS) -o test_vfx test_vfx.c test_vfx_stub.c ../src/game/crafting/brewing.c ../src/game/crafting/enchanting.c $(LDFLAGS)

# Chunk-brewing VFX test (ensures chunk_manager_update calls baking updates)
test_chunk_brewing_vfx: test_chunk_brewing_vfx.c test_vfx_stub.c
	$(CC) $(CFLAGS) -o test_chunk_brewing_vfx test_chunk_brewing_vfx.c test_vfx_stub.c ../src/game/crafting/brewing.c ../src/game/chunk/chunk_stubs.c $(LDFLAGS)

test_phase1: test_phase1.c
	$(CC) $(CFLAGS) -o test_phase1 test_phase1.c ../src/render/animation_system.c -lm

test_phase2a: test_phase2a.c
	$(CC) $(CFLAGS) -o test_phase2a test_phase2a.c $(LDFLAGS)

test_phase2b: test_phase2b.c
	$(CC) $(CFLAGS) -o test_phase2b test_phase2b.c $(LDFLAGS)

clean:
	rm -f test_gltf test_water test_mesh_wave test_bucket test_game test_recipe_cache test_processing_queue test_experience_level test_damage_calc test_falling_blocks test_game_state test_enchanting test_brewing test_inventory test_vfx test_chunk_brewing_vfx test_phase1 test_phase2a test_phase2b

test: test_gltf
	@echo "Testing with sample GLTF..."
	./test_gltf ../assets/models/characters/spirit.gltf || echo "No model found yet - download from Mixamo!"
