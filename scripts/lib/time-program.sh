#!/bin/bash

/usr/bin/time -f "real %E user %U sys %S mem %M KB" "$@"
