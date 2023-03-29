# Liquid-DSP
mkdir -p $HOME/build/jgaeddert
cd $HOME/build/jgaeddert
git clone https://github.com/jgaeddert/liquid-dsp.git

cd $HOME/build/jgaeddert/liquid-dsp
./bootstrap.sh

echo "Configuring liquid-dsp.."
./configure > /dev/null

echo "Building liquid-dsp.."
make -j$(nproc) > /dev/null
sudo make install
