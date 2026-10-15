/*
 * atmosphere_scattering_test.c
 * Rayleigh/Mie coefficients, phase functions, density, optical depth,
 * transmittance, and ray-sphere intersection.
 * Compiles atmosphere_scattering.c directly and does not link Engine.
 */

#include "environment/atmosphere/sky/atmosphere_scattering.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static void expect_near(float actual, float expected, const char *msg) {
    float diff = fabsf(actual - expected);
    float tol = 1.0e-4f * fabsf(expected) + 1.0e-6f;
    if (diff > tol) {
        fprintf(stderr, "FAIL: %s (actual %.9g expected %.9g)\n", msg, actual,
                expected);
        exit(1);
    }
}

static int finite3(simd_float3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

static AtmosphereDensityProfile exp_profile(float exp_term, float exp_scale) {
    AtmosphereDensityProfile profile;
    profile.exp_term = exp_term;
    profile.exp_scale = exp_scale;
    profile.linear_term = 0.0f;
    profile.constant_term = 0.0f;
    profile.width = 0.0f;
    return profile;
}

static ScatteringCoefficients coeffs_rgb(float r, float g, float b, float mie) {
    ScatteringCoefficients coeffs;
    coeffs.rayleigh = (simd_float3){r, g, b};
    coeffs.mie = mie;
    coeffs.ozone = (simd_float3){0.0f, 0.0f, 0.0f};
    return coeffs;
}

static void test_rayleigh_coefficient(void) {
    simd_float3 rgb;
    float at_blue;
    float at_double;

    printf("Testing Rayleigh coefficient...\n");
    expect_true(atm_rayleigh_coefficient(0.0f, 1.00029f, 2.5e25f) == 0.0f,
                "zero wavelength");
    expect_true(atm_rayleigh_coefficient(-0.44f, 1.00029f, 2.5e25f) == 0.0f,
                "negative wavelength");

    at_blue = atm_rayleigh_coefficient(ATM_WAVELENGTH_B, 1.00029f, 2.5e25f);
    at_double = atm_rayleigh_coefficient(0.880f, 1.0f, 1.0f);
    expect_near(at_blue, 3.31e-5f, "440nm base");
    /* lambda^-4: doubling wavelength divides the coefficient by 16. */
    expect_near(at_double, 3.31e-5f / 16.0f, "880nm scale");
    expect_true(atm_rayleigh_coefficient(ATM_WAVELENGTH_B, 1.0f, 1.0f) >
                    atm_rayleigh_coefficient(ATM_WAVELENGTH_R, 1.0f, 1.0f),
                "blue scatters more than red");

    rgb = atm_rayleigh_coefficient_rgb();
    expect_near(rgb.x, 5.802e-6f, "rayleigh r");
    expect_near(rgb.y, 1.3558e-5f, "rayleigh g");
    expect_near(rgb.z, 3.3100e-5f, "rayleigh b");
    expect_true(rgb.z > rgb.y && rgb.y > rgb.x, "rgb wavelength order");
}

static void test_phase_functions(void) {
    const float pi = (float)ATM_PI;
    float rayleigh_side;
    float hg_iso;
    float forward;
    float back;

    printf("Testing phase functions...\n");
    rayleigh_side = 3.0f / (16.0f * pi);
    expect_near(atm_rayleigh_phase(0.0f), rayleigh_side, "rayleigh side");
    expect_near(atm_rayleigh_phase(1.0f), rayleigh_side * 2.0f, "rayleigh forward");
    expect_near(atm_rayleigh_phase(-1.0f), atm_rayleigh_phase(1.0f),
                "rayleigh symmetry");
    expect_near(atm_rayleigh_phase(0.5f), rayleigh_side * 1.25f, "rayleigh 0.5");

    expect_near(atm_mie_coefficient(2.0f, 0.01f), 0.004f, "mie turbidity");
    expect_near(atm_mie_coefficient(0.0f, 0.01f), 0.0f, "mie clear");

    hg_iso = 1.0f / (4.0f * pi);
    expect_near(atm_mie_phase_hg(0.0f, 0.0f), hg_iso, "hg isotropic side");
    expect_near(atm_mie_phase_hg(1.0f, 0.0f), hg_iso, "hg isotropic forward");
    expect_near(atm_mie_phase_hg(-1.0f, 0.0f), hg_iso, "hg isotropic back");

    forward = atm_mie_phase_hg(1.0f, 0.8f);
    back = atm_mie_phase_hg(-1.0f, 0.8f);
    expect_true(forward > back * 10.0f, "hg forward lobe");
    expect_true(isfinite(atm_mie_phase_hg(1.0f, 1.0f)), "hg den guard");
    expect_near(atm_mie_phase_hg(1.0f, 1.0f), 0.0f, "hg g=1 numerator");

    expect_near(atm_mie_phase_cornette_shanks(0.0f, 0.0f), rayleigh_side,
                "cornette matches rayleigh at g=0");
    expect_true(atm_mie_phase_cornette_shanks(1.0f, 0.76f) >
                    atm_mie_phase_cornette_shanks(-1.0f, 0.76f),
                "cornette forward lobe");
}

static void test_ozone_and_density(void) {
    simd_float3 ozone;
    AtmosphereDensityProfile linear;
    AtmosphereLayerParams earth;
    float sea;
    float scale_height;

    printf("Testing ozone and density...\n");
    ozone = atm_ozone_absorption_rgb();
    expect_near(ozone.x, 0.650e-6f, "ozone r");
    expect_near(ozone.y, 1.881e-6f, "ozone g");
    expect_near(ozone.z, 0.085e-6f, "ozone b");
    expect_true(ozone.y > ozone.x && ozone.x > ozone.z, "chappuis bands");

    sea = atm_density_profile_eval(0.0f, exp_profile(1.0f, 8.0f));
    scale_height = atm_density_profile_eval(8.0f, exp_profile(1.0f, 8.0f));
    expect_near(sea, 1.0f, "sea level density");
    expect_near(scale_height, expf(-1.0f), "one scale height");

    linear.exp_term = 0.0f;
    linear.exp_scale = 1.0f;
    linear.linear_term = 2.0f;
    linear.constant_term = 3.0f;
    linear.width = 0.0f;
    expect_near(atm_density_profile_eval(4.0f, linear), 11.0f, "linear density");

    linear.linear_term = -1.0f;
    linear.constant_term = -5.0f;
    expect_near(atm_density_profile_eval(10.0f, linear), 0.0f, "density clamp");

    atm_get_earth_density_profiles(NULL);
    atm_get_earth_density_profiles(&earth);
    expect_near(earth.rayleigh.exp_term, 1.0f, "earth rayleigh term");
    expect_near(earth.rayleigh.exp_scale, 8.0f, "earth rayleigh scale");
    expect_near(earth.mie.exp_scale, 1.2f, "earth mie scale");
    expect_near(earth.mie.exp_term, 1.0f, "earth mie term");
    expect_true(earth.rayleigh.linear_term == 0.0f && earth.mie.linear_term == 0.0f,
                "earth linear terms");
    expect_near(atm_density_profile_eval(0.0f, earth.rayleigh), 1.0f,
                "earth surface rayleigh");
    expect_near(atm_density_profile_eval(1.2f, earth.mie), expf(-1.0f),
                "earth mie scale height");
}

static void test_transmittance_and_depth(void) {
    ScatteringCoefficients coeffs;
    AtmosphereLayerParams profiles;
    simd_float3 origin;
    simd_float3 up;
    simd_float3 depth;
    simd_float3 trans;
    simd_float3 zero_depth;
    float rho;
    const float planet_radius = 100.0f;
    const float distance = 8.0f;

    printf("Testing transmittance and optical depth...\n");
    zero_depth = (simd_float3){0.0f, 0.0f, 0.0f};
    trans = atm_transmittance_from_optical_depth(zero_depth);
    expect_near(trans.x, 1.0f, "clear x");
    expect_near(trans.y, 1.0f, "clear y");
    expect_near(trans.z, 1.0f, "clear z");

    depth = (simd_float3){1.0f, 2.0f, 4.0f};
    trans = atm_transmittance_from_optical_depth(depth);
    expect_near(trans.x, expf(-1.0f), "trans x");
    expect_near(trans.y, expf(-2.0f), "trans y");
    expect_near(trans.z, expf(-4.0f), "trans z");
    expect_true(trans.x > trans.y && trans.y > trans.z, "transmittance order");

    coeffs = coeffs_rgb(2.0f, 0.0f, 0.0f, 0.0f);
    profiles.rayleigh = exp_profile(1.0f, 8.0f);
    profiles.mie = exp_profile(0.0f, 1.2f);
    profiles.ozone = exp_profile(0.0f, 1.0f);
    origin = (simd_float3){0.0f, planet_radius, 0.0f};
    up = (simd_float3){0.0f, 1.0f, 0.0f};
    rho = atm_density_profile_eval(distance * 0.5f, profiles.rayleigh);
    depth = atm_optical_depth(origin, up, distance, planet_radius, 100.0f, coeffs,
                              profiles, 1);
    expect_near(depth.x, rho * coeffs.rayleigh.x * distance, "vertical depth");
    expect_near(depth.y, 0.0f, "depth y");
    expect_near(depth.z, 0.0f, "depth z");
    expect_true(depth.x > 0.0f, "positive depth");
}

static void test_ray_sphere(void) {
    float t0;
    float t1;
    simd_float3 origin;
    simd_float3 dir;
    simd_float3 center;

    printf("Testing ray-sphere intersection...\n");
    center = (simd_float3){0.0f, 0.0f, 0.0f};
    origin = (simd_float3){0.0f, 0.0f, 10.0f};
    dir = (simd_float3){0.0f, 0.0f, -1.0f};
    t0 = -1000.0f;
    t1 = -1000.0f;
    expect_true(atm_ray_sphere_intersect(origin, dir, center, 1.0f, &t0, &t1),
                "hit exists");
    expect_near(t0, 9.0f, "near hit");
    expect_near(t1, 11.0f, "far hit");

    expect_true(atm_ray_sphere_intersect(origin, dir, center, 1.0f, NULL, NULL),
                "null outputs");

    origin = (simd_float3){0.0f, 5.0f, 0.0f};
    dir = (simd_float3){1.0f, 0.0f, 0.0f};
    expect_true(!atm_ray_sphere_intersect(origin, dir, center, 1.0f, &t0, &t1),
                "miss");

    origin = (simd_float3){0.0f, 0.0f, 0.0f};
    dir = (simd_float3){1.0f, 0.0f, 0.0f};
    expect_true(atm_ray_sphere_intersect(origin, dir, center, 2.0f, &t0, &t1),
                "from center");
    expect_near(t0, -2.0f, "center near");
    expect_near(t1, 2.0f, "center far");
}

static void test_earth_and_validate(void) {
    ScatteringCoefficients earth;
    ScatteringCoefficients bad;
    simd_float3 rgb;
    simd_float3 ozone;

    printf("Testing Earth coefficients and validation...\n");
    earth = atm_get_earth_coefficients();
    rgb = atm_rayleigh_coefficient_rgb();
    ozone = atm_ozone_absorption_rgb();
    expect_near(earth.rayleigh.x, rgb.x, "earth rayleigh x");
    expect_near(earth.rayleigh.y, rgb.y, "earth rayleigh y");
    expect_near(earth.rayleigh.z, rgb.z, "earth rayleigh z");
    expect_near(earth.mie, 2.1e-5f, "earth mie");
    expect_near(earth.ozone.x, ozone.x, "earth ozone x");
    expect_near(earth.ozone.y, ozone.y, "earth ozone y");
    expect_near(earth.ozone.z, ozone.z, "earth ozone z");

    bad = coeffs_rgb(0.0f, 0.0f, 0.0f, 0.0f);
    expect_true(!atm_validate_parameters(bad, 0.0f, 100.0f), "zero radius");
    expect_true(!atm_validate_parameters(bad, 6360.0f, 0.0f), "zero height");
    expect_true(!atm_validate_parameters(bad, -1.0f, 100.0f), "negative radius");
    expect_true(atm_validate_parameters(earth, ATM_EARTH_RADIUS, ATM_ATMOSPHERE_HEIGHT),
                "earth parameters");
}

static void test_combined_scattering(void) {
    ScatteringCoefficients coeffs;
    ScatteringCoefficients zero;
    AtmosphereLayerParams profiles;
    simd_float3 origin;
    simd_float3 up;
    simd_float3 down;
    simd_float3 toward;
    simd_float3 away;
    simd_float3 dark;
    const float planet_radius = 100.0f;
    const float atmo_height = 50.0f;

    printf("Testing combined scattering...\n");
    profiles.rayleigh = exp_profile(1.0f, 8.0f);
    profiles.mie = exp_profile(1.0f, 2.0f);
    profiles.ozone = exp_profile(0.0f, 1.0f);
    origin = (simd_float3){0.0f, planet_radius, 0.0f};
    up = (simd_float3){0.0f, 1.0f, 0.0f};
    down = (simd_float3){0.0f, -1.0f, 0.0f};

    zero = coeffs_rgb(0.0f, 0.0f, 0.0f, 0.0f);
    dark = atm_combined_scattering(origin, up, up, 20.0f, planet_radius, atmo_height,
                                   zero, profiles, 4);
    expect_true(finite3(dark), "zero coeffs finite");
    expect_near(dark.x, 0.0f, "zero x");
    expect_near(dark.y, 0.0f, "zero y");
    expect_near(dark.z, 0.0f, "zero z");

    coeffs = coeffs_rgb(0.02f, 0.03f, 0.05f, 0.2f);
    toward = atm_combined_scattering(origin, up, up, 20.0f, planet_radius, atmo_height,
                                     coeffs, profiles, 4);
    away = atm_combined_scattering(origin, up, down, 20.0f, planet_radius, atmo_height,
                                   coeffs, profiles, 4);
    expect_true(finite3(toward) && finite3(away), "scattering finite");
    expect_true(toward.x > 0.0f && toward.y > 0.0f && toward.z > 0.0f,
                "sunward positive");
    expect_true(toward.x > away.x && toward.y > away.y && toward.z > away.z,
                "forward lobe brighter than backscatter");
}

int main(void) {
    test_rayleigh_coefficient();
    test_phase_functions();
    test_ozone_and_density();
    test_transmittance_and_depth();
    test_ray_sphere();
    test_earth_and_validate();
    test_combined_scattering();
    printf("All atmosphere scattering tests passed\n");
    return 0;
}
