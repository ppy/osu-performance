FROM ubuntu:20.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake libmysqlclient-dev libcurl4-openssl-dev

WORKDIR /src
COPY dependencies/ /src/dependencies/
COPY include/ /src/include/
COPY src/ /src/src/
COPY CMakeLists.txt /src/

RUN mkdir build && cd build && cmake .. && make -j

FROM ubuntu:20.04

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libmysqlclient21 libcurl4 jq

WORKDIR /srv
COPY --from=builder /src/bin/osu-performance /srv/osu-performance
COPY ./scripts/docker-entrypoint.sh /srv/docker-entrypoint.sh
RUN chown -R 1000:1000 /srv

USER 1000:1000

ENTRYPOINT [ "/srv/docker-entrypoint.sh" ]
