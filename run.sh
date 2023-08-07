#!/bin/bash
R CMD SHLIB -c src/matching.cpp

mv src/matching.so .

Rscript src/main.r
