#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "util.h"

#define NAF_LOG_PATH NAF_CONFIG_DIR "/nickel-article-fix.log"

static pthread_mutex_t naf_log_lock = PTHREAD_MUTEX_INITIALIZER;
static bool naf_log_closed = false;

// Caller holds naf_log_lock.
static void naf_log_append(const char *buf, size_t len) {
    if (naf_log_closed || !len || len > NAF_LOG_MAX_BYTES)
        return;

    // The directory normally ships in KoboRoot.tgz; recreate it only if a user removed it.
    if (mkdir(NAF_CONFIG_DIR, 0755) != 0 && errno != EEXIST)
        return;

    struct stat st;
    if (stat(NAF_LOG_PATH, &st) == 0) {
        if ((size_t)st.st_size + len > NAF_LOG_MAX_BYTES) {
            if (rename(NAF_LOG_PATH, NAF_LOG_PATH ".old") != 0) {
                nh_log("log rotation failed: %s", strerror(errno));
                return; // syslog still has the line
            }
        }
    } else if (errno != ENOENT) {
        return;
    }

    FILE *f = fopen(NAF_LOG_PATH, "a");
    if (!f)
        return;
    fwrite(buf, 1, len, f);
    fclose(f);
}

void naf_log_line(const char *file, int line, const char *fmt, ...) {
    char msg[768];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    msg[sizeof(msg) - 1] = '\0';

    nh_log("%s (%s:%d)", msg, file, line);

    // localtime_r: the hook logs from Nickel's sync thread while init may log from the GUI thread.
    time_t now = time(NULL);
    struct tm tmbuf;
    struct tm *tm = localtime_r(&now, &tmbuf);

    char out[1024];
    int n = 0;
    if (tm) {
        n = snprintf(out, sizeof(out), "%04d-%02d-%02d %02d:%02d:%02d ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        if (n < 0 || (size_t)n >= sizeof(out))
            n = 0;
    }
    int m = snprintf(out + n, sizeof(out) - (size_t)n, "NickelArticleFix " NH_VERSION ": %s (%s:%d)\n", msg, file, line);
    if (m < 0)
        return;
    size_t len = (size_t)n + (size_t)m;
    if (len >= sizeof(out)) {
        len = sizeof(out) - 1;
        out[len - 1] = '\n';
    }

    pthread_mutex_lock(&naf_log_lock);
    naf_log_append(out, len);
    pthread_mutex_unlock(&naf_log_lock);
}

void naf_log_close(void) {
    pthread_mutex_lock(&naf_log_lock);
    naf_log_closed = true;
    pthread_mutex_unlock(&naf_log_lock);
}

void naf_get_firmware_version(char *out, size_t out_size) {
    if (!out || !out_size)
        return;
    snprintf(out, out_size, "unavailable");
    FILE *f = fopen("/mnt/onboard/.kobo/version", "r");
    if (!f)
        return;
    char vline[512];
    char *got = fgets(vline, sizeof(vline), f);
    fclose(f);
    if (!got)
        return;
    vline[strcspn(vline, "\r\n")] = '\0';
    char *field = vline;
    for (int i = 0; i < 2; i++) {
        field = strchr(field, ',');
        if (!field)
            return;
        field++;
    }
    char *end = strchr(field, ',');
    if (end)
        *end = '\0';
    if (*field)
        snprintf(out, out_size, "%s", field);
}
