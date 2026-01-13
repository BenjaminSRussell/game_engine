/**
 * @file acl_wrapper.h
 * @brief Wrapper for ACL animation compression library.
 */
#ifndef ANIMATION_COMPRESSION_ACL_WRAPPER_H
#define ANIMATION_COMPRESSION_ACL_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ACL wrapper.
 */
void acl_wrapper_init(void);

/**
 * Compress animation clip using ACL.
 */
void acl_compress(void *animation_clip, void *compressed_output);

/**
 * Decompress ACL data back to animation clip.
 */
void acl_decompress(void *compressed_data, void *output_clip);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_COMPRESSION_ACL_WRAPPER_H
