/*
 * motion_matching_test.c
 * Pose search by velocity, trajectory, and feet.
 * Compiles the motion database directly and does not link Engine.
 */

#include "animation/motion_matching/motion_database.h"
#include "core/logging/unified_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
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

static void fill_pose(MotionDbPose *pose, u32 id, float velocity_x, float traj_x, float foot_x) {
    int i;
    memset(pose, 0, sizeof(*pose));
    pose->animation_id = id;
    pose->time = (float)id;
    pose->frame_index = id;
    pose->features.velocity = vec3_create(velocity_x, 0.0f, 0.0f);
    pose->features.angular_velocity = vec3_zero();
    for (i = 0; i < MOTION_MATCHING_TRAJECTORY_POINTS; i++) {
        pose->features.trajectory_pos[i] = vec3_create(traj_x, (float)i, 0.0f);
        pose->features.trajectory_dir[i] = vec3_create(0.0f, 0.0f, 1.0f);
    }
    pose->features.left_foot_pos = vec3_create(foot_x, 0.0f, 0.0f);
    pose->features.right_foot_pos = vec3_create(-foot_x, 0.0f, 0.0f);
    pose->features.left_foot_vel = vec3_zero();
    pose->features.right_foot_vel = vec3_zero();
}

int main(void) {
    MotionDatabase *db;
    MotionDbPose slow;
    MotionDbPose fast;
    MotionFeatures query;
    MotionFeatures extracted;
    const MotionDbPose *got;
    float cost = -1.0f;
    int idx;

    memset(&query, 0, sizeof(query));
    if (motion_database_create(0) != NULL) {
        fail("zero capacity should fail");
    }
    if (motion_database_add_pose(NULL, NULL)) {
        fail("null database accepted a pose");
    }
    if (motion_database_get_pose_count(NULL) != 0) {
        fail("null count");
    }
    if (motion_database_search(NULL, &query, NULL) != -1) {
        fail("null search");
    }

    db = motion_database_create(2);
    if (!db) {
        fail("create");
    }
    if (motion_database_get_pose_count(db) != 0) {
        fail("new database is not empty");
    }
    if (motion_database_search(db, &query, &cost) != -1) {
        fail("empty search should miss");
    }
    if (motion_database_add_pose(db, NULL)) {
        fail("null pose accepted");
    }

    fill_pose(&slow, 10, 1.0f, 0.0f, 0.2f);
    fill_pose(&fast, 20, 6.0f, 4.0f, 1.5f);
    if (!motion_database_add_pose(db, &slow) || !motion_database_add_pose(db, &fast)) {
        fail("add pose");
    }
    if (motion_database_get_pose_count(db) != 2) {
        fail("pose count");
    }
    if (motion_database_add_pose(db, &slow)) {
        fail("database accepted a pose past capacity");
    }

    query = slow.features;
    query.velocity = vec3_create(1.2f, 0.0f, 0.0f);
    cost = -1.0f;
    idx = motion_database_search(db, &query, &cost);
    if (idx != 0 || !(cost >= 0.0f) || cost > 1.0f) {
        fprintf(stderr, "FAIL: slow match idx=%d cost=%f\n", idx, cost);
        exit(1);
    }
    got = motion_database_get_pose(db, (u32)idx);
    if (!got || got->animation_id != 10) {
        fail("slow pose identity");
    }

    query = fast.features;
    cost = -1.0f;
    idx = motion_database_search(db, &query, &cost);
    if (idx != 1 || cost > 0.0001f) {
        fprintf(stderr, "FAIL: exact fast match idx=%d cost=%f\n", idx, cost);
        exit(1);
    }
    if (motion_database_get_pose(db, 2) != NULL) {
        fail("out of range pose");
    }

    motion_database_extract_features(&extracted, vec3_create(6.0f, 0.0f, 0.0f),
                                     NULL, NULL,
                                     fast.features.left_foot_pos, fast.features.right_foot_pos,
                                     vec3_zero(), vec3_zero());
    if (extracted.trajectory_pos[0].x != 0.0f || extracted.angular_velocity.x != 0.0f) {
        fail("null trajectory was not cleared");
    }
    motion_database_extract_features(NULL, vec3_zero(), NULL, NULL,
                                     vec3_zero(), vec3_zero(), vec3_zero(), vec3_zero());

    motion_database_destroy(db);
    motion_database_destroy(NULL);
    printf("motion matching tests passed\n");
    return 0;
}
