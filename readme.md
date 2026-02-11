# SRB2 Retro

SRB2 Retro is an updated fork of [Sonic Robo Blast 2](https://srb2.org) 2.0.7.  
The goal of SRB2 Retro is to include essential fixes and QOL improvements seen in 2.1 and 2.2.

## Dependencies
- SDL1.2/SDL2
- SDL-Mixer/SDL2-Mixer
- zlib
- libpng

## Compiling

SRB2 Retro has 2 method of compiling:

### CMake

Supported Systems:
- Windows (TESTING REQUIRED)
- macOS (TESTING REQUIRED)
- Linux

See [Wiki/Source code compiling](http://wiki.srb2.org/wiki/Source_code_compiling)

Binaries will be in /build/src/sdl2/

NOTE: For systems using GCC15+, add " -DCMAKE_C_CFLAGS="-std=gnu17" " to the end of your build command

### Makefiles

Supported Systems:
- Windows
- Linux

See [Wiki/Source code compiling](http://wiki.srb2.org/wiki/Source_code_compiling)

Binaries will be in /bin/SYSTEM-NAME

NOTE: For systems using GCC15+, run " export CFLAGS="-std=gnu17" " nefore your build command

## Disclaimer
Sonic Team Junior is in no way affiliated with SEGA or Sonic Team. We do not claim ownership of any of SEGA's intellectual property used in SRB2.
