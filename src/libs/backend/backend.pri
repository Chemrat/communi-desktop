######################################################################
# Communi
######################################################################

DEFINES += IRC_STATIC
INCDIR = $$PWD/libcommuni/include
DEPENDPATH += $$INCDIR/IrcCore $$INCDIR/IrcModel $$INCDIR/IrcUtil
INCLUDEPATH += $$INCDIR/IrcCore $$INCDIR/IrcModel $$INCDIR/IrcUtil

LIBNAME = backend
include(../lib.pri)