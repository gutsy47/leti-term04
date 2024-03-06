#!/bin/bash
g++ -D MODE=1 -o writer main.cpp
g++ -D MODE=0 -o reader main.cpp