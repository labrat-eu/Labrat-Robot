#!/bin/sh

# Initialization script for the git repository.

# Clone submodules.
git submodule update --init

# Set git paths
git config --local core.hooksPath .githooks/
git config commit.template .gittemplates/commit.txt

# Exit normally.
exit 0
