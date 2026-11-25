FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev gcc-avr avr-libc binutils-avr && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN make -C simavr RELEASE=1 all && cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu016f_boundaries.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe
RUN set -eux; for phase in 0 2 4 8; do echo "NU016F CASE $phase"; offset=$phase; asm=tests/weightos_nu016f_boundaries.S; if [ "$phase" = 8 ]; then offset=0; asm=tests/weightos_nu016h_mux_controls.S; fi; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DPHASE=$offset -x assembler-with-cpp $asm -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; grep -E 'NU016B (WRITE|RECONFIG)|NU016G TRANSFER|NU016F RESUME' /tmp/events; done
CMD ["/bin/sh","-c","exit 0"]
