#ifndef LFS_USER_H
#define LFS_USER_H

#include "lfs.h"

#define USE_INTERNAL_FLASH  0
#define dir_path  "myapp"

/*保存msh指令表的路径*/
#define msh_path  "myapp/msh.txt"

/*保存目标ip地址的参数*/
#define dest_ip_path  "myapp/dest_ip.txt"

/*保存目标端口的参数*/
#define dest_port_path  "myapp/dest_port.txt"

/*保存上电次数的文件路径*/
#define boot_path  "myapp/boot.txt"

/*=============================================================================
 * 块设备配置 — 基于 hal_flash 模块
 *
 * 这里使用 Flash 空闲区域中的 8KB 空间。
 *
 * ★ BLOCK_SIZE = 1024 (1KB):
 *    STM32F103C8T6 Flash 硬件页大小为 1KB。
 *    BLOCK_SIZE 对齐到硬件页，确保每次擦除正好擦除一个硬件页。
 *    注意：hal_flash.h 中的 PAGE_SIZE 已同步修正为 1024。
 *============================================================================*/

/* littlefs 存储区起始地址（位于 APP 和 DOWNLOAD 区域之后） */
#define LFS_STORAGE_ADDR    (0x0800E000U)

/* 块设备参数 — 1KB 匹配硬件页大小 */
#define BLOCK_SIZE     (1024)            /* 1KB = STM32F103C8T6 一页 */
#define BLOCK_COUNT    (8)               /* 8 blocks × 1KB = 8KB */
#define READ_SIZE      (1)               /* 最小读取单位 1 字节 */
#define PROG_SIZE      (2)               /* 半字(16bit)编程 */
#define CACHE_SIZE     (256)             /* 缓存大小 */
#define LOOKAHEAD_SIZE (16)              /* 预查找缓冲 */


typedef uint8_t (*lfs_read_function_t)(uint32_t address, uint8_t *data, uint32_t size);
typedef uint8_t (*lfs_erase_function_t)(uint32_t address, uint32_t size);
typedef uint8_t (*lfs_write_function_t)(uint32_t address, uint8_t *data, uint32_t size);

int lfs_user_init(const struct lfs_config *config);

void lfs_user_deinit(void);


int lfs_user_read_file(const char *path, void *buffer, lfs_size_t size);

int lfs_user_write_file(const char *path, const void *buffer, lfs_size_t size);

int lfs_user_append_file(const char *path, const void *buffer, lfs_size_t size);

int lfs_user_edit_file(const char *path, lfs_off_t offset,const void *buffer, lfs_size_t size);

int lfs_user_create_file(const char *path);

int lfs_user_delete_file(const char *path);

int lfs_user_mkdir(const char *path);

int lfs_user_rmdir(const char *path);

int lfs_user_rmdir_r(const char *path);

int lfs_user_exist(const char *path);

int lfs_user_isdir(const char *path);

int lfs_user_stat(const char *path, struct lfs_info *info);

int lfs_user_listdir(const char *path, char *buffer, lfs_size_t bufsize);

int lfs_user_rename(const char *oldpath, const char *newpath);

int lfs_info_init(uint8_t mode);

int test_folder_and_file(void);

int lfs_user_read(char *folder_path , char *file_path , uint8_t *buffer , uint32_t size);
#endif /* LFS_USER_H */
