gitache_config(
  GIT_REPOSITORY
    "https://github.com/CarloWood/farmhash.git"
  GIT_TAG
    "master"
  BOOTSTRAP_COMMAND
    autoreconf -fi
  CONFIGURE_ARGS
    --disable-option-checking --with-gnu-ld
)
