#define CGLTF_IMPLEMENTATION
#include "../include/vendor/cgltf.h>
#include <stdio.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "assets/models/characters/spirit.gltf";

    cgltf_options options = {0};
    cgltf_data *data = NULL;

    printf("Loading GLTF: %s\n", path);

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        printf("❌ Failed to parse GLTF: error %d\n", result);
        return 1;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        printf("❌ Failed to load buffers: error %d\n", result);
        cgltf_free(data);
        return 1;
    }

    printf("\n✅ Loaded GLTF successfully!\n\n");
    printf("Model Information:\n");
    printf("  Meshes:     %zu\n", data->meshes_count);
    printf("  Nodes:      %zu\n", data->nodes_count);
    printf("  Materials:  %zu\n", data->materials_count);
    printf("  Textures:   %zu\n", data->textures_count);
    printf("  Animations: %zu\n", data->animations_count);
    printf("  Skins:      %zu\n", data->skins_count);

    if (data->meshes_count > 0) {
        printf("\nFirst Mesh:\n");
        cgltf_mesh *mesh = &data->meshes[0];
        printf("  Name:       %s\n", mesh->name ? mesh->name : "(unnamed)");
        printf("  Primitives: %zu\n", mesh->primitives_count);

        if (mesh->primitives_count > 0) {
            cgltf_primitive *prim = &mesh->primitives[0];
            printf("  Attributes: %zu\n", prim->attributes_count);

            for (size_t i = 0; i < prim->attributes_count; i++) {
                cgltf_attribute *attr = &prim->attributes[i];
                const char *type_name = "UNKNOWN";

                switch (attr->type) {
                    case cgltf_attribute_type_position: type_name = "POSITION"; break;
                    case cgltf_attribute_type_normal: type_name = "NORMAL"; break;
                    case cgltf_attribute_type_texcoord: type_name = "TEXCOORD"; break;
                    case cgltf_attribute_type_joints: type_name = "JOINTS"; break;
                    case cgltf_attribute_type_weights: type_name = "WEIGHTS"; break;
                    default: break;
                }

                printf("    - %-12s: %zu elements\n", type_name, attr->data->count);
            }

            if (prim->indices) {
                printf("  Indices:    %zu\n", prim->indices->count);
            }
        }
    }

    if (data->animations_count > 0) {
        printf("\nAnimations:\n");
        for (size_t i = 0; i < data->animations_count; i++) {
            cgltf_animation *anim = &data->animations[i];
            printf("  [%zu] %s - %zu channels\n",
                   i,
                   anim->name ? anim->name : "(unnamed)",
                   anim->channels_count);
        }
    }

    cgltf_free(data);
    return 0;
}
