#ifndef NAF_UTIL_H
#define NAF_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include <NickelHook.h>

#if !(defined(NAF_CONFIG_DIR) && defined(NAF_CONFIG_DIR_DISP))
#error "NAF_CONFIG_DIR not set (it should be done by the Makefile)"
#endif

// Rotate before a line would push the log past this size, keeping one ".old" generation.
#ifndef NAF_LOG_MAX_BYTES
#define NAF_LOG_MAX_BYTES (64 * 1024)
#endif

// Write one line to syslog (nh_log) and append it to NAF_CONFIG_DIR/nickel-article-fix.log.
// Each call opens, appends and closes the file: the mod logs a few lines per boot and one per
// downloaded article, so nothing is buffered and no handle on /mnt/onboard is kept open.
// Thread-safe; called from Nickel's GUI thread (init) and its "sync" thread (the hook).
void naf_log_line(const char *file, int line, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

// Stop writing to the log file (syslog still receives lines). Uninstall calls this before it
// deletes the log, so a late line cannot recreate it.
void naf_log_close(void);

// Read only the firmware field (the third) of /mnt/onboard/.kobo/version. Field 0 is the serial
// number and is never returned. On failure, `out` is "unavailable".
void naf_get_firmware_version(char *out, size_t out_size);

#define NAF_LOG(fmt, ...) naf_log_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
