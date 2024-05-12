# Dockerfile to generate a build image for Ubuntu including all build dependencies.

FROM ubuntu:latest
LABEL maintainer="labrat.eu"

# Install common packages
RUN apt-get update
RUN apt-get install -y python3-venv python3-pip git cmake make

# Python setup
RUN python3 -m venv /.venv
ENV VIRTUAL_ENV /.venv
ENV PATH /.venv/bin:$PATH
RUN python3 -m pip install --upgrade conan
RUN python3 -m pip install regex

# Clear the unused apt files.
RUN rm -rf /var/lib/apt/lists/*

# Install doxygen
ADD https://github.com/doxygen/doxygen/releases/download/Release_1_10_0/doxygen-1.10.0.linux.bin.tar.gz doxygen.tar.gz
RUN tar -xaf doxygen.tar.gz
RUN cd doxygen-1.10.0; make install
RUN cd ..; rm -rf doxygen-1.10.0*

# Add conan profile
ADD profile /opt/conan_profile
