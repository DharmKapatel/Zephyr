/**
 * @file logger.c
 * @brief Sensor data logger module using Zephyr RTOS, LittleFS, and message queues.
 *
 * This module collects data from multiple sensor threads (HTS, Pressure, IMU),
 * synchronizes them, merges into a single snapshot, and logs the results to
 * flash storage in a circular buffer format. Metadata is persisted to maintain
 * the buffer state across resets.
 *
 * Features:
 * - Per-sensor message queues.
 * - Thread-safe file access using mutex.
 * - Circular buffer implementation with metadata persistence.
 * - Automatic file creation and preallocation.
 *
 * @author Dharm
 * @date 2025
 */

#include "logger.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(logger);

#define MOUNT_POINT "/lfs"              /**< Filesystem mount point */
#define LOG_FILE_PATH "/lfs/sensor_data"/**< Path to main sensor log file */
#define META_FILE_PATH MOUNT_POINT "/logger_meta" /**< Path to metadata file */

/** Per-sensor message queues */
K_MSGQ_DEFINE(hts_msgq, sizeof(struct sensor_message), MAX_QUEUE, 4);
K_MSGQ_DEFINE(press_msgq, sizeof(struct sensor_message), MAX_QUEUE, 4);
K_MSGQ_DEFINE(imu_msgq, sizeof(struct sensor_message), MAX_QUEUE, 4);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_config);

/** Filesystem mount configuration */
static struct fs_mount_t mount = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_config,
    .storage_dev = (void *)FIXED_PARTITION_ID(lfs1_partition),
    .mnt_point = MOUNT_POINT,
};

/** Logger thread control block */
static struct k_thread logger_thread_data;
/** Logger thread stack */
static K_THREAD_STACK_DEFINE(logger_stack, 4096);
/** File mutex for thread-safe logging */
static K_MUTEX_DEFINE(file_mutex);

/** Global buffer holding latest merged snapshot from sensors */
static sensors_shared_buf global_buf;

/** Entry size in the log file */
static const size_t ENTRY_SIZE = sizeof(struct sensor_message);

/**
 * @struct logger_meta
 * @brief Metadata structure for circular buffer management.
 *
 * Persists state of the circular log across system reboots.
 */
struct logger_meta {
    uint32_t head_index;    /**< Index of next write location */
    uint32_t entries_count; /**< Number of valid entries in the log */
} __packed;

/** Global metadata instance */
static struct logger_meta meta;

/**
 * @brief Save metadata to persistent storage.
 *
 * @param m Pointer to metadata structure.
 * @return 0 on success, -1 on failure.
 */
static int meta_save(const struct logger_meta *m);

/**
 * @brief Load metadata from persistent storage.
 *
 * @param m Pointer to metadata structure to populate.
 * @return 0 on success, -1 on failure.
 */
static int meta_load(struct logger_meta *m);

/**
 * @brief Ensure that the sensor data file exists.
 *
 * Creates and pre-allocates the log file for circular buffer storage
 * if it does not already exist.
 */
static void ensure_data_file(void);

/**
 * @brief Write a snapshot entry to the log file (under lock).
 *
 * Updates metadata after successful write.
 *
 * @param out_msg Pointer to sensor message snapshot to write.
 */
static void write_snapshot_to_file_locked(const struct sensor_message *out_msg);

/**
 * @brief Logger thread function.
 *
 * Waits for fresh data from all three sensors (HTS, Pressure, IMU),
 * merges the results into a snapshot, and writes it to persistent storage.
 *
 * @param a Unused.
 * @param b Unused.
 * @param c Unused.
 */
static void logger_thread(void *a, void *b, void *c);

/**
 * @brief Initialize the logger module.
 *
 * - Mounts the filesystem.
 * - Ensures log and metadata files exist.
 * - Starts the logger thread.
 */
void logger_init(void);

/**
 * @brief Enqueue a sensor message into the logger (default HTS queue).
 *
 * Provides compatibility for enqueuing without selecting a specific sensor queue.
 *
 * @param msg Pointer to sensor message to enqueue.
 */
void logger_enqueue(const struct sensor_message *msg);

/* ================= Function Implementations ================= */

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

static void ensure_data_file(void)
{
    struct fs_file_t f;
    fs_file_t_init(&f);

    if (fs_open(&f, LOG_FILE_PATH, FS_O_READ) == 0) {
        fs_close(&f);
        LOG_INF("%s exists", LOG_FILE_PATH);
        return;
    }

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

static void write_snapshot_to_file_locked(const struct sensor_message *out_msg)
{
    struct fs_file_t file;
    fs_file_t_init(&file);

    if (fs_open(&file, LOG_FILE_PATH, FS_O_RDWR) == 0) {
        off_t offset = (off_t)meta.head_index * ENTRY_SIZE;
        fs_seek(&file, offset, FS_SEEK_SET);
        ssize_t w = fs_write(&file, out_msg, ENTRY_SIZE);
        fs_close(&file);

        if (w == (ssize_t)ENTRY_SIZE) {
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
}

static void logger_thread(void *a, void *b, void *c)
{
    struct sensor_message incoming;
    struct sensor_message out_msg;

    memset(&global_buf, 0, sizeof(global_buf));

    while (1) {
        bool have_hts = false;
        bool have_press = false;
        bool have_imu = false;

        int64_t ts_hts = 0, ts_press = 0, ts_imu = 0;

        while (!(have_hts && have_press && have_imu)) {
            if (!have_hts) {
                if (k_msgq_get(&hts_msgq, &incoming, K_MSEC(500)) == 0) {
                    global_buf.hts_data.temperature = incoming.data.hts_data.temperature;
                    global_buf.hts_data.humidity = incoming.data.hts_data.humidity;
                    ts_hts = incoming.timestamp;
                    have_hts = true;
                    continue;
                }
            }

            if (!have_press) {
                if (k_msgq_get(&press_msgq, &incoming, K_MSEC(500)) == 0) {
                    global_buf.lps_data.pressure = incoming.data.lps_data.pressure;
                    ts_press = incoming.timestamp;
                    have_press = true;
                    continue;
                }
            }

            if (!have_imu) {
                if (k_msgq_get(&imu_msgq, &incoming, K_MSEC(500)) == 0) {
                    global_buf.imu_data.accel.x = incoming.data.imu_data.accel.x;
                    global_buf.imu_data.accel.y = incoming.data.imu_data.accel.y;
                    global_buf.imu_data.accel.z = incoming.data.imu_data.accel.z;
                    global_buf.imu_data.gyro.x  = incoming.data.imu_data.gyro.x;
                    global_buf.imu_data.gyro.y  = incoming.data.imu_data.gyro.y;
                    global_buf.imu_data.gyro.z  = incoming.data.imu_data.gyro.z;
                    ts_imu = incoming.timestamp;
                    have_imu = true;
                    continue;
                }
            }

            if (!(have_hts || have_press || have_imu)) {
                k_sleep(K_MSEC(100));
            }
        }

        int64_t max_ts = ts_hts;
        if (ts_press > max_ts) max_ts = ts_press;
        if (ts_imu > max_ts)   max_ts = ts_imu;

        out_msg.timestamp = max_ts ? max_ts : k_uptime_get();
        out_msg.data = global_buf;

        k_mutex_lock(&file_mutex, K_FOREVER);
        write_snapshot_to_file_locked(&out_msg);
        k_mutex_unlock(&file_mutex);
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

    ensure_data_file();

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

    k_thread_create(&logger_thread_data, logger_stack,
                    K_THREAD_STACK_SIZEOF(logger_stack),
                    logger_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);
}

void logger_enqueue(const struct sensor_message *msg)
{
    if (!msg) return;
    if (k_msgq_put(&hts_msgq, msg, K_NO_WAIT) != 0) {
        LOG_WRN("logger_enqueue: hts_msgq full, dropped");
    }
}
