# NickelArticleFix

A [NickelHook](https://github.com/pgaskin/NickelHook) mod for Kobo e-readers that fixes each Instapaper
article as soon as Nickel has finished downloading it:

1. converts GIF, PNG, SVG and WebP images to JPEG, which Kobo's article view displays reliably;
2. detects the article language (en/de/nl) and sets it, so hyphenation uses the right dictionary;
3. adds a stylesheet (ligatures, hyphenation, centred headings readable in dark mode, styled quotes and figures).

No button to press and no NickelMenu entry: every article is fixed in the background right after it
arrives.

Idea and ImageMagick binaries: qkqw's
[NickelMenu script for fixing Pocket images](https://www.mobileread.com/forums/showthread.php?t=347036)
on MobileRead.

## How it works

The plugin hooks one Nickel function through its PLT entry:
`InstapaperDownloadArticleContentsCommand::downloadArticleDone(Volume const&, Device const&)`. Nickel
emits this Qt signal once per article, after `index.html` has been written and closed and the article
has been saved to the database. The hook first calls Nickel's original, so the signal behaves
exactly as stock. Only then does it work out the article folder (`/mnt/onboard/.kobo/articles/<id>`)
and start

```
/bin/sh .adds/nickel-article-fix/fix-articles.sh <article folder>
```

detached, without waiting for it. All file work happens in that separate process, never inside
Nickel.

Safety, following [NickelTypeFix](https://github.com/nicoverbruggen/NickelTypeFix)'s practices:

- No firmware files are patched, on disk or in memory, apart from the one PLT entry NickelHook sets.
  There are no byte patches and no detours.
- The hook and all three lookups are optional. If one is missing on some firmware, the mod switches
  itself off and logs a problem instead of guessing.
- NickelHook's failsafe disables the plugin if Nickel crashes within 3 seconds of loading it.
- The hook keeps no pointers to Nickel objects, catches any internal error, and does nothing until
  initialisation has finished.
- The script accepts only a real folder directly under `/mnt/onboard/.kobo/articles`.

## Compatibility

Developed and tested on a Kobo Clara BW with firmware 4.45.23697. The hooked symbol and the lookups
are present from 4.44.23552 onwards. `nickelbench check-source src` reports the annotations in
`src/nickelarticlefix.cc` compatible with every firmware in its database.

## Install

1. Copy `KoboRoot.tgz` (built as below, or from a release) to `KOBOeReader/.kobo/`.
2. Eject the device. It installs the update and restarts.

Everything is installed to `KOBOeReader/.adds/nickel-article-fix/`, plus the plugin in
`/usr/local/Kobo/imageformats/`.

Articles already on the device are not processed. To fix them once, run this over SSH:

```sh
for d in /mnt/onboard/.kobo/articles/*; do sh /mnt/onboard/.adds/nickel-article-fix/fix-articles.sh "$d"; done
```

## Configuration

`KOBOeReader/.adds/nickel-article-fix/config` is created on first boot. Changes take effect after a reboot.

| Key | Default | Meaning |
|---|---|---|
| `enabled` | `1` | Master switch. `0` = load, log the startup line, do nothing else. |
| `log` | `0` | Verbose logging (thread, `index.html` status, script PID). Problems are logged either way. |
| `script` | `/mnt/onboard/.adds/nickel-article-fix/fix-articles.sh` | Script run per article. |

### Custom CSS

`style.default.css` is replaced on every update. To customise, copy it to `style.css` in the same
folder and edit that. When `style.css` exists it is used instead, and updates and uninstall never
touch it. The stylesheet is added only to articles that have no `<style>` block yet, so a change
applies to articles downloaded afterwards.

## Logs

- `nickel-article-fix.log`: the plugin's log, with one startup line per boot and one line per
  article.
- `fix-articles.log`: the script's log, with one line per article, for example
  `2026-10-10 15:23:49 /mnt/onboard/.kobo/articles/2047964975: lang=de, css=style.default.css, images converted=5 failed=0`.

Both logs are rotated, keeping one `.old` generation each.

## Uninstall

Delete `KOBOeReader/.adds/nickel-article-fix/uninstall` and reboot. This removes the plugin and every
file the mod installed or created. Files you added yourself (such as `style.css`) stay, and so does
the folder holding them.

If Nickel misbehaves, stop syncing by turning Wi-Fi off, connect USB, and set `enabled=0` in the
config.

## Build

Requires Podman or Docker. The build runs in the
[nickelbench](https://github.com/nicoverbruggen/nickelbench) toolchain image.

```sh
git clone --recurse-submodules https://github.com/tent4kel/NickelArticleFix
cd NickelArticleFix
./build.sh        # clean, build, strip, KoboRoot.tgz, nickelbench compatibility check
```

## Layout

```
src/nickelarticlefix.cc   hook, init, uninstall
src/config.c, config.h    key=value config, written with defaults when missing
src/util.c, util.h        logging, firmware version
res/fix-articles.sh       the per-article script
res/style.default.css     default stylesheet
res/magick, lib/, fonts/  ImageMagick 7 for ARM, DejaVu Sans (see THIRD_PARTY.md)
res/doc, res/uninstall    installed next to the config
```

## Credits

- [NickelHook](https://github.com/pgaskin/NickelHook) by Patrick Gaskin: the hooking framework,
  compiled into the plugin.
- [NickelTypeFix](https://github.com/nicoverbruggen/NickelTypeFix) by Nico Verbruggen: the model
  for this mod's safety practices, and the source of `build.sh`.
- [nickelbench](https://github.com/nicoverbruggen/nickelbench) by Nico Verbruggen: the build toolchain
  image and the firmware compatibility check.
- [NickelMenu](https://github.com/pgaskin/NickelMenu) by Patrick Gaskin: the way the script is
  launched.
- qkqw's [MobileRead thread](https://www.mobileread.com/forums/showthread.php?t=347036): the
  original image-fixing idea and the ImageMagick build.

## Licence

MIT (see `LICENSE`), which also carries the notices for the parts of `build.sh` taken from
NickelTypeFix. The bundled third-party binaries and the font keep their own licences; see
`THIRD_PARTY.md`.
