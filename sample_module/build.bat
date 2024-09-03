@echo off

clang++ -std=c++20 -I../src -shared sample_module.cpp -o sample_module.dll
