#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

// Test Phase 1: Build System Restructuring
// Validates BUILD_EDITOR option and WITH_EDITOR preprocessor flag

TEST_CASE("WITH_EDITOR preprocessor flag is defined in editor builds", "[build-system][phase1]") {
    #ifdef WITH_EDITOR
        REQUIRE(true); // WITH_EDITOR is defined as expected
    #else
        FAIL("WITH_EDITOR should be defined when BUILD_EDITOR=ON");
    #endif
}

TEST_CASE("EngineLib target should compile successfully", "[build-system][phase1]") {
    // This test simply existing proves EngineLib compiled and linked
    REQUIRE(true);
}

// Test reflection system exists (from Phase 0 analysis)
extern "C" {
#include <core/introspection/type_registry.h>
}

TEST_CASE("Type registry initializes correctly", "[reflection][phase0]") {
    type_registry_init();
    
    // Should have registered basic primitives
    const TypeDefinition* int_type = type_registry_get("int");
    const TypeDefinition* float_type = type_registry_get("float");
    
    REQUIRE(int_type != nullptr);
    REQUIRE(float_type != nullptr);
    
    REQUIRE(int_type->size == sizeof(int));
    REQUIRE(float_type->size == sizeof(float));
}

TEST_CASE("Type registry can register custom types", "[reflection][phase0]") {
    type_registry_init();
    
    typedef struct {
        float x, y, z;
    } TestVector3;
    
    int type_id = type_registry_register("TestVector3", sizeof(TestVector3), 
                                         TYPE_KIND_STRUCT, "Test 3D vector");
    
    REQUIRE(type_id >= 0);
    
    // Add fields
    type_registry_add_field(type_id, "x", "float", offsetof(TestVector3, x), sizeof(float));
    type_registry_add_field(type_id, "y", "float", offsetof(TestVector3, y), sizeof(float));
    type_registry_add_field(type_id, "z", "float", offsetof(TestVector3, z), sizeof(float));
    
    // Retrieve and verify
    const TypeDefinition* vec3_type = type_registry_get("TestVector3");
    REQUIRE(vec3_type != nullptr);
    REQUIRE(vec3_type->field_count == 3);
    REQUIRE(strcmp(vec3_type->fields[0].name, "x") == 0);
    REQUIRE(strcmp(vec3_type->fields[1].name, "y") == 0);
    REQUIRE(strcmp(vec3_type->fields[2].name, "z") == 0);
}

#ifdef WITH_EDITOR
#include <editor/placeholder_types.h>

TEST_CASE("Editor placeholder types compile", "[editor][phase1]") {
    // Verify placeholder types exist
    UndoRedoSystem undo_system;
    undo_system.command_count = 0;
    undo_system.current_index = -1;
    
    REQUIRE(undo_system.command_count == 0);
    REQUIRE(MAX_UNDO_COMMANDS == 256);
    REQUIRE(VIEWPORT_COUNT == 4);
}
#endif

// Test JSON serialization (from Phase 0 analysis)
extern "C" {
#include <core/json.h>
}

TEST_CASE("JSON parser handles simple objects", "[serialization][phase0]") {
    const char *json_str = R"({"name": "test", "value": 42, "active": true})";
    
    JsonValue *root = json_parse(json_str);
    REQUIRE(root != nullptr);
    REQUIRE(root->type == JSON_OBJECT);
    
    JsonValue *name_val = json_object_get(root, "name");
    REQUIRE(name_val != nullptr);
    REQUIRE(strcmp(json_string_value(name_val), "test") == 0);
    
    JsonValue *value_val = json_object_get(root, "value");
    REQUIRE(value_val != nullptr);
    REQUIRE(json_number_value(value_val) == 42.0);
    
    JsonValue *active_val = json_object_get(root, "active");
    REQUIRE(active_val != nullptr);
    REQUIRE(json_bool_value(active_val) == true);
    
    json_free(root);
}

TEST_CASE("JSON parser handles arrays", "[serialization][phase0]") {
    const char *json_str = R"([1, 2, 3, 4, 5])";
    
    JsonValue *root = json_parse(json_str);
    REQUIRE(root != nullptr);
    REQUIRE(root->type == JSON_ARRAY);
    REQUIRE(root->array.count == 5);
    
    for (int i = 0; i < 5; ++i) {
        REQUIRE(json_number_value(root->array.values[i]) == (double)(i + 1));
    }
    
    json_free(root);
}

TEST_CASE("JSON parser handles nested structures", "[serialization][phase0]") {
    const char *json_str = R"({
        "entity": {
            "id": 123,
            "position": [1.0, 2.0, 3.0],
            "components": ["Transform", "Mesh", "RigidBody"]
        }
    })";
    
    JsonValue *root = json_parse(json_str);
    REQUIRE(root != nullptr);
    
    JsonValue *entity = json_object_get(root, "entity");
    REQUIRE(entity != nullptr);
    
    JsonValue *id = json_object_get(entity, "id");
    REQUIRE(id != nullptr);
    REQUIRE(json_number_value(id) == 123.0);
    
    JsonValue *position = json_object_get(entity, "position");
    REQUIRE(position != nullptr);
    REQUIRE(position->type == JSON_ARRAY);
    REQUIRE(position->array.count == 3);
    
    JsonValue *components = json_object_get(entity, "components");
    REQUIRE(components != nullptr);
    REQUIRE(components->type == JSON_ARRAY);
    REQUIRE(components->array.count == 3);
    
    json_free(root);
}
