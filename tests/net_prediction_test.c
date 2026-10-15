/*
 * net_prediction_test.c
 * Client prediction: frame history, position integration, reconciliation,
 * and rollback replay. Compiles net_prediction_impl.c directly and stubs
 * the logger. Does not link Engine.
 */

#include "network/network_prediction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static Vec3 v3(float x, float y, float z) {
  Vec3 v;
  memset(&v, 0, sizeof(v));
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

static int near_eq(float a, float b) {
  float d = a - b;
  if (d < 0.0f)
    d = -d;
  return d < 0.001f;
}

static void expect_vec(Vec3 v, float x, float y, float z, const char *label) {
  if (!near_eq(v.x, x) || !near_eq(v.y, y) || !near_eq(v.z, z)) {
    fprintf(stderr, "FAIL: %s got (%.4f, %.4f, %.4f) expected (%.4f, %.4f, %.4f)\n",
            label, v.x, v.y, v.z, x, y, z);
    exit(1);
  }
}

static void test_null_and_create(void) {
  ClientPrediction *pred;
  Vec3 origin;
  Vec3 predicted;
  PredictionFrame ignored;

  printf("Testing create, destroy, and null guards...\n");
  pred = prediction_create();
  if (pred == NULL)
    fail("prediction_create");
  prediction_destroy(pred);

  prediction_destroy(NULL);
  origin = v3(1.0f, 2.0f, 3.0f);
  predicted = prediction_predict_position(NULL, 1, origin, v3(0, 0, 0),
                                          v3(0, 0, 0), 0.016f);
  expect_vec(predicted, 1.0f, 2.0f, 3.0f, "null predict returns current");
  prediction_reconcile(NULL, 1, 1, origin, v3(0, 0, 0));
  prediction_add_frame(NULL, 1, origin, v3(0, 0, 0));
  prediction_clear_history(NULL);
  prediction_clear_old_frames(NULL, 1);
  prediction_enable_components(NULL, STATE_COMPONENT_POSITION);
  if (prediction_was_mispredicted(NULL))
    fail("null mispredicted");
  if (prediction_get_last_acknowledged_frame(NULL) != 0)
    fail("null ack frame");
  if (prediction_get_frame(NULL, 1, &ignored))
    fail("null get_frame");
  if (prediction_get_frames_to_resimulate(NULL, &ignored, 1) != 0)
    fail("null resimulate");
  expect_vec(prediction_get_current_position(NULL), 0, 0, 0, "null position");
  expect_vec(prediction_get_current_velocity(NULL), 0, 0, 0, "null velocity");
}

static void test_frame_history(void) {
  ClientPrediction *pred;
  PredictionFrame frame;
  uint32_t input;
  PredictionFrame replay[8];
  uint32_t replay_count;
  int i;

  printf("Testing frame history...\n");
  pred = prediction_create();
  if (!pred)
    fail("create");

  expect_vec(prediction_get_current_position(pred), 0, 0, 0, "empty position");
  if (prediction_get_frame(pred, 1, &frame))
    fail("empty history should miss");

  prediction_add_frame(pred, 1, v3(1, 2, 3), v3(4, 0, 0));
  input = 0xA1B2C3D4u;
  prediction_add_frame_with_input(pred, 2, v3(5, 6, 7), v3(0, 1, 0), &input,
                                  (uint32_t)sizeof(input));
  prediction_add_frame(pred, 3, v3(8, 9, 10), v3(0, 0, 2));

  expect_vec(prediction_get_current_position(pred), 8, 9, 10, "current pos");
  expect_vec(prediction_get_current_velocity(pred), 0, 0, 2, "current vel");

  if (!prediction_get_frame(pred, 1, &frame))
    fail("missing frame 1");
  expect_vec(frame.position, 1, 2, 3, "frame 1 pos");
  expect_vec(frame.velocity, 4, 0, 0, "frame 1 vel");
  if (frame.is_confirmed)
    fail("frame 1 should be unconfirmed");

  if (!prediction_get_frame(pred, 2, &frame))
    fail("missing frame 2");
  if (!frame.input_state || frame.input_size != sizeof(input) ||
      memcmp(frame.input_state, &input, sizeof(input)) != 0)
    fail("frame 2 input");

  if (prediction_get_frame(pred, 99, &frame))
    fail("unexpected frame 99");

  replay_count = prediction_get_frames_to_resimulate(pred, replay, 8);
  if (replay_count != 3)
    fail("resimulate count");
  if (replay[0].frame_number != 1 || replay[2].frame_number != 3)
    fail("resimulate order");

  prediction_clear_old_frames(pred, 3);
  if (prediction_get_frame(pred, 1, &frame) || prediction_get_frame(pred, 2, &frame))
    fail("old frames still present");
  if (!prediction_get_frame(pred, 3, &frame))
    fail("frame 3 dropped");
  expect_vec(prediction_get_current_position(pred), 8, 9, 10, "pos after clear");

  prediction_clear_history(pred);
  if (prediction_get_frame(pred, 3, &frame))
    fail("history not cleared");
  expect_vec(prediction_get_current_position(pred), 0, 0, 0, "pos after reset");
  if (prediction_was_mispredicted(pred))
    fail("mispredict flag after clear");

  for (i = 1; i <= MAX_PREDICTION_HISTORY + 1; i++) {
    prediction_add_frame(pred, (uint32_t)i, v3((float)i, 0, 0), v3(0, 0, 0));
  }
  if (prediction_get_frame(pred, 1, &frame))
    fail("ring buffer kept oldest frame");
  if (!prediction_get_frame(pred, 2, &frame))
    fail("ring buffer dropped frame 2");
  if (!prediction_get_frame(pred, (uint32_t)(MAX_PREDICTION_HISTORY + 1), &frame))
    fail("ring buffer missing newest");
  expect_vec(prediction_get_current_position(pred),
             (float)(MAX_PREDICTION_HISTORY + 1), 0, 0, "newest position");

  prediction_destroy(pred);
}

static void test_predict_position(void) {
  ClientPrediction *pred;
  Vec3 pos;
  float pos_error = -1.0f;

  printf("Testing position integration...\n");
  pred = prediction_create();
  if (!pred)
    fail("create");

  pos = prediction_predict_position(pred, 7, v3(0, 0, 0), v3(10, 0, 0),
                                    v3(0, 0, 0), 0.5f);
  expect_vec(pos, 5, 0, 0, "constant velocity");
  expect_vec(prediction_get_smoothed_position(pred, 7), 5, 0, 0,
             "stored predicted position");

  pos = prediction_predict_position(pred, 7, pos, v3(10, 0, 0), v3(0, -20, 0),
                                    0.5f);
  expect_vec(pos, 10, -5, 0, "accelerated step");

  prediction_get_entity_errors(pred, 7, &pos_error, NULL, NULL, NULL);
  if (!near_eq(pos_error, 0.0f))
    fail("error before reconcile");

  prediction_destroy(pred);
}

static void test_predict_full_state(void) {
  ClientPrediction *pred;
  PredictionFrame current;
  PredictionFrame next;

  printf("Testing full-state position and animation...\n");
  pred = prediction_create();
  if (!pred)
    fail("create");

  memset(&current, 0, sizeof(current));
  memset(&next, 0, sizeof(next));
  current.frame_number = 4;
  current.timestamp = 1.0f;
  current.position = v3(0, 10, 0);
  current.velocity = v3(0, 0, 0);
  current.physics_state.force = v3(0, -20, 0);
  current.physics_state.mass = 2.0f;
  current.animation_state.playback_time = 0.25f;
  current.health = 80.0f;

  prediction_predict_full_state(pred, 3, &current, 0.5f, &next);
  if (next.frame_number != 5)
    fail("frame increment");
  if (!near_eq(next.timestamp, 1.5f))
    fail("timestamp");
  expect_vec(next.position, 0, 7.5f, 0, "force over mass");
  expect_vec(next.velocity, 0, -5, 0, "integrated velocity");
  if (!near_eq(next.animation_state.playback_time, 0.75f))
    fail("animation time");
  if (!near_eq(next.health, 80.0f))
    fail("health should stay put");

  prediction_enable_components(pred, 0);
  memset(&next, 0, sizeof(next));
  prediction_predict_full_state(pred, 3, &current, 0.5f, &next);
  expect_vec(next.position, 0, 10, 0, "position disabled");
  if (!near_eq(next.animation_state.playback_time, 0.25f))
    fail("animation disabled");

  prediction_predict_full_state(NULL, 3, &current, 0.5f, &next);
  prediction_predict_full_state(pred, 3, NULL, 0.5f, &next);
  prediction_predict_full_state(pred, 3, &current, 0.5f, NULL);

  prediction_destroy(pred);
}

static void test_reconcile_and_rollback(void) {
  ClientPrediction *pred;
  PredictionFrame frame;
  float accuracy = 0.0f;
  float rtt = 0.0f;
  float loss = 0.0f;
  uint32_t mispredictions = 99;
  float pos_error = -1.0f;
  float entity_accuracy;

  printf("Testing reconcile threshold and rollback replay...\n");
  pred = prediction_create();
  if (!pred)
    fail("create");

  prediction_enable_components(pred, STATE_COMPONENT_POSITION);
  prediction_set_network_quality(pred, 0.25f, 0.2f);
  prediction_get_stats(pred, &accuracy, &rtt, &loss, &mispredictions);
  if (!near_eq(accuracy, 1.0f) || !near_eq(rtt, 0.25f) || !near_eq(loss, 0.2f) ||
      mispredictions != 0)
    fail("initial stats");

  prediction_predict_position(pred, 1, v3(0, 0, 0), v3(0, 0, 0), v3(0, 0, 0),
                              0.016f);
  prediction_add_frame(pred, 4, v3(0, 0, 0), v3(0, 0, 0));
  prediction_reconcile(pred, 1, 4, v3(0.2f, 0, 0), v3(0, 0, 0));
  if (prediction_was_mispredicted(pred))
    fail("small error should smooth, not rollback");
  prediction_get_entity_errors(pred, 1, &pos_error, NULL, NULL, NULL);
  if (!near_eq(pos_error, 0.2f))
    fail("small position error");
  entity_accuracy = prediction_get_entity_accuracy(pred, 1);
  if (!near_eq(entity_accuracy, 0.98f))
    fail("entity accuracy");

  prediction_update_smoothing(pred, 0.016f);
  expect_vec(prediction_get_smoothed_position(pred, 1), 0, 0, 0,
             "small error keeps predicted position");

  prediction_destroy(pred);
  pred = prediction_create();
  if (!pred)
    fail("create 2");
  prediction_enable_components(pred, STATE_COMPONENT_POSITION);

  prediction_add_frame(pred, 5, v3(0, 0, 0), v3(0, 0, 0));
  prediction_add_frame(pred, 6, v3(0, 0, 0), v3(100, 0, 0));
  prediction_reconcile(pred, 9, 5, v3(10, 0, 0), v3(1, 0, 0));
  if (!prediction_was_mispredicted(pred))
    fail("large error should mispredict");
  if (!prediction_get_frame(pred, 6, &frame))
    fail("replayed frame missing");
  expect_vec(frame.position, 1.6f, 0, 0, "replayed frame position");
  expect_vec(frame.velocity, 100, 0, 0, "replay keeps frame velocity");
  if (!prediction_get_frame(pred, 5, &frame))
    fail("confirmed frame missing");
  expect_vec(frame.position, 0, 0, 0, "confirmed frame not rewritten");

  prediction_get_stats(pred, NULL, NULL, NULL, &mispredictions);
  if (mispredictions != 1)
    fail("misprediction stat");

  prediction_update_smoothing(pred, 0.2f);
  if (prediction_was_mispredicted(pred))
    fail("smoothing should finish");

  prediction_destroy(pred);
}

int main(void) {
  test_null_and_create();
  test_frame_history();
  test_predict_position();
  test_predict_full_state();
  test_reconcile_and_rollback();
  printf("All net prediction tests passed.\n");
  return 0;
}
