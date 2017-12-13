#!/bin/bash

join -1 2 -2 2 <(grep 'i$' $1 | sort -k2 ) <(grep 'i$' $2 | sort -k2 ) | cut -d' ' -f1,2,4 | sort -n -k3,3
