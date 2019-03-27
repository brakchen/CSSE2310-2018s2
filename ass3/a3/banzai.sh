#!/bin/bash

tee $2.in | ./banzai $* | $2.out
