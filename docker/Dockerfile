FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -qq
RUN apt-get install -y software-properties-common wget && add-apt-repository -y ppa:ubuntu-wine/ppa && dpkg --add-architecture i386 && apt-get update -qq

RUN apt-get install -y pkg-config libqt5websockets5-dev libusb-dev libusb-1.0-0-dev qt-sdk qt5-qmake qt5-default
RUN apt-get install -y debhelper dpkg-dev dh-make

RUN wget https://github.com/stedolan/jq/releases/download/jq-1.5/jq-linux64 && mv jq-linux64 /usr/bin/jq && chmod +x /usr/bin/jq
ENV OK_SH_JQ_BIN /usr/bin/jq

RUN apt-get install -y curl git-buildpackage
RUN git clone https://github.com/whiteinge/ok.sh.git /usr/lib/ok.sh && chmod +x /usr/lib/ok.sh/ok.sh && ln -s /usr/lib/ok.sh/ok.sh /usr/bin/ok.sh

ADD config/.netrc /root
ADD entrypoint.sh /
ADD tools.sh /usr/local/bin

ENTRYPOINT ["/entrypoint.sh"]
