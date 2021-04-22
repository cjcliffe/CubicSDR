# wxWidgets
if [ -f "$HOME/build/wxWidgets/staticlib/bin/wx-config" ]; then
    echo "wxWidgets cache found; skipping.."
    exit 0
else
    echo "wxWidgets cache not found; building.."    
fi


echo "Fetch wxWidgets.."
cd $HOME/build
mkdir -p $HOME/build/wxWidgets/staticlib
cd wxWidgets
wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxWidgets-3.1.5.tar.bz2 > /dev/null

echo "Unpacking wxWidgets.."
tar -xvjf wxWidgets-3.1.5.tar.bz2 > /dev/null
cd wxWidgets-3.1.5/
./autogen.sh

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then 
./configure --with-opengl --disable-glcanvasegl --disable-shared --enable-monolithic --with-libjpeg --with-libtiff --with-libpng --with-zlib --disable-sdltest --enable-unicode --enable-display --enable-propgrid --disable-webkit --disable-webview --disable-webviewwebkit --prefix=`echo $HOME/build/wxWidgets/staticlib` CXXFLAGS="-std=c++0x"
fi

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
./configure --with-opengl --disable-glcanvasegl --disable-shared --enable-monolithic --with-libjpeg --with-libtiff --with-libpng --with-zlib --with-mac --disable-sdltest --enable-unicode --enable-display --enable-propgrid --disable-webkit --disable-webview --disable-webviewwebkit --prefix=`echo $HOME/build/wxWidgets/staticlib` CXXFLAGS="-std=c++0x" --with-libiconv=/usr
fi

echo "Building wxWidgets.."
make -j2
make install

