gitache_config(
  GIT_REPOSITORY
    "https://github.com/wolfSSL/wolfssl.git"
  GIT_TAG
    "655022cfc51e63dc59b08903b73a8abd5e36f110"
  BOOTSTRAP_COMMAND
    autoreconf -fi
  CONFIGURE_ARGS
    --disable-examples --disable-oldnames --disable-memory --disable-tlsv10 --enable-all --enable-intelasm --enable-writedup --enable-maxfragment --enable-sni
)
