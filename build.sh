
NAME=chkarchpasswd
TARGET=$NAME.dll
OBJS=$NAME.o 
LIBSYLPH=./lib/libsylph-0-1.a
LIBSYLPHEED=./lib/libsylpheed-plugin-0-1.a
INC=" -I. -I./include -I../../ -I../../libsylph -I../../src "
INC="$INC `pkg-config --cflags glib-2.0 cairo gdk-2.0 atk `"
DEF=" $DEF -DDEBUG -DHAVE_CONFIG_H -DUNICODE -D_UNICODE"

LIBS=" `pkg-config --libs glib-2.0 gobject-2.0 gtk+-2.0`"

function compile ()
{
    com="gcc -Wall -c $DEF $INC $NAME.c"
    echo $com
    eval $com
    if [ $? != 0 ]; then
        echo "compile error"
        exit
    fi
    com="gcc -shared -o $TARGET $OBJS -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS -lssleay32 -leay32 -lws2_32 -liconv -lonig"
    echo $com
    eval $com
    if [ $? != 0 ]; then
        echo "done"
    else
        DEST="/C/Users/$LOGNAME/AppData/Roaming/Sylpheed/plugins"
        if [ -d "$DEST" ]; then
            com="cp $TARGET $DEST/$NAME.dll"
            echo $com
            eval $com
        else
            DEST="/C/Documents and Settings/$LOGNAME/Application Data/Sylpheed/plugins"
            if [ -d "$DEST" ]; then
                com="cp $TARGET \"$DEST/$NAME.dll\""
                echo $com
                eval $com
            fi
        fi
    fi

}

if [ -z "$1" ]; then
    compile
else
    while [  $# -ne 0 ]; do
        case "$1" in
            -debug|--debug)
                DEF=" -DDEBUG -DHAVE_CONFIG_H"
                shift
                ;;
            pot)
                mkdir -p po
                com="xgettext $NAME.c -k_ -kN_ -o po/$NAME.pot"
                echo $com
                eval $com
                shift
                ;;
            po)
                com="msgmerge po/ja.po po/$NAME.pot -o po/ja.po"
                echo $com
                eval $com
                shift
                ;;
            mo)
                com="msgfmt po/ja.po -o po/$NAME.mo"
                echo $com
                eval $com
                DEST="/C/apps/Sylpheed/lib/locale/ja/LC_MESSAGES"
                if [ -d "$DEST" ]; then
                    com="cp po/$NAME.mo $DEST/$NAME.mo"
                    echo $com
                    eval $com
                fi
                exit
                ;;
            ui)
                com="gcc -o testui.exe testui.c $INC -L./lib $LIBSYLPH $LIBSYLPHEED $LIBS"
                echo $com
                eval $com
                shift
                ;;
            -r|release)
                shift
                if [ ! -z "$1" ]; then
                    shift
                    r=$1
                    zip sylpheed-$NAME-$r.zip $NAME.dll
                    zip -r sylpheed-$NAME-$r.zip README.ja.txt
                    zip -r sylpheed-$NAME-$r.zip $NAME.c
                    zip -r sylpheed-$NAME-$r.zip po/$NAME.mo
                    zip -r sylpheed-$NAME-$r.zip *.xpm
                fi
                ;;
            -c|-compile)
                shift
                if [ ! -z "$1" ]; then
                    if [ "$1" = "stable" ]; then
                        DEF="$DEF -DSTABLE_RELEASE";
                        shift
                    fi
                fi
                compile
                ;;
            zip)
                shift
                LIBS="$LIBS -L./lib -lunzip32 "
                com="gcc -Wall $DEF $INC testunzip32.c -o testunzip32.exe $LIBS"
                ;;
            7z)
                shift
                LIBS="$LIBS -L./lib -lunzip32 -lunlha32 ./lib/lib7zip32.a"
                com="gcc -Wall $DEF $INC test7zip32.c -o test7zip32.exe $LIBS"
                ;;
            lha)
                shift
                LIBS="$LIBS -L./lib -lunzip32 -lunlha32 ./lib/lib7zip32.a"
                com="gcc -Wall $DEF $INC testlha32.c -o testlha32.exe $LIBS"
                ;;
            scan)
                shift
                com="gcc -Wall $DEF $INC testgscanner.c -o testgscanner.exe $LIBS"
                ;;
            def)
                shift
                PKG=libsylph-0-1
                com="(cd lib;pexports $PKG.dll > $PKG.dll.def)"
                echo $com
                eval $com
                com="(cd lib;dlltool --dllname $PKG.dll --input-def $PKG.dll.def --output-lib $PKG.a)"
                echo $com
                eval $com
                com="(cd lib;pexports $PKG.dll > $PKG.dll.def)"
                echo $com
                eval $com
                PKG=libsylpheed-plugin-0-1
                com="(cd lib;dlltool --dllname $PKG.dll --input-def $PKG.dll.def --output-lib $PKG.a)"
                echo $com
                eval $com
                exit
                ;;
            release)
                shift
                if [ ! -z "$1" ]; then
                    VER=$1
                    shift
                    zip sylpheed-$NAME-$VER.zip chkarchpasswd.dll
                    zip -r sylpheed-$NAME-$VER.zip README.ja.txt
                    zip -r sylpheed-$NAME-$VER.zip ChangeLog
          #zip -r sylpheed-$NAME-$VER.zip $NAME.c
                    zip -r sylpheed-$NAME-$VER.zip po/$NAME.mo
                    zip -r sylpheed-$NAME-$VER.zip *.xpm
                    sha1sum sylpheed-$NAME-$VER.zip > sylpheed-$NAME-$VER.zip.sha1sum
                fi
                
                ;;
            *)
                compile
                ;;
        esac
    done
fi
