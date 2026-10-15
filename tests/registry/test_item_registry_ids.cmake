# Issue #269. inventory/item_registry.h resolves through the Game target's
# include dir (src/game/blockgame/include). The test is compiled directly, as
# the test_widget_focus block is: add_engine_test links Engine, which does not
# build on origin/main.
add_executable(test_item_registry_ids
    tests/unit/item_registry_ids_test.c
)
target_include_directories(test_item_registry_ids PRIVATE
    ${CMAKE_SOURCE_DIR}/src/game/blockgame/include
    ${CMAKE_SOURCE_DIR}/src/engine/include
)
add_test(NAME test_item_registry_ids COMMAND test_item_registry_ids)
