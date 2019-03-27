#!/bin/bash

tee $3.in | ./shenzi $* 2> $3.err | tee $3.out
