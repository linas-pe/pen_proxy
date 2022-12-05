#!/bin/bash

source .github/bin/common.sh

cd "$workdir" || exit 1
tar -czf "../${target_pkg_name}" -- *
