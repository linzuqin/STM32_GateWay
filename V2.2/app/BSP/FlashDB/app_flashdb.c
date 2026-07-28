#include "app_flashdb.h"
#include "app_w25qxx.h"
#include "sfud.h"
#include "fal.h"
#include "flashdb.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ FlashDB ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

/* KVDB database object (internal, caller doesn't need to know) */
static struct fdb_kvdb kvdb_obj;

static uint32_t boot = 0;

void app_flashdb_init(void)
{
    /* 1. Initialize W25Q128 SPI flash */
    app_w25qxx_init();

    /* 2. SFUD: detect flash chip and configure parameters */
    sfud_init();

    /* 3. FAL: init flash devices and build partition table */
    fal_init();

    /* 4. FlashDB KVDB: init with no default KV */
    {
        struct fdb_default_kv default_kv;
        default_kv.kvs = NULL;
        default_kv.num = 0;
        fdb_kvdb_init(&kvdb_obj, "my_kvdb", "fdb_kvdb1", &default_kv, NULL);
    }

    DEBUG_PRINT("FlashDB initialized successfully.\r\n");
		
		app_flashdb_get("boot" , &boot , sizeof(boot));
		boot ++;
		DEBUG_PRINT("上电次数:%d\r\n" , boot);
		app_flashdb_set("boot" , &boot , sizeof(boot));

}

int app_flashdb_set(const char *key, const void *value, size_t len)
{
    struct fdb_blob blob;

    if (key == NULL || value == NULL || len == 0)
        return -1;

    fdb_blob_make(&blob, value, len);
    if (fdb_kv_set_blob(&kvdb_obj, key, &blob) != FDB_NO_ERR)
        return -1;

    return 0;
}

int app_flashdb_get(const char *key, void *value, size_t len)
{
    struct fdb_blob blob;
    size_t read_len;

    if (key == NULL || value == NULL || len == 0)
        return -1;

    fdb_blob_make(&blob, value, len);
    read_len = fdb_kv_get_blob(&kvdb_obj, key, &blob);
    if (read_len == 0)
        return 0; /* key not found */

    return (int)read_len;
}
