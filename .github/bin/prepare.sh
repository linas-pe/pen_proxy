#!/bin/bash

source .github/bin/common.sh

if [ ! -f "$depfile" ]; then
    exit 0
fi

.github/bin/ftp_tool.sh -d || exit 1

while read -r line
do
    filename="$line-$branch_name-$os.tar.gz"
    sudo tar xf "$filename" -C /usr || exit 1
    if [ "$line" == "pen_utils" ]; then
        ln -s /usr/include/pen_utils/pen.m4 . || exit 1
    fi
done < "$depfile"

