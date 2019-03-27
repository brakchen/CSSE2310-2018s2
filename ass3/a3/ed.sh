#!/bin/bash

tee $2.in | ./ed $* 2> $2.err | tee $2.out
