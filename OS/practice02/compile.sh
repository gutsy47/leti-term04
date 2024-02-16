#!/bin/bash
for i in {1..3}
do
  g++ -o "r$i" "main0$i.cpp" -lpthread
done