/**
 * COMPREHENSIVE PHYSICS TESTS - PART 2: Fluid Dynamics
 * Tests for: Viscosity, Navier-Stokes, FLIP, SPH
 */

#include "physics_test.h"
#include <physics/fluids/viscosity_solver.h>
#include <physics/fluids/navier_stokes.h>
#include <physics/fluids/flip_solver.h>

// ============================================================================
// VISCOSITY TESTS
// ============================================================================

void test_viscosity_xsph_kernel() {
    // Test XSPH kernel values
    float h = 1.0f;
    
    // At r=0: maximum value
    float w0 = xsph_kernel(0.0f, h);
    ASSERT_TRUE(w0 > 0.0f);
    
    // At r=h: zero value
    float wh = xsph_kernel(h, h);
    ASSERT_FLOAT_EQ(wh, 0.0f, 0.001f);
    
    // At r > h: zero value
    float w_far = xsph_kernel(2.0f, h);
    ASSERT_FLOAT_EQ(w_far, 0.0f, 0.001f);
    
    // At r < h: decreasing function
    float w_half = xsph_kernel(0.5f * h, h);
    ASSERT_TRUE(w_half > 0.0f);
    ASSERT_TRUE(w_half < w0);
}

void test_viscosity_temperature_dependent() {
    // Water at different temperatures
    float visc_cold = get_viscosity_for_temperature(5.0f);
    float visc_warm = get_viscosity_for_temperature(20.0f);
    float visc_hot = get_viscosity_for_temperature(80.0f);
    
    // Viscosity decreases with temperature
    ASSERT_TRUE(visc_cold > visc_warm);
    ASSERT_TRUE(visc_warm > visc_hot);
    
    // Ice should have very high viscosity
    float visc_ice = get_viscosity_for_temperature(-10.0f);
    ASSERT_TRUE(visc_ice > 100.0f);
}

void test_viscosity_non_newtonian() {
    float base_visc = 1.0f;
    
    // Shear-thinning (n < 1, e.g., ketchup)
    float visc_thin = apply_non_newtonian(10.0f, base_visc, 0.5f);
    ASSERT_TRUE(visc_thin < base_visc);
    
    // Shear-thickening (n > 1, e.g., cornstarch)
    float visc_thick = apply_non_newtonian(10.0f, base_visc, 1.5f);
    ASSERT_TRUE(visc_thick > base_visc);
    
    // Newtonian (n = 1)
    float visc_newton = apply_non_newtonian(10.0f, base_visc, 1.0f);
    ASSERT_FLOAT_EQ(visc_newton, base_visc, 0.1f);
}

// ============================================================================
// NAVIER-STOKES TESTS
// ============================================================================

void test_navier_stokes_grid_creation() {
    SmokeGrid *grid = smoke_grid_create(32, 1.0f);
    ASSERT_NOT_NULL(grid);
    ASSERT_INT_EQ(grid->size, 32);
    ASSERT_FLOAT_EQ(grid->cell_size, 1.0f, 0.01f);
    
    ASSERT_NOT_NULL(grid->velocity_x);
    ASSERT_NOT_NULL(grid->velocity_y);
    ASSERT_NOT_NULL(grid->velocity_z);
    ASSERT_NOT_NULL(grid->density);
    ASSERT_NOT_NULL(grid->temperature);
    ASSERT_NOT_NULL(grid->pressure);
    
    // Free
    free(grid->velocity_x);
    free(grid->velocity_y);
    free(grid->velocity_z);
    free(grid->velocity_x_prev);
    free(grid->velocity_y_prev);
    free(grid->velocity_z_prev);
    free(grid->density);
    free(grid->density_prev);
    free(grid->temperature);
    free(grid->temperature_prev);
    free(grid->pressure);
    free(grid->divergence);
    free(grid);
}

void test_navier_stokes_divergence_free() {
    SmokeGrid *grid = smoke_grid_create(16, 1.0f);
    
    // Set non-zero velocity
    grid->velocity_x[8*16*16 + 8*16 + 8] = 1.0f;
    
    // Solve pressure (should make divergence-free)
    solve_pressure(grid, 20);
    
    // Check divergence is near zero (within tolerance)
    float div_sum = 0.0f;
    for (uint32_t i = 0; i < 16*16*16; i++) {
        div_sum += fabsf(grid->divergence[i]);
    }
    
    // Average divergence should be small
    float avg_div = div_sum / (16.0f * 16.0f * 16.0f);
    ASSERT_TRUE(avg_div < 0.1f);
    
    // Free
    free(grid->velocity_x);
    free(grid->velocity_y);
    free(grid->velocity_z);
    free(grid->velocity_x_prev);
    free(grid->velocity_y_prev);
    free(grid->velocity_z_prev);
    free(grid->density);
    free(grid->density_prev);
    free(grid->temperature);
    free(grid->temperature_prev);
    free(grid->pressure);
    free(grid->divergence);
    free(grid);
}

void test_navier_stokes_buoyancy() {
    SmokeGrid *grid = smoke_grid_create(16, 1.0f);
    
    // Set temperature and density
    grid->temperature[8*16*16 + 8*16 + 8] = 100.0f; // Hot
    grid->density[8*16*16 + 8*16 + 8] = 0.5f;
    
    // Apply buoyancy
    apply_buoyancy(grid, 0.016f);
    
    // Upward velocity should increase
    ASSERT_TRUE(grid->velocity_y[8*16*16 + 8*16 + 8] > 0.0f);
    
    // Free
    free(grid->velocity_x);
    free(grid->velocity_y);
    free(grid->velocity_z);
    free(grid->velocity_x_prev);
    free(grid->velocity_y_prev);
    free(grid->velocity_z_prev);
    free(grid->density);
    free(grid->density_prev);
    free(grid->temperature);
    free(grid->temperature_prev);
    free(grid->pressure);
    free(grid->divergence);
    free(grid);
}

// ============================================================================
// FLIP SOLVER TESTS
// ============================================================================

void test_flip_solver_creation() {
    FlipSolver *solver = flip_solver_create(32, 1.0f, 1000);
    ASSERT_NOT_NULL(solver);
    ASSERT_INT_EQ(solver->grid_size, 32);
    ASSERT_INT_EQ(solver->max_particles, 1000);
    ASSERT_FLOAT_EQ(solver->cell_size, 1.0f, 0.01f);
    
    // PIC/FLIP ratio should be small (mostly FLIP)
    ASSERT_TRUE(solver->pic_flip_ratio < 0.1f);
    
    // Free
    free(solver->particles);
    free(solver->u);
    free(solver->v);
    free(solver->w);
    free(solver->u_temp);
    free(solver->v_temp);
    free(solver->w_temp);
    free(solver->pressure);
    free(solver->divergence);
    free(solver);
}

void test_flip_particle_transfer() {
    FlipSolver *solver = flip_solver_create(16, 1.0f, 100);
    
    // Add some particles
    solver->particle_count = 10;
    for (uint32_t i = 0; i < 10; i++) {
        solver->particles[i].position[0] = 5.0f + i * 0.5f;
        solver->particles[i].position[1] = 5.0f;
        solver->particles[i].position[2] = 5.0f;
        solver->particles[i].velocity[0] = 1.0f;
        solver->particles[i].velocity[1] = 0.0f;
        solver->particles[i].velocity[2] = 0.0f;
    }
    
    // Transfer P2G
    flip_p2g_transfer(solver);
    
    // Grid should have velocity
    bool has_velocity = false;
    for (uint32_t i = 0; i < 16*16*16; i++) {
        if (fabsf(solver->u[i]) > 0.001f) {
            has_velocity = true;
            break;
        }
    }
    ASSERT_TRUE(has_velocity);
    
    // Free
    free(solver->particles);
    free(solver->u);
    free(solver->v);
    free(solver->w);
    free(solver->u_temp);
    free(solver->v_temp);
    free(solver->w_temp);
    free(solver->pressure);
    free(solver->divergence);
    free(solver);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    TEST_SUITE_BEGIN("Physics Fluid Dynamics");
    
    printf("\n--- Viscosity Tests ---\n");
    RUN_TEST(test_viscosity_xsph_kernel);
    RUN_TEST(test_viscosity_temperature_dependent);
    RUN_TEST(test_viscosity_non_newtonian);
    
    printf("\n--- Navier-Stokes Tests ---\n");
    RUN_TEST(test_navier_stokes_grid_creation);
    RUN_TEST(test_navier_stokes_divergence_free);
    RUN_TEST(test_navier_stokes_buoyancy);
    
    printf("\n--- FLIP Solver Tests ---\n");
    RUN_TEST(test_flip_solver_creation);
    RUN_TEST(test_flip_particle_transfer);
    
    TEST_SUITE_END();
}
