# Ephemeral source-build acceptance runner for WeightOS/simavr validation.
# GitHub Actions is not used. Any native regression failure stops the build.
FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev \
    gcc-avr avr-libc binutils-avr && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN set -eux; \
    make -C simavr RELEASE=1 all; \
    make -C tests RELEASE=1 all; \
    make -C tests RELEASE=1 run_tests; \
    echo 'WEIGHTOS_PRESCALER_NATIVE_GATE=PASS'
RUN set -eux; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
    -Isimavr/sim tests/weightos_v126_irq_probe.c \
    -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/weightos_v126_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/weightos_v126_probe; \
    echo 'WEIGHTOS_V126_FORK_IRQ_PROBE=PASS'
RUN set -eux; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
    -Isimavr/sim tests/weightos_shared_phase_probe.c \
    -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/weightos_phase; \
    for n in 3 11; do \
      avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start \
        -DNOP_COUNT=$n -x assembler-with-cpp \
        tests/weightos_shared_phase.S -o /tmp/phase.elf; \
      avr-objcopy -O binary -j .text /tmp/phase.elf /tmp/phase.bin; \
      test "$(od -An -tx1 -N2 /tmp/phase.bin | tr -d ' \\n')" = "7fc0"; \
      LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/weightos_phase /tmp/phase.bin $n; \
    done
CMD ["/bin/sh","-c","echo WEIGHTOS_PRESCALER_BUILD_SUCCEEDED; exit 0"]
