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
    && mkdir AFLNet_Legion \
    && groupadd ubuntu \
    && useradd -rm -d /home/ubuntu -s /bin/bash -g ubuntu -G sudo -u 1000 ubuntu -p "$(openssl passwd -1 ubuntu)"

# Set the timezone of docker for loggingss
RUN ln -fs /usr/share/zoneinfo/$TZ /etc/localtime

# Use ubuntu as default username
USER ubuntu
WORKDIR /home/ubuntu
# Remember to mount AFLNet_Legion at /home/ubuntu/AFLNet_Legion

# Environment
ENV WORKDIR='/home/ubuntu' \
    TZ=Australia/Melbourne \
    AFLNET='/home/ubuntu/AFLNet_Legion' \
    AFL_NET='/home/ubuntu/AFLNet_Legion' \
    AFL_PATH='/home/ubuntu/AFLNet_Legion' \
    PATH="${PATH}:/home/ubuntu/AFLNet_Legion" \
    LLVM_CONFIG='llvm-config-3.8' \
    AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1 \
    AFL_NO_AFFINITY=1

COPY aflnet_setup /home/ubuntu/


# LightFTP-related
COPY LightFTP_* /home/ubuntu/
COPY Setup /home/ubuntu/

RUN /bin/bash -c "./LightFTP_prepare"
