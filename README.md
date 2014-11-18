CubicSDR
========

Cross-Platform Software-Defined Radio Application

Utilizes: 
--------
  - liquid-dsp (http://liquidsdr.org/)
  - OpenGL (https://www.opengl.org/)
  - portaudio (http://www.portaudio.com/)
  - wxWidgets (https://www.wxwidgets.org/)
  - CMake (http://www.cmake.org/)


Basic Goals:
-----------
  - Simple UI
  - Minimal configuration
  - Neat Visuals
  - LSB / USB / AM / FM / WFM / WBFM stereo demodulation
  - Multiple demodulators per IQ stream
  - Separate visualizations per demodulator
  - Frequency locked/floating demodulators
  - IQ Recording and playback
  - Audio Recording


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
