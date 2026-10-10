# Third-party components

The MIT licence in `LICENSE` covers only this project's own files (`src/`, `res/fix-articles.sh`,
`res/style.default.css`, `res/doc`, `res/uninstall`, `Makefile`, `build.sh`).

## NickelHook

Git submodule `NickelHook/`, https://github.com/pgaskin/NickelHook (MIT), pinned at bcd90ba.

## ImageMagick and libraries, DejaVu Sans (`res/magick`, `res/lib/`, `res/fonts/`)

These prebuilt ARM (armhf) binaries and the font were **not built by this project**. They come from
the MobileRead thread that this mod's idea is based on:

> qkqw, "NickelMenu script for fixing Pocket images", MobileRead Forums, Kobo Developer's Corner,
> 2022-05-25. https://www.mobileread.com/forums/showthread.php?t=347036

The thread states no licence for the bundled files, and their exact build configuration is not
known. Each component is distributed by its upstream project under the licence below. Anyone
redistributing them should check these terms and, where a licence asks for it, point to the
upstream source.

| File | Component | Upstream licence |
|---|---|---|
| `magick`, `lib/libMagickCore-7.Q8.so.10`, `lib/libMagickWand-7.Q8.so.10` | ImageMagick 7 (Q8) | ImageMagick License (Apache-2.0 style), https://imagemagick.org/script/license.php |
| `lib/libfreetype.so.6` | FreeType | FreeType License (FTL) or GPL-2.0, at the recipient's choice |
| `lib/libjpeg.so.9` | IJG libjpeg 9 | IJG License |
| `lib/libltdl.so.7` | GNU libltdl (libtool) | LGPL-2.1-or-later; source: https://www.gnu.org/software/libtool/ |
| `lib/libpng16.so.16` | libpng 1.6 | libpng License |
| `lib/libwebp.so.7` | libwebp | BSD-3-Clause |
| `lib/libxml2.so.2` | libxml2 | MIT |
| `lib/libz.so.1` | zlib | zlib License |
| `fonts/DejaVuSans.ttf` | DejaVu Sans | Bitstream Vera Fonts License (DejaVu changes are public domain) |

MD5 checksums of the committed files:

```
    6a9f0fcbfa7ac80a145c3c5c23408a01  magick
    708118888edd9ec61c5e2cc931f8ac33  lib/libfreetype.so.6
    b7740ba993fffe40f07aa20b219e2501  lib/libjpeg.so.9
    c9c811b99d25975a4570cd51a2cd1443  lib/libltdl.so.7
    00eee5e91d71d7a3a19f05dd984b41d3  lib/libMagickCore-7.Q8.so.10
    e02825c27338f7b1ebd96c8e3f9fdd42  lib/libMagickWand-7.Q8.so.10
    b0f36ea137378d88b3f09955f1ec900f  lib/libpng16.so.16
    18654ffd95d6efde7e4943c946318e84  lib/libwebp.so.7
    975e072b84109f7da6acd57753f9c18d  lib/libxml2.so.2
    6a2c2dfcc336233e359a4f47f624e25e  lib/libz.so.1
    be189a7e2711cdf2a7f6275c60cbc7e2  fonts/DejaVuSans.ttf
```
