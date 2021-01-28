FROM ubuntu:16.04

# Install dependencies
RUN apt-get -y update \
    && apt-get -y install apt-utils build-essential clang graphviz-dev git libgnutls-dev libglib2.0-dev cmake vim tzdata gdb libc6-dbg dpkg-dev libgnutls28-dev \
    && sed -ri '/^# deb-src .* xenial(-updates){0,1} main restricted$/s/^# //g' /etc/apt/sources.list \
    && apt update \
    && mkdir -p /opt/src \
    && cd /opt/src \
    && apt source libc6 \
    && mkdir -p /build/glibc-e6zv40/ \
    && ln -s /opt/src/glibc-2.23 /build/glibc-e6zv40/ \
    && mkdir AFLNet_MCTS \
    && groupadd ubuntu \
    && useradd -rm -d /home/ubuntu -s /bin/bash -g ubuntu -G sudo -u 1000 ubuntu -p "$(openssl passwd -1 ubuntu)"

# Set the timezone of docker for loggingss
RUN ln -fs /usr/share/zoneinfo/$TZ /etc/localtime

# Use ubuntu as default username
USER ubuntu
WORKDIR /home/ubuntu
# Remember to mount AFLNet_MCTS at /home/ubuntu/AFLNet_MCTS

# Environment
ENV WORKDIR='/home/ubuntu' \
    TZ=Australia/Melbourne \
    AFLNET='/home/ubuntu/AFLNet_MCTS' \
    AFL_NET='/home/ubuntu/AFLNet_MCTS' \
    AFL_PATH='/home/ubuntu/AFLNet_MCTS' \
    PATH="${PATH}:/home/ubuntu/AFLNet_MCTS" \
    LLVM_CONFIG='llvm-config-3.8'

COPY aflnet_setup /home/ubuntu/


# LightFTP-related
COPY LightFTP_* /home/ubuntu/
COPY Setup /home/ubuntu/

RUN /bin/bash -c "./LightFTP_prepare"