// NickelArticleFix: report each Instapaper article once Nickel has finished writing it.
//
// Seam (REPORT.md §3, §5 B): InstapaperDownloadArticleContentsCommand::finish() writes the final
// index.html (IOUtil::rawToFile opens, writes and closes it), saves the volume to the database,
// and then emits the Qt signal downloadArticleDone(Volume const&, Device const&) through libnickel's
// PLT. That signal function is hooked here. The hook calls Nickel's original first, so the signal
// and everything connected to it behave exactly as stock, and only then reports the article.
//
// For each article, the configured script (default fix-articles.sh) is started detached, as
// `/bin/sh <script> <article folder>`, with its output appended to NAF_CONFIG_DIR/fix-articles.log.
// The mod does not wait for it and does not touch the article's files itself.
//
// Safety rules followed here:
// - The hook and every lookup are optional. A missing one turns the feature off and is logged.
// - NickelHook installs hooks before it calls naf_init. Until init has finished, the hook is a
//   plain pass-through.
// - The hook runs on Nickel's "sync" worker thread, inside Nickel's call. It copies two strings,
//   writes one log line and starts the script with QProcess::startDetached (the same call
//   NickelMenu's cmd_spawn uses), which returns once the script has been exec'd. Nothing is kept:
//   the Volume and Device references are only valid during the call.
// - Nothing in the firmware is patched, on disk or in memory, beyond the PLT entry NickelHook sets.

#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <QByteArray>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <NickelHook.h>

#include "config.h"
#include "util.h"

// ---- Nickel functions -------------------------------------------------------------------------
// Signatures follow the 4.45.23697 call sites (REPORT.md §3.2, §3.4). A QString return is
// written through a hidden pointer in r0, before `this`/the first argument.

// void InstapaperDownloadArticleContentsCommand::downloadArticleDone(Volume const&, Device const&)
static void (*real_downloadArticleDone)(void *self, const void *volume, const void *device) = nullptr;
// QString Content::getId() const; Content is at offset 0 of Volume (finish() passes the same pointer).
static void (*naf_contentGetId)(QString *out, const void *content) = nullptr;
// static QString VolumeManager::getArticlePath(Device const&, Volume const&)
static void (*naf_getArticlePath)(QString *out, const void *device, const void *volume) = nullptr;
// bool Volume::isInstapaper() const; a sanity check only, the seam is Instapaper-specific.
static bool (*naf_volumeIsInstapaper)(const void *volume) = nullptr;

// Set at the end of naf_init when the feature is enabled and every required seam resolved.
static bool naf_ready = false;

static bool naf_verbose() { return naf_config()->log; }

#define NAF_SCRIPT_LOG NAF_CONFIG_DIR "/fix-articles.log"
#define NAF_SCRIPT_LOG_MAX_BYTES (256 * 1024)

// Start `/bin/sh <script> <dir>` detached, output appended to NAF_SCRIPT_LOG. The paths are passed
// as positional parameters, never spliced into the command string, so no quoting is involved.
static void naf_launch(const QString &dir) {
    const char *script = naf_config()->script;

    // Keep one ".old" generation. A script still writing to the old file keeps doing so.
    struct stat st;
    if (stat(NAF_SCRIPT_LOG, &st) == 0 && st.st_size > NAF_SCRIPT_LOG_MAX_BYTES)
        rename(NAF_SCRIPT_LOG, NAF_SCRIPT_LOG ".old");

    QStringList args;
    args << QStringLiteral("-c")
         << QStringLiteral("exec /bin/sh \"$1\" \"$2\" </dev/null >>\"$0\" 2>&1")
         << QStringLiteral(NAF_SCRIPT_LOG)
         << QString::fromUtf8(script)
         << dir;

    qint64 pid = 0;
    if (QProcess::startDetached(QStringLiteral("/bin/sh"), args, QStringLiteral("/"), &pid)) {
        if (naf_verbose())
            NAF_LOG("  started %s (pid %lld)", script, (long long)pid);
    } else {
        NAF_LOG("problem: could not start /bin/sh for %s", script);
    }
}

// The article ID becomes one path component, and later a script argument. Accept only IDs that
// cannot leave the article directory.
static bool naf_id_is_safe(const QString &id) {
    if (id.isEmpty() || id == QLatin1String(".") || id == QLatin1String(".."))
        return false;
    for (int i = 0; i < id.size(); i++) {
        const ushort c = id.at(i).unicode();
        if (c < 0x20 || c == '/' || c == '\\' || c == 0x7f)
            return false;
    }
    return true;
}

static void naf_report(const void *volume, const void *device) {
    if (naf_volumeIsInstapaper && !naf_volumeIsInstapaper(volume)) {
        NAF_LOG("note: downloadArticleDone fired for a volume that is not an Instapaper article; ignored");
        return;
    }

    QString id;
    naf_contentGetId(&id, volume);
    QString base;
    naf_getArticlePath(&base, device, volume);

    const QByteArray id8 = id.toUtf8();
    const QByteArray base8 = base.toUtf8();

    if (!naf_id_is_safe(id) || !base.startsWith(QLatin1Char('/'))) {
        NAF_LOG("problem: unusable article location (id '%s', base '%s'); not reported",
                id8.constData(), base8.constData());
        return;
    }

    const QString dir = base + QLatin1Char('/') + id;
    const QByteArray dir8 = dir.toUtf8();
    NAF_LOG("article downloaded: %s", dir8.constData());

    if (naf_verbose()) {
        const QByteArray index8 = dir8 + "/index.html";
        struct stat st;
        const bool exists = stat(index8.constData(), &st) == 0;
        const long tid = (long)syscall(SYS_gettid);
        NAF_LOG("  id=%s thread=%ld (%s) index.html=%s size=%lld mtime=%ld",
                id8.constData(), tid, tid == (long)getpid() ? "main" : "worker",
                exists ? "present" : strerror(errno),
                exists ? (long long)st.st_size : -1LL, exists ? (long)st.st_mtime : 0L);
    }

    naf_launch(dir);
}

// ---- hook -------------------------------------------------------------------------------------

extern "C" __attribute__((visibility("default")))
void _naf_downloadArticleDone(void *self, const void *volume, const void *device) {
    // Nickel first: its relay to SyncClient/N3SyncManager and the UI is unchanged.
    if (real_downloadArticleDone)
        real_downloadArticleDone(self, volume, device);

    if (!__atomic_load_n(&naf_ready, __ATOMIC_ACQUIRE) || !volume || !device)
        return;

    try {
        naf_report(volume, device);
    } catch (...) {
        // Qt reports allocation failure by throwing; never let it unwind into Nickel.
        NAF_LOG("problem: an internal error (likely low memory) skipped one article report");
    }
}

// ---- init / uninstall -------------------------------------------------------------------------

static int naf_init() {
    const naf_config_t *cfg = naf_config();

    char fw[64];
    naf_get_firmware_version(fw, sizeof(fw));

    if (!cfg->enabled) {
        NAF_LOG("started on firmware %s; disabled in %s/config, nothing will be reported", fw, NAF_CONFIG_DIR_DISP);
        return 0;
    }

    if (cfg->log)
        NAF_LOG("seams: downloadArticleDone=%p Content::getId=%p getArticlePath=%p isInstapaper=%p",
                (void *)real_downloadArticleDone, (void *)naf_contentGetId,
                (void *)naf_getArticlePath, (void *)naf_volumeIsInstapaper);

    bool ready = true;
    if (!real_downloadArticleDone) {
        NAF_LOG("problem: could not hook InstapaperDownloadArticleContentsCommand::downloadArticleDone on this firmware; feature off");
        ready = false;
    }
    if (!naf_contentGetId || !naf_getArticlePath) {
        NAF_LOG("problem: Content::getId or VolumeManager::getArticlePath is missing on this firmware; feature off");
        ready = false;
    }
    if (!naf_volumeIsInstapaper)
        NAF_LOG("note: Volume::isInstapaper is missing; reporting without that check");
    // Not fatal: the script may be copied later. Each launch failure shows in fix-articles.log.
    if (access(cfg->script, R_OK) != 0)
        NAF_LOG("problem: script %s is not readable (%s); articles will be logged, the script will fail",
                cfg->script, strerror(errno));

    // Publish last: the hook reads this from the sync thread.
    __atomic_store_n(&naf_ready, ready, __ATOMIC_RELEASE);
    NAF_LOG("started on firmware %s; running %s per article: %s", fw, cfg->script, ready ? "active" : "OFF");
    return 0;
}

static bool naf_del(const char *p) {
    return (access(p, F_OK) != 0 && errno == ENOENT) ? true : nh_delete_file(p);
}

static bool naf_uninstall() {
    naf_log_close();
    bool ok = true;
    ok = naf_del(NAF_CONFIG_DIR "/doc") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/config") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/nickel-article-fix.log") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/nickel-article-fix.log.old") && ok;
    ok = naf_del(NAF_SCRIPT_LOG) && ok;
    ok = naf_del(NAF_SCRIPT_LOG ".old") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/fix-articles.sh") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/style.default.css") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/magick") && ok;
    ok = naf_del(NAF_CONFIG_DIR "/fonts/DejaVuSans.ttf") && ok;
    // ImageMagick's libraries, as shipped in KoboRoot.tgz (Makefile KOBOROOT).
    static const char *const libs[] = {
        NAF_CONFIG_DIR "/lib/libMagickCore-7.Q8.so.10",
        NAF_CONFIG_DIR "/lib/libMagickWand-7.Q8.so.10",
        NAF_CONFIG_DIR "/lib/libfreetype.so.6",
        NAF_CONFIG_DIR "/lib/libjpeg.so.9",
        NAF_CONFIG_DIR "/lib/libpng16.so.16",
        NAF_CONFIG_DIR "/lib/libwebp.so.7",
        NAF_CONFIG_DIR "/lib/libxml2.so.2",
        NAF_CONFIG_DIR "/lib/libz.so.1",
    };
    for (const char *lib : libs)
        ok = naf_del(lib) && ok;
    ok = naf_del(NAF_CONFIG_DIR "/uninstall") && ok;
    // rmdir only: anything the user added is theirs to remove. While it is present the folder
    // stays, which is not a failure.
    if (access(NAF_CONFIG_DIR "/lib", F_OK) == 0)
        nh_delete_dir(NAF_CONFIG_DIR "/lib");
    if (access(NAF_CONFIG_DIR "/fonts", F_OK) == 0)
        nh_delete_dir(NAF_CONFIG_DIR "/fonts");
    if (access(NAF_CONFIG_DIR, F_OK) == 0)
        nh_delete_dir(NAF_CONFIG_DIR);
    return ok;
}

// ---- wiring -----------------------------------------------------------------------------------

static struct nh_info NickelArticleFixInfo = {
    .name            = "NickelArticleFix",
    .desc            = "Runs a script on each downloaded Instapaper article.",
    .uninstall_flag  = NAF_CONFIG_DIR "/uninstall-now",
    .uninstall_xflag = NAF_CONFIG_DIR "/uninstall",
    .failsafe_delay  = 3,
};

static struct nh_hook NickelArticleFixHooks[] = {
    { .sym = "_ZN40InstapaperDownloadArticleContentsCommand19downloadArticleDoneERK6VolumeRK6Device",
      .sym_new = "_naf_downloadArticleDone", .lib = "libnickel.so.1.0.0",
      .out = nh_symoutptr(real_downloadArticleDone), .desc = "article written and saved", .optional = true },
    //nb hook libnickel 4.44.23552 * _ZN40InstapaperDownloadArticleContentsCommand19downloadArticleDoneERK6VolumeRK6Device
    {0},
};

static struct nh_dlsym NickelArticleFixDlsym[] = {
    { .name = "_ZNK7Content5getIdEv", .out = nh_symoutptr(naf_contentGetId), .desc = "article ID", .optional = true },
    //nb lookup * 4.44.23552 * _ZNK7Content5getIdEv
    { .name = "_ZN13VolumeManager14getArticlePathERK6DeviceRK6Volume", .out = nh_symoutptr(naf_getArticlePath), .desc = "article base directory", .optional = true },
    //nb lookup * 4.44.23552 * _ZN13VolumeManager14getArticlePathERK6DeviceRK6Volume
    { .name = "_ZNK6Volume12isInstapaperEv", .out = nh_symoutptr(naf_volumeIsInstapaper), .desc = "sanity check", .optional = true },
    //nb lookup * 4.44.23552 * _ZNK6Volume12isInstapaperEv
    {0},
};

NickelHook(
    .init      = &naf_init,
    .info      = &NickelArticleFixInfo,
    .hook      = NickelArticleFixHooks,
    .dlsym     = NickelArticleFixDlsym,
    .uninstall = &naf_uninstall,
)
