# Usage: ./update.sh <ogg_src_directory>
#
# Copies the needed files from a directory containing the original
# libogg source that we need for the Mozilla HTML5 media support.
#
# Before executing this script, make sure you've already ran ./configure
# on the libogg source to ensure config_types.h exists.
cp $1/src/crctable.h ./include/crctable.h
cp $1/include/ogg/config_types.h ./include/ogg/config_types.h
cp $1/include/ogg/ogg.h ./include/ogg/ogg.h
cp $1/include/ogg/os_types.h ./include/ogg/os_types.h
cp $1/CHANGES ./CHANGES
cp $1/COPYING ./COPYING
cp $1/README.md ./README.md
cp $1/src/bitwise.c ./src/ogg_bitwise.c
cp $1/src/framing.c ./src/ogg_framing.c
cp $1/AUTHORS ./AUTHORS
patch -p0 < solaris-types.patch
# memory-reporting.patch adds ogg_alloc.c, make sure it doesn't exist to avoid
# unpleasantries.
rm -f ./src/ogg_alloc.c
patch -p0 < memory-reporting.patch