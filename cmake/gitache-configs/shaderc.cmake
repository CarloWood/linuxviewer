gitache_config(
  GIT_REPOSITORY
    "https://github.com/google/shaderc"
  GIT_TAG
    "v2023.2"
  BOOTSTRAP_COMMAND
    ./utils/git-sync-deps
  CMAKE_ARGS
    "-DSHADERC_SKIP_TESTS:BOOL=ON -DCMAKE_BUILD_TYPE=Release"
)
