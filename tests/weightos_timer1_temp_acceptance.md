# Neural Uno / simavr — 16-bit TCNT1 TEMP transaction acceptance

Date: 2026-11-19 UTC
Repository: `pedrosaunu/simavr`
Branch: `experiment/weightos-timer1-temp-fix`
Decision: **NARROW EXPERIMENT ACCEPTED; NOT GENERAL AVR TIMER1/SILICON CONFORMANCE. NO PR.**

## Objective and independent criterion

The ATmega328P classic 16-bit timer register interface uses a temporary
high-byte register (TEMP) for guest CPU accesses. For TCNT1:

- Writing TCNT1H stages TEMP; it does not immediately modify the committed
  16-bit counter.
- Writing TCNT1L commits that low byte together with TEMP's high byte.
- Reading TCNT1L reads the counter low byte and latches the committed counter
  high byte into TEMP; the following TCNT1H read returns that TEMP value.
- The TEMP register is shared with OCR1A/B and ICR1. Those shared-register
  interactions are **not implemented or validated in this increment**.

Reference: Microchip ATmega328P datasheet, section on accessing 16-bit
registers (high-byte temporary register), including TCNT1, OCR1A/B and ICR1.

Pinned guest source: `tests/weightos_timer1_temp.S`.
Strict host probe: `tests/weightos_timer1_temp_probe.c`.
Timer1 is deliberately **stopped** (`TCCR1B.CS=0`, `TIMSK1=0`).
Expected full register sequence is defined independently of simavr logs:
`r17..r22 = 34 12 78 12 CD AB` (hex).

## RED baseline reproduced

Branch: `experiment/weightos-timer1-temp-atomicity`
Railway Docker deployment: `4334ec0c-7011-42ff-a9fd-541dd2488cca`
Result: Docker build SUCCESS, **observational fixture RED**.

```
TIMER1_TEMP reg=r17 actual=0x34 expected=0x34 equal=1
TIMER1_TEMP reg=r18 actual=0x56 expected=0x12 equal=0
TIMER1_TEMP reg=r19 actual=0x78 expected=0x78 equal=1
TIMER1_TEMP reg=r20 actual=0x56 expected=0x12 equal=0
TIMER1_TEMP reg=r21 actual=0xcd expected=0xcd equal=1
TIMER1_TEMP reg=r22 actual=0xab expected=0xab equal=1
TIMER1_TEMP_SUMMARY finished=1 cycles=32 mismatches=2
```

The pre-fix code let CPU TCNT1H accesses overwrite the backing high counter
byte directly; subsequent low-byte reads did not correctly latch and expose
the committed value independently of the staged TEMP byte.

## Fix design — explicitly opted in for ATmega328/P

- Added `AVR_CPU_TIMING_TIMER16_TEMP` to the opt-in timing capability flags.
- The ATmega328 core factory enables the flag. Other core factories are not
  changed.
- `avr_timer_t` holds distinct committed high-byte and TEMP high-byte
  state for the TCNT register.
- TCNT1H CPU writes update TEMP without changing the committed 16-bit TCNT.
- TCNT1L CPU writes commit the staged high byte and the low byte.
- TCNT1L CPU reads latch the committed high byte into TEMP; TCNT1H reads
  fetch that temporary byte.
- Stopped and selected shared-prescaler hold/reset paths maintain the
  newly separated committed high byte.

This is a **TCNT-only incremental model**; its new callbacks are enabled only
when the CPU timing flag is set on a 16-bit timer. The rest of the generic
16-bit register family is not yet part of this acceptance claim.

## GREEN strict execution and mutation controls

Railway project: `WeightOS simavr TEMP 20261118`
Docker deployment: `beafc8a5-fef1-4297-8582-84e0aedd26c1`
Exact tested commit: `f52512330bd5845c7e7f5d8027375442226c4471`
**Deployment status: SUCCESS**; no GitHub Actions were used.

```
TIMER1_TEMP reg=r17 actual=0x34 expected=0x34 equal=1
TIMER1_TEMP reg=r18 actual=0x12 expected=0x12 equal=1
TIMER1_TEMP reg=r19 actual=0x78 expected=0x78 equal=1
TIMER1_TEMP reg=r20 actual=0x12 expected=0x12 equal=1
TIMER1_TEMP reg=r21 actual=0xcd expected=0xcd equal=1
TIMER1_TEMP reg=r22 actual=0xab expected=0xab equal=1
TIMER1_TEMP_SUMMARY finished=1 cycles=32 mismatches=0
TIMER1_TEMP_STRICT_AND_MUTATIONS=PASS
```

Two independent deliberate corruptions of expected results were rejected
by the same strict comparator:

- `--negative-high`: forced expected r18 from 0x12 to 0x13:
  one mismatch, nonzero exit.
- `--negative-low`: forced expected r17 from 0x34 to 0x35:
  one mismatch, nonzero exit.

**Previous strict gates continue passing in the same build:**

| Gate | Result |
|---|---|
| Native simulator regression suite | 29 / 29 |
| Independent Timer1-only overflow scalar timing oracle | 6 / 6 cycles and 6 / 6 saved return PCs |
| Dual Timer0+Timer1 v1.26 differential (archived neural) | 13 / 13 IRQ events, 0 mismatches |
| Independent mapped-I/O clock edges | 9 / 9 ATmega328P; 9 / 9 on a manually opted-in ATmega2560 instance |
| Synthetic interrupt entry, two-/three-byte saved PC | 4 / 5 cycles, both pass |
| CPU two-cycle STS crossing peripheral event due before retirement | strict PASS (event at 2 observes pre-write value 0; write at 3; later event at 4 sees new value) |
| Dual IRQ differential expected-cycle and return-PC negative controls | both correctly rejected |
| Stopped Timer1 TEMP byte-value mutations | both correctly rejected |

The prior independent fresh native sparse-weight Neural Uno v1.26
1,200-step run reproduced the archived weights-based trace and matched
the corrected simavr 13 IRQ checkpoints. That fresh run was performed
against the earlier edge-ordering checkpoint; the **new TEMP-only change**
was not separately rerun through neural weights or ONNX Runtime, and
therefore does not add a new neural acceptance claim.

## Unresolved boundaries — conditions to expand acceptance

1. **Shared TEMP** across TCNT1, OCR1A/B and ICR1: mixed writes/read-latches
   must be tested without allowing OCR/ICR staging to corrupt the counter.
2. **Live Timer1 operation**: prescalers /1, /8, /64 etc., normal/CTC/PWM,
   compare match, input capture, overflow, TSM/PSRSYNC, simultaneous
   event/read/write edge collisions and nested interrupts.
3. Same-cycle CPU write vs cycle-timer event arbitration remains an
   experimental convention, not hardware-validated.
4. Native test coverage is not exhaustive and the opt-in additions change
   the public C struct ABI of libsimavr.
5. Silicon-level observations, ONNX Runtime replay and a complete
   instruction/peripheral conformance matrix have not been performed.
6. A stopped-timer TCNT-only fix is **not sufficient** to call the simavr
   fork a complete ATmega328P reference implementation.

## Decision

Retain this branch and its RED/GREEN and mutation fixtures. No merging
into `master` or `fix/atmega328p-shared-prescaler`, no upstream PR.
Next milestone: create standalone independent shared-TEMP
TCNT/OCR/ICR interleaving fixtures; move the timer implementation to a
shared logical TEMP model if those fixtures identify missing behavior;
then validate live prescaler and capture/compare modes and eventually
replay WeightOS's ONNX executable against the independently justified
simulator configuration.
