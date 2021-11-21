FROM jazz_base_stable

LABEL author=kaalam.ai

WORKDIR /home/jazz

RUN apt-get install -y git

ADD ./download_and_store.sh /home/jazz

EXPOSE 8899

CMD ["/home/jazz/start.sh"]
