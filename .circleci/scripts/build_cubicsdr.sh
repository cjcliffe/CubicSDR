# CubicSDR
mkdir -p $HOME/build/cjcliffe/CubicSDR-build
cd $HOME/build/cjcliffe/CubicSDR-build
cmake ~/project -DCMAKE_BUILD_TYPE=Release -DwxWidgets_CONFIG_EXECUTABLE=$HOME/build/wxWidgets/staticlib/bin/wx-config -DUSE_HAMLIB=1 -DENABLE_DIGITAL_LAB=1
make -j$(nproc)
