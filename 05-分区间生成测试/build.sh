#!/bin/bash
sudo apt install libjsoncpp-dev
g++ -std=c++11 case_gen.cpp -ljsoncpp -o case_gen
./case_gen config.json 10
