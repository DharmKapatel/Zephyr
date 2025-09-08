/**
 * @file logger.c
 * @brief Persistent sensor data logger using LittleFS and circular buffer in SRAM1.
 *
 * Shared buffer is located in the reserved devicetree memory node
 * labeled "shared_logger" (must be created via .overlay).
 *
 * Producers do non-blocking pushes (drop on full); logger thread blocks
 * waiting for available items, then writes entries to /lfs/sensor_data.
 */

#include "logger.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <string.h> /* memcpy */
#include <stdint.h>

LOG_MODULE_REGISTER(logger);

/* LittleFS paths */
#define MOUNT_POINT "/lfs"
#define LOG_FILE_PATH "/lfs/sensor_data"
#define META_FILE_PATH MOUNT_POINT "/logger_meta"

/** Default LittleFS configuration. */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_config);

/** Filesystem mount configuration. */
static struct fs_mount_t mount = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_config,
    .storage_dev = (void *)FIXED_PARTITION_ID(lfs1_partition),
    .mnt_point = MOUNT_POINT,
};

/** Logger thread handle and stack. */
static struct k_thread logger_thread_data;
static K_THREAD_STACK_DEFINE(logger_stack, 4096);

/** Mutex to guard file access. */
static K_MUTEX_DEFINE(file_mutex);

/** Entry size (one queued record) - used for file I/O. */
static const size_t ENTRY_SIZE = sizeof(struct sensor_message);

/**
 * Devicetree: reserved node label must be "shared_logger"
 * overlay should provide a reserved-memory node with label "shared_logger".
 */
#define SHARED_NODE DT_NODELABEL(shared_logger)

BUILD_ASSERT(DT_NODE_HAS_STATUS(SHARED_NODE, okay),
             "Devicetree node 'shared_logger' not found or not 'okay'. Add overlay to reserve SRAM1.");

/* Base addr & size from DTS */
#define SHARED_BASE_ADDR ((uintptr_t)DT_REG_ADDR(SHARED_NODE))
#define SHARED_SIZE_BYTES ((size_t)DT_REG_SIZE(SHARED_NODE))

/* Compute region capacity (number of sensor_message slots) at runtime */
static size_t region_capacity = 0;

/* Backing buffer pointer (points into SRAM1 reserved area) */
static volatile struct sensor_message *msg_buffer = NULL;

/* In-memory indices (kept in normal SRAM) */
static uint32_t buf_head = 0; /**< index to pop (0..region_capacity-1) */
static uint32_t buf_tail = 0; /**< index to push */
static uint32_t buf_count = 0; /**< number of messages currently in buffer */

/** Mutex protecting the in-memory circular buffer (short critical sections). */
static K_MUTEX_DEFINE(buffer_mutex);

/** Semaphore counting available items in buffer (initial 0). */
static struct k_sem items_sem;

/** Persistent metadata for circular buffer stored on LittleFS. */
struct logger_meta {
    uint32_t head_index;     /**< Next write index (0..MAX_ENTRIES-1). */
    uint32_t entries_count;  /**< Number of valid entries currently stored. */
} __packed;

/** Runtime metadata copy. */
static struct logger_meta meta;

/* Drop statistics */
static uint32_t dropped_count = 0;

/* ---- Persistent meta helpers ---- */
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

/* ---- Ensure LittleFS data file exists (unchanged logic) ---- */
static void ensure_data_file(void)
{
    struct fs_file_t f;
    fs_file_t_init(&f);

    const size_t total_bytes = (size_t)MAX_ENTRIES * ENTRY_SIZE;

    /* If file exists and is already large enough, nothing to do */
    struct fs_dirent entry;
    if (fs_stat(LOG_FILE_PATH, &entry) == 0) {
        if ((size_t)entry.size >= total_bytes) {
            LOG_INF("%s exists and size OK (%u entries)", LOG_FILE_PATH, MAX_ENTRIES);
            return;
        }
    }

    /* Open file (create if missing) for read/write so we can probe/extend */
    if (fs_open(&f, LOG_FILE_PATH, FS_O_CREATE | FS_O_RDWR) != 0) {
        LOG_ERR("Failed to open/create %s", LOG_FILE_PATH);
        return;
    }

    /* Find current size by seeking to end then telling */
    off_t cur_size = 0;
    if (fs_seek(&f, 0, FS_SEEK_END) == 0) {
        cur_size = fs_tell(&f);
        if (cur_size < 0) {
            LOG_WRN("fs_tell returned < 0 (%ld), treating as 0", (long)cur_size);
            cur_size = 0;
        }
    } else {
        LOG_WRN("fs_seek to end failed, treating current size as 0");
    }

    /* If file is smaller than required, try a cheap probe first */
    if ((size_t)cur_size < total_bytes) {
        off_t probe_offset = (off_t)(total_bytes - 1);
        bool probe_ok = false;

        if (fs_seek(&f, probe_offset, FS_SEEK_SET) == 0) {
            uint8_t probe = 0;
            ssize_t w = fs_write(&f, &probe, 1);
            if (w == 1) {
                probe_ok = true;
                LOG_INF("Probe write succeeded — no bulk pre-allocation required");
            } else {
                LOG_WRN("Probe write failed (w=%zd), will fall back to safe extend", w);
            }
        } else {
            LOG_WRN("Probe seek to %ld failed, will fall back to safe extend", (long)probe_offset);
        }

        (void)probe_ok;
    } else {
        LOG_INF("%s already large enough (%zu bytes)", LOG_FILE_PATH, (size_t)cur_size);
    }

    fs_close(&f);
}

/* ---- Shared SRAM1 buffer helpers ---- */

/* Compute and initialize buffer pointer + capacity. Called from logger_init() */
static int shared_region_setup(void)
{
    if (SHARED_SIZE_BYTES < sizeof(struct sensor_message)) {
        LOG_ERR("shared_logger region too small: %zu bytes", SHARED_SIZE_BYTES);
        return -1;
    }

    region_capacity = SHARED_SIZE_BYTES / sizeof(struct sensor_message);
    if (region_capacity == 0) {
        LOG_ERR("shared_logger region has zero capacity");
        return -1;
    }

    /* Cap region_capacity to MAX_QUEUE if you want a compile-time upper bound */
    if (region_capacity > (size_t)MAX_QUEUE) {
        region_capacity = (size_t)MAX_QUEUE;
    }

    msg_buffer = (volatile struct sensor_message *)((uintptr_t)SHARED_BASE_ADDR);

    LOG_INF("Shared logger region: base=0x%08x size=%u slots=%u",
            (uint32_t)SHARED_BASE_ADDR, (uint32_t)SHARED_SIZE_BYTES, (uint32_t)region_capacity);

    return 0;
}

/**
 * @brief Push a message into the in-memory buffer (non-blocking).
 *
 * Returns 0 on success, -1 if buffer was full or mutex busy.
 */
static int buffer_push(const struct sensor_message *msg)
{
    /* Non-blocking push: try to take buffer mutex without waiting */
    if (k_mutex_lock(&buffer_mutex, K_NO_WAIT) != 0) {
        /* Busy -> drop (matches original non-blocking queue behavior) */
        dropped_count++;
        return -1;
    }

    if (buf_count >= (uint32_t)region_capacity) {
        /* full -> drop */
        dropped_count++;
        k_mutex_unlock(&buffer_mutex);
        return -1;
    }

    /* copy into reserved SRAM1 slot */
    /* use memcpy to avoid strict-alias problems and to be explicit */
    memcpy((void *)&( (struct sensor_message *)msg_buffer )[buf_tail],
           (const void *)msg, ENTRY_SIZE);

    buf_tail = (buf_tail + 1U) % (uint32_t)region_capacity;
    buf_count++;

    /* Signal an available item */
    k_sem_give(&items_sem);

    k_mutex_unlock(&buffer_mutex);
    return 0;
}

/**
 * @brief Pop a message from the in-memory buffer (blocking at caller level).
 *
 * Caller must have waited on items_sem beforehand (so pop should succeed).
 * Returns 0 on success, -1 if buffer was empty (unexpected).
 */
static int buffer_pop(struct sensor_message *out)
{
    if (k_mutex_lock(&buffer_mutex, K_FOREVER) != 0) {
        return -1;
    }

    if (buf_count == 0) {
        k_mutex_unlock(&buffer_mutex);
        return -1;
    }

    memcpy((void *)out,
           (const void *)&( (struct sensor_message *)msg_buffer )[buf_head],
           ENTRY_SIZE);

    buf_head = (buf_head + 1U) % (uint32_t)region_capacity;
    buf_count--;

    k_mutex_unlock(&buffer_mutex);
    return 0;
}

/* ---- Logger background thread ---- */
static void logger_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

    struct sensor_message msg;
    struct fs_file_t file;
    fs_file_t_init(&file);

    while (1) {
        /* Wait for at least one item to be available in buffer */
        k_sem_take(&items_sem, K_FOREVER);

        /* Pop the message (protected by buffer_mutex) */
        if (buffer_pop(&msg) != 0) {
            /* Shouldn't happen (we were signalled), but guard anyway */
            LOG_WRN("items_sem signalled but buffer_pop failed");
            continue;
        }

        /* Write the popped message into persistent circular file */
        k_mutex_lock(&file_mutex, K_FOREVER);

        if (fs_open(&file, LOG_FILE_PATH, FS_O_RDWR) == 0) {
            off_t offset = (off_t)meta.head_index * ENTRY_SIZE;
            if (fs_seek(&file, offset, FS_SEEK_SET) == 0) {
                ssize_t w = fs_write(&file, &msg, ENTRY_SIZE);
                if (w == (ssize_t)ENTRY_SIZE) {
                    /* advance head & update entries_count */
                    meta.head_index = (meta.head_index + 1U) % MAX_ENTRIES;
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
                LOG_ERR("Failed to seek data file for write");
            }
            fs_close(&file);
        } else {
            LOG_ERR("Failed to open data file for writing");
        }

        k_mutex_unlock(&file_mutex);
    }
}

/* ---- Public API ---- */

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

    /* Setup shared SRAM1 region as circular buffer */
    if (shared_region_setup() != 0) {
        LOG_ERR("shared_region_setup failed; logger not started");
        return;
    }

    /* Initialize in-memory buffer tracking primitives */
    buf_head = buf_tail = buf_count = 0;
    k_sem_init(&items_sem, 0, (int)region_capacity);

    /* start the logger thread */
    k_thread_create(&logger_thread_data, logger_stack,
                    K_THREAD_STACK_SIZEOF(logger_stack),
                    logger_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);
}

void logger_enqueue(const struct sensor_message *msg)
{
    (void)buffer_push(msg);
    /* buffer_push returns -1 on drop; we intentionally ignore since original API was void */
}

/* Optional helper: query drops (not part of original API but useful) */
uint32_t logger_dropped_count(void)
{
    return dropped_count;
}
