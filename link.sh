#!/usr/bin/env bash

RED="\033[1;31m"
ILC="\033[3m"
ORG="\033[1;33m"
RST="\033[0m"

function _info()
{
    echo -e "${ORG}[🚧] INFO:\t${RST} ${ILC}$1${RST}"
}

function _error()
{
    echo -e "${RED}${BOLD}[❌] ERROR:\n${RST}\t$1\n\t${ILC}\"$2\"${RST}"
    _info "$0 <input_file.s> <output_file>"
    exit 84
}

function main()
{
    local input="$1"
    local output="$2"

    as "$input" -o "$input.o"
    ld "$input.o" -o $output
}

if [ "$#" -ne 2 ]; then
    _error "Invalid number of arguments" "Got: $# expected 2"
fi

main $1 $2
