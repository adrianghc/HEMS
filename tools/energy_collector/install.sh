#!/bin/bash

if ! [ -x "$(command -v python3)" ]; then
  echo 'Error: python3 is not installed.' >&2
  exit 1
fi
# if ! [ -x "$(command -v pip3)" ]; then
#   echo 'Error: pip3 is not installed.' >&2
#   exit 1
# fi

# pip3 install -r requirements.txt

crontab -l | { cat; echo $"5 0 * * * python3 $PWD/collector.py"; } | crontab -
