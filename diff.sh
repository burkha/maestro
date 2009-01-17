#!/bin/bash

for file in `svn st -q | cut -c 8-`
do
  tkdiff $file
done
