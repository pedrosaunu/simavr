# ATmega328P Timer1 CPU-visible shared TEMP — scoped acceptance

**Date:** 2026-11-19. **Result:** PASS, stopped Timer1 CPU-visible register
access only. **Tested source:** `7019454a9a89c7c56ac481315e7f2766e90294f8`.
No PR; no change to the fork's `master` branch; no GitHub Actions.

## Why the original WeightOS differential failed

Unmodified Debian libsimavr 1.6 exposed two problems under a single result:
(1) reading `avr->data[...]` directly observes internal simulator backing,
not necessarily the value an AVR `LDS` instruction would read; (2) the
simulator did not yet implement shared TEMP transactions across all four
Timer1 16-bit register families.

The corrected opt-in fork implements:

- TCNT1H/OCR1AH/OCR1BH/ICR1H high writes stage the **same TEMP** without
  mutating committed register high bytes.
- Low writes atomically commit the staged high byte and newly written low.
- Reading TCNT1L or ICR1L updates TEMP from the committed high byte.
  Reading TCNT1H or ICR1H returns TEMP. Reading OCR1AH/BH returns the
  direct committed byte and **must not** modify TEMP.
- ICR1 writes require an ICR-as-TOP waveform generation mode. The mode
  register must update even when the timer is stopped with CS=0.
- A separate committed ICR1H shadow prevents the generic simavr core's
  CPU-read return-value writeback from corrupting timer hardware state.
- The feature remains enabled only where the ATmega328/P core sets
  `AVR_CPU_TIMING_TIMER16_TEMP`. Other models are unaffected by default.

## Executed acceptance

The dedicated `Dockerfile.shared-temp` builds the fork and invokes native
regressions, the prior 6-GPR TCNT fixture, a new 9-GPR cross-register
stopped-timer fixture, and three mutation controls. Both source and
guest-observed outputs are captured in
`tests/evidence/timer1-shared-temp-v131-railway.jsonl`.

The **exact** Neural Uno v1.28 guest firmware is compiled from
`tests/weightos_v128_exact_guest.S` and verified to have SHA-256
`10766ac960e63ac09df30784cc0206bb0d711ffd5e71eb68da966a82fcb284d1`.
Guest execution yields `r17=0x00` and `r18=0x56`; two deliberate
expectation corruptions return nonzero results.

Railway deployment: `709fce0e-bcb0-401a-8869-44239bf4973e`, SUCCESS.
The complete machine-readable result is
`tests/evidence/timer1-shared-temp-v131.json`.

## Exact limits

This test **does not validate** physical ATmega328P, live timer counting,
CTC counter cycles, capture, output compare flags, PWM, interrupt priorities,
prescaler phase, simultaneous callback timing, or full simavr conformance.
The fork is not an independent physical hardware oracle; it is an
independently implemented instruction executor with separately reviewed
datasheet-directed expectations.

The original unmodified Debian libsimavr 1.6 failure is still historical
evidence. This new result demonstrates that the specifically corrected fork
agrees with WeightOS for a stated subset, not that the old result never
occurred.

## Reproduction

```sh
docker build -f Dockerfile.shared-temp -t simavr-temp-v131 .
docker run --rm simavr-temp-v131
```

The build fails on missing native test dependencies, incorrect firmware
checksum, wrong CPU-visible values, unrecognized WGM mode, or undetected
mutations. These constraints are verified without GitHub Actions.
