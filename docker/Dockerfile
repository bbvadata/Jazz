FROM ubuntu

MAINTAINER kaalam.ai

WORKDIR /home/jadmin

RUN apt-get update && apt-get install -y libmicrohttpd-dev

RUN ln -s /usr/lib/x86_64-linux-gnu/libmicrohttpd.so.10 /usr/lib/x86_64-linux-gnu/libmicrohttpd.so.12

RUN mkdir /home/jadmin/mdb

ADD ./jazz /home/jadmin
ADD ./jazz_config.ini /home/jadmin/config/jazz_config.ini

EXPOSE 8888

CMD ["/home/jadmin/jazz", "start"]
