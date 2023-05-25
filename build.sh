#!/usr/bin/env sh
echo '[INFO] Feral Installer - (c) Copyright 2020 Maximilian Götz (https://www.maxbits.net/)'
echo '[INFO] Please make sure that Sudo, Git, CMake, LibGMP and LibMPFR are installed!'
echo '[INFO] Installation started. Fetching system cores...'

# Get current working directory
CWD="$(pwd)"

exit_handler() {
    echo '[INFO] Gracefully exiting.'
    rm -rf ~/.feral-installer
    cd $CWD
}

# Get OS ID
SYSNAME=$(uname)

export CFLAGS="$CFLAGS -I/usr/local/include"
export CXXFLAGS="$CXXFLAGS -I/usr/local/include"

if [ "$SYSNAME" = "Darwin" ]; then
    BREWGMP="$(brew --prefix gmp)"
    BREWMPFR="$(brew --prefix mpfr)"
    BREWMPC="$(brew --prefix mpc)"
    echo "[INFO] GMP prefix: $BREWGMP"
    echo "[INFO] MPFR prefix: $BREWMPFR"
    echo "[INFO] MPC prefix: $BREWMPC"
    export CFLAGS="$CFLAGS -I$BREWGMP/include -I$BREWMPFR/include -I$BREWMPC/include"
    export CXXFLAGS="$CXXFLAGS -I$BREWGMP/include -I$BREWMPFR/include -I$BREWMPC/include"
    export LDFLAGS="$LDFLAGS -L$BREWGMP/lib -L$BREWMPFR/lib -L$BREWMPC/lib"
fi
CORES=1
# Get system cores for faster build
if [ "$SYSNAME" = "Linux" ]; then
    CORES=$(nproc)
else # for BSD and macOS
    CORES=$(sysctl -n hw.ncpu)
fi
echo "[INFO] Using $CORES cores"

echo '[INFO] Checking for Git, Sudo and CMake...'
# No sudo on macOS since /usr/local is not usually root owned (as long as homebrew is installed)
SUDO="sudo"
if [ "$SYSNAME" = "Darwin" ] && hash brew 2>/dev/null || [ "$(id -u)" = "0" ]; then
    SUDO=""
else
    echo '[INFO] Using sudo.'
fi

# Check for Git
if hash git 2>/dev/null; then
    echo '[INFO] Git found.'
else
    echo '[FATAL] Git not available! Install it to continue! Aborting...'
    exit
fi

# Check for sudo
if [ "$SUDO" = "sudo" ]; then
    if hash sudo 2>/dev/null; then
        echo '[INFO] sudo found.'
    else
        echo '[FATAL] sudo not available! Install it to continue! Aborting...'
        exit
    fi
fi

# Check for CMake
if hash cmake 2>/dev/null; then
    echo '[INFO] CMake found.'
else
    echo '[FATAL] CMake not available! Install it to continue! Aborting...'
    exit
fi

# Trap SIGINT so that if the program is ctrl-c'd while being in ~/.feral-installer,
# we can delete that directory and get back to the original directory
trap exit_handler INT

build() {
    mkdir build && cd build
    if [ -n "$CI" ]; then
        export DISABLE_MARCH_NATIVE="true"
    fi
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if [ "$?" -ne 0 ]; then exit 1; fi
    make -j$CORES install
    if [ "$?" -ne 0 ]; then exit 1; fi
    cd ..
}

if [ -z "$CI" ]; then
    # Create a temporary directory
    echo '[INFO] Creating temporary directory at ~/.feral-installer...'
    mkdir ~/.feral-installer && cd ~/.feral-installer

    # Clone the language source code
    echo '[INFO] Cloning required repositories...'
    git clone https://github.com/Feral-Lang/Feral.git
    cd Feral
fi

echo '[INFO] Building language compiler...'
build

echo '[INFO] Done! Cleaning up...'
rm -rf ~/.feral-installer

cd $CWD