libdir = $(plugindir)

lib_LTLIBRARIES = chkarchpasswd.la

chkarchpasswd_la_SOURCES = chkarchpasswd.c
chkarchpasswd_la_LDFLAGS = -module -no-undefined -avoid-version

INCLUDES = \
	$(GTK_CFLAGS) \
	-I$(includedir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libsylph \
	-I$(top_srcdir)/src \
        -I$(top_srcdir)/$(subdir)/include

chkarchpasswd_la_LIBADD = \
	$(GTK_LIBS) \
	$(top_builddir)/src/libsylpheed-plugin-0.la \
	$(top_builddir)/libsylph/libsylph-0.la

install:
install-plugin: install-am
