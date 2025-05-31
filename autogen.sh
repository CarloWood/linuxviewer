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

# Do some git sanity checks.
if test -d .git; then
  PUSH_RECURSESUBMODULES="$(git config push.recurseSubmodules)"
  if test -z "$PUSH_RECURSESUBMODULES"; then
    # Use this as default for now.
    git config push.recurseSubmodules check
    echo -e "\n*** WARNING: git config push.recurseSubmodules was not set!"
    echo "***      To prevent pushing a project that references unpushed submodules,"
    echo "***      this config was set to 'check'. Use instead the command"
    echo "***      > git config push.recurseSubmodules on-demand"
    echo "***      to automatically push submodules when pushing a reference to them."
    echo "***      See http://stackoverflow.com/a/10878273/1487069 and"
    echo "***      http://stackoverflow.com/a/34615803/1487069 for more info."
    echo
  fi
fi

if test -z "$AUTOGEN_CMAKE_ONLY"; then
  # Run the autotool commands.
  cwm4/scripts/bootstrap.sh
fi

if [ -r ".build-instructions" ]; then
  # Set sensible values for:
  # REPOBASE                            : the root of the project (repository base).
  # BUILDDIR                            : the directory that will be used to build the project.
  # CMAKE_CONFIG                        : one of Debug, RelWithDebInfo, RelWithDebug, BetaTest or Release.
  # CMAKE_CONFIGURE_OPTIONS_STR         : other options, separated by '|' symbols.
  # CONFIGURE_OPTIONS                   : options for the autotools `configure` script (if any).
  source "./.build-instructions"

  # Convert CMAKE_CONFIGURE_OPTIONS_STR back to an array.
  # If you use a bash array locally for your options, then convert them to CMAKE_CONFIGURE_OPTIONS_STR
  # for the sake of these printed instructions (if you want) with:
  # export CMAKE_CONFIGURE_OPTIONS_STR=$(printf "%s|" "${CMAKE_CONFIGURE_OPTIONS[@]}")
  IFS='|' read -ra CMAKE_CONFIGURE_OPTIONS <<< "$CMAKE_CONFIGURE_OPTIONS_STR"
fi

if [ -e CMakeLists.txt ]; then
  echo -e "\nBuilding with cmake:\n"
  echo "To make a $CMAKE_CONFIG build, run:"
  [ -d "$BUILDDIR" ] || echo "mkdir $BUILDDIR"
  echo -n "cmake -S \"$REPOBASE\" -B \"$BUILDDIR\" -DCMAKE_BUILD_TYPE=\"$CMAKE_CONFIG\""
  # Put quotes around options that contain spaces.
  for option in "${CMAKE_CONFIGURE_OPTIONS[@]}"; do
    if [[ $option == *" "* ]]; then
        printf ' "%s"' "$option"
    else
        printf ' %s' "$option"
    fi
  done
  echo
  echo "cmake --build "$BUILDDIR" --config "$CMAKE_CONFIG" --parallel $(nproc)"
fi

if [ -e Makefile.am ]; then
  echo -e "\nBuilding with autotools:\n"
  project_name=$(basename "$PWD")
  # Give general instructions for building using autotools.
  [ -d ../$project_name-objdir ] || echo "mkdir ../$project_name-objdir"
  echo "cd ../$project_name-objdir"
  echo -n "../$project_name/configure --enable-maintainer-mode "
  if [ -n "$CONFIGURE_OPTIONS" ]; then
    echo "$CONFIGURE_OPTIONS"
  else
    echo "[--help]"
  fi
fi
