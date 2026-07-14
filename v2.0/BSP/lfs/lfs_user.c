#include "lfs_user.h"
#include <string.h>
#include "hal_flash.h"
#include "app_w25qxx.h"

static lfs_t g_lfs;

#define LFS_USER_MAX_OPEN_FILES    4

static lfs_file_t g_file_pool[LFS_USER_MAX_OPEN_FILES];
static uint8_t   g_file_used[LFS_USER_MAX_OPEN_FILES];

static int g_mounted = 0;


static lfs_read_function_t lfs_read = NULL;
static lfs_erase_function_t lfs_erase = NULL;
static lfs_write_function_t lfs_write = NULL;

static uint8_t *g_read_buf = NULL;

static uint8_t *g_prog_buf = NULL;

static uint8_t *g_lookahead_buf = NULL;

/*保存上电次数的路径*/

extern uint32_t boot;


/**
 * 
 * @brief 地址转换
 * @param lfs 
 * @param block 
 * @param off 
 * @return uint32_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-30 23:44:37
 * @copyright Copyright (c) 2026
 */
static uint32_t lfs_addr(const struct lfs_config *lfs , lfs_block_t block, lfs_off_t off)
{
    return block * lfs->block_size + off;
}

/**
 * @brief 从片内 Flash 读取数据
 */
static int lfs_flash_read(const struct lfs_config *c,
                      lfs_block_t block,
                      lfs_off_t   off,
                      void       *buffer,
                      lfs_size_t  size)
{
    int ret = 0;
    if(lfs_read == NULL)
    {
        ret = LFS_ERR_IO;
    }
    else
    {
        ret = lfs_read(lfs_addr(c , block, off), (uint8_t *)buffer, size);

    }
    return ret ? LFS_ERR_IO : 0;
}

/**
 * @brief 向片内 Flash 写入数据
 */
static int lfs_flash_write(const struct lfs_config *c,
                      lfs_block_t  block,
                      lfs_off_t    off,
                      const void  *buffer,
                      lfs_size_t   size)
{
    uint8_t ret = 0;
    if(lfs_write == NULL)
    {
        ret = 0;
    }
    else
    {
        ret = lfs_write(lfs_addr(c , block, off), (uint8_t *)buffer, size);

    }
    return ret ? LFS_ERR_IO : 0;
}

/**
 * @brief 擦除一个 block
 */
static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t addr = lfs_addr(c , block, 0);
    uint8_t ret = 0;
    if(lfs_erase == NULL)
    {
        ret = 0;
    }
    else
    {
       ret = lfs_erase(addr, c->block_size);
    }
    return ret ? LFS_ERR_IO : 0;
}

/**
 * @brief 同步（无缓存策略，直接返回成功）
 */
static int lfs_flash_sync(const struct lfs_config *c)
{
    return 0;
}

static lfs_file_t *lfs_user_alloc_file(void)
{
    for (int i = 0; i < LFS_USER_MAX_OPEN_FILES; i++)
    {
        if (!g_file_used[i])
        {
            g_file_used[i] = 1;
            return &g_file_pool[i];
        }
    }
    return NULL;
}

static void lfs_user_free_file(lfs_file_t *fp)
{
    for (int i = 0; i < LFS_USER_MAX_OPEN_FILES; i++)
    {
        if (&g_file_pool[i] == fp)
        {
            g_file_used[i] = 0;
            break;
        }
    }
}

static int lfs_user_is_dir(const char *path)
{
    struct lfs_info info;
    int err;

    err = lfs_stat(&g_lfs, path, &info);
    if (err)
        return err;   /* LFS_ERR_NOENT 或其他错误 */

    return (info.type == LFS_TYPE_DIR) ? 1 : 0;
}

static int path_join(const char *dir, const char *name, char *buf, lfs_size_t len)
{
    lfs_size_t dlen = (lfs_size_t)strlen(dir);
    lfs_size_t nlen = (lfs_size_t)strlen(name);
    lfs_size_t sep  = (dlen > 0 && dir[dlen - 1] != '/') ? 1 : 0;

    if (dlen + sep + nlen + 1 > len)
        return LFS_ERR_NAMETOOLONG;

    memcpy(buf, dir, dlen);
    if (sep)    buf[dlen]     = '/';
    memcpy(buf + dlen + sep, name, nlen);
    buf[dlen + sep + nlen] = '\0';

    return LFS_ERR_OK;
}

int lfs_user_read_file(const char *path, void *buffer, lfs_size_t size)
{
    int err;
    lfs_file_t *fp;
    lfs_ssize_t ret;

    if (!g_mounted || path == NULL || buffer == NULL || size == 0)
        return LFS_ERR_INVAL;

    fp = lfs_user_alloc_file();
    if (fp == NULL)
        return LFS_ERR_NOMEM;

    err = lfs_file_open(&g_lfs, fp, path, LFS_O_RDONLY);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    ret = lfs_file_read(&g_lfs, fp, buffer, size);
    if (ret < 0)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return (int)ret;
    }

    err = lfs_file_close(&g_lfs, fp);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    lfs_user_free_file(fp);
    return (int)ret;
}

int lfs_user_write_file(const char *path, const void *buffer, lfs_size_t size)
{
    int err;
    lfs_file_t *fp;
    lfs_ssize_t ret;

    if (!g_mounted || path == NULL || buffer == NULL || size == 0)
        return LFS_ERR_INVAL;

    fp = lfs_user_alloc_file();
    if (fp == NULL)
        return LFS_ERR_NOMEM;

    err = lfs_file_open(&g_lfs, fp, path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    ret = lfs_file_write(&g_lfs, fp, buffer, size);
    if (ret < 0)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return (int)ret;
    }

    err = lfs_file_close(&g_lfs, fp);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    lfs_user_free_file(fp);
    return (int)ret;
}

int lfs_user_append_file(const char *path, const void *buffer, lfs_size_t size)
{
    int err;
    lfs_file_t *fp;
    lfs_ssize_t ret;

    if (!g_mounted || path == NULL || buffer == NULL || size == 0)
        return LFS_ERR_INVAL;

    fp = lfs_user_alloc_file();
    if (fp == NULL)
        return LFS_ERR_NOMEM;

    err = lfs_file_open(&g_lfs, fp, path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    ret = lfs_file_write(&g_lfs, fp, buffer, size);
    if (ret < 0)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return (int)ret;
    }

    err = lfs_file_close(&g_lfs, fp);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    lfs_user_free_file(fp);
    return (int)ret;
}

int lfs_user_edit_file(const char *path, lfs_off_t offset,const void *buffer, lfs_size_t size)
{
    int err;
    lfs_file_t *fp;
    lfs_ssize_t ret;

    if (!g_mounted || path == NULL || buffer == NULL || size == 0)
        return LFS_ERR_INVAL;

    fp = lfs_user_alloc_file();
    if (fp == NULL)
        return LFS_ERR_NOMEM;

    /* 以读写方式打开（不截断），文件必须已存在 */
    err = lfs_file_open(&g_lfs, fp, path, LFS_O_RDWR);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    /* 定位到偏移位置 */
    ret = lfs_file_seek(&g_lfs, fp, offset, LFS_SEEK_SET);
    if (ret < 0)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return (int)ret;
    }

    ret = lfs_file_write(&g_lfs, fp, buffer, size);
    if (ret < 0)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return (int)ret;
    }

    /* 若写入后文件变短，需截断 */
    err = lfs_file_truncate(&g_lfs, fp, offset + size);
    if (err)
    {
        lfs_file_close(&g_lfs, fp);
        lfs_user_free_file(fp);
        return err;
    }

    err = lfs_file_close(&g_lfs, fp);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    lfs_user_free_file(fp);
    return (int)ret;
}

int lfs_user_create_file(const char *path)
{
    int err;
    lfs_file_t *fp;

    if (!g_mounted || path == NULL)
        return LFS_ERR_INVAL;

    fp = lfs_user_alloc_file();
    if (fp == NULL)
        return LFS_ERR_NOMEM;

    err = lfs_file_open(&g_lfs, fp, path, LFS_O_RDWR | LFS_O_CREAT);
    if (err)
    {
        lfs_user_free_file(fp);
        return err;
    }

    err = lfs_file_close(&g_lfs, fp);
    lfs_user_free_file(fp);
    return err;
}

int lfs_user_delete_file(const char *path)
{
    int ret;

    if (!g_mounted || path == NULL)
        return LFS_ERR_INVAL;

    /* 如果是目录则拒绝删除 */
    ret = lfs_user_is_dir(path);
    if (ret < 0)
        return ret;         /* 不存在或错误 */
    if (ret == 1)
        return LFS_ERR_ISDIR;  /* 是目录，要用 rmdir */

    return lfs_remove(&g_lfs, path);
}

int lfs_user_mkdir(const char *path)
{
    int ret;

    if (!g_mounted || path == NULL)
        return LFS_ERR_INVAL;

    /* 尝试逐级创建中间目录 */
    static char tmp[LFS_NAME_MAX + 1];
    const char *p = path;

    /* 跳过开头的 '/' */
    while (*p == '/')
        p++;

    while (*p)
    {
        const char *slash = strchr(p, '/');
        if (slash == NULL)
        {
            /* 最后一级，直接创建 */
            ret = lfs_mkdir(&g_lfs, path);
            if (ret && ret != LFS_ERR_EXIST)
                return ret;
            return LFS_ERR_OK;
        }

        /* 复制到 tmp */
        size_t len = (size_t)(slash - p);
        memcpy(tmp, p, len);
        tmp[len] = '\0';

        ret = lfs_mkdir(&g_lfs, tmp);
        if (ret && ret != LFS_ERR_EXIST)
            return ret;

        p = slash + 1;
    }

    return LFS_ERR_OK;
}

int lfs_user_rmdir(const char *path)
{
    int ret;

    if (!g_mounted || path == NULL)
        return LFS_ERR_INVAL;

    ret = lfs_user_is_dir(path);
    if (ret < 0)
        return ret;         /* 不存在或错误 */
    if (ret == 0)
        return LFS_ERR_NOTDIR; /* 是文件，不是目录 */

    return lfs_remove(&g_lfs, path);
}

static int rmdir_r_recursive(const char *path)
{
    int err;
    lfs_dir_t dir;
    struct lfs_info info;
    char child[LFS_NAME_MAX + 1];

    err = lfs_dir_open(&g_lfs, &dir, path);
    if (err)
        return err;

    while (1)
    {
        err = lfs_dir_read(&g_lfs, &dir, &info);
        if (err <= 0)
            break;  /* 0 = 读取完毕，负值 = 错误 */

        /* 跳过 . 和 .. */
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
            continue;

        /* 构造子路径 */
        err = path_join(path, info.name, child, sizeof(child));
        if (err)
        {
            lfs_dir_close(&g_lfs, &dir);
            return err;
        }

        if (info.type == LFS_TYPE_DIR)
        {
            err = rmdir_r_recursive(child);
            if (err)
            {
                lfs_dir_close(&g_lfs, &dir);
                return err;
            }
        }
        else
        {
            err = lfs_remove(&g_lfs, child);
            if (err)
            {
                lfs_dir_close(&g_lfs, &dir);
                return err;
            }
        }
    }

    err = lfs_dir_close(&g_lfs, &dir);
    if (err)
        return err;

    /* 删除自身 */
    return lfs_remove(&g_lfs, path);
}

int lfs_user_rmdir_r(const char *path)
{
    int ret;

    if (!g_mounted || path == NULL)
        return LFS_ERR_INVAL;

    ret = lfs_user_is_dir(path);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return LFS_ERR_NOTDIR;

    return rmdir_r_recursive(path);
}

int lfs_user_exist(const char *path)
{
    struct lfs_info info;

    if (!g_mounted || path == NULL)
        return 0;

    return (lfs_stat(&g_lfs, path, &info) == 0) ? 1 : 0;
}

int lfs_user_isdir(const char *path)
{
    return lfs_user_is_dir(path);
}

int lfs_user_stat(const char *path, struct lfs_info *info)
{
    if (!g_mounted || path == NULL || info == NULL)
        return LFS_ERR_INVAL;

    return lfs_stat(&g_lfs, path, info);
}

int lfs_user_listdir(const char *path, char *buffer, lfs_size_t bufsize)
{
    lfs_dir_t dir;
    struct lfs_info info;
    int err;
    lfs_size_t pos = 0;

    if (!g_mounted || path == NULL || buffer == NULL || bufsize == 0)
        return LFS_ERR_INVAL;

    err = lfs_dir_open(&g_lfs, &dir, path);
    if (err)
        return err;

    while (1)
    {
        err = lfs_dir_read(&g_lfs, &dir, &info);
        if (err < 0)
        {
            lfs_dir_close(&g_lfs, &dir);
            return err;
        }
        if (err == 0)
            break;  /* 无更多条目 */

        /* 跳过 . 和 .. */
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
            continue;

        /* 写入 "name\n" 或 "name/\n"（目录加后缀） */
        lfs_size_t nlen = (lfs_size_t)strlen(info.name);
        int is_dir = (info.type == LFS_TYPE_DIR);
        lfs_size_t need = nlen + (is_dir ? 2 : 1);  /* 名称 + 后缀(\n 或 /\n) */

        if (pos + need >= bufsize)
            break;  /* 缓冲区不足 */

        memcpy(buffer + pos, info.name, nlen);
        pos += nlen;
        if (is_dir)
        {
            buffer[pos] = '/';
            pos++;
        }
        buffer[pos] = '\n';
        pos++;
    }

    lfs_dir_close(&g_lfs, &dir);
    buffer[pos] = '\0';
    return (int)pos;
}

int lfs_user_rename(const char *oldpath, const char *newpath)
{
    if (!g_mounted || oldpath == NULL || newpath == NULL)
        return LFS_ERR_INVAL;

    return lfs_rename(&g_lfs, oldpath, newpath);
}


/**
 * 
 * @brief 
 * @param mode 0:内部flash 1:外部flash
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-30 23:26:00
 * @copyright Copyright (c) 2026
 */
static void lfs_config_init(struct lfs_config *config , uint8_t mode)
{
    /* 必须清零，否则未设置的字段（name_max/file_max/attr_max 等）会带栈垃圾值 */
    memset(config, 0, sizeof(*config));

    if(mode == 1)
    {
        app_w25qxx_init();
        printf("lfs: mounting W25Q128 ...\n");
        config->read_size       = 1;          /* 字节读取 */
        config->prog_size       = 256;        /* 页编程（W25Q128 一页 256 字节） */
        config->block_size      = 4096;       /* 4 KB = 扇区擦除单位 */
        config->block_count     = 4096;       /* 16 MB / 4 KB = 4096 块 */
        config->block_cycles    = 500;
        config->cache_size      = 256;
        config->lookahead_size  = 512;        /* 4096 bits / 8 = 512 bytes */
        lfs_read  = app_w25qxx_read;
        lfs_erase = app_w25qxx_erase;
        lfs_write = app_w25qxx_write;  /* 纯页写入，LFS管理擦除 */

        g_lookahead_buf = (uint8_t *)malloc(sizeof(uint8_t) * 512);
        g_read_buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);
        g_prog_buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);

        config->read_buffer      = g_read_buf;
        config->prog_buffer      = g_prog_buf;
        config->lookahead_buffer = g_lookahead_buf; /* 使用大缓冲区 */
    }else if(mode == 0)
    {
        printf("lfs: mounting internal flash ...\n");

        config->read_size       = 1;
        config->prog_size       = 2;
        config->block_size      = 1024;
        config->block_count     = 2;
        config->block_cycles    = 500;
        config->cache_size      = 256;
        config->lookahead_size  = 16;

        lfs_read  = flash_read;
        lfs_erase = flash_erase;
        lfs_write = flash_write;

        g_lookahead_buf = (uint8_t *)malloc(sizeof(uint8_t) * 16);
        g_read_buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);
        g_prog_buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);

        config->read_buffer      = g_read_buf;
        config->prog_buffer      = g_prog_buf;
        config->lookahead_buffer = g_lookahead_buf;
    }

//    config->context = NULL;

    config->read = lfs_flash_read;
    config->prog = lfs_flash_write;
    config->erase = lfs_flash_erase;
    config->sync = lfs_flash_sync;

}

int lfs_user_init(const struct lfs_config *config)
{
    int err;

    if (config == NULL)
        return LFS_ERR_INVAL;

    g_mounted = 0;

    for (int i = 0; i < LFS_USER_MAX_OPEN_FILES; i++)
    {
        g_file_pool[i].type = 0;
        g_file_used[i] = 0;
    }

    err = lfs_mount(&g_lfs, config);
    if (err)
    {
        err = lfs_format(&g_lfs, config);
        if (err)
            return err;

        err = lfs_mount(&g_lfs, config);
        if (err)
            return err;
    }

    g_mounted = 1;
    return LFS_ERR_OK;
}

void lfs_user_deinit(void)
{
    if (g_mounted)
    {
        (void)lfs_unmount(&g_lfs);
        g_mounted = 0;
    }
}

int lfs_info_init(uint8_t mode)
{
    int ret;
    static struct lfs_config config;
    lfs_config_init(&config , mode);
    ret = lfs_user_init(&config);
    if (ret != LFS_ERR_OK)
    {
        printf("[FAIL] lfs_user_init: err=%d\n", ret);
    }
    else
    {
        printf("[OK]  文件系统初始化成功\n\n");
    }
    return ret;
}

int lfs_user_read(char *folder_path , char *file_path , uint8_t *buffer , uint32_t size)
{
	int ret;
	
	/**********确认目录**********/
	ret = lfs_user_isdir(folder_path);
	if (ret == 1)
	{
		
	}
	else
	{
		ret = lfs_user_mkdir(folder_path);
		if (ret != LFS_ERR_OK)
		{
				printf("[FAIL] 创建文件夹 %s 失败: err=%d\n",folder_path , ret);
				return ret;
		}
	}

	/**********确认文件**********/
	if(lfs_user_exist(file_path) == 0)
	{
		printf("未检测到%s 创建%s\r\n", file_path, file_path);

		ret = lfs_user_write_file(file_path, buffer, size);
		if (ret < 0)
		{
				printf("[FAIL] 写入 %s 失败: err=%d\n", file_path , ret);
				return ret;
		}
	}
	
	/**********读取变量**********/
	ret = lfs_user_read_file(file_path, buffer, size);
	if (ret < 0)
	{
		printf("[FAIL] 读取 %s 失败: err=%d\n",file_path , ret);
		return ret;
	}
	printf("%s 读取成功\r\n" , file_path);
	return ret;
}

int test_folder_and_file(void)
{
	int ret;

    lfs_user_read(dir_path , boot_path , (uint8_t *)&boot , sizeof(uint32_t));

	/**********列出目录**********/
	char list[128];
	ret = lfs_user_listdir(dir_path, list, sizeof(list));
	if (ret >= 0)
	{
		printf("[OK] %s/ 内容:\n%s", dir_path , list);
	}

	/**********更新变量**********/
	boot ++;
	ret = lfs_user_write_file(boot_path, &boot, sizeof(uint32_t));
	if (ret < 0)
	{
			printf("[FAIL] 写入 %s 失败: err=%d\n", boot_path , ret);
			return ret;
	}
	printf("更新boot的值:%d\r\n" , boot);

	return 0;
}

int lfs_user_save_tcp_client_dest_info(uint8_t *dest_ip , uint16_t desp_port)
{
	int ret = -1;

	/*保存ip*/
	if(lfs_user_exist(dest_ip_path) == 0)
	{
		printf("未检测到%s 创建%s\r\n", dest_ip_path, dest_ip_path);
	}
	ret = lfs_user_write_file(dest_ip_path, dest_ip, sizeof(dest_ip));
	if (ret < 0)
	{
		printf("[FAIL] 写入 %s 失败: err=%d\n", dest_ip_path , ret);
	}
	else
	{
		/*保存端口*/
		if(lfs_user_exist(dest_port_path) == 0)
		{
			printf("未检测到%s 创建%s\r\n", dest_port_path, dest_port_path);
		}

		ret = lfs_user_write_file(dest_port_path, &desp_port, sizeof(desp_port));
		if (ret < 0)
		{
			printf("[FAIL] 写入 %s 失败: err=%d\n", dest_port_path , ret);
		}
	}
	return ret;
}
