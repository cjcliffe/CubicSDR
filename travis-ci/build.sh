cd $HOME/build
# Liquid-DSP
mkdir jgaeddert/
git clone https://github.com/jgaeddert/liquid-dsp.git jgaeddert/liquid-dsp
cd jgaeddert/liquid-dsp/
./bootstrap.sh
./configure
make -j2
sudo make install

cd $HOME/build
# Soapy SDR
mkdir pothosware/
git clone https://github.com/pothosware/SoapySDR.git pothosware/SoapySDR
mkdir pothosware/SoapySDR-build
cmake pothosware/SoapySDR pothosware/SoapySDR-build
cd pothosware/SoapySDR-build
make -j2
sudo make install

cd $HOME/build
# CubicSDR
mkdir cjcliffe/CubicSDR-build
cmake cjcliffe/CubicSDR cjcliffe/CubicSDR-build
cd cjcliffe/CubicSDR-build
make -j2
