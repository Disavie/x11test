#!/usr/bin/bash
if [[ $1 == "--debug" ]]; then
  gcc -fsanitize=address -g xcb-main.c -lxcb -lxcb-keysyms -O0
else
  gcc xcb-main.c -lxcb -lxcb-keysyms

fi
