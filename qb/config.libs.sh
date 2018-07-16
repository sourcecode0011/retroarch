check_switch_c C99 -std=gnu99  "Cannot find C99 compatible compiler."

check_switch_c NOUNUSED -Wno-unused-result
add_define_make NOUNUSED "$HAVE_NOUNUSED"
check_switch_c NOUNUSED_VARIABLE -Wno-unused-variable
add_define_make NOUNUSED_VARIABLE "$HAVE_NOUNUSED_VARIABLE"

# There are still broken 64-bit Linux distros out there. :)
[ -z "$CROSS_COMPILE" ] && [ -d /usr/lib64 ] && add_library_dirs /usr/lib64

[ -z "$CROSS_COMPILE" ] && [ -d /opt/local/lib ] && add_library_dirs /opt/local/lib

DYLIB=-ldl;
CLIB=-lc
PTHREADLIB=-lpthread
SOCKETLIB=-lc
SOCKETHEADER=

if [ "$OS" = 'BSD' ]; then
   DYLIB=-lc;
elif [ "$OS" = 'Haiku' ]; then
   DYLIB=""
   CLIB=-lroot
   PTHREADLIB=-lroot
   SOCKETLIB=-lnetwork
elif [ "$OS" = 'Win32' ]; then
   SOCKETLIB=-lws2_32
   SOCKETHEADER="#include <winsock2.h>"
   DYLIB=
elif [ "$OS" = 'Cygwin' ]; then
   echo "Error: Cygwin is not a supported platform. See https://bot.libretro.com/docs/compilation/windows/"
   exit 1
fi

add_define_make DYLIB_LIB "$DYLIB"

[ "$OS" = 'Darwin' ] && HAVE_X11=no # X11 breaks on recent OSXes even if present.

[ -d /opt/vc/lib ] && add_library_dirs /opt/vc/lib && add_library_dirs /opt/vc/lib/GL
check_lib VIDEOCORE -lbcm_host bcm_host_init "-lvcos -lvchiq_arm"
check_lib SYSTEMD -lsystemd sd_get_machine_names

if [ "$HAVE_VIDEOCORE" = 'yes' ]; then
   [ -d /opt/vc/include ] && add_include_dirs /opt/vc/include
   [ -d /opt/vc/include/interface/vcos/pthreads ] && add_include_dirs /opt/vc/include/interface/vcos/pthreads
   [ -d /opt/vc/include/interface/vmcs_host/linux ] && add_include_dirs /opt/vc/include/interface/vmcs_host/linux
   HAVE_OPENGLES='auto'
   EXTRA_GL_LIBS="-lEGL -lGLESv2 -lbcm_host -lvcos -lvchiq_arm"
fi

if [ "$HAVE_NEON" = "yes" ]; then
   CFLAGS="$CFLAGS -mfpu=neon -marm"
   CXXFLAGS="$CXXFLAGS -mfpu=neon -marm"
   ASFLAGS="$ASFLAGS -mfpu=neon"
fi

if [ "$HAVE_7ZIP" = "yes" ]; then
   add_include_dirs ./decompress/7zip/
fi

if [ "$HAVE_PRESERVE_DYLIB" = "yes" ]; then
   echo "Notice: Disabling dlclose() of shared objects for Valgrind support."
   add_define_make HAVE_PRESERVE_DYLIB "1"
fi

if [ "$HAVE_FLOATHARD" = "yes" ]; then
   CFLAGS="$CFLAGS -mfloat-abi=hard"
   CXXFLAGS="$CXXFLAGS -mfloat-abi=hard"
   ASFLAGS="$ASFLAGS -mfloat-abi=hard"
fi

if [ "$HAVE_FLOATSOFTFP" = "yes" ]; then
   CFLAGS="$CFLAGS -mfloat-abi=softfp"
   CXXFLAGS="$CXXFLAGS -mfloat-abi=softfp"
   ASFLAGS="$ASFLAGS -mfloat-abi=softfp"
fi

if [ "$HAVE_NEON" = "yes" ]; then
   CFLAGS="$CFLAGS -mfpu=neon -marm"
   CXXFLAGS="$CXXFLAGS -mfpu=neon -marm"
   ASFLAGS="$ASFLAGS -mfpu=neon"
fi

if [ "$HAVE_FLOATHARD" = "yes" ]; then
   CFLAGS="$CFLAGS -mfloat-abi=hard"
   CXXFLAGS="$CXXFLAGS -mfloat-abi=hard"
   ASFLAGS="$ASFLAGS -mfloat-abi=hard"
fi

if [ "$HAVE_FLOATSOFTFP" = "yes" ]; then
   CFLAGS="$CFLAGS -mfloat-abi=softfp"
   CXXFLAGS="$CXXFLAGS -mfloat-abi=softfp"
   ASFLAGS="$ASFLAGS -mfloat-abi=softfp"
fi

if [ "$HAVE_SSE" = "yes" ]; then
   CFLAGS="$CFLAGS -msse -msse2"
   CXXFLAGS="$CXXFLAGS -msse -msse2"
fi

if [ "$HAVE_EGL" != "no" -a "$OS" != 'Win32' ]; then
   check_pkgconf EGL egl
   # some systems have EGL libs, but no pkgconfig
   if [ "$HAVE_EGL" = "no" ]; then
      HAVE_EGL=auto && check_lib EGL "-lEGL $EXTRA_GL_LIBS"
      [ "$HAVE_EGL" = "yes" ] && EGL_LIBS=-lEGL
   else
      EGL_LIBS="$EGL_LIBS $EXTRA_GL_LIBS"
   fi
fi

if [ "HAVE_SSA" != "no" ]; then
   check_lib SSA -lass ass_library_init
fi

if [ "$HAVE_EXYNOS" != "no" ]; then
   check_pkgconf EXYNOS libdrm_exynos
   check_pkgconf DRM libdrm
fi

if [ "$HAVE_DISPMANX" != "no" ]; then
   PKG_CONF_USED="$PKG_CONF_USED DISPMANX"
fi

if [ "$LIBRETRO" ]; then
   echo "Notice: Explicit libretro used, disabling dynamic libretro loading ..."
   HAVE_DYNAMIC='no'
else LIBRETRO="-lretro"
fi

[ "$HAVE_DYNAMIC" = 'yes' ] || {
   #check_lib RETRO "$LIBRETRO" retro_init "$DYLIB" "Cannot find libretro, did you forget --with-libretro=\"-lretro\"?"
   check_lib RETRO "$LIBRETRO" "$DYLIB" "Cannot find libretro, did you forget --with-libretro=\"-lretro\"?"
   add_define_make libretro "$LIBRETRO"
}

if [ "$ASSETS_DIR" ]; then
   add_define_make ASSETS_DIR "$ASSETS_DIR"
else
   add_define_make ASSETS_DIR "${PREFIX}/share"
fi

if [ "$BIN_DIR" ]; then
   add_define_make BIN_DIR "$BIN_DIR"
else
   add_define_make BIN_DIR "${PREFIX}/bin"
fi

if [ "$MAN_DIR" ]; then
   add_define_make MAN_DIR "$MAN_DIR"
else
   add_define_make MAN_DIR "${PREFIX}/share/man"
fi

if [ "$OS" = 'DOS' ]; then
   HAVE_SHADERPIPELINE=no
   HAVE_LANGEXTRA=no
fi

if [ "$OS" = 'Win32' ]; then
   HAVE_THREADS=yes
   HAVE_THREAD_STORAGE=yes
   HAVE_DYLIB=yes
else
   check_lib THREADS "$PTHREADLIB" pthread_create

   if [ "$HAVE_THREADS" = 'yes' ]; then
      check_lib THREAD_STORAGE "$PTHREADLIB" pthread_key_create
   else
      HAVE_THREAD_STORAGE=no
   fi

   check_lib DYLIB "$DYLIB" dlopen
fi

check_lib NETWORKING "$SOCKETLIB" socket "" "$SOCKETHEADER"

if [ "$HAVE_NETWORKING" = 'yes' ]; then
   HAVE_GETADDRINFO=auto
   HAVE_SOCKET_LEGACY=no

   # WinXP+ implements getaddrinfo()
   if [ "$OS" = 'Win32' ]; then
      HAVE_GETADDRINFO=yes
   else
      check_lib GETADDRINFO "$SOCKETLIB" getaddrinfo
      if [ "$HAVE_GETADDRINFO" != 'yes' ]; then
         HAVE_SOCKET_LEGACY=yes
		 echo "Notice: RetroArch will use legacy socket support"
      fi
   fi
   HAVE_NETWORK_CMD=yes
   HAVE_NETWORKGAMEPAD=yes

   if [ "$HAVE_MINIUPNPC" != "no" ]; then
      check_lib MINIUPNPC "-lminiupnpc"
   fi
else
   echo "Warning: All networking features have been disabled."
   HAVE_NETWORK_CMD='no'
   HAVE_NETWORKGAMEPAD='no'
   HAVE_CHEEVOS='no'
fi

check_lib STDIN_CMD "$CLIB" fcntl

if [ "$HAVE_NETWORK_CMD" = "yes" ] || [ "$HAVE_STDIN_CMD" = "yes" ]; then
   HAVE_COMMAND='yes'
else
   HAVE_COMMAND='no'
fi

check_lib GETOPT_LONG "$CLIB" getopt_long

if [ "$HAVE_DYLIB" = 'no' ] && [ "$HAVE_DYNAMIC" = 'yes' ]; then
   echo "Error: Dynamic loading of libretro is enabled, but your platform does not appear to have dlopen(), use --disable-dynamic or --with-libretro=\"-lretro\"".
   exit 1
fi

check_pkgconf ALSA alsa
check_lib CACA -lcaca
check_header OSS sys/soundcard.h
check_header OSS_BSD soundcard.h
check_lib OSS_LIB -lossaudio

if [ "$OS" = 'Darwin' ]; then
   check_lib AL "-framework OpenAL" alcOpenDevice
   HAVE_SDL=no
else
   check_lib AL -lopenal alcOpenDevice
fi

check_pkgconf RSOUND rsound 1.1
check_pkgconf ROAR libroar
check_pkgconf JACK jack 0.120.1
check_pkgconf PULSE libpulse

if [ "$OS" = 'Darwin' ]; then
check_lib COREAUDIO "-framework AudioUnit" AudioUnitInitialize

check_lib CORETEXT "-framework CoreText" CTFontCreateWithName

check_lib COCOA "-framework AppKit" NSApplicationMain
check_lib AVFOUNDATION "-framework AVFoundation"
check_lib CORELOCATION "-framework CoreLocation"
check_lib IOHIDMANAGER "-framework IOKit" IOHIDManagerCreate
fi

check_pkgconf SDL sdl 1.2.10
check_pkgconf SDL2 sdl2 2.0.0

if [ "$HAVE_SDL2" = 'yes' ]; then
   if [ "$HAVE_SDL2" = 'yes' ] && [ "$HAVE_SDL" = 'yes' ]; then
      echo "Notice: SDL drivers will be replaced by SDL2 ones."
      HAVE_SDL=no
   elif [ "$HAVE_SDL2" = 'no' ]; then
      echo "Warning: SDL2 not found, skipping."
      HAVE_SDL2=no
   fi
fi

check_pkgconf LIBUSB libusb-1.0 1.0.16

if [ "$OS" = 'Win32' ]; then
   check_lib DINPUT -ldinput8
   check_lib D3D9 -ld3d9
   check_lib DSOUND -ldsound

   if [ "$HAVE_DINPUT" != 'no' ]; then
      HAVE_XINPUT=yes
   fi

   HAVE_XAUDIO=yes
else
   HAVE_D3D9=no
fi

if [ "$HAVE_OPENGL" != 'no' ] && [ "$HAVE_OPENGLES" != 'yes' ]; then
   if [ "$OS" = 'Darwin' ]; then
      check_header OPENGL "OpenGL/gl.h"
      check_lib OPENGL "-framework OpenGL"
   elif [ "$OS" = 'Win32' ]; then
      check_header OPENGL "GL/gl.h"
      check_lib OPENGL -lopengl32
   else
      check_header OPENGL "GL/gl.h"
      check_lib OPENGL -lGL
   fi

   if [ "$HAVE_OPENGL" = 'yes' ]; then
      if [ "$OS" = 'Darwin' ]; then
         check_lib CG "-framework Cg" cgCreateContext
         [ "$HAVE_CG" = 'yes' ] && CG_LIBS='-framework Cg'
      elif [ "$OS" = 'Win32' ]; then
         check_lib_cxx CG -lcg cgCreateContext
         [ "$HAVE_CG" = 'yes' ] && CG_LIBS='-lcg -lcgGL'
      else
         # On some distros, -lCg doesn't link against -lstdc++ it seems ...
         check_lib_cxx CG -lCg cgCreateContext
         [ "$HAVE_CG" = 'yes' ] && CG_LIBS='-lCg -lCgGL'
      fi

      # fix undefined variables
      PKG_CONF_USED="$PKG_CONF_USED CG"

      check_pkgconf OSMESA osmesa
   else
      echo "Notice: Ignoring Cg. Desktop OpenGL is not enabled."
      HAVE_CG='no'
   fi
fi

if [ "$HAVE_ZLIB" != 'no' ]; then
   check_pkgconf ZLIB zlib

   if [ "$HAVE_ZLIB" = 'no' ]; then
      HAVE_ZLIB='auto'
      check_lib ZLIB '-lz'
   fi
fi

if [ "$HAVE_THREADS" != 'no' ]; then
   if [ "$HAVE_FFMPEG" != 'no' ]; then
      check_pkgconf AVCODEC libavcodec 54
      check_pkgconf AVFORMAT libavformat 54
      check_pkgconf AVDEVICE libavdevice
      check_pkgconf SWRESAMPLE libswresample
      check_pkgconf AVRESAMPLE libavresample
      check_pkgconf AVUTIL libavutil 51
      check_pkgconf SWSCALE libswscale 2.1
      check_header AV_CHANNEL_LAYOUT libavutil/channel_layout.h

      HAVE_FFMPEG='yes'
      if [ "$HAVE_AVCODEC" = 'no' ] || [ "$HAVE_SWRESAMPLE" = 'no' ] || [ "$HAVE_AVFORMAT" = 'no' ] || [ "$HAVE_AVUTIL" = 'no' ] || [ "$HAVE_SWSCALE" = 'no' ]; then
         HAVE_FFMPEG='no'
         echo "Notice: FFmpeg built-in support disabled due to missing or unsuitable packages."
      fi
   fi
else
   echo "Notice: Not building with threading support. Will skip FFmpeg."
   HAVE_FFMPEG='no'
fi

if [ "$OS" != 'Win32' ]; then
   check_lib DYNAMIC "$DYLIB" dlopen
fi

if [ "$HAVE_KMS" != "no" ]; then
   check_pkgconf GBM gbm 9.0
   check_pkgconf DRM libdrm
   if [ "$HAVE_GBM" = "yes" ] && [ "$HAVE_DRM" = "yes" ] && [ "$HAVE_EGL" = "yes" ]; then
      HAVE_KMS=yes
   elif [ "$HAVE_KMS" = "yes" ]; then
      echo "Error: Cannot find libgbm, libdrm and EGL libraries required for KMS. Compile without --enable-kms."
      exit 1
   else
      HAVE_KMS=no
   fi
fi

check_pkgconf LIBXML2 libxml-2.0

if [ "$HAVE_EGL" = "yes" ]; then
   if [ "$HAVE_OPENGLES" != "no" ]; then
      if [ "$OPENGLES_LIBS" ] || [ "$OPENGLES_CFLAGS" ]; then
         echo "Notice: Using custom OpenGLES CFLAGS ($OPENGLES_CFLAGS) and LDFLAGS ($OPENGLES_LIBS)."
         add_define_make OPENGLES_LIBS "$OPENGLES_LIBS"
         add_define_make OPENGLES_CFLAGS "$OPENGLES_CFLAGS"
      else
         HAVE_OPENGLES=auto check_pkgconf OPENGLES glesv2
         [ "$HAVE_OPENGLES" = "no" ] && HAVE_OPENGLES=auto check_lib OPENGLES "-lGLESv2 $EXTRA_GL_LIBS" && add_define_make OPENGLES_LIBS "-lGLESv2 $EXTRA_GL_LIBS"
      fi
   fi
   if [ "$HAVE_VG" != "no" ]; then
      check_pkgconf VG vg
      if [ "$HAVE_VG" = "no" ]; then
         HAVE_VG=auto check_lib VG "-lOpenVG $EXTRA_GL_LIBS"
         [ "$HAVE_VG" = "yes" ] && VG_LIBS=-lOpenVG
      fi
   fi
else
   HAVE_VG=no
   HAVE_OPENGLES=no
fi

check_pkgconf V4L2 libv4l2

if [ "$OS" = 'Darwin' ]; then
   check_lib FBO "-framework OpenGL" glFramebufferTexture2D
elif [ "$OS" = 'Win32' ]; then
   HAVE_FBO=yes
else
   if [ "$HAVE_OPENGLES" = "yes" ]; then
      [ $HAVE_FBO != "no" ] && HAVE_FBO=yes
   else
      check_lib FBO -lGL glFramebufferTexture2D
   fi
fi

check_pkgconf FREETYPE freetype2
check_pkgconf X11 x11
check_pkgconf XCB xcb
[ "$HAVE_X11" = "no" ] && HAVE_XEXT=no && HAVE_XF86VM=no && HAVE_XINERAMA=no && HAVE_XSHM=no

check_pkgconf WAYLAND wayland-egl
check_pkgconf WAYLAND_CURSOR wayland-cursor

check_pkgconf XKBCOMMON xkbcommon 0.3.2
check_pkgconf DBUS dbus-1
check_pkgconf XEXT xext
check_pkgconf XF86VM xxf86vm
check_pkgconf XINERAMA xinerama
if [ "$HAVE_X11" = 'yes' ] && [ "$HAVE_XEXT" = 'yes' ] && [ "$HAVE_XF86VM" = 'yes' ]; then
   check_pkgconf XVIDEO xv
else
   echo "Notice: X11, Xext or xf86vm not present. Skipping X11 code paths."
   HAVE_X11='no'
   HAVE_XVIDEO='no'
fi

if [ "$HAVE_UDEV" != "no" ]; then
   check_pkgconf UDEV libudev
   if [ "$HAVE_UDEV" = "no" ]; then
      HAVE_UDEV=auto && check_lib UDEV "-ludev"
      [ "$HAVE_UDEV" = "yes" ] && UDEV_LIBS=-ludev
   fi
fi

check_header XSHM X11/Xlib.h X11/extensions/XShm.h

check_header PARPORT linux/parport.h
check_header PARPORT linux/ppdev.h

if [ "$OS" != 'Win32' ]; then
   check_lib STRL "$CLIB" strlcpy
fi
check_lib STRCASESTR "$CLIB" strcasestr
check_lib MMAP "$CLIB" mmap
check_lib VULKAN -lvulkan vkCreateInstance

if [ "$HAVE_VULKAN" != 'no' ] && [ ! -e deps/glslang/glslang/README.md ]; then
	echo "Warning: glslang submodule not loaded, can't use Vulkan."
	echo "To fix, use:  git submodule init && git submodule update"
	HAVE_VULKAN=no
fi

if [ "$HAVE_VULKAN" != 'no' ] && [ ! -e deps/SPIRV-Cross/README.md ]; then
	echo "Warning: SPIRV-Cross submodule not loaded, can't use Vulkan."
	echo "To fix, use:  git submodule init && git submodule update"
	HAVE_VULKAN=no
fi

check_pkgconf PYTHON python3

if [ "$HAVE_MATERIALUI" != 'no' ] || [ "$HAVE_XMB" != 'no' ] || [ "$HAVE_ZARCH" != 'no' ]; then
	if [ "$HAVE_RGUI" = 'no' ]; then
		HAVE_MATERIALUI=no
		HAVE_XMB=no
    HAVE_ZARCH=no
		echo "Notice: RGUI not available, MaterialUI, XMB and ZARCH will also be disabled."
	elif [ "$HAVE_OPENGL" = 'no' ] && [ "$HAVE_OPENGLES" = 'no' ] && [ "$HAVE_VULKAN" = 'no' ]; then
    if [ "$OS" = 'Win32' ]; then
      HAVE_SHADERPIPELINE=no
      HAVE_VULKAN=no
		  echo "Notice: Hardware rendering context not available."
    else
      if [ "$HAVE_CACA" = 'yes' ]; then
		    echo "Notice: Hardware rendering context not available."
      else
    		HAVE_MATERIALUI=no
	    	HAVE_XMB=no
        HAVE_ZARCH=no
		    echo "Notice: Hardware rendering context not available, XMB, MaterialUI and ZARCH will also be disabled."
      fi
    fi
	fi
fi


check_macro NEON __ARM_NEON__

add_define_make OS "$OS"

if [ "$HAVE_ZLIB" = 'no' ] && [ "HAVE_RPNG" != 'no' ]; then
   HAVE_RPNG=no
   echo "Notice: zlib is not available, RPNG will also be disabled."
fi

if [ "$HAVE_THREADS" = 'no' ] && [ "HAVE_CHEEVOS" != 'no' ]; then
   HAVE_CHEEVOS=no
   echo "Notice: Threads are not available, Cheevos will also be disabled."
fi

if [ "$HAVE_THREADS" = 'no' ] && [ "HAVE_LIBUSB" != 'no' ]; then
   HAVE_LIBUSB=no
   echo "Notice: Threads are not available, libusb will also be disabled."
fi

if [ "$HAVE_V4L2" != 'no' ]; then
   HAVE_VIDEO_PROCESSOR=yes
fi

# Creates config.mk and config.h.
add_define_make GLOBAL_CONFIG_DIR "$GLOBAL_CONFIG_DIR"
VARS=$(eval set | grep ^HAVE_ | sed s/=.*// | sed s/^HAVE_//)
create_config_make config.mk $VARS
create_config_header config.h $VARS
