# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstheliostvsrc2.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstheliostvsrc2_la_SOURCES = gstheliostvsrc2.cpp gstheliostvsrc2.h TcpClient.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstheliostvsrc2_la_CPPFLAGS = $(GST_CFLAGS) $(BOOST_CPPFLAGS)
libgstheliostvsrc2_la_LIBADD = $(GST_LIBS) $(BOOST_SYSTEM_LIB) -lheliostv
libgstheliostvsrc2_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(BOOST_LDFLAGS)
libgstheliostvsrc2_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstheliostvsrc2.h
