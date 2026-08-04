#!/bin/sh
BINARY_HOME=./bin
INPUT_HOME=./input
INPUT=${INPUT_HOME}/AB_NYC_2019.csv

cat ${INPUT} | ${BINARY_HOME}/mapper | sort -k1,1 | ${BINARY_HOME}/reducer_mean > output_mean
cat output_mean

cat ${INPUT} | ${BINARY_HOME}/mapper | sort -k1,1 | ${BINARY_HOME}/reducer_variance > output_variance
cat output_variance
