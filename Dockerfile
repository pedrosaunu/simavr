FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends gcc-avr avr-libc binutils-avr && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN set -eux; for p in 0 1 2 3 4 5 6 7; do avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DPHASE=$p -x assembler-with-cpp tests/weightos_nu016f_boundaries.S -o /tmp/prefix.elf; avr-objcopy -O binary -j .text /tmp/prefix.elf /tmp/prefix.bin; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DPHASE=$p -x assembler-with-cpp tests/weightos_nu016fg_hardware.S -o /tmp/hardware.elf; avr-objcopy -O binary -j .text /tmp/hardware.elf /tmp/hardware.bin; n=$(wc -c </tmp/prefix.bin); head -c "$n" /tmp/hardware.bin >/tmp/actual.bin; cmp /tmp/prefix.bin /tmp/actual.bin; avr-objcopy -O ihex -j .text /tmp/hardware.elf /tmp/hardware.hex; echo "NU016HW PREFIX $p $n"; sha256sum /tmp/prefix.bin /tmp/hardware.bin; echo "NU016HW HEX $p"; cat /tmp/hardware.hex; done
CMD ["/bin/sh","-c","exit 0"]
