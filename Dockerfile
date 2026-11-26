FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev gcc-avr avr-libc binutils-avr python3 && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN make -C simavr RELEASE=1 all && cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu016p_probe.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe
RUN python3 tests/nu016p_generate.py /tmp/hardware
RUN set -eux; for id in 0 1 2; do echo "NU016P CASE $id"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/hardware/prefix-$id.S -o /tmp/prefix.elf; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/hardware/hardware-$id.S -o /tmp/full.elf; avr-objcopy -O binary -j .text /tmp/prefix.elf /tmp/prefix.bin; avr-objcopy -O binary -j .text /tmp/full.elf /tmp/full.bin; cmp -n "$(wc -c </tmp/prefix.bin)" /tmp/prefix.bin /tmp/full.bin; echo NU016P PREFIX; sha256sum /tmp/prefix.bin; echo NU016P FULL; sha256sum /tmp/full.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/prefix.bin >/tmp/trace 2>/tmp/events; grep 'NU016P FRAME' /tmp/trace; echo NU016P HEX_BEGIN; avr-objcopy -O ihex -j .text /tmp/full.elf /tmp/full.hex; cat /tmp/full.hex; echo NU016P HEX_END; done
CMD ["/bin/sh","-c","exit 0"]
