FROM ubuntu

LABEL author=kaalam.ai

WORKDIR /home/jazz

COPY zeromq-4.3.4.tar.gz /home/jazz

RUN tar -xvf zeromq-4.3.4.tar.gz
RUN apt-get update --fix-missing
RUN apt-get install -y libmicrohttpd-dev
RUN apt-get install -y libcurl4-gnutls-dev

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get install -y g++
RUN apt-get install -y make
RUN apt-get install -y xutils-dev
RUN apt-get install -y libtool
RUN apt-get install -y automake
RUN apt-get install -y pkg-config

WORKDIR /home/jazz/zeromq-4.3.4

RUN ./autogen.sh
RUN ./configure
RUN make
RUN make install
RUN ln -s /usr/local/lib/libzmq.so.5 /usr/lib/x86_64-linux-gnu/

WORKDIR /home/jazz

RUN mkdir /home/jazz/jazz_dbg_mdb

ADD ./start.sh /home/jazz
ADD ./jazz /home/jazz
ADD ./jazz_config.ini /home/jazz/config/jazz_config.ini
ADD ./www /home/jazz/config/www/
