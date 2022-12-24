gitache_config(
  GIT_REPOSITORY
    "https://github.com/CarloWood/libcwd.git"
  GIT_TAG
    "master"
  CMAKE_ARGS
    "-DEnableLibcwdAlloc:BOOL=OFF -DEnableLibcwdLocation:BOOL=ON"
)
