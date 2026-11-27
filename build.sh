#!/usr/bin/bash
if [[ $1 == "--debug" ]]; then
  gcc -fsanitize=address -g xcb-main.c -lxcb
else
  gcc xcb-main.c -lxcb
fi
