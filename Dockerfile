FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev gcc-avr avr-libc binutils-avr python3 && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN make -C simavr RELEASE=1 all && cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu016f_boundaries.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe
RUN set -eux; echo "NU016F CASE 200"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp tests/weightos_nu016j_temp_controls.S -o /tmp/temp.elf; avr-objcopy -O binary -j .text /tmp/temp.elf /tmp/temp.bin; sha256sum /tmp/temp.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/temp.bin 2>/tmp/events
CMD ["/bin/sh","-c","exit 0"]
