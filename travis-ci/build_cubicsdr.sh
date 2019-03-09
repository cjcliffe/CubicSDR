# CubicSDR
cd $HOME/build
mkdir cjcliffe/CubicSDR-build
cd cjcliffe/CubicSDR-build
cmake ../CubicSDR -DCMAKE_BUILD_TYPE=Release -DwxWidgets_CONFIG_EXECUTABLE=$HOME/build/wxWidgets/staticlib/bin/wx-config -DUSE_HAMLIB=1 -DENABLE_DIGITAL_LAB=1
make -j2

