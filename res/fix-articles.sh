#!/bin/sh
#
# Fix one downloaded Instapaper article. NickelArticleFix runs this once per article, as
#   /bin/sh fix-articles.sh <article folder>
# after Nickel has written the folder. The folder must be a direct child of $ARTICLES.
#
# 1. Converts GIF, PNG, SVG and WebP images to JPEG, keeping each file name.
# 2. Detects the language (en/de/nl/fr/es/it/pt/tr, the ones Kobo ships hyphenation
#    dictionaries for), sets it on the first <div lang="..."> and as
#    * { -webkit-locale: "xx"; } in its own one-line <style id="naf-locale"> block. The locale rule
#    is what picks the hyphenation dictionary: Kobo's WebKit ignores lang here, and Nickel's
#    reader stylesheet sets -webkit-locale from the database (usually "en"). Our rule wins because
#    Nickel's isn't !important.
# 3. Appends style.css (or style.default.css) to index.html as a <style> block, unless it
#    already has one.
#
# Every step is safe to repeat, so there is no "already processed" marker. To fix every article
# already on the device, run it per folder:
#   for d in /mnt/onboard/.kobo/articles/*; do sh fix-articles.sh "$d"; done
#
# Prints one summary line. Exit status: 0 done, 1 usage, 2 folder rejected.

ARTICLES="/mnt/onboard/.kobo/articles"

# magick, lib/ and fonts/ sit next to this script (KOBOeReader/.adds/nickel-article-fix).
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
MAGICK="${SCRIPTDIR}/magick"
FONT="${SCRIPTDIR}/fonts/DejaVuSans.ttf"   # used for text in SVG images, if present
[ -f "$FONT" ] || FONT=""
LD_LIBRARY_PATH="${SCRIPTDIR}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH

# Function words scored by detect_lang, one language per line, code first. Each word is in one
# list only, so shared words (de, la, en, e, es, das, ...) can't tip the result; ties go to the
# earlier line. Words must be lower case; UTF-8 is fine.
LANGS="en the and of to that with it for are this
de und der die ist nicht ein eine mit sich auch auf dem den zu dass sie wird
nl het een niet van dat zijn ook voor op wordt naar
fr les est et une pas qui dans pour avec sont au à
es el los las y al pero muy su sus hay fue más
it di che non gli della delle dei degli alla sono anche questo per nel più è
pt não uma os em ao na nos dos pelo pela são foi isso também é
tr ve bir bu için ile çok olarak gibi daha olan değil ama kadar sonra göre"

# CSS appended to index.html (inside <style>...</style>). style.css is yours and is never
# shipped, so updates keep it; without it, the shipped style.default.css is used.
STYLE="${SCRIPTDIR}/style.css"
[ -f "$STYLE" ] || STYLE="${SCRIPTDIR}/style.default.css"
[ -f "$STYLE" ] || STYLE=""

# The folder name is built from Nickel's data. Accept only a real directory directly under
# $ARTICLES, so nothing else can ever be touched.
d="${1%/}"
if [ "$#" -ne 1 ]; then
    echo "usage: $0 ARTICLE_FOLDER" >&2
    exit 1
fi
case "$d" in
    "$ARTICLES"/*/*|"$ARTICLES"/.|"$ARTICLES"/..|*'
'*) d="" ;;
    "$ARTICLES"/?*) ;;
    *) d="" ;;
esac
if [ -z "$d" ] || [ ! -d "$d" ]; then
    echo "$(date '+%F %T') rejected (not a folder under $ARTICLES): $1"
    exit 2
fi

# Print the language whose function words occur most often in $1; en on a tie or no match.
# Words are split on spaces, ASCII punctuation and digits, the no-break space and the
# typographic apostrophe (l’homme, dell’arte).
detect_lang() {
    printf "%s\n" "$1" | awk -v langs="$LANGS" '
        BEGIN {
            n = split(langs, rows, "\n")
            for (i = 1; i <= n; i++) {
                k = split(rows[i], w, " ")
                code[i] = w[1]
                for (j = 2; j <= k; j++) lang_of[w[j]] = i
            }
        }
        {
            gsub(/\342\200\231|\302\240/, " ")
            gsub(/[!-@[-`{-~]/, " ")
            for (i = 1; i <= NF; i++) if ($i in lang_of) score[lang_of[$i]]++
        }
        END {
            best = 1
            for (i = 2; i <= n; i++) if (score[i] > score[best]) best = i
            print code[best]
        }'
}

# 1. Images. Nickel stores them flat in the folder, named by hash without an extension, so the
#    format comes from magick identify. [0] = first frame only (animated GIFs).
converted=0
failed=0
for i in "$d"/*; do
    [ -f "$i" ] || continue
    case "$i" in *.html|*.tmp) continue ;; esac
    case "$("$MAGICK" identify -limit time 60 -quiet -format "%m" "$i[0]" 2>/dev/null)" in
        GIF|PNG|SVG|WEBP) ;;
        *) continue ;;
    esac
    # Transparency is flattened onto white, the page colour. $$ keeps the temporary name unique
    # if two runs ever overlap on one folder.
    if "$MAGICK" convert -limit time 60 ${FONT:+-font "$FONT"} "$i[0]" \
            -background white -alpha remove -alpha off "$i.$$.jpg" && mv -f "$i.$$.jpg" "$i"; then
        converted=$((converted + 1))
    else
        rm -f "$i.$$.jpg"
        failed=$((failed + 1))
    fi
done

# 2 + 3. index.html, rewritten once through a temporary file.
f="$d/index.html"
lang="-"
if [ -f "$f" ]; then
    # First 2000 bytes without <style> blocks (ours would be read as English on a re-run), tags
    # stripped, lower case; the raw start if that leaves nothing.
    snippet="$(awk '/<style/ { skip = 1 } !skip { print } /<\/style>/ { skip = 0 }' "$f" |
        head -c2000 | sed 's/<[^>]*>//g' | tr '[:upper:]' '[:lower:]')"
    [ -z "$snippet" ] && snippet="$(head -c500 "$f" | tr '[:upper:]' '[:lower:]')"
    lang="$(detect_lang "$snippet")"

    tmp="$f.$$.tmp"
    # The CSS file is read by awk itself (getline), so backslashes in it are kept as they are.
    if awk -v lang="$lang" -v style="$STYLE" '
            BEGIN { locale = "<style id=\"naf-locale\">* { -webkit-locale: \"" lang "\"; }</style>" }
            !replaced && /<div lang="/ { sub(/lang="[^"]*"/, "lang=\"" lang "\""); replaced = 1 }
            /<style id="naf-locale">/ { if (!has_locale) print locale; has_locale = 1; next }
            tolower($0) ~ /<style/ { has_style = 1 }
            { print }
            END {
                if (!has_style && style != "") {
                    print "<style>"
                    while ((getline line < style) > 0) print line
                    print "</style>"
                }
                if (!has_locale) print locale
            }
        ' "$f" > "$tmp" && mv -f "$tmp" "$f"; then
        :
    else
        rm -f "$tmp"
        echo "$(date '+%F %T') $d: could not rewrite index.html"
        lang="failed"
    fi
fi

css="${STYLE##*/}"
echo "$(date '+%F %T') $d: lang=$lang, css=${css:-none}, images converted=$converted failed=$failed"
