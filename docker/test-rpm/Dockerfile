FROM rockylinux:9
COPY package.rpm /tmp/
RUN rpm -i /tmp/package.rpm
RUN test -f /usr/bin/dolphin_memory_engine_daemon && \
    test -f /lib/systemd/system/dolphin-memory-engine-daemon.service
