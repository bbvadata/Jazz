FROM jazz_base_stable

LABEL author=kaalam.ai

WORKDIR /home/jazz

RUN apt-get install -y git
RUN apt-get install -y p7zip-full

ADD ./download_and_store.sh /home/jazz
ADD ./download_store_private.sh /home/jazz

EXPOSE 8899

CMD ["/home/jazz/start.sh"]
