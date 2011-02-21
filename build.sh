
TARGET=chkarchpasswd.dll
OBJS=chkarchpasswd.o
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
INC=" -I. -I../../ -I../../libsylph -I../../src "
INC="$INC `pkg-config --cflags glib-2.0 cairo gdk-2.0 atk `"
DEF=" -DHAVE_CONFIG_H"
com="gcc -Wall -c $DEF $INC chkarchpasswd.c"
echo $com
eval $com

#LIBS=" -lglib-2.0-0  -lintl"
LIBS=" `pkg-config --libs glib-2.0 gobject-2.0 gtk+-2.0` -L./lib -lunzip32"

com="gcc -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv "
echo $com
eval $com

