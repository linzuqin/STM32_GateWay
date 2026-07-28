#ifndef _APP_FLASHDB_H_
#define _APP_FLASHDB_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief  Initialize FlashDB (W25QXX + SFUD + FAL + KVDB).
 * @note   No parameters needed. Call once at startup.
 */
void app_flashdb_init(void);

/**
 * @brief  Set a key-value pair (blob data).
 * @param  key   Key string (null-terminated)
 * @param  value Pointer to the value data
 * @param  len   Length of the value data in bytes
 * @return 0 on success, -1 on failure
 */
int app_flashdb_set(const char *key, const void *value, size_t len);

/**
 * @brief  Get a value by key (blob data).
 * @param  key   Key string (null-terminated)
 * @param  value Pointer to buffer to receive the value
 * @param  len   Size of the buffer in bytes
 * @return > 0: actual read length, 0: key not found, -1: error
 */
int app_flashdb_get(const char *key, void *value, size_t len);

#endif /* _APP_FLASHDB_H_ */
