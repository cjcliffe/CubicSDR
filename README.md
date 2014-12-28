CubicSDR
========

Cross-Platform Software-Defined Radio Application

Utilizes: 
--------
  - liquid-dsp (http://liquidsdr.org/ https://github.com/jgaeddert/liquid-dsp)
  - FFTW (http://www.fftw.org/ https://github.com/FFTW/fftw3)
  - RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/ http://github.com/thestk/rtaudio/)
  - Osmocom RTLSDR (http://sdr.osmocom.org/trac/wiki/rtl-sdr)
  - LodePNG (http://lodev.org/lodepng/)
  - BMFont (http://www.angelcode.com/ http://www.angelcode.com/products/bmfont/)
  - Bitstream Vera font (http://en.wikipedia.org/wiki/Bitstream_Vera)
  - OpenGL (https://www.opengl.org/)
  - wxWidgets (https://www.wxwidgets.org/)
  - CMake (http://www.cmake.org/)


Basic Goals and Status:
----------------------
  - Simple UI
  - Devices
    - [x] RTL-SDR (RTL2832U)
    - [ ] HackRF
    - [ ] Whatever else I can get my hands on
  - Minimal configuration
    - [ ] Device
    - [ ] Bandwidth
    - [ ] Default audio device and settings
    - [ ] Color scheme
    - [ ] Load/Save session
  - Neat Visuals
    - [x] Scope
    - [x] Spectrum
    - [x] Waterfall
    - [ ] More 2D visuals
    - [ ] 3D visuals
  - Demodulation:
    - [x] Multiple demodulators per IQ stream
    - [ ] Audio device selection
    - [ ] Modes
      - [x] FM
      - [x] WFM
      - [x] WBFM stereo
      - [ ] AM
      - [ ] LSB
      - [ ] USB
    - Controls
      - [ ] Display Frequency and allow manual adjustments
      - [ ] Allow selection of demodulation type
      - [ ] Display separate zoomed-in view of current waterfall and spectrum, allow adjustments
      - [ ] Display signal level and allow squelch control
      - [ ] Display audio output selection
      - [ ] Volume control
      - [ ] Basic noise reduction / filter controls?
  - Basic Input Controls
    - [x] Drag spectrum to change center frequency
    - [x] Clicking waterfall adds demodulator when none visible
    - [x] Keyboard arrows adjust frequency, shift for faster change
    - [x] Drag center of demod on waterfall to change frequency
    - [x] Drag edge of demod on waterfall to change bandwidth
    - [x] Hover demod on waterfall and press 's' to toggle squelch
    - [x] Hover demod on waterfall and press 'space' to toggle stereo
    - [x] Hover demod on waterfall and press 'd' or 'delete' to delete it
    - [x] Hold shift on waterfall to create a new demodulator
    - [x] Hold alt on waterfall to drag a range for the demodulator
    - [x] Hold alt+shift on waterfall to drag a range for a new demodulator
  - IQ Recording and playback
    - [ ] Recording
    - [ ] Playback
  - Audio
    - [ ] Recording


Advanced Goals:
--------------
  - Basic demodulator filter(s) that can be enabled and tweaked visually
  - Support multiple simultaneous device usage
    * Categorize devices by antenna connections
    * Allow locked frequencies to activate unused devices to continue demodulation on same antenna
  - Implement digital demodulation supported by liquid-dsp: (http://liquidsdr.org/doc/modem.html)
    * PSK, DPSK, ASK, QAM, APSK, BPSK, QPSK, OOK, SQAM, Star Modem(?)
  - Integrate LUA
    * Expose liquid-dsp functionality
    * Scriptable liquid-dsp demodulation
    * Scriptable digital demodulation output handlers
      - Create block output devices on *nix?
      - Create socket outputs?
      - Visual outputs?
    * Take control of additional devices and spawning new demodulators (i.e. trunkers)
    * Script manager / live editor
    * Provide scriptable liquid-dsp modulation for trancievers?


Target Platforms:
----------------
  - OSX
  - Windows
  - Linux


License:
-------
  - GPL
