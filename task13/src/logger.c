#include "logger.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <string.h> /* memset */

LOG_MODULE_REGISTER(logger);

#define MOUNT_POINT "/lfs"
#define LOG_FILE_PATH "/lfs/sensor_data"

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_message), MAX_QUEUE, 4);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_config);

static struct fs_mount_t mount = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_config,
    .storage_dev = (void *)FIXED_PARTITION_ID(lfs1_partition),
    .mnt_point = MOUNT_POINT,
};

static struct k_thread logger_thread_data;
static K_THREAD_STACK_DEFINE(logger_stack, 4096);
static K_MUTEX_DEFINE(file_mutex);

/* Entry size (one queued record) */
static const size_t ENTRY_SIZE = sizeof(struct sensor_message);

/* Persistent meta for circular buffer */
struct logger_meta {
    uint32_t head_index;     /* next write index (0..MAX_ENTRIES-1) */
    uint32_t entries_count;  /* number of valid entries currently stored (<= MAX_ENTRIES) */
} __packed;

static struct logger_meta meta;

/* meta file path */
#define META_FILE_PATH MOUNT_POINT "/logger_meta"

/* persist/load meta helpers */
static int meta_save(const struct logger_meta *m)
{
    struct fs_file_t f;
    fs_file_t_init(&f);
    if (fs_open(&f, META_FILE_PATH, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE) != 0) {
        return -1;
    }
    ssize_t written = fs_write(&f, (const void *)m, sizeof(*m));
    fs_close(&f);
    return (written == (ssize_t)sizeof(*m)) ? 0 : -1;
}

static int meta_load(struct logger_meta *m)
{
    struct fs_file_t f;
    fs_file_t_init(&f);
    if (fs_open(&f, META_FILE_PATH, FS_O_READ) != 0) {
        return -1;
    }
    ssize_t r = fs_read(&f, (void *)m, sizeof(*m));
    fs_close(&f);
    return (r == (ssize_t)sizeof(*m)) ? 0 : -1;
}

/* Create data file if missing. If first-run, pre-allocate zero bytes for MAX_ENTRIES */
static void ensure_data_file(void)
{
    struct fs_file_t f;
    fs_file_t_init(&f);

    /* If file already exists, do nothing */
    if (fs_open(&f, LOG_FILE_PATH, FS_O_READ) == 0) {
        fs_close(&f);
        LOG_INF("%s exists", LOG_FILE_PATH);
        return;
    }

    /* Create & pre-allocate file sized for the circular buffer */
    if (fs_open(&f, LOG_FILE_PATH, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE) != 0) {
        LOG_ERR("Failed to create %s", LOG_FILE_PATH);
        return;
    }

    const size_t total_bytes = (size_t)MAX_ENTRIES * ENTRY_SIZE;
    const size_t chunk = 256;
    uint8_t zbuf[chunk];
    memset(zbuf, 0, chunk);

    size_t left = total_bytes;
    while (left) {
        size_t towrite = (left > chunk) ? chunk : left;
        ssize_t w = fs_write(&f, zbuf, towrite);
        if (w <= 0) {
            LOG_ERR("Failed to pre-allocate file (w=%zd)", w);
            break;
        }
        left -= (size_t)w;
    }

    fs_close(&f);
    LOG_INF("Created and pre-allocated %s (%u entries)", LOG_FILE_PATH, MAX_ENTRIES);
}

static void logger_thread(void *a, void *b, void *c)
{
    struct sensor_message msg;
    struct fs_file_t file;
    fs_file_t_init(&file);

    while (1) {
        if (k_msgq_get(&sensor_msgq, &msg, K_FOREVER) == 0) {
            k_mutex_lock(&file_mutex, K_FOREVER);

            /* Open for read/write (we'll seek & overwrite at a specific offset) */
            if (fs_open(&file, LOG_FILE_PATH, FS_O_RDWR) == 0) {
                off_t offset = (off_t)meta.head_index * ENTRY_SIZE;
                fs_seek(&file, offset, FS_SEEK_SET);
                ssize_t w = fs_write(&file, &msg, ENTRY_SIZE);
                fs_close(&file);

                if (w == (ssize_t)ENTRY_SIZE) {
                    /* advance head & update entries_count */
                    meta.head_index = (meta.head_index + 1) % MAX_ENTRIES;
                    if (meta.entries_count < MAX_ENTRIES) {
                        meta.entries_count++;
                    }
                    if (meta_save(&meta) != 0) {
                        LOG_ERR("Failed to persist meta");
                    }
                } else {
                    LOG_ERR("Failed to write full entry (w=%zd)", w);
                }
            } else {
                LOG_ERR("Failed to open data file for writing");
            }

            k_mutex_unlock(&file_mutex);
        }
    }
}

void logger_init(void)
{
    int rc = fs_mount(&mount);
    if (rc == 0) {
        LOG_INF("Filesystem mounted at %s", mount.mnt_point);
    } else {
        LOG_ERR("Failed to mount FS (%d)", rc);
        return;
    }

    /* Ensure data file exists (creates & preallocates if necessary) */
    ensure_data_file();

    /* Try to load persisted meta; if not present, initialize it and persist */
    if (meta_load(&meta) != 0) {
        LOG_INF("No logger meta found: initializing meta");
        meta.head_index = 0;
        meta.entries_count = 0;
        if (meta_save(&meta) != 0) {
            LOG_ERR("Failed to create meta file");
        } else {
            LOG_INF("Created meta file %s", META_FILE_PATH);
        }
    } else {
        LOG_INF("Loaded logger meta: head=%u entries=%u",
                meta.head_index, meta.entries_count);
    }

    /* start the logger thread */
    k_thread_create(&logger_thread_data, logger_stack,
                    K_THREAD_STACK_SIZEOF(logger_stack),
                    logger_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);
}

void logger_enqueue(const struct sensor_message *msg)
{
    k_msgq_put(&sensor_msgq, msg, K_NO_WAIT);
}
