# wxWidgets
if [ ! -f "$HOME/build/wxWidgets/staticlib/bin/wx-config" ]; then

echo "wxWidgets cache not found; building."
cd $HOME/build
mkdir -p $HOME/build/wxWidgets/staticlib
cd wxWidgets
wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.2/wxWidgets-3.1.2.tar.bz2

echo "Unpacking wxWidgets.."
tar -xvjf wxWidgets-3.1.2.tar.bz2 > /dev/null
cd wxWidgets-3.1.2/
./autogen.sh 

- if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then 
./configure --with-opengl --disable-shared --enable-monolithic --with-libjpeg --with-libtiff --with-libpng --with-zlib --disable-sdltest --enable-unicode --enable-display --enable-propgrid --disable-webkit --disable-webview --disable-webviewwebkit --prefix=`echo $HOME/build/wxWidgets/staticlib` CXXFLAGS="-std=c++0x"
fi

- if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
./configure --with-opengl --disable-shared --enable-monolithic --with-libjpeg --with-libtiff --with-libpng --with-zlib --with-mac --disable-sdltest --enable-unicode --enable-display --enable-propgrid --disable-webkit --disable-webview --disable-webviewwebkit --with-macosx-version-min=10.9  --prefix=`echo $HOME/build/wxWidgets/staticlib` CXXFLAGS="-std=c++0x" --with-libiconv=/usr

echo "Building wxWidgets.."
make -j2
make install

fi