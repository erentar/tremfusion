FROM ubuntu:xenial
RUN apt update -y
RUN apt install -y curl cmake libgl1-mesa-dev libsdl2-dev libcurl4-openssl-dev libopenal-dev libfreetype6-dev mingw-w64 g++-mingw-w64 g++-multilib git zip vim-nox rsync libncurses-dev libsdl1.2-dev
COPY . /usr/src/
RUN cd /usr/src && make -j

ENTRYPOINT /bin/bash

# run container with:
#docker build -t tremulous:latest . && xhost +local:root && docker run --net=host -it -e DISPLAY=:0 -v /tmp/.X11-unix:/tmp/.X11-unix  --device /dev/snd tremulous:latest
# see http://wiki.ros.org/docker/Tutorials/GUI for more info on running GUI applications with docker.
# as running the game inside docker is heavily dependant on environment and requires questionable security practices, it is advised that one copies the built game out of the container through `docker cp`.
