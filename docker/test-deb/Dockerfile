FROM ubuntu:24.04
COPY package.deb /tmp/
RUN apt-get update && apt-get install -y /tmp/package.deb
RUN test -f /usr/bin/dolphin_memory_engine_daemon && \
    test -f /lib/systemd/system/dolphin-memory-engine-daemon.service
