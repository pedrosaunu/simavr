# WeightOS / simavr — mapped-I/O retirement and classic AVR IRQ timing

Date: 2026-11-19 (UTC)

**Disposition: validated opt-in experiment for ATmega328/328P; NOT an upstream-ready
simavr timing correction. No pull request requested or opened.**

## Scope and actual code

The CPU core has one address-independent writeback path in
`simavr/sim/sim_core.c`: guest opcodes that store to mapped I/O can stage
one write, committing its callback **after** the instruction's cycle count has
advanced. Pure GPR/SRAM stores preserve legacy immediate behavior. The
special-case tests `TCNT0=0x46` and `TCNT1L=0x84` were removed from the
opcode handlers. The path is used for direct, indirect and bit-manipulation
fallback stores; special `bit_io` callbacks are not yet covered.

`simavr/sim/sim_interrupts.c` can charge `2 + avr->address_size` cycles
to classic AVR interrupt entry, processing scheduled peripherals during the
entry interval before executing the vector opcode. This models 4 cycles with a
two-byte return PC and 5 cycles with a three-byte PC, separately from the
cycles of the vector's jump instruction.

Features are **explicit opt-ins** on each `avr_t` instance:
`AVR_CPU_TIMING_IO_WRITEBACK` and `AVR_CPU_TIMING_IRQ_ENTRY`. Both
are enabled by the `atmega328` core factory, used by both `atmega328`
and `atmega328p` aliases. Other device factories retain zero flags.

A test can activate the same mechanism on an ATmega2560 *instance* without
changing the production ATmega2560 core default.

## Why opt-in is necessary

The first full attempt enabled both behaviors globally and failed the native
ATmega88 analog-comparator regression, 28/29 native tests:
`Check analog comparator interrupts` expected `YYYYYY`, observed `YYYYNY`.
A controlled ablation that restored legacy IRQ latency on other devices but
kept generic I/O retirement still failed with the same comparator output.
Therefore the causally identified incompatibility is global I/O retirement,
not global IRQ entry latency in that case.

Core-level opt-in restores 29/29 native tests. This does **not** prove
that delaying the I/O callback is the right electrical timing for every
peripheral or AVR derivative.

## Executed gates

**Railway** project `WeightOS simavr TEMP 20261118`, build
`dd444777-16a2-4f2f-b591-5388e9499e45`,
commit `c5ecb571e7dfc2047fd17f461237e42d02f38b87`,
build status **SUCCESS**. Recompiled simavr/libsimavr and the native tests;
no GitHub Actions usage.

| Gate | Observed |
|---|---|
| Native simavr suite across supported devices | 29 successes / 29 |
| Shared Timer0/1 divide-by-8 phase probes | 2/2 pass |
| v1.26 combined Timer0/1 interrupts | 13 interrupts (7+6) |
| Pinned Timer1-only firmware SHA-256 | verified |
| Independent Timer1 scalar oracle | 6/6 vector cycles, 6/6 saved return PCs |
| v1.26 dual-timer archived neural trace | 13/13 cycles, vectors, saved PCs, SP, SREG, GPIO, register hash |
| Deliberately corrupted expected cycle or PC | both rejected |
| Independent mapped I/O bus callback clock test, ATmega328P | 9/9 events |
| Same I/O mechanism on manually opted-in ATmega2560 instance | 9/9 events |
| Synthetic interrupt-entry clock test, ATmega328P 2-byte PC | 4 cycles, correct stack and vector |
| Same clock-entry mechanism on manually opted-in ATmega2560 3-byte PC | 5 cycles, correct stack and vector |

The 9 I/O events use firmware `tests/weightos_io_retirement.S` and probe
`tests/weightos_io_retirement_probe.c`; their expected callback cycles
are computed from the instruction schedule (2, 4, 8, 12, 16, 18, 20,
23, 26), not from simavr logs. They exercise `OUT`, `STS`, `ST X`,
`ST Y`, `ST Z`, `CBI`, `SBI`, `STD Y+q`, and `STD Z+q`, through
one ordinary I/O address.

The 2-/3-byte PC test `tests/weightos_irq_width_probe.c` injects a
synthetic enabled interrupt through simavr's native IRQ API. It validates
entry clocks, saved PC word, stack delta, vector and interrupt masking.
It **does not** prove complete timer ISR or sleep wake-up semantics on
ATmega2560.

The multi-ISR dual-timer gate uses **archived Neural Uno v1.26 data**.
It was **not** a re-execution of neural/ONNX weights in this Railway
Docker build. The isolated Timer1 scalar oracle is a separate
implementation, but applies only to its pinned firmware.

## Datasheet corroboration

- Microchip classic AVR interrupt entry: four minimum clocks with a
  two-byte PC, five clocks when the PC uses more than two bytes:
  https://onlinedocs.microchip.com/oxy/GUID-AAB4173A-6BB6-4A4B-A053-1ED838585692-en-US-4/GUID-A6E4418F-6F23-47E2-9D0B-32E7A41E8EA5.html
- ATmega328/P: 4-clock minimum IRQ response.
- ATmega640/1280/1281/2560/2561: 5-clock minimum:
  https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf

## Explicit remaining gaps — do not claim AVR conformance

1. I/O **reads** still occur at the decoder's earlier execution point.
   The staged writeback path only covers one guest data-store operation.
2. Device-specific `bit_io` fast callbacks (for `SBI/CBI`) may occur
   before retirement; the fallback mapped-I/O path is the one tested.
3. Pending timer callbacks whose deadline falls *within* a multi-cycle
   instruction are processed after opcode retirement by the current
   execution loop. Edge ordering at exactly the same clock is not yet a
   separately proved CPU/peripheral bus contract.
4. Synchronous timer TOP/CTC/PWM, Timer2 asynchronous prescaler, hardware
   16-bit TEMP register behavior, capture, nested interrupts, interrupt
   priority, sleep wake-up and other AVR derivatives are not yet
   independently verified at cycle accuracy.
5. Neither real ATmega silicon nor a fresh ONNX neural-weight replay was
   run in this build. Cross-implementation agreement on pinned firmware
   is necessary evidence, not sufficient chip equivalence.
6. The shared `avr_t` structure now has a timing-feature member: this
   changes the library's C ABI for consumers compiled against older headers.
   Versioning or internalization of the flags is needed before release.
7. The 29 native tests do not constitute exhaustive chip coverage.

## Decision and continuation

**Keep this branch experimental.** Do not merge into `master` or the
existing shared-prescaler fix branch; do not open a PR. The previous
address-special-case branch remains historical evidence only.

Next release-gate candidate: independently checked Timer0/1 modes and
phase/timestamp edge ordering for several timer reload values and prescalers,
followed by fresh Neural Uno/ONNX vs simavr execution on identical firmware,
without copying expected outputs from either side. Explicitly define a
silicon-backed acceptance gate before claiming cycle-level ATmega328P
conformance.
