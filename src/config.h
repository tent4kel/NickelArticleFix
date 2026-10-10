#ifndef NAF_CONFIG_H
#define NAF_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// Settings from NAF_CONFIG_DIR/config, in `key=value` lines with `#` comments. The file is
// written with the defaults when missing. Keys:
//   enabled  master switch (default 1)
//   log      verbose logging (default 0); problems are logged regardless
//   script   absolute path of the script run once per article (default NAF_DEFAULT_SCRIPT)
#define NAF_DEFAULT_SCRIPT NAF_CONFIG_DIR "/fix-articles.sh"
typedef struct naf_config_t {
    bool enabled;
    bool log;
    char script[200];
    bool problem; // unknown key, malformed line or invalid value seen; forces verbose logging
} naf_config_t;

// Parse the config once (thread-safe) and return it. Never fails: a missing or unreadable file
// yields the defaults, with the problem logged.
const naf_config_t *naf_config(void);

#ifdef __cplusplus
}
#endif
#endif
