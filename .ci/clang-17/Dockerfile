# Dockerfile to generate a build image for Ubuntu including all build dependencies.

FROM ubuntu:rolling
LABEL maintainer="labrat.eu"

# Run and remove the script.
RUN apt-get update
RUN apt-get install -y clang-17 libstdc++-12-dev python3-venv python3-pip git cmake make socat

# Clear the unused apt files.
RUN rm -rf /var/lib/apt/lists/*
