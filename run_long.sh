#!/usr/bin/env bash
#
# Inspired by
# https://github.com/travis-ci/travis-ci/issues/4190#issuecomment-169987525
#

# start command and send to background
"$@" &

minutes=0  # passed [min]
limit=50   # limit [min]

while kill -0 $! >/dev/null 2>&1
do
  echo -n -e " \b"  # print something and backspace it

  if [ $minutes >= $limit ]; then
    break;
  fi

  minutes=$((minutes+1))

  # next time in a minute
  sleep 60
done
