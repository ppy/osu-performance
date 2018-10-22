FROM ubuntu:xenial

ENV DEBIAN_FRONTEND noninteractive

RUN apt update && \
	apt install -y \
	wget \
	lsb-release

RUN wget https://dev.mysql.com/get/mysql-apt-config_0.8.10-1_all.deb
RUN echo mysql-apt-config mysql-apt-config/select-server select mysql-8.0 | debconf-set-selections
RUN dpkg -i mysql-apt-config*
RUN rm mysql-apt-config*

RUN apt update && \
	apt install -y \
	cmake \
	build-essential \
	libcurl4-openssl-dev \
	libmysqlclient-dev
