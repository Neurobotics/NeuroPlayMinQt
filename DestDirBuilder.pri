CONFIG *= c++14

IS_DEBUG = false

PLATFORM = "x86"
win32-msvc*:contains(QMAKE_TARGET.arch, x86_64): {
    CONFIG += IS_X64
    DEFINES += IS_X64
    PLATFORM = "x64"
}

win32-g++: {
     #FOR MINGW

    contains(QT_ARCH, i386): {
    DEFINES += __MINGW32__
    } else {
    DEFINES += __MINGW64__
    CONFIG += IS_X64
    DEFINES += IS_X64
    PLATFORM = "x64"
    }
}

unix { PLATFORM = $$QMAKE_HOST.arch }

PROJECTNAME = $${TARGET}
CONFIG += skip_target_version_ext

#Adding a "d" for debug
CONFIG(debug, debug|release) {
TARGET = $$join(TARGET,,,d)
}

#Define DEBUG constant
CONFIG(debug,debug|release)
{
    IS_DEBUG = true
}

COMPILER = ''

win32: {
    win32-msvc* {
        MSVC_VER = $$(VisualStudioVersion)
        equals(MSVC_VER, 10.0){ COMPILER = 'msvc2008' }
        equals(MSVC_VER, 11.0){ COMPILER = 'msvc2010' }
        equals(MSVC_VER, 12.0){ COMPILER = 'msvc2012' }
        equals(MSVC_VER, 13.0){ COMPILER = 'msvc2013' }
        equals(MSVC_VER, 14.0){ COMPILER = 'msvc2015' }
        equals(MSVC_VER, 15.0){ COMPILER = 'msvc2017' }
        equals(MSVC_VER, 16.0){ COMPILER = 'msvc2019' }
    }
    else {
        COMPILER = 'mingw'
    }
}
else { COMPILER = 'clang' }

win32-g++: { #FOR MINGW
    DEFINES += __MINGW32__
    DESTDIR = "$$PWD/../build"
}

DESTDIR = "$$PWD/../build-qt$$QT_VERSION-$${COMPILER}-$${PLATFORM}"

DIRS = $${DESTDIR} \
       $${DESTDIR}/debug \
       $${DESTDIR}/release


CONFIG(debug, debug|release): DESTDIR = $$join(DESTDIR,,,/debug)
else:                         DESTDIR = $$join(DESTDIR,,,/release)

ROOTDESTDIR = $${DESTDIR}

DEPENDPATH += $${DESTDIR}

#Loop over all given directories and append
#the build directory to make absolute paths
for(DIR, DIRS) {
     #add these absolute paths to a variable which
     #ends up as 'mkcommands = path1 path2 path3 ...'
#     message(CREATING DIR $${DIR})
     mkcommands += $$OUT_PWD/$$DIR
}


#make a custom createDir variable with
#the command mkdir path1 path2 path3 ...
createDirs.commands = $(MKDIR) $$mkcommands
#Do Svenn-Arne explained magic!
first.depends += createDirs
QMAKE_EXTRA_TARGETS += createDirs

QMAKE_TARGET_COMPANY = "Neurobotics"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2004-2021 Neurobotics, Ltd <info@neurobotics.ru>"
