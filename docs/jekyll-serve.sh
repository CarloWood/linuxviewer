#! /bin/bash

# Create a cleanup function to remove the temporary configuration file.
function cleanup ()
{
  # Remove the temporary _config.yml file
  rm -f .generated/_config.temp.yml
}

trap cleanup EXIT INT TERM

if [ ! -e _config.yml.in ]; then
  echo "Please run $0 in the docs/ directory."
  exit 1
fi

# Generate a temporary _config.yml file with the environment variables replaced
envsubst < _config.yml.in > .generated/_config.temp.yml

INCREMENTAL=
CONFIG="$REPOBASE/docs/config.yml"

if [ -e "${CONFIG}" ] && cmp -s "${CONFIG}" .generated/_config.temp.yml ; then
  #FIXME: uncomment this ...   INCREMENTAL="--incremental"
  true
else
  cp .generated/_config.temp.yml "${CONFIG}"
fi

# Run Jekyll with the temporary configuration file until control-C is pressed.
echo "Running: bundle exec jekyll serve ${INCREMENTAL} --config .generated/_config.temp.yml"
bundle exec jekyll serve ${INCREMENTAL} --config .generated/_config.temp.yml
