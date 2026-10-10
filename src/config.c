#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "util.h"

static const char naf_default_config[] =
    "#\n"
    "# NickelArticleFix configuration. Changes take effect on reboot.\n"
    "#\n"
    "# Master switch. 0 = the mod loads and logs its startup line but does nothing.\n"
    "enabled=1\n"
    "\n"
    "# Verbose logging to nickel-article-fix.log. Problems are logged either way.\n"
    "log=0\n"
    "\n"
    "# Script run once per downloaded article, as: /bin/sh <script> <article folder>.\n"
    "# Its output goes to fix-articles.log in this folder.\n"
    "script=" NAF_DEFAULT_SCRIPT "\n";

// Write the defaults through a unique temporary file and rename it into place, so a power cut
// cannot leave a truncated config behind.
static void naf_config_write_default(void) {
    if (mkdir(NAF_CONFIG_DIR, 0755) != 0 && errno != EEXIST) {
        NAF_LOG("warning: could not create %s (%s)", NAF_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    char tmp[256];
    int n = snprintf(tmp, sizeof(tmp), NAF_CONFIG_DIR "/config.tmp.%ld", (long)getpid());
    if (n < 0 || (size_t)n >= sizeof(tmp))
        return;

    int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) {
        NAF_LOG("warning: could not write default config to %s/config (%s)", NAF_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    size_t len = sizeof(naf_default_config) - 1;
    bool ok = write(fd, naf_default_config, len) == (ssize_t)len;
    if (ok && fsync(fd) != 0)
        ok = false;
    if (close(fd) != 0)
        ok = false;
    if (!ok || rename(tmp, NAF_CONFIG_DIR "/config") != 0) {
        NAF_LOG("warning: could not install default config at %s/config (%s)", NAF_CONFIG_DIR_DISP, strerror(errno));
        unlink(tmp);
        return;
    }

    int dir_fd = open(NAF_CONFIG_DIR, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir_fd >= 0) {
        fsync(dir_fd);
        close(dir_fd);
    }
}

static char *naf_trim(char *s) {
    char *e = s + strlen(s);
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        s++;
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
        e--;
    *e = '\0';
    return s;
}

static bool naf_parse_bool(const char *key, const char *val, bool def, bool *problem) {
    if (!strcmp(val, "1") || !strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on"))
        return true;
    if (!strcmp(val, "0") || !strcasecmp(val, "false") || !strcasecmp(val, "no") || !strcasecmp(val, "off"))
        return false;
    *problem = true;
    NAF_LOG("warning: %s/config: invalid value '%s' for '%s'; using %d", NAF_CONFIG_DIR_DISP, val, key, def ? 1 : 0);
    return def;
}

static naf_config_t naf_cfg = { .enabled = true, .log = false, .script = NAF_DEFAULT_SCRIPT, .problem = false };
static pthread_once_t naf_cfg_once = PTHREAD_ONCE_INIT;

static void naf_config_parse(void) {
    FILE *f = fopen(NAF_CONFIG_DIR "/config", "r");
    if (!f && errno == ENOENT) {
        naf_config_write_default();
        f = fopen(NAF_CONFIG_DIR "/config", "r");
    }
    if (!f) {
        NAF_LOG("could not open %s/config (%s); using built-in defaults", NAF_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    char buf[256];
    int lineno = 0;
    while (fgets(buf, sizeof(buf), f)) {
        lineno++;
        char *hash = strchr(buf, '#');
        if (hash)
            *hash = '\0';
        char *line = naf_trim(buf);
        if (!*line)
            continue;

        char *eq = strchr(line, '=');
        if (!eq) {
            naf_cfg.problem = true;
            NAF_LOG("warning: %s/config: line %d: expected key=value, ignoring line", NAF_CONFIG_DIR_DISP, lineno);
            continue;
        }
        *eq = '\0';
        char *key = naf_trim(line);
        char *val = naf_trim(eq + 1);

        if (!strcmp(key, "enabled"))
            naf_cfg.enabled = naf_parse_bool(key, val, true, &naf_cfg.problem);
        else if (!strcmp(key, "log"))
            naf_cfg.log = naf_parse_bool(key, val, false, &naf_cfg.problem);
        else if (!strcmp(key, "script")) {
            if (val[0] != '/' || strlen(val) >= sizeof(naf_cfg.script)) {
                naf_cfg.problem = true;
                NAF_LOG("warning: %s/config: 'script' must be an absolute path shorter than %u bytes; using %s",
                        NAF_CONFIG_DIR_DISP, (unsigned)sizeof(naf_cfg.script), NAF_DEFAULT_SCRIPT);
            } else {
                strcpy(naf_cfg.script, val);
            }
        } else {
            naf_cfg.problem = true;
            NAF_LOG("warning: %s/config: line %d: unknown setting '%s' (ignored)", NAF_CONFIG_DIR_DISP, lineno, key);
        }
    }
    fclose(f);

    if (naf_cfg.problem)
        naf_cfg.log = true;
}

const naf_config_t *naf_config(void) {
    pthread_once(&naf_cfg_once, naf_config_parse);
    return &naf_cfg;
}
