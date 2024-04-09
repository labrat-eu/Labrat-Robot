# Dockerfile to generate a build image for Ubuntu including all build dependencies.

FROM ubuntu:devel
LABEL maintainer="labrat.eu"

# Install common packages
RUN apt-get update
RUN apt-get install -y python3-venv python3-pip git cmake make socat

# Python setup
RUN python3 -m venv /.venv
ENV VIRTUAL_ENV /.venv
ENV PATH /.venv/bin:$PATH
RUN python3 -m pip install --upgrade conan
RUN python3 -m pip install regex

# Install toolchain
RUN apt-get install -y g++-14 libstdc++-14-dev

# Clear the unused apt files.
RUN rm -rf /var/lib/apt/lists/*

# Add conan profile
ADD profile /opt/conan_profile
