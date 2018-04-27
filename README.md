CubicSDR
========

Cross-Platform Software-Defined Radio Application

- Please see the [CubicSDR GitHub Wiki](https://github.com/cjcliffe/CubicSDR/wiki) for build instructions.
- Manual is available (work-in-progress) at [cubicsdr.readthedocs.io](http://cubicsdr.readthedocs.io).
- See also the current [CubicSDR Releases](https://github.com/cjcliffe/CubicSDR/releases) page for available binaries.

Utilizes: 
--------
  - liquid-dsp (http://liquidsdr.org/ -- https://github.com/jgaeddert/liquid-dsp)
  - SoapySDR (http://www.pothosware.com/ -- https://github.com/pothosware/SoapySDR)
  - RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/ -- http://github.com/thestk/rtaudio/)
  - LodePNG (http://lodev.org/lodepng/)
  - BMFont (http://www.angelcode.com/ -- http://www.angelcode.com/products/bmfont/)
  - Bitstream Vera font (http://en.wikipedia.org/wiki/Bitstream_Vera)
  - OpenGL (https://www.opengl.org/)
  - wxWidgets (https://www.wxwidgets.org/)
  - CMake (http://www.cmake.org/)

Optional Libs:
--------
  - FFTW3 (can be compiled into liquid-dsp if desired) (http://www.fftw.org/ -- https://github.com/FFTW/fftw3)
  - hamlib (https://github.com/Hamlib/Hamlib)

Features and Status:
--------------------
  - Please see the issues on GitHub or visit https://github.com/cjcliffe/CubicSDR/wiki/CubicSDR-Roadmap-and-Ideas for more information.
  - A manual is in development at https://github.com/cjcliffe/CubicSDR/issues/248 if you would like to contribute.

Recommended minimum requirements:
--------------------
  - Multi-core processor system with at least 1GB RAM.
  - Graphics card with at least 128MB video memory and OpenGL 3.x or ES 2.0 support.
  - OSX 10.9+ for Mac binary releases.
  - Windows 7+ for 64 or 32-bit Windows binary releases.
  - Linux and other embedded distribution support yet to be indexed, known to at least work on Debian 8+ and Ubuntu 14+.
  - Raspberry Pi2 support is being experimented with; earlier versions of CubicSDR known to work with Banana Pi.

Target Platforms:
----------------
  - [x] OSX
  - [x] Windows
  - [x] Linux
  - [ ] HTML5


License:
-------
  - GPL-2.0+
