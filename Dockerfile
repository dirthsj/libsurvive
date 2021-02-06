FROM ubuntu:20.04
RUN export DEBIAN_FRONTEND=noninteractive
RUN ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime
RUN apt-get update
RUN apt-get install build-essential zlib1g-dev libx11-dev libusb-1.0-0-dev freeglut3-dev liblapacke-dev libatlas-base-dev cmake -y --no-install-recommends
RUN dpkg-reconfigure --frontend noninteractive tzdata
