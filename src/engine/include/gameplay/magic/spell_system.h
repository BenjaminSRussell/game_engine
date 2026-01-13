#ifndef SPELL_SYSTEM_H
#define SPELL_SYSTEM_H

#include "spell_types.h"

// Forward declaration
struct AbilitySystemComponent;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the spell combination system.
 * @return true if initialization was successful, false otherwise.
 */
bool spell_system_init(void);

/**
 * @brief Shuts down the spell combination system.
 */
void spell_system_shutdown(void);

/**
 * @brief Registers a new spell definition.
 * @param spell The spell definition to register.
 * @return true if registered successfully, false if ID already exists or storage full.
 */
bool spell_system_register_spell(SpellDefinition spell);

/**
 * @brief Registers a combination of elements that results in a spell.
 * @param elements Array of magic elements.
 * @param count Number of elements.
 * @param spell_id The ID of the spell this combination produces.
 * @return true if registered successfully.
 */
bool spell_system_register_combination(const MagicElement* elements, u32 count, u32 spell_id);

/**
 * @brief Combines elements to find a matching spell.
 * @param elements Array of magic elements.
 * @param count Number of elements.
 * @return The ID of the resulting spell, or 0 if no match found.
 *         (Assuming spell IDs start at 1, 0 is invalid).
 */
u32 spell_system_combine(const MagicElement* elements, u32 count);

/**
 * @brief Retrieves the spell definition for a given ID.
 * @param spell_id The ID of the spell.
 * @return Pointer to the spell definition, or NULL if not found.
 */
const SpellDefinition* spell_system_get_spell(u32 spell_id);

/**
 * @brief Helper function to get the name of an element.
 */
const char* spell_system_get_element_name(MagicElement element);

#ifdef __cplusplus
}
#endif

#endif // SPELL_SYSTEM_H
