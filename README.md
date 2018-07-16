[![Build Status](https://travis-ci.org/libretro/RetroArch.svg?branch=master)](https://travis-ci.org/libretro/RetroArch)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/8936/badge.svg)](https://scan.coverity.com/projects/retroarch)

# RetroArch

RetroArch is the reference frontend for the libretro API.
Popular examples of implementations for this API includes videogame system emulators and game engines, but also
more generalized 3D programs.
These programs are instantiated as dynamic libraries. We refer to these as "libretro cores".

## libretro

[libretro](http://libretro.com) is an API that exposes generic audio/video/input callbacks.
A frontend for libretro (such as RetroArch) handles video output, audio output, input and application lifecycle.
A libretro core written in portable C or C++ can run seamlessly on many platforms with very little/no porting effort.

While RetroArch is the reference frontend for libretro, several other projects have used the libretro
interface to include support for emulators and/or game engines. libretro is completely open and free for anyone to use.

[libretro API header](https://github.com/libretro/RetroArch/blob/master/libretro-common/include/libretro.h)

## Binaries

Latest Windows binaries are currently hosted on the [buildbot](http://buildbot.libretro.com/).

## Support

To reach developers, either make an issue here on GitHub, make a thread on the [forum](http://www.libretro.com/forums/),
or visit our IRC channel: #retroarch @ irc.freenode.org.

## Documentation

See our [wiki](https://github.com/libretro/RetroArch/wiki). On Unix, man-pages are provided.
More developer-centric stuff is found [here](https://github.com/libretro/libretro.github.com/wiki/Documentation-devs).

## Related projects

   - Cg/HLSL shaders: [common-shaders](https://github.com/libretro/common-shaders)
   - Helper scripts to build libretro implementations: [libretro-super](https://github.com/libretro/libretro-super)

## Philosophy

RetroArch attempts to be small and lean,
while still having all the useful core features expected from an emulator.
It is designed to be very portable and features a gamepad-centric UI.
It also has a full-featured command-line interface.

In some areas, RetroArch goes beyond and emphasizes on not-so-common technical features such as multi-pass shader support,
real-time rewind (Braid-style), video recording (using FFmpeg), etc.

RetroArch also emphasizes on being easy to integrate into various launcher frontends.

## Platforms

RetroArch has been ported to the following platforms outside PC:

   - PlayStation 3
   - Xbox 360 (Libxenon/XeXDK)
   - Xbox 1
   - Wii, Gamecube (Libogc)
   - Nintendo 3DS
   - Raspberry Pi
   - Android
   - iOS
   - Blackberry

## Dependencies (PC)

There are no true hard dependencies per se.

On Windows, RetroArch can run with only Win32 as dependency.

On Linux, there are no true dependencies. For optimal usage, the
following dependencies come as recommended:

   - GL headers / Vulkan headers
   - X11 headers and libs, or EGL/KMS/GBM

OSX port of RetroArch requires latest versions of XCode to build.

RetroArch can utilize these libraries if enabled:

   - nvidia-cg-toolkit
   - libxml2 (GLSL XML shaders)
   - libfreetype2 (TTF font rendering on screen)

RetroArch needs at least one of these audio driver libraries:

   - ALSA
   - OSS
   - RoarAudio
   - RSound
   - OpenAL
   - JACK
   - SDL
   - PulseAudio
   - XAudio2 (Win32, Xbox 360)
   - DirectSound (Win32, Xbox 1)
   - CoreAudio (OSX, iOS)

To run properly, RetroArch requires a libretro implementation present, however, as it's typically loaded
dynamically, it's not required at build time.

## Dependencies (Console ports, mobile)

Console ports have their own dependencies, but generally do not require
anything other than what the respective SDKs provide.

## Configuring

The default configuration is defined in config.def.h.
It is not recommended to change this unless you know what you're doing.
These can later be tweaked by using a config file.
A sample configuration file is installed to /etc/retroarch.cfg. This is the system-wide config file.

RetroArch will on startup create a config file in $XDG\_CONFIG\_HOME/retroarch/retroarch.cfg if doesn't exist.
Users only need to configure a certain option if the desired value deviates from the value defined in config.def.h.

To configure joypads, use the built-in menu or the `retroarch-joyconfig` command-line tool.

## Compiling and installing
**Linux**

- Prerequisites:
```bash
sudo apt-get install -y make git-core curl g++ pkg-config libglu1-mesa-dev freeglut3-dev mesa-common-dev libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev
```
- Compiling:
```bash
./configure
make
```

**Mac**

- Prerequisites: [XCode](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0CB4QFjAA&url=https%3A%2F%2Fitunes.apple.com%2Fus%2Fapp%2Fxcode%2Fid497799835%3Fmt%3D12&ei=ZmfeVNPtIILVoASBnoCYBw&usg=AFQjCNGrxKmVtXUdvUU3MhqZhP4MHT6Gtg&sig2=RIXKsWQ79YTQBt_lK5fdKA&bvm=bv.85970519,d.cGU), [Cg](https://developer.nvidia.com/cg-toolkit-download).
- You can open the project (**pkg/apple/RetroArch.xcodeproj**) in the Xcode IDE and build (**&#8984;-B**) and run (**&#8984;-R**) it there. Or you can use the command line...
- Debug:
```bash
# Build
xcodebuild -target RetroArch -configuration Debug -project pkg/apple/RetroArch.xcodeproj
# Run
open ./pkg/apple/build/Debug/RetroArch.app/
```
- Release:
```bash
# Build
xcodebuild -target RetroArch -configuration Release -project pkg/apple/RetroArch.xcodeproj
# Run
open ./pkg/apple/build/Release/RetroArch.app/
```

**PC**

Instructions for compiling on PC can be found in the [wiki](https://github.com/Themaister/RetroArch/wiki).

**PlayStation 3**

RetroArch PS3 needs to be compiled in the following order:

1) Compile RetroArch Salamander

    make -f Makefile.ps3.salamander

2) Finally, compile RetroArch packed together with the GUI:

    make -f Makefile.ps3

**PlayStation 3 - Creating a PKG installable file**

You can add `pkg` as a parameter in order to make a PKG file - for example:

    make -f Makefile.ps3 pkg

This creates an NPDRM package. This can be installed on debug PS3s.

To make a non-NPDRM package that can be installed on a jailbroken/CFW PS3 (such as PSGroove or PS3 CFWs and other 3.55 CFW derivatives), do:

    make -f Makefile.ps3 pkg-signed

If you're using Kmeaw 3.55 firmware, the package needs to be signed:

    make -f Makefile.ps3 pkg-signed-cfw

NOTE: A pre-existing libretro library needs to be present in the root directory in order to link RetroArch PS3. This file needs to be called ***`libretro_ps3.a`***.

**Xbox 360 (XeXDK)**

You will need Microsoft Visual Studio 2010 installed (or higher) in order to compile RetroArch 360.

The solution file can be found at the following location:

    pkg/msvc-360/RetroArch-360.sln

NOTE: A pre-existing libretro library needs to be present in the `pkg/msvc-360/RetroArch-360/Release` directory in order to link RetroArch 360. This file needs to be
called ***`libretro_xdk360.lib`***.

**Xbox 360 (Libxenon)**

You will need to have the libxenon libraries and a working Devkit Xenon toolchain installed in order to compile RetroArch 360 Libxenon.

    make -f Makefile.xenon

NOTE: A pre-existing libretro library needs to be present in the root directory in order to link RetroArch 360 Libxenon. This file needs to be called ***`libretro_xenon360.a`***.

**Wii**

You will need to have the libogc libraries and a working Devkit PPC toolchain installed in order to compile RetroArch Wii.

    make -f Makefile.griffin platform=wii

NOTE: A pre-existing libretro library needs to be present in the root directory in order to link RetroArch Wii. This file needs to be called ***`libretro_wii.a`***.
