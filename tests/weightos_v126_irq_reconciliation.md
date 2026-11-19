# WeightOS / simavr v1.26 — dual-timer IRQ timing reconciliation

**Date:** 2026-11-19 UTC

**Status: experimental differential gate passed, general-purpose timing fix NOT accepted. No pull request opened.**

## Context and provenance

This work continues the shared synchronous prescaler candidate in
`fix/atmega328p-shared-prescaler`, but it has **not** been applied there or to
`master`. It remains in an isolated family of experiment branches, culminating
in `test/weightos-v126-dual-strict`.

The archived WeightOS Neural Uno v1.26 engineering source provides:

- the **same** compiled multi-IRQ firmware: SHA-256
  `ad15fee7130e39a7a1fc87d48a0eb95bd15709f85c3a7e6708fa9bf799b698c2`;
- the isolated Timer1 firmware (318 bytes) SHA-256
  `02b3bb49a1ea0d535187c23473f2fa220b75a9e28f8597560df554ae80b91177`;
- `reports/dual_irq_v125_multi/neural.jsonl` and
  `reports/dual_irq_v125_multi/acceptance.json`, the **archived**
  neural model's cycles, vectors, stack return PCs, GPIO, register state;
- `scripts/timer1_boundary_oracle_v126.py`, a narrow scalar instruction-cycle
  oracle derived from the published AVR timing assumptions and pinned firmware.

The tests run against a freshly built libsimavr in a disposable Railway Linux
Docker build. GitHub Actions and physical ATmega hardware were **not used**.

## Causal ablations

| Variant | Isolated Timer1 exact vector cycles | Isolated Timer1 return-PC matches | Dual firmware observations |
|---|---:|---:|---|
| Shared-prescaler candidate only | 0/6 | 3/6 | 13 IRQs; old timing disagreement |
| +4 cycles at IRQ entry, processing peripheral clocks | 0/6 | 3/6 | Still differs |
| Above +deferred completion of 2-cycle STS to TCNT1L | 6/6 | 6/6 | 9/13 neural-matching cycles, 9/13 return PCs |
| Above +deferred completion of 1-cycle OUT to TCNT0 | **6/6** | **6/6** | **13/13 neural-matching cycles and saved PCs** |

The isolated scalar oracle predicts Timer1 vector cycles
`278, 548, 818, 1088, 1358, 1628` and saved word PC `0x0091` for
all six entries. The final experiment matches them all **without changing
the scalar oracle or the firmware**.

The dual test separately compares actual simulator vector events against the
**archived neural implementation**. Vector cycles:
`279, 298, 535, 549, 792, 819, 1047, 1089, 1303, 1359, 1560, 1629, 1815`.
This is *cross-implementation differential agreement*, **not an independent
datasheet-derived dual-timer oracle**.

### New strict checks

The Dockerfile performs:

1. Fresh simavr library and native suite (29/29 tests).
2. Timer0/Timer1 shared /8 phase checks (2/2 probes).
3. Repeated combined /1 overflow IRQ probe (13 entries, Timer0=7, Timer1=6).
4. Pinned isolated Timer1 /1 firmware SHA verification and **strict** independent
   scalar six-vector timing/return-PC oracle (6/6, 6/6).
5. **Strict** dual neural differential comparison of 13 vector cycles, vector
   identities, stack-return addresses, SP, SREG, DDRB/PORTB, and all 32
   architectural register bytes via FNV-1a fingerprints. The hash is
   noncryptographic and is **not** a byte-for-byte full memory comparison.
6. Two negative controls, intentionally shifting one expected cycle and one
   expected return PC. Both must fail their own strict tests.

Tests from the strict build were executed at Railway deployment
`57ae3bb8-5ac4-4218-a53b-27da432b5179`
(commit `64a467d56633c9ac8f5968035eac713d03aedcad`).

## Why this is NOT an upstream-quality fix yet

The experiment deliberately perturbs `avr->cycle` around narrow I/O writes,
for `atmega328` and literal data addresses `0x84` (TCNT1L) and `0x46`
(TCNT0). The IRQ change is likewise local to the internal ATmega328 core name.

These are **instrumentation shortcuts** to establish causal phase effects,
not an architecture to ship. In particular:

- The general I/O timing API should represent the completion edge
  of `STS`, `OUT`, `ST X/Y/Z`, `SBI/CBI`, and peripheral-generated writes
  without manually rewriting `avr->cycle` in one opcode or device address.
- Clock advancement during interrupt entry must be coherent with pending
  flags, interrupt priority, sleep, pending/ongoing IRQ state, event callbacks
  and delayed instruction-boundary delivery.
- Timing must be independently checked across clock-select settings,
  normal/CTC/PWM, Timer2/PSRASY, interrupt nesting, 16-bit TEMP register
  behavior, other AVR devices and regressions beyond the native 29.
- Physical silicon phase measurement has **not** been performed; full
  ATmega328P CPU/peripheral conformance remains **unproven**.
- The WeightOS neural weights and ONNX binary were **not changed** by
  this experiment. The corrected simavr branch was checked against
  an **archived neural trace**, not by re-executing the neural weights
  inside the same Railway build.

## Decision

**Accept the experimental *diagnosis and its pinned regression harness*.**
Do **not** promote the prototype as a general simavr correction, do not
silently overwrite `fix/atmega328p-shared-prescaler`, do not reopen
the earlier WeightOS model release gate by assumption, and do not open a PR.

Next engineering milestone: replace the address-specific clock mutation
with a general bus-access and interrupt-entry timing mechanism; require
new independent instruction/clock fixtures and cross-device testing
before any promotion.
