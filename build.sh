#gcc -shared -o chkarchpasswd.dll ./.libs/chkarchpasswd.o  ../../libsylph/.libs/libsylph-0.a ../../src/.libs/libsylpheed-plugin-0.a `pkg-config --libs gtk+-2.0` `pkg-config --libs glib-2.0` -L/mingw/local/lib -lssleay32 -leay32 -lws2_32 -liconv -lonig   /mingw/lib/libiconv.a 

TARGET=chkarchpasswd.dll
OBJS=chkarchpasswd.o
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
#LIBS=" -lglib-2.0-0  -lintl"
LIBS=" `pkg-config --libs glib-2.0` `pkg-config --libs gobject-2.0` `pkg-config --libs gtk+-2.0`"
INC=" -I. -I../../ -I../../libsylph -I../../src `pkg-config --cflags glib-2.0` `pkg-config --cflags cairo` `pkg-config --cflags gdk-2.0`"
DEF=" -DHAVE_CONFIG_H"
com="gcc -Wall -c $DEF $INC chkarchpasswd.c"
echo $com
eval $com

com="gcc -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv -lonig"
echo $com
eval $com

