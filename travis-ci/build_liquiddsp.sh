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