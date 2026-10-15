# Issue #323: unit tests for the thread pool and job queue in
# src/engine/core/threading/job.c. Not add_engine_test: that macro links Engine,
# which does not build on origin/main (missing GLFW, then gpu_profiler.c:80).
# job.c is compiled directly; the test stubs its logger entry point.
add_executable(test_job
    tests/unit/core/threading/job_test.c
    src/engine/core/threading/job.c)
target_include_directories(test_job PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_job PRIVATE Threads::Threads)
add_test(NAME test_job COMMAND test_job)
# Bounds a hang in the compat-layer busy wait, which has no timeout of its own.
set_tests_properties(test_job PROPERTIES TIMEOUT 60)
