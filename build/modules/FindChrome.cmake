INCLUDE(FindPkgConfig)
SET(CHROME_FOUND TRUE)
SET(CHROMIUMDIR ${CHROME_ROOT})
IF(NOT CHROME_MODE)
  SET(CHROMIUMMODE Release)
ELSE()
  SET(CHROMIUMMODE ${CHROME_MODE})
ENDIF()
STRING(COMPARE EQUAL ${CHROMIUMMODE} "Debug" CHROMIUM_ISDEBUG)
IF(CHROMIUM_ISDEBUG)
SET(CHROMIUM_DEBUGFLAGS -D_DEBUG)
ELSE()
SET(CHROMIUM_DEBUGFLAGS -DNDEBUG)
ENDIF()

IF(APPLE)
IF(NOT CHROMIUMBUILDER)
SET(CHROMIUMBUILDER,xcode)
ENDIF()
ENDIF()

IF(CHROMIUMBUILDER)
  STRING(COMPARE EQUAL ${CHROMIUMBUILDER} "xcode" CHROMIUM_ISXCODE)
ENDIF()
IF(CHROMIUMBUILDER)
  STRING(COMPARE EQUAL ${CHROMIUMBUILDER} "scons" CHROMIUM_ISSCONS)
ENDIF()

IF(CHROMIUM_ISXCODE)
  SET(CHROMIUM_PLAT mac)
  SET(CHROMIUM_PLAT_CFLAGS -pthread)
  SET(SNOW)

  SET(CHROMIUM_PLAT_LDFLAGS -dynamiclib -pthread ${CHROMIUMDIR}/src/third_party/WebKit/WebKitLibraries/libWebKitSystemInterface${SNOW}Leopard.a -framework CoreAudio -framework AudioToolbox -framework Cocoa -framework QuartzCore -framework Security -framework SecurityInterface -framework SystemConfiguration -ObjC -framework Carbon chromium/src/xcodebuild/chrome.build/${CHROMIUMMODE}/chrome_dll.build/Objects-normal/i386/keystone_glue.o -framework OpenGL -framework JavaScriptCore)
  SET(CHROMIUM_START_GROUP)
  SET(CHROMIUM_END_GROUP)
  SET(CHROMIUM_DLLEXT dylib)
ELSE()
  PKG_CHECK_MODULES(CHROMIUM_PLAT gtk+-2.0 glib-2.0 gthread-2.0 gio-unix-2.0)  # will set MONO_FOUND
  SET(CHROMIUM_PLAT linux)
  SET(CHROMIUM_PLAT_LDFLAGS ${CHROMIUM_PLAT_LDFLAGS}  -lssl3 -lnss3 -lnssutil3 -lsmime3 -lplds4 -lplc4 -lnspr4 -lpthread -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -llinux_versioninfo -lpangocairo-1.0 -lgio-2.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lfontconfig -lfreetype -lrt -lgconf-2 -lglib-2.0 -lX11 -lasound -lharfbuzz -lharfbuzz_interface -lsandbox -lnonnacl_util_linux)
  SET(CHROMIUM_START_GROUP -Wl,--start-group)
  SET(CHROMIUM_END_GROUP -Wl,--end-group)
  SET(CHROMIUM_DLLEXT so)
ENDIF()

IF(NOT CHROMIUM_ISSCONS)
  IF(NOT CHROMIUM_ISXCODE)
    SET(CHLIBS $(CHROMIUMLIBPATH))
    IF(NOT CHROMIUM_CHLIBS)
      SET(CHROMIUM_CHLIBS ${CHROMIUMDIR}/src/out/${CHROMIUMMODE}/obj.target)
    ENDIF()

   SET(CHROME_LIBRARY_DIRS ${CHROMIUM_CHLIBS} ${CHROMIUM_CHLIBS}/app ${CHROMIUM_CHLIBS}/base ${CHROMIUM_CHLIBS}/ipc ${CHROMIUM_CHLIBS}/chrome ${CHROMIUM_CHLIBS}/net ${CHROMIUM_CHLIBS}/media ${CHROMIUM_CHLIBS}/webkit ${CHROMIUM_CHLIBS}/sandbox ${CHROMIUM_CHLIBS}/skia ${CHROMIUM_CHLIBS}/printing ${CHROMIUM_CHLIBS}/v8/tools/gyp ${CHROMIUM_CHLIBS}/sdch ${CHROMIUM_CHLIBS}/build/temp_gyp ${CHROMIUM_CHLIBS}/native_client/src/trusted/plugin/ ${CHROMIUM_CHLIBS}/native_client/src/shared/srpc ${CHROMIUM_CHLIBS}/native_client/src/shared/imc ${CHROMIUM_CHLIBS}/native_client/src/shared/platform ${CHROMIUM_CHLIBS}/native_client/src/trusted/nonnacl_util ${CHROMIUM_CHLIBS}/native_client/src/trusted/nonnacl_util/linux ${CHROMIUM_CHLIBS}/native_client/src/trusted/service_runtime/ ${CHROMIUM_CHLIBS}/ ${CHROMIUM_CHLIBS}/native_client/src/trusted/desc/ ${CHROMIUM_CHLIBS}/third_party/bzip2 ${CHROMIUM_CHLIBS}/third_party/ffmpeg ${CHROMIUM_CHLIBS}/third_party/harfbuzz ${CHROMIUM_CHLIBS}/third_party/hunspell ${CHROMIUM_CHLIBS}/third_party/icu ${CHROMIUM_CHLIBS}/third_party/libevent ${CHROMIUM_CHLIBS}/third_party/libjpeg ${CHROMIUM_CHLIBS}/third_party/libpng ${CHROMIUM_CHLIBS}/third_party/libxml ${CHROMIUM_CHLIBS}/third_party/libxslt ${CHROMIUM_CHLIBS}/third_party/lzma_sdk ${CHROMIUM_CHLIBS}/third_party/modp_b64 ${CHROMIUM_CHLIBS}/third_party/sqlite ${CHROMIUM_CHLIBS}/third_party/zlib ${CHROMIUM_CHLIBS}/third_party/WebKit/JavaScriptCore/JavaScriptCore.gyp ${CHROMIUM_CHLIBS}/third_party/WebKit/WebCore/WebCore.gyp)


    SET(CHROMIUM_TPLIBS -levent -lzlib -lpng -lxml -ljpeg -lxslt -lbzip2 -lsqlite -lgoogle_nacl_imc_c)

    SET(CHROMIUM_GENINCLUDES ${CHROMIUM_CHLIBS}/gen/chrome)
  ELSE(NOT CHROMIUM_ISXCODE)

    SET(CHROMIUM_CHLIBS ${CHROMIUMLIBPATH})
    IF (NOT CHROMIUN_CHLIBS)
       SET(CHROMIUM_CHLIBS ${CHROMIUMDIR}/src/xcodebuild/${CHROMIUMMODE})
    ENDIF()

    SET(CHROME_LIBRARY_DIRS ${CHROMIUM_CHLIBS})

    SET(CHROMIUM_TPLIBS ${CHLIBS}/libevent.a ${CHLIBS}/libxslt.a ${CHLIBS}/libjpeg.a ${CHLIBS}/libpng.a ${CHLIBS}/libz.a ${CHLIBS}/libxml2.a ${CHLIBS}/libbz2.a ${CHLIBS}/libsqlite3.a ${CHLIBS}/libprofile_import.a -llibgoogle_nacl_imc_c)

    SET(GENINCLUDES ${CHROMIUMDIR}/src/xcodebuild/DerivedSources/${CHROMIUMMODE}/chrome)
    
  ENDIF(NOT CHROMIUM_ISXCODE)
ELSE(NOT CHROMIUM_ISSCONS)

  SET(GENINCLUDES ${CHROMIUMDIR}/src/out/${CHROMIUMMODE}) #not sure

  IF(NOT CHROMIUMLIBPATH)
    SET(CHROMIUMLIBPATH ${CHROMIUMDIR}/src/out/${CHROMIUMMODE}/lib)
  ENDIF()

  SET(CHROME_LIBRARY_DIRS ${CHROMIUMLIBPATH})
  SET(CHROMIUM_TPLIBS -levent -lxslt -ljpeg -lpng -lz -lxml2 -lbz2 -lsqlite -lgoogle_nacl_imc_c)
ENDIF(NOT CHROMIUM_ISSCONS)

SET(CHROMIUMLIBS ${CHROMIUMLDFLAGS} ${CHROMIUM_TPLIBS}  -ldl -lm -lcommon -lbrowser -ldebugger -lrenderer -lutility -lprinting -lapp_base -lbase -licui18n -licuuc -licudata -lskia -lnet -lgoogleurl -lsdch -lmodp_b64 -lv8_snapshot -lv8_base -lglue -lpcre -lwtf -lwebkit -lwebcore -lmedia -lffmpeg -lhunspell -lplugin -l appcache -lipc -lworker -ldatabase -lcommon_constants -lnpGoogleNaClPluginChrome -lnonnacl_srpc -lplatform -lsel -lsel_ldr_launcher -lnonnacl_util_chrome -lnrd_xfer -lgio -lexpiration -lnacl -lbase_i18n)
SET(CHROMIUM_ARCHFLAGS)
# Flags that affect both compiling and linking
SET(CHROMIUM_CLIBFLAGS ${CHROMIUM_ARCHFLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC -pthread -Wall -fno-rtti)
SET(CHROME_INCLUDE_DIRS include ${GENINCLUDES} ${CHROMIUMDIR}/src/ ${CHROMIUMDIR}/src/third_party/npapi ${CHROMIUMDIR}/src/third_party/WebKit/JavaScriptCore ${CHROMIUMDIR}/src/third_party/icu/public/common ${CHROMIUMDIR}/src/skia/config ${CHROMIUMDIR}/src/third_party/skia/include/core ${CHROMIUMDIR}/src/webkit/api/public ${CHROMIUMDIR}/src/third_party/WebKit/WebCore/platform/text)
SET(CHROME_CFLAGS ${CHROMIUM_DEBUGFLAGS} ${CHROMIUM_CLIBFLAGS} ${CHROMIUM_PLAT_CFLAGS} -Wall -DNVALGRIND -D_REENTRANT -D__STDC_FORMAT_MACROS -DCHROMIUM_BUILD -DU_STATIC_IMPLEMENTATION -g )

SET(CHROME_LDFLAGS -shared ${CHROMIUM_CLIBFLAGS} -g ${CHROMIUM_START_GROUP} ${CHROMIUM_PLAT_LDFLAGS} ${CHROMIUMLIBS} ${CHROMIUM_END_GROUP})

#OBJDIR=$(CHROMIUMMODE)
#EXEDIR=$(CHROMIUMMODE)

FOREACH(CHROMIUMLIB ${CHROMIUM_LIBRARY_DIRS})
  IF(NOT EXISTS ${CHROMIUMLIB})
    IF(CHROME_FOUND)
      MESSAGE(STATUS "Failed to find Chrome library directory at ${CHROMIUMLIB}")
    ENDIF()
    SET(CHROME_FOUND FALSE)
  ENDIF()
ENDFOREACH()
FOREACH(CHROMIUMINC ${CHROMIUM_INCLUDE_DIRS})
  IF(NOT EXISTS ${CHROMIUMINC})
    IF(CHROME_FOUND)
      MESSAGE(STATUS "Failed to find Chrome include directory at ${CHROMIUMINC}")
    ENDIF()
    SET(CHROME_FOUND FALSE)
  ENDIF()
ENDFOREACH()

IF(CHROME_FOUND)
  MESSAGE(STATUS "Found Chrome: headers at ${CHROME_INCLUDE_DIRS}, libraries at ${CHROME_LIBRARY_DIRS}")
  MESSAGE(STATUS "Found Chrome: LDFLAGS at ${CHROME_LDFLAGS}")
  MESSAGE(STATUS "Found Chrome: CFLAGS at ${CHROME_CFLAGS}")
ENDIF(CHROME_FOUND)
