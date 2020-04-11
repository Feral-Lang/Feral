#!/usr/bin/env sh
echo '[INFO] Feral Installer - (c) Copyright 2020 Maximilian Götz (https://www.maxbits.net/)'
echo '[INFO] Please make sure that Git, CMake, LibGMP and LibMPFR are installed!'
echo '[INFO] Installation started. Checking for Git, sudo and CMake...'

# Check for Git
if hash git 2>/dev/null; then
    echo '[INFO] Git found.'
else
    echo '[FATAL] Git not available! Install it to continue! Aborting...'
    exit
fi

# Check for sudo
if hash sudo 2>/dev/null; then
    echo '[INFO] sudo found.'
else
    echo '[FATAL] sudo not available! Install it to continue! Aborting...'
    exit
fi

# Check for CMake
if hash cmake 2>/dev/null; then
    echo '[INFO] CMake found.'
else
    echo '[FATAL] CMake not available! Install it to continue! Aborting...'
    exit
fi

# Create a temporary directory
echo '[INFO] Creating temporary directory at ~/.feral-installer...'
mkdir ~/.feral-installer && cd ~/.feral-installer

# Clone the language source code
echo '[INFO] Cloning required repositories...'
git clone https://github.com/Feral-Lang/Feral.git
git clone https://github.com/Feral-Lang/Feral-Std.git

build() {
    mkdir build && cd build
    cmake ..
    sudo make install
    cd ..
}

echo '[INFO] Building language...'
cd Feral
build
cd ..

echo '[INFO] Building standard library...'
cd Feral-Std
build
cd ../..

echo '[INFO] Done! Cleaning up...'
rm -rf ~/.feral-installer
