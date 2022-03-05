### Introduction ###

LinuxViewer is work in progress.

My intention is to write an application that can connect
with [OpenSimulator](http://opensimulator.org/) grids and [SecondLife](https://secondlife.com/).

### Prerequisites ###

#### Reqquired packages ####

 On debian/ubuntu:

    sudo apt install gawk doxygen graphviz
    sudo apt install libboost-dev libsparsehash-dev

 On Archlinux:

    sudo pacman -S gawk doxygen graphviz
    sudo pacman -S boost sparsehash

#### Environment ####

    export GITACHE_ROOT=/opt/gitache_root

Where `/opt/gitache_root` is some (empty) directory that you
have write access to. It will be used to cache packages so you
don't have to recompile them (it is a 'cache', but you might
want to keep it around).

### Downloading ###

    git clone --recursive https://github.com/CarloWood/linuxviewer.git

### Configuration ###

    cd linuxviewer
    mkdir build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebug -DEnableDebugGlobal:BOOL=OFF
    
Optionally add `-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON` to see the compile commands.
Other build types are: `Release`, `RelWithDebInfo`, `Debug` and `BetaTest`.
See [stackoverflow](https://stackoverflow.com/a/59314670/1487069) for an explanation
of the different types.

### Building ###

    NUMBER_OF_CORES=8
    cmake --build build --config RelWithDebug --parallel ${NUMBER_OF_CORES}
