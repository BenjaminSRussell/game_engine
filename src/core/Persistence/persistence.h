#ifndef PERSISTENCE_H
#define PERSISTENCE_H



/**
 * @brief Saves player data to a file.
 * @param playerData A pointer to the player data to be saved.
 * @return 0 on success, -1 on failure.
 */
int save_player_data(void* playerData);

/**
 * @brief Loads player data from a file.
 * @param playerData A pointer to the player data to be loaded into.
 * @return 0 on success, -1 on failure.
 */
int load_player_data(void* playerData);

/**
 * @brief Saves world data to a file.
 * @param worldData A pointer to the world data to be saved.
 * @return 0 on success, -1 on failure.
 */
int save_world_data(void* worldData);

/**
 * @brief Loads world data from a file.
 * @param worldData A pointer to the world data to be loaded into.
 * @return 0 on success, -1 on failure.
 */
int load_world_data(void* worldData);

/**
 * @brief Creates a backup of the save files.
 * @return 0 on success, -1 on failure.
 */
int create_backup();

#endif // PERSISTENCE_H
