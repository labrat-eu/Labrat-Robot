# Dockerfile to generate a build image for Ubuntu including all build dependencies.

FROM ubuntu:rolling
LABEL maintainer="labrat.eu"

# Run and remove the script.
RUN apt-get update
RUN apt-get install -y g++-13 libstdc++-12-dev python3-venv python3-pip git cmake make

# Clear the unused apt files.
RUN rm -rf /var/lib/apt/lists/*

# Install doxygen
ADD https://github.com/doxygen/doxygen/releases/download/Release_1_10_0/doxygen-1.10.0.linux.bin.tar.gz doxygen.tar.gz
RUN tar -xaf doxygen.tar.gz
RUN cd doxygen-1.10.0; make install
RUN cd ..; rm -rf doxygen-1.10.0*
