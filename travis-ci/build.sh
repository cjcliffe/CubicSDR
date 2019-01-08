# Liquid-DSP
cd $HOME/build
mkdir jgaeddert/
cd jgaeddert/
git clone https://github.com/jgaeddert/liquid-dsp.git
cd liquid-dsp/
./bootstrap.sh
./configure
make -j2
sudo make install

# Soapy SDR
cd $HOME/build
mkdir pothosware/
cd pothosware/
git clone https://github.com/pothosware/SoapySDR.git
mkdir SoapySDR-build
cd SoapySDR-build
cmake ../SoapySDR -DCMAKE_BUILD_TYPE=Release
make -j2
sudo make install

# CubicSDR
cd $HOME/build
mkdir cjcliffe/CubicSDR-build
cd cjcliffe/CubicSDR-build
cmake ../CubicSDR -DCMAKE_BUILD_TYPE=Release
make -j2
