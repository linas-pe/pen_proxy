#!/bin/bash

os=$(echo "$RUNNER_OS" | awk '{print tolower($0)}')
proj_name=$(basename "${GITHUB_REPOSITORY}")

branch_name=$(basename "${GITHUB_REF_NAME}")
if [ "$branch_name" == "merge" ]; then
    branch_name=$(basename "${GITHUB_BASE_REF}")
fi

target_name=${proj_name}-${branch_name}-${os}
target_pkg_name=${target_name}.tar.gz
workdir=${GITHUB_WORKSPACE}/${target_name}
depfile=.Pen.deps

export os proj_name branch_name target_name target_pkg_name workdir depfile

