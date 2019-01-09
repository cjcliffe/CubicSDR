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