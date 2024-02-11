#!/bin/bash
g++ -c main.cpp
g++ -o main main.o -lpthread
rm main.o