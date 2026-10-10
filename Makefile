include NickelHook/NickelHook.mk

override LIBRARY  := src/libnickelarticlefix.so
override SOURCES  += src/nickelarticlefix.cc src/config.c src/util.c

override CFLAGS   += -Wall -Wextra -Werror -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden -fvisibility-inlines-hidden

ifeq ($(NAF_CONFIG_DIR),)
override NAF_CONFIG_DIR := /mnt/onboard/.adds/nickel-article-fix
endif
override CPPFLAGS += -DNAF_CONFIG_DIR='"$(NAF_CONFIG_DIR)"' -DNAF_CONFIG_DIR_DISP='"$(patsubst /mnt/onboard/%,KOBOeReader/%,$(NAF_CONFIG_DIR))"'

override KOBOROOT += res/doc:$(NAF_CONFIG_DIR)/doc res/uninstall:$(NAF_CONFIG_DIR)/uninstall res/fix-articles.sh:$(NAF_CONFIG_DIR)/fix-articles.sh res/style.default.css:$(NAF_CONFIG_DIR)/style.default.css res/magick:$(NAF_CONFIG_DIR)/magick res/fonts/DejaVuSans.ttf:$(NAF_CONFIG_DIR)/fonts/DejaVuSans.ttf
override KOBOROOT += res/lib/libMagickCore-7.Q8.so.10:$(NAF_CONFIG_DIR)/lib/libMagickCore-7.Q8.so.10 res/lib/libMagickWand-7.Q8.so.10:$(NAF_CONFIG_DIR)/lib/libMagickWand-7.Q8.so.10 res/lib/libfreetype.so.6:$(NAF_CONFIG_DIR)/lib/libfreetype.so.6 res/lib/libjpeg.so.9:$(NAF_CONFIG_DIR)/lib/libjpeg.so.9 res/lib/libpng16.so.16:$(NAF_CONFIG_DIR)/lib/libpng16.so.16 res/lib/libwebp.so.7:$(NAF_CONFIG_DIR)/lib/libwebp.so.7 res/lib/libxml2.so.2:$(NAF_CONFIG_DIR)/lib/libxml2.so.2 res/lib/libz.so.1:$(NAF_CONFIG_DIR)/lib/libz.so.1

override SKIPCONFIGURE += strip check
strip:
	$(STRIP) --strip-unneeded $(LIBRARY)
check:
	nickelbench check-source src
.PHONY: strip check

include NickelHook/NickelHook.mk
