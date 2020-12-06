#! /bin/sh

if test -f .git; then
  echo "Error: don't run $0 inside a submodule. Run it from the parent project's directory."
  exit 1
fi

if test "$(realpath $0)" != "$(realpath $(pwd)/autogen.sh)"; then
  echo "Error: run autogen.sh from the directory it resides in."
  exit 1
fi

# Check if we want to configure for cmake (only).
if command -v cmake >/dev/null; then
  if test -n "$AUTOGEN_CMAKE_ONLY" -o ! -e configure.ac; then
    AUTOGEN_CMAKE_ONLY=1
  elif test -e CMakeLists.txt; then
    echo "*** Configuring for both autotools and cmake."
    echo "*** Set AUTOGEN_CMAKE_ONLY=1 in environment to only configure for cmake."
  fi
fi

if test -d .git; then
  # Take care of git submodule related stuff.
  # The following line is parsed by configure.ac to find the maintainer hash. Do not change its format!
  MAINTAINER_HASH=15014aea5069544f695943cfe3a5348c
  # If this was a clone without --recursive, fix that fact.
  if test ! -e cwm4/scripts/real_maintainer.sh; then
    git submodule update --init --recursive
  fi
  # If new git submodules were added by someone else, get them.
  if git submodule status --recursive | grep '^-' >/dev/null; then
    git submodule update --init --recursive
  fi
  # Update autogen.sh and cwm4 itself if we are the real maintainer.
  if test -f cwm4/scripts/real_maintainer.sh; then
    cwm4/scripts/real_maintainer.sh $MAINTAINER_HASH
    RET=$?
    # A return value of 2 means we need to continue below.
    # Otherwise, abort/stop returning that value.
    if test $RET -ne 2; then
      exit $RET
    fi
  fi
  if test -z "$AUTOGEN_CMAKE_ONLY"; then
    cwm4/scripts/do_submodules.sh
  fi
else
  # Clueless user check.
  if test -f configure; then
    echo "You only need to run './autogen.sh' when you checked out this project from the git repository."
    echo "Just run ./configure [--help]."
    if test -e cwm4/scripts/bootstrap.sh; then
      echo "If you insist on running it and know what you are doing, then first remove the 'configure' script."
    fi
    exit 0
  elif test ! -e cwm4/scripts/real_maintainer.sh; then
    echo "Houston, we have a problem: the cwm4 git submodule is missing from your source tree!?"
    echo "I'd suggest to clone the source code of this project from github:"
    echo "git clone --recursive https://github.com/CarloWood/linuxviewer.git"
    exit 1
  fi
fi

if test -z "$AUTOGEN_CMAKE_ONLY"; then
  # Run the autotool commands.
  exec cwm4/scripts/bootstrap.sh
elif test -n "$CMAKE_CONFIGURE_OPTIONS"; then
  # I uses bash functions 'configure' and 'make' basically passing $CMAKE_CONFIGURE_OPTIONS
  # to cmake and running 'make' inside build-release; see below.
  echo "Now run: configure && make"
else
  echo "To make a Release build, run:"
  echo "mkdir build-Release"
  echo "cd build-Release"
  echo "cmake .."
  echo "make"
fi
