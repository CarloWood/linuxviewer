#! /bin/bash

# Create a cleanup function to remove the temporary configuration file.
function cleanup ()
{
  # Remove the temporary _config.yml file
  rm -f _config.temp.yml
}

trap cleanup EXIT INT TERM

if [ ! -e _config.yml.in ]; then
  echo "Please run $0 in the docs/ directory."
  exit 1
fi

# Generate a temporary _config.yml file with the environment variables replaced
envsubst < _config.yml.in > _config.temp.yml

echo "Running: bundle exec jekyll clean --config _config.temp.yml"
bundle exec jekyll clean --config _config.temp.yml
