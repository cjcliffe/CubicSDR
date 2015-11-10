CubicSDR
========

Cross-Platform Software-Defined Radio Application

Please see the [CubicSDR GitHub Wiki](https://github.com/cjcliffe/CubicSDR/wiki) for build instructions.

Utilizes: 
--------
  - liquid-dsp (http://liquidsdr.org/ -- https://github.com/jgaeddert/liquid-dsp)
  - SoapySDR (http://www.pothosware.com/ -- https://github.com/pothosware/SoapySDR)
  - FFTW (http://www.fftw.org/ -- https://github.com/FFTW/fftw3)
  - RtAudio (http://www.music.mcgill.ca/~gary/rtaudio/ -- http://github.com/thestk/rtaudio/)
  - LodePNG (http://lodev.org/lodepng/)
  - BMFont (http://www.angelcode.com/ -- http://www.angelcode.com/products/bmfont/)
  - Bitstream Vera font (http://en.wikipedia.org/wiki/Bitstream_Vera)
  - OpenGL (https://www.opengl.org/)
  - wxWidgets (https://www.wxwidgets.org/)
  - CMake (http://www.cmake.org/)

Features and Status:
--------------------
  - Simple UI
  - Devices
    - [x] SoapySDR Device support (known working checked)
      - [x] SoapySDRPlay for SDRPlay (Maintained by C.J.)
      - [x] SoapyRTLSDR for RTL-SDR (Maintained by C.J.)
      - [x] SoapyHackRF for HackRF
      - [x] SoapyBladeRF for BladeRF
      - [ ] SoapyUHD for Ettus USRP (untested)
      - [x] SoapyRemote, use any SoapySDR Device via network (works on Pi)
      - [ ] SoapyAirSpy (WIP by C.J.)
      - [ ] SoapyAudio (WIP by C.J.)
      - [x] SoapyOsmo for GrOsmoSDR devices
        - [ ] OsmoSDR
        - [ ] MiriSDR
        - [ ] RFSpace
        - [x] AirSpy
  - Basic Features
    - [x] Device Selection
    - [x] Bandwidth
    - [x] Color scheme
    - [x] Load/Save session
    - [x] Audio sample rate
    - [x] Device PPM
    - [x] Waterfall speed
    - [x] Spectrum average speed
    - [x] Gain Controls
    - [ ] Bookmarks
    - [ ] History
    - [ ] Default preferences
      - [ ] Audio defaults
      - [x] Device defaults
    - [ ] Run any device as rtl_tcp server and visualize control
  - Neat Visuals
    - [ ] 2D visuals
      - [x] Y Scope
      - [x] Spectrum
      - [x] Waterfall
      - [x] Add faint grid for sense of scale
      - [x] Audio Spectrum
      - [ ] X/Y Scope
      - [ ] Indicate outer spectrum edges when zoomed
    - [ ] 3D visuals
      - [ ] I/Q helix
  - Demodulation:
    - [ ] Basic modular expansion
    - [x] Multiple demodulators per IQ stream
    - [x] Audio device selection
    - [x] Modes
      - [x] FM
      - [x] FM stereo
      - [x] AM
      - [x] LSB
      - [x] USB
      - [x] DSB
      - [x] I/Q
    - [x] Controls
      - [x] Display Frequency and allow manual adjustments
      - [x] Allow selection of demodulation type
      - [x] Display separate zoomed-in view of current waterfall and spectrum, allow adjustments
      - [x] Display signal level and allow squelch control
      - [x] Display audio output selection
      - [x] Volume control
      - [x] Direct frequency input
      - [x] Mute
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
      - [ ] Record waterfall output to PNG file chunks of desired height 
      - [ ] Record I/Q input data
      - [ ] Simultaneously record demod decimated I/Q and audio
    - [ ] Playback
  - Audio
    - [ ] Recording
  - Implement digital demodulation supported by liquid-dsp: (http://liquidsdr.org/doc/modem.html)
    - [ ] Demodulator Lab
      - [ ] Demodulator I/Q input filtering
      - [ ] Audio output filtering
      - [ ] Toggle current demodulator exclusively into "Lab" mode
        - [ ] Additional visualizations for audio and I/Q stream
          - [ ] Audio Spectrum
          - [ ] Constellation / X-Y Scope
        - [ ] Digital demodulation status and controls
        - [ ] Demodulator AGC, Equalization controls
          - [ ] Lock AGC and Equalization when digital lock obtained
        - [ ] Digital output
          - [ ] Output console
          - [ ] Capture file (auto?)
          - [ ] Network output
          - [ ] Block device output
    - [ ] Digital modes available to implement
      - [ ] PSK
      - [ ] DPSK
      - [ ] ASK
      - [ ] QAM 
      - [ ] APSK
      - [ ] BPSK 
      - [ ] QPSK
      - [ ] OOK 
      - [ ] SQAM 
      - [ ] Star Modem 
      - [ ] MFSK
      - [ ] CPFSK
  - Optimization
    - [x] Eliminate large waterfall texture uploads
    - [ ] Update visuals to OpenGL 3.x / OpenGL ES
    - [x] Resolve constant refresh on visuals that don't change often
    - [ ] Resolve all driver/platform vertical sync issues
    - [x] Group and divide IQ data distribution workload instead of 100% distribution per instance


Advanced Goals and ideas:
------------------------
  - Design a plan for expansion via modules (dylib/dll/lua)
  - Support shell-based stdin/stdout tools for direct output/playback to/from CLI audio processing apps (i.e. DSD on OSX)
  - Update visuals to support OpenGL ES
  - Basic demodulator filter(s) that can be enabled and tweaked visually
  - Support multiple simultaneous device usage
    * Categorize devices by antenna connections
    * Allow locked frequencies to activate unused devices to continue demodulation on same antenna
  - Integrate LUA
    * Expose liquid-dsp functionality
    * Scriptable liquid-dsp demodulation
    * Scriptable digital demodulation output handlers
      - Create block output devices on *nix?
      - Create socket outputs?
      - Visual outputs?
    * Take control of additional devices and spawning new demodulators (i.e. trunkers)
    * Script manager / live editor
    * Provide scriptable liquid-dsp modulation for transceivers?
    * Allow scripts to launch/run headless (no UI)
  - "PVR" like mode with waterfall time shifting
  - L/R and surround-sound balance settings for separating and listening to mono streams
  - Add tool for converting decimated I/Q recording to video
    - Select video features such as title/demodulation/scope/waterfall/etc.
    - Render to video from GL frames->ffmpeg/mencoder /w demodulated audio
  - Accessibility / Control
    - USB/MIDI control surfaces
    - Joystick / gamepad input
    - Vibration / force-feeback
  - Investigate compilation via emscripten using SoapyRemote for input
    - Create web server+SoapyRemote bundle for embedded devices
    - Use emscripten compiled CubicSDR via embedded web server 

Target Platforms:
----------------
  - [x] OSX
  - [x] Windows
  - [x] Linux
  - [ ] HTML5


License:
-------
  - GPL
