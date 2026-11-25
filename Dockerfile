FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev gcc-avr avr-libc binutils-avr python3 && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN make -C simavr RELEASE=1 all && cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu016f_boundaries.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe
RUN python3 tests/nu016i_generate.py /tmp/matrix
RUN set -eux; for id in $(seq 0 16); do echo "NU016F CASE $id"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/matrix/case-$id.S -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; done
CMD ["/bin/sh","-c","exit 0"]
