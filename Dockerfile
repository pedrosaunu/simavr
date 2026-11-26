FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev gcc-avr avr-libc binutils-avr python3 && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN make -C simavr RELEASE=1 all && cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu016f_boundaries.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe
RUN python3 tests/nu016i_generate.py /tmp/matrix
RUN set -eux; for id in $(seq 0 16); do echo "NU016F CASE $id"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/matrix/case-$id.S -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; grep -E 'NU016J ARM|NU016B TOV' /tmp/events; done
RUN set -eux; for phase in 0 1 2 3 4 5 6 7 8; do echo "NU016F CASE $((100+phase))"; offset=$phase; asm=tests/weightos_nu016f_boundaries.S; if [ "$phase" = 8 ]; then offset=0; asm=tests/weightos_nu016h_mux_controls.S; fi; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DPHASE=$offset -x assembler-with-cpp $asm -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; grep 'NU016J ARM' /tmp/events; done
RUN set -eux; echo "NU016F CASE 200"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp tests/weightos_nu016j_temp_controls.S -o /tmp/temp.elf; avr-objcopy -O binary -j .text /tmp/temp.elf /tmp/temp.bin; sha256sum /tmp/temp.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/temp.bin 2>/tmp/events
RUN python3 tests/nu016k_generate.py /tmp/matrix
RUN set -eux; for id in $(seq 300 311); do echo "NU016F CASE $id"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/matrix/case-$id.S -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; grep -E 'NU016K TRANSFER|NU016BUS WRITE|NU016J ARM' /tmp/events; done
RUN python3 tests/nu016ka_generate.py /tmp/matrix
RUN set -eux; for id in $(seq 400 409); do echo "NU016F CASE $id"; avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp /tmp/matrix/case-$id.S -o /tmp/pwm.elf; avr-objcopy -O binary -j .text /tmp/pwm.elf /tmp/pwm.bin; sha256sum /tmp/pwm.bin; NU016_BUS_PHASE=2 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/pwm.bin 2>/tmp/events; grep -E 'NU016K TRANSFER|NU016BUS WRITE|NU016J ARM|NU016KA RESUME' /tmp/events; done
RUN sh tests/nu016k_legacy_regression.sh
CMD ["/bin/sh","-c","exit 0"]

