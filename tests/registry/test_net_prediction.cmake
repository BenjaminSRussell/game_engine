# Client prediction, reconciliation, and rollback replay.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles
# net_prediction_impl.c directly and stubs the logger.
add_executable(test_net_prediction
    tests/net_prediction_test.c
    src/engine/network/prediction/net_prediction_impl.c
)
target_include_directories(test_net_prediction PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_net_prediction PRIVATE m)
add_test(NAME test_net_prediction COMMAND test_net_prediction)
