# Ephemeral source-build acceptance runner for WeightOS/simavr validation.
# GitHub Actions is not used. Build fails if any upstream native test fails.
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
# Build-only job; no running server is required. Remove the Railway service afterward.
CMD ["/bin/sh","-c","echo WEIGHTOS_PRESCALER_BUILD_SUCCEEDED; exit 0"]
