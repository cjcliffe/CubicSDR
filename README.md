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

Features and Status:
--------------------
  - Simple UI
  - Devices
    - [x] RTL-SDR
    - [ ] rtl_tcp
    - [ ] HackRF
    - [ ] Whatever else I can get my hands on
  - Basic Features
    - [x] Device Selection
    - [x] Bandwidth
    - [x] Color scheme
    - [x] Load/Save session
    - [ ] Audio sample rate
    - [ ] Device PPM
    - [ ] Default preferences
      - [ ] Audio defaults
      - [ ] Device defaults
    - [ ] Bookmarks
  - Neat Visuals
    - [x] Scope
    - [x] Spectrum
    - [x] Waterfall
    - [ ] Audio Spectrum
    - [ ] More 2D visuals
      - [x] Add faint grid for sense of scale
      - [ ] Indicate outer spectrum edges when zoomed
    - [ ] 3D visuals
  - Demodulation:
    - [ ] Basic modular expansion
    - [x] Multiple demodulators per IQ stream
    - [x] Audio device selection
    - [x] Modes
      - [x] FM
      - [x] FM stereo
        - [ ] Experimental, needs improvement
      - [x] AM
      - [x] LSB
      - [x] USB
      - [x] DSB
      - [ ] RAW
    - [ ] Controls
      - [x] Display Frequency and allow manual adjustments
      - [x] Allow selection of demodulation type
      - [x] Display separate zoomed-in view of current waterfall and spectrum, allow adjustments
      - [x] Display signal level and allow squelch control
      - [x] Display audio output selection
      - [x] Volume control
      - [ ] Demodulator input filtering
      - [ ] Audio filtering
  - Basic Input Controls
    - [x] Drag spectrum to change center frequency
    - [x] Hold shift and click on waterfall to create a new demodulator
    - [x] Clicking waterfall adds new demodulator when none visible
    - [x] Drag center of demodulator on waterfall to change frequency
    - [x] Drag edge of demodulator on waterfall to change bandwidth
    - [x] Double-Click to move demodulator to frequency within it's current range
    - [x] Hold alt and drag range on waterfall to set demodulator frequency + bandwidth
    - [x] Hold alt+shift and drag range on waterfall for a new demodulator
    - [x] Hover demodulator on waterfall and press 'space' to toggle stereo
    - [x] Hover demodulator on waterfall and press 'd' or 'delete' to delete it
    - [x] Keyboard arrows adjust frequency, shift for faster change
    - [ ] Undo / Redo action
  - I/Q Recording and playback
    - [ ] Recording
      - [ ] Record I/Q input data
      - [ ] Simultaneously record demod decimated I/Q and audio
    - [ ] Playback
  - Audio
    - [ ] Recording
  - Optimization
    - [x] Eliminate large waterfall texture uploads
    - [ ] Update visuals to OpenGL 3.x
    - [ ] Resolve constant refresh on visuals that don't change often
    - [ ] Resolve driver/platform vertical sync issues
    - [ ] Group and divide IQ data distribution workload instead of 100% distribution per instance

Advanced Goals:
---------------
  - Design a plan for expansion via modules (dylib/dll/lua)
  - Support shell-based stdin/stdout tools for direct output/playback to/from CLI audio processing apps (i.e. DSD on OSX)
  - Update visuals to support OpenGL ES
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
  - "PVR" like mode with waterfall time shifting
  - L/R and surround-sound balance settings for separating and listening to mono streams
  - Add tool for converting decimated I/Q recording to video
    - Select video features such as title/demodulation/scope/waterfall/etc
    - Render to video from GL frames->ffmpeg/mencoder /w demodulated audio
  - Accessability / Control
    - USB/MIDI control surfaces
    - Joystick / gamepad input
    - Vibration / force-feeback
  - Investigate compilation via emscripten using rtl_tcp for input
    - Create self-contained web server+rtl_tcp bundle for embedded devices
    - Use emscripten compiled CubicSDR via embedded web server 

Target Platforms:
----------------
  - [x] OSX
  - [x] Windows
  - [x] Linux


License:
-------
  - GPL
