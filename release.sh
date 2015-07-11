#!/bin/bash

set -e

if [[ -n "$(git status --porcelain)" ]]; then
    echo -e "commit/stash changes first\n"
    git status
    exit 1
fi


gitid=$(git log -1 --pretty=format:"%h")
date=$(date +%F)

vmaj=$(sed -n 's/^#define FW_MAJOR_V \([0-9]\)$/\1/p' youyue858d.ino)
vmia=$(sed -n 's/^#define FW_MINOR_V_A \([0-9]\)$/\1/p' youyue858d.ino)
vmib=$(sed -n 's/^#define FW_MINOR_V_B \([0-9]\)$/\1/p' youyue858d.ino)
vers="V${vmaj}.${vmia}${vmib}"

if [[ -z ${vmaj} || -z ${vmia} || -z ${vmib}  ]]; then
    echo "no valid version found: ${vmaj:-x}.${vmia:-x}${vmib:-x}"
    exit 2
fi

vline=$(cut -d'\n' -f5 youyue858d.ino | sed "s/^ \\* ${vers}$//" )

echo "vline: '$vline'"

if [[ -n ${vline} ]]; then
    echo "invalid version in line 5!"
    exit 2
fi

indent -linux -l150 youyue858d.ino
indent -linux -l150 youyue858d.h

file328="${date}__commit-${gitid}__ATmega328P-8MHz-RC-osc__FUSES-0xE2-0xDF-0xFD__${vers}-WDT.hex"
file168="${date}__commit-${gitid}__ATmega168-8MHz-RC-osc__FUSES-0xE2-0xDD-0xFD__${vers}-WDT.hex"

echo "processing ${vers}...".

make BOARD_SUB=atmega168
make BOARD_SUB=atmega328

git rm -f binaries/*hex || true

cp build-lilypad-atmega328/Youyue-858D-plus.hex binaries/${file328}
cp build-lilypad-atmega168/Youyue-858D-plus.hex binaries/${file168}

cd binaries
sha1sum ${file168} ${file328} > SHA1SUMS.TXT

git add -f ${file168} ${file328} SHA1SUMS.TXT

git commit -m "release binaries ${vers}"
git tag -a "v${vmaj}.${vmia}${vmib}" -m "$(git log -1 --skip=1 --pretty=format:"%B")"
