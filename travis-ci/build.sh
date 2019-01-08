# Liquid-DSP
cd $HOME/build
mkdir jgaeddert/
cd jgaeddert/
git clone https://github.com/jgaeddert/liquid-dsp.git
cd liquid-dsp/
./bootstrap.sh
echo "Configuring liquid-dsp.."
./configure > /dev/null
echo "Building liquid-dsp.."
make -j2 > /dev/null
sudo make install

# Soapy SDR
cd $HOME/build
mkdir pothosware/
cd pothosware/
git clone https://github.com/pothosware/SoapySDR.git
mkdir SoapySDR-build
cd SoapySDR-build
cmake ../SoapySDR -DCMAKE_BUILD_TYPE=Release
echo "Building SoapySDR.."
make -j2 > /dev/null
sudo make install

# wxWidgets
if [ ! -d "$HOME/build/wxWidgets/staticlib" ]
then
echo "wxWidgets cache not found; building."
cd $HOME/build
mkdir -p $HOME/build/wxWidgets/staticlib
cd wxWidgets
wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.2/wxWidgets-3.1.2.tar.bz2
echo "Unpacking wxWidgets.."
tar -xvjf wxWidgets-3.1.2.tar.bz2 > /dev/null
cd wxWidgets-3.1.2/
./autogen.sh 
./configure --with-opengl --disable-shared --enable-monolithic --with-libjpeg --with-libtiff --with-libpng --with-zlib --disable-sdltest --enable-unicode --enable-display --enable-propgrid --disable-webkit --disable-webview --disable-webviewwebkit --prefix=`echo $HOME/build/wxWidgets/staticlib` CXXFLAGS="-std=c++0x"
echo "Building wxWidgets.."
make -j2 > /dev/null
make install
fi


# CubicSDR
cd $HOME/build
mkdir cjcliffe/CubicSDR-build
cd cjcliffe/CubicSDR-build
cmake ../CubicSDR -DCMAKE_BUILD_TYPE=Release -DwxWidgets_CONFIG_EXECUTABLE=$HOME/build/wxWidgets/staticlib/bin/wx-config
make -j2

