# Third-party components

The MIT licence in `LICENSE` covers only this project's own files (`src/`, `res/fix-articles.sh`,
`res/style.default.css`, `res/doc`, `res/uninstall`, `Makefile`, `build.sh`).

## NickelHook (compiled into the plugin)

Git submodule `NickelHook/`, https://github.com/pgaskin/NickelHook, pinned at bcd90ba.
MIT, Copyright (c) 2020-2025 Patrick Gaskin. Its `nh.c` and plugin glue are linked into
`libnickelarticlefix.so`; the licence text is in `NickelHook/LICENSE`.

## NickelTypeFix (`build.sh`)

`build.sh` is adapted from NickelTypeFix's build script, https://github.com/nicoverbruggen/NickelTypeFix
(MIT, Copyright (c) 2020-2025 Patrick Gaskin, Copyright (c) 2026 Nico Verbruggen). Both notices
are carried in `LICENSE` and in the script's header. NickelTypeFix's practices (optional symbols,
config and uninstall conventions, failsafe use) also served as the model for this mod.

## nickelbench (build tool, not redistributed)

https://github.com/nicoverbruggen/nickelbench, MIT, Copyright (c) 2026 Nico Verbruggen.
`build.sh` builds inside its toolchain image (`ghcr.io/nicoverbruggen/nickelbench`).
`nickelbench check-source src` checks the `//nb hook` and `//nb lookup` annotations in
`src/nickelarticlefix.cc` against its firmware symbol database.

## NickelMenu (reference only)

https://github.com/pgaskin/NickelMenu (MIT). The script is launched the way NickelMenu's
`cmd_spawn` action starts commands (`QProcess::startDetached("/bin/sh", ...)`). No NickelMenu code
is included.

## ImageMagick and libraries, DejaVu Sans (`res/magick`, `res/lib/`, `res/fonts/`)

These prebuilt ARM (armhf) binaries and the font were **not built by this project**. They come from
the MobileRead thread that this mod's idea is based on:

> qkqw, "NickelMenu script for fixing Pocket images", MobileRead Forums, Kobo Developer's Corner,
> 2022-05-25. https://www.mobileread.com/forums/showthread.php?t=347036

The thread states no licence for the bundled files, and their build configuration is not
known. Each component is distributed by its upstream project under the licence below. All
licences are permissive; no copyleft code is bundled.

The versions were read from the binaries themselves, from embedded version strings or by
disassembling the function that reports the version. Each source link points to exactly that
version.

| File | Component | Version | Source | Licence |
|---|---|---|---|---|
| `magick`, `lib/libMagickCore-7.Q8.so.10`, `lib/libMagickWand-7.Q8.so.10` | ImageMagick (Q8, no modules) | 7.1.0-41 beta, built from commit `56dba8e42` (2022-07-03) | [ImageMagick/ImageMagick@56dba8e](https://github.com/ImageMagick/ImageMagick/commit/56dba8e426519c4e95405b2018618edafe434daf) (11 commits before the `7.1.0-41` tag) | [ImageMagick License](https://imagemagick.org/script/license.php) (Apache-2.0 style) |
| `lib/libfreetype.so.6` | FreeType | 2.11.1 | [freetype/freetype `VER-2-11-1`](https://github.com/freetype/freetype/tree/VER-2-11-1) (mirror of gitlab.freedesktop.org) | FreeType License (FTL), or GPL-2.0 at the recipient's choice |
| `lib/libjpeg.so.9` | IJG libjpeg | 9e (16 Jan 2022) | https://ijg.org/files/jpegsrc.v9e.tar.gz | IJG License |
| `lib/libpng16.so.16` | libpng | 1.6.37 | [pnggroup/libpng `v1.6.37`](https://github.com/pnggroup/libpng/tree/v1.6.37) | libpng License |
| `lib/libwebp.so.7` | libwebp | 1.2.1 | [webmproject/libwebp `v1.2.1`](https://github.com/webmproject/libwebp/tree/v1.2.1) (mirror of chromium.googlesource.com) | BSD-3-Clause |
| `lib/libxml2.so.2` | libxml2 | 2.9.14 | [GNOME/libxml2 `v2.9.14`](https://github.com/GNOME/libxml2/tree/v2.9.14) (mirror of gitlab.gnome.org) | MIT |
| `lib/libz.so.1` | zlib | 1.2.12 | [madler/zlib `v1.2.12`](https://github.com/madler/zlib/tree/v1.2.12) | zlib License |
| `fonts/DejaVuSans.ttf` | DejaVu Sans | 2.37 | [dejavu-fonts/dejavu-fonts `version_2_37`](https://github.com/dejavu-fonts/dejavu-fonts/tree/version_2_37) | Bitstream Vera Fonts License (DejaVu changes are public domain) |

The original bundle also contained `libltdl.so.7` (GNU libtool, LGPL). Nothing bundled links
against it, because this ImageMagick was built without loadable modules, so it is not shipped.

MD5 checksums of the committed files:

```
    6a9f0fcbfa7ac80a145c3c5c23408a01  magick
    708118888edd9ec61c5e2cc931f8ac33  lib/libfreetype.so.6
    b7740ba993fffe40f07aa20b219e2501  lib/libjpeg.so.9
    00eee5e91d71d7a3a19f05dd984b41d3  lib/libMagickCore-7.Q8.so.10
    e02825c27338f7b1ebd96c8e3f9fdd42  lib/libMagickWand-7.Q8.so.10
    b0f36ea137378d88b3f09955f1ec900f  lib/libpng16.so.16
    18654ffd95d6efde7e4943c946318e84  lib/libwebp.so.7
    975e072b84109f7da6acd57753f9c18d  lib/libxml2.so.2
    6a2c2dfcc336233e359a4f47f624e25e  lib/libz.so.1
    be189a7e2711cdf2a7f6275c60cbc7e2  fonts/DejaVuSans.ttf
```
