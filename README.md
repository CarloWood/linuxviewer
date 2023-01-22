### Introduction ###

LinuxViewer is work in progress.

My intention is to write an application that can connect
with [OpenSimulator](http://opensimulator.org/) grids and [SecondLife](https://secondlife.com/).

### Prerequisites ###

#### Required packages ####

 On Archlinux:

    sudo pacman -Sy base-devel git cmake ninja
    sudo pacman -Sy boost sparsehash eigen systemd
    sudo pacman -Sy blas lapack libxml++ libxcb libxkbcommon-x11 xorgproto xcb-proto
    sudo pacman -Sy shaderc vulkan-headers vulkan-icd-loader vulkan-validation-layers

 Note that ninja is an alternative make and is only required if you intend to
 configure with -GNinja (see below).

 On Debian/Ubuntu:

    sudo apt install build-essential autoconf pkg-config libtool gawk git cmake ninja-build
    sudo apt install libboost-all-dev libsparsehash-dev libeigen3-dev libsystemd-dev
    sudo apt install libblas-dev liblapack-dev libxml++2.6-dev libxcb-xkb-dev libxkbcommon-x11-dev
    sudo apt install libvulkan-dev wget

 Debian/Ubuntu do not have libxml++3.0, at the time of writing, but linuxviewer also supports
 libxml++2.6 now for that reason. It is never tested though.

 Also the package shaderc does not seem to exist; it is therefore added to gitache if
 not found on the system.

 Finally, the Vulkan SDK is not maintained by debian/ubuntu. You have to add the third-party
 repository of LunarG as described here:
 https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html
 Scroll down to `Install the SDK`.

 For example (but please check that webpage for up to date info!), for Ubuntu 20.04 (Focal Fossa) you'd do:

    wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
    sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-focal.list http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list
    sudo apt update
    sudo apt install vulkan-sdk

 Please see the above link to vulkan.lunarg.com for instructions for your Ubuntu version.

#### Environment ####

    export GITACHE_ROOT=/opt/gitache_root

Where `/opt/gitache_root` is some (empty) directory that you
have write access to; create it if it doesn't exist yet.
It will be used to cache packages so you don't have to recompile
them (it is a 'cache', but you might want to keep it around).

### Downloading ###

    git clone --recursive https://github.com/CarloWood/linuxviewer.git

### Configuration ###

    cd linuxviewer
    CMAKE_CONFIG=RelWithDebug   # Use Debug if you are a developer.
    BUILD_DIR=build
    cmake -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${CMAKE_CONFIG} -DEnableDebugGlobal:BOOL=OFF
    
Optionally add `-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON` to see the compile commands.
Also optionally, add -GNinja to the cmake arguments (this is what I am developing with).
Other build types are: `Release`, `RelWithDebInfo`, `Debug` and `BetaTest`.
See [stackoverflow](https://stackoverflow.com/a/59314670/1487069) for an explanation
of the different types.

### Building ###

    NUMBER_OF_CPUS=$(($(grep '^cpu cores' /proc/cpuinfo | wc --lines) - 2))
    cmake --build ${BUILD_DIR} --config ${CMAKE_CONFIG} --parallel ${NUMBER_OF_CPUS}

Just set `NUMBER_OF_CPUS` to whatever you want, of course.
