# Soapy SDR
mkdir -p $HOME/build/pothosware/SoapySDR-build

cd $HOME/build/pothosware
git clone https://github.com/pothosware/SoapySDR.git

cd $HOME/build/pothosware/SoapySDR-build
cmake $HOME/build/pothosware/SoapySDR -DCMAKE_BUILD_TYPE=Release

echo "Building SoapySDR.."
make -j2 > /dev/null

sudo make install
