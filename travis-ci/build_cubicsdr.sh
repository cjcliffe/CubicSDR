# CubicSDR
cd $HOME/build
mkdir cjcliffe/CubicSDR-build
cd cjcliffe/CubicSDR-build
cmake ../CubicSDR -DCMAKE_BUILD_TYPE=Release -DwxWidgets_CONFIG_EXECUTABLE=$HOME/build/wxWidgets/staticlib/bin/wx-config
make -j2

