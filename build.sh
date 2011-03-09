
TARGET=chkarchpasswd.dll
OBJS=chkarchpasswd.o 
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
INC=" -I. -I./include -I../../ -I../../libsylph -I../../src "
INC="$INC `pkg-config --cflags glib-2.0 cairo gdk-2.0 atk `"
DEF=" -DHAVE_CONFIG_H -DUNICODE -D_UNICODE"

LIBS=" `pkg-config --libs glib-2.0 gobject-2.0 gtk+-2.0`"

if [ -z "$1" ]; then

#     com="gcc -Wall -c $DEF $INC compose.c"
#     echo $com
#     eval $com
#     if [ $? != 0  ]; then
#         echo "compile error"
#         exit
#     fi
    com="gcc -Wall -c $DEF $INC chkarchpasswd.c"
    echo $com
    eval $com
    if [ $? != 0  ]; then
        echo "compile error"
        exit
    fi
#LIBS=" -lglib-2.0-0  -lintl"

    com="gcc -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv "
    echo $com
    eval $com

fi

if [ ! -z "$1" ]; then
  case "$1" in
      zip)
          LIBS="$LIBS -L./lib -lunzip32 "
          com="gcc -Wall $DEF $INC testunzip32.c -o testunzip32.exe $LIBS"
          ;;
      7z)
          LIBS="$LIBS -L./lib -lunzip32 -lunlha32 ./lib/lib7zip32.a"
          com="gcc -Wall $DEF $INC test7zip32.c -o test7zip32.exe $LIBS"
          ;;
      lha)
          LIBS="$LIBS -L./lib -lunzip32 -lunlha32 ./lib/lib7zip32.a"
          com="gcc -Wall $DEF $INC testlha32.c -o testlha32.exe $LIBS"
          ;;
      pot)
          com="xgettext chkarchpasswd.c -k_ -kN_ -o po/chkarchpasswd.pot"
          ;;
      po)
          com="msgmerge po/ja.po po/chkarchpasswd.pot -o po/ja.po"
          ;;
      mo)
          com="msgfmt po/ja.po -o po/chkarchpasswd.mo"
          ;;
      scan)
          com="gcc -Wall $DEF $INC testgscanner.c -o testgscanner.exe $LIBS"
          ;;
  esac
  echo $com
  eval $com
fi