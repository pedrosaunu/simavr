# Experimental strict regression gate. A passing differential gate is NOT silicon conformance.
FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
    ca-certificates git make gcc g++ libc6-dev pkg-config libelf-dev \
    gcc-avr avr-libc binutils-avr > /tmp/apt.log 2>&1 && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
COPY . .
RUN set -eux; \
    make -C simavr RELEASE=1 all >/tmp/core-build.log 2>&1 || { tail -n 80 /tmp/core-build.log; exit 1; }; \
    make -C tests RELEASE=1 all >/tmp/test-build.log 2>&1 || { tail -n 80 /tmp/test-build.log; exit 1; }; \
    make -C tests RELEASE=1 run_tests; \
    echo 'IRQ_EXPERIMENT_NATIVE_REGRESSIONS=PASS'
RUN set -eux; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
    -Isimavr/sim tests/weightos_v126_irq_probe.c \
    -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/weightos_v126_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/weightos_v126_probe; \
    echo 'IRQ_EXPERIMENT_V126_DUAL_TIMER=PASS'
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
# WeightOS v1.26: keep the isolated Timer1 /1 IRQ timing divergence visible.
# Compile exactly the separately pinned guest AVR fixture; never "fix"
# the test by rewriting expected cycles from the simulator's own output.
RUN set -eux; \
    avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start \
      -x assembler-with-cpp tests/weightos_timer1_only_v126.S \
      -o /tmp/timer1_only_v126.elf; \
    avr-objcopy -O binary -j .text \
      /tmp/timer1_only_v126.elf /tmp/timer1_only_v126.bin; \
    echo '02b3bb49a1ea0d535187c23473f2fa220b75a9e28f8597560df554ae80b91177  /tmp/timer1_only_v126.bin' | sha256sum -c -; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
      -Isimavr/sim tests/weightos_timer1_irq_oracle_probe.c \
      -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
      -o /tmp/weightos_timer1_v126_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
      /tmp/weightos_timer1_v126_probe /tmp/timer1_only_v126.bin --strict; \
    echo 'IRQ_EXPERIMENT_TIMER1_ISOLATED_STRICT=PASS'

# The following is a differential gate against archived neural v1.26
# trace values, not a second independent physical-timing oracle.
# Mutation cases must be rejected to demonstrate a fail-closed comparator.
RUN set -eux; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
      -Isimavr/sim tests/weightos_v126_dual_strict.c \
      -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
      -o /tmp/weightos_dual_strict; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/weightos_dual_strict; \
    if LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
      /tmp/weightos_dual_strict --negative-cycle; then exit 71; fi; \
    if LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
      /tmp/weightos_dual_strict --negative-return-pc; then exit 72; fi; \
    echo 'IRQ_EXPERIMENT_DUAL_NEURAL_DIFFERENTIAL=PASS'

# Independently scheduled mapped-I/O bus edges, two classic AVR cores.
RUN set -eux; \
    avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start \
      -x assembler-with-cpp tests/weightos_io_retirement.S \
      -o /tmp/io_retirement.elf; \
    avr-objcopy -O binary -j .text \
      /tmp/io_retirement.elf /tmp/io_retirement.bin; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
      -Isimavr/sim tests/weightos_io_retirement_probe.c \
      -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
      -o /tmp/io_retirement_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
      /tmp/io_retirement_probe /tmp/io_retirement.bin

# Independent classic AVR 2/3-byte interrupt-entry latency and stack check.
RUN set -eux; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
      -Isimavr/sim tests/weightos_irq_width_probe.c \
      -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
      -o /tmp/weightos_irq_width_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/weightos_irq_width_probe

# Independent strict test: 2-cycle STS crossing a scheduled peripheral
# callback deadline must preserve pre-write visibility at that event.
RUN set -eux; \
    avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start \
      -x assembler-with-cpp tests/weightos_multicycle_edge.S \
      -o /tmp/edge.elf; \
    avr-objcopy -O binary -j .text /tmp/edge.elf /tmp/edge.bin; \
    cc -std=c11 -O2 -Wall -Wextra -Werror \
      -Isimavr/sim tests/weightos_multicycle_edge_probe.c \
      -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
      -o /tmp/edge_probe; \
    LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
      /tmp/edge_probe /tmp/edge.bin --strict; \
    echo 'EDGE_ORDER_STRICT=PASS'

CMD ["/bin/sh","-c","echo IRQ_EXPERIMENT_COMPLETE_NOT_CONFORMANCE; exit 0"]
