#!/usr/bin/env sh
echo '[INFO] Feral Installer - (c) Copyright 2020 Maximilian Götz (https://www.maxbits.net/)'
echo '[INFO] Installation started. Checking for Git and Homebrew...'

# Check for Git
if hash git 2>/dev/null; then
    echo '[INFO] Git found.'
else
    echo '[FATAL] Git not available! Install it to continue! Aborting...'
    exit
fi

# Check for Homebrew
if hash brew 2>/dev/null; then
    echo '[INFO] Homebrew found.'
else
    echo '[FATAL] Homebrew not available! Install it to continue! Aborting...'
    exit
fi

# Create a temporary directory
echo '[INFO] Creating temporary directory at ~/.feral-installer...'
mkdir ~/.feral-installer && cd $_

# Clone the language source code
echo '[INFO] Cloning required repositories...'
git clone https://github.com/Feral-Lang/Feral.git
git clone https://github.com/Feral-Lang/Feral-Std.git

# Installing dependencies
echo '[INFO] Installing dependencies...'
brew install cmake
brew install mpfr
brew install gmp

build() {
    mkdir build && cd $_
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
