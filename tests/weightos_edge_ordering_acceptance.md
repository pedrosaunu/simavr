# WeightOS / simavr — instruction-crossing peripheral event order

Date: 2026-11-19 UTC.
Branch: `experiment/weightos-io-event-ordering`.
**Status: strict experimental regression gate PASS; NO silicon certification; NO PR.**

## Independent RED → GREEN fixture

- Pinned guest AVR source: `tests/weightos_multicycle_edge.S`.
- Driver: `tests/weightos_multicycle_edge_probe.c`.
- Guest: one-clock `LDI r16, 0xA5`, followed by two-clock
  `STS 0x003E, r16` (retirement at cycle 3).
- Native cycle-timer callbacks scheduled **independently** for cycles
  **2 and 4**. Expected bus visibility is derived from instruction
  timing, not from existing WeightOS or simavr traces.

Initial experiment with the previous simulator and corrected fixture:
```
EDGE_WRITE clock=3 value=0xa5
EDGE_EVENT index=1 when=2 clock=3 value=0xa5
EDGE_EVENT index=2 when=4 clock=4 value=0xa5
EDGE_ORDER_SUMMARY strict=0 known_red=1 ticks=2 writes=1
```
That is a real error: callback due at cycle 2 was delayed until clock 3,
after the guest's store became visible.

## Scheduling change

`simavr/sim/sim_cycle_timers.c` now implements
`avr_cycle_timer_process_before(avr, retirement_cycle)` to dispatch
already-due events **strictly earlier than** a guest I/O writeback.
Callbacks execute with `avr->cycle` equal to their respective scheduled
cycle, so any newly registered cycle timers are relative to the correct
clock. A callback requesting `TIMER_RESET` resets the device and
cancels the CPU's uncommitted pending store.

The opted-in instruction retirement path invokes this before making a
guest mapped-I/O store visible. Events scheduled **exactly on the same
retirement cycle** are deliberately left for the outer event loop, i.e.
CPU store first, event second. This is a specified **experimental
tie-break** and requires an independent chip/logic-analyzer test before
it can be claimed as silicon-equivalent for every peripheral.

Latest independently executed Railway Docker build:
`3183e02b-9f2a-49e1-bcec-601a49789fc0`
(commit `be71e04c91053f79b00a4e0a3183aba6038f2abb`), **SUCCESS**.
```
EDGE_EVENT index=1 when=2 clock=2 value=0x00
EDGE_WRITE clock=3 value=0xa5
EDGE_EVENT index=2 when=4 clock=4 value=0xa5
EDGE_ORDER_SUMMARY strict=1 known_red=0 ticks=2 writes=1
EDGE_ORDER_STRICT=PASS
```

Existing strict gates in that same build:

- Native simavr regressions: 29/29.
- Isolated Timer1 independent scalar oracle: 6/6 vector clocks and saved PCs.
- Dual Timer0/Timer1 archived neural differential: 13/13 IRQ vectors,
  clocks, saved PCs and regfile checks.
- Mapped I/O callback edges: 9/9 on ATmega328P and 9/9 on an
  explicitly opted-in ATmega2560 instance.
- Synthetic 2- and 3-byte return-PC interrupt entry: 4 and 5 clocks.
- Mutated-cycle and mutated-return-PC negative controls: both rejected.

## Fresh neural execution, separate from the simulator build

The exact archived experimental Neural Uno v1.26 C sparse-weight
runner source (`integrations/simavr/neural_c_state_trace_v118.c`) was
**recompiled and run** with 1,200 steps in an independent local Linux
executor, using:

- Neural model weight SHA-256:
  `a0f3efb8ce2d866ed6cf6c4a7d77e9198b1acb9b22f29b2594d5e115b89c19bb`.
- Guest firmware SHA-256:
  `ad15fee7130e39a7a1fc87d48a0eb95bd15709f85c3a7e6708fa9bf799b698c2`.
- 58,053 neurons, 137,914 taps, 1,345 recurrent state bits,
  no guest instruction interpreter in this engine.
- Re-executed 13 selected IRQ state snapshots to extract actual guest
  stack return-PC words, not merely stored trace labels.

Compared with the **separately built and executed fork** strict probe
for the identical firmware, the fresh neural run has:
`13/13` vector clocks, `13/13` vectors,
`13/13` saved guest return PCs and `13/13` full register-file
FNV-1a fingerprints matching. The fresh 1,200-row trace and final
bitstate also match their original archived counterpart byte for byte.

The independent replay is **native sparse-weight execution**, not ONNX
Runtime. The current Docker gate contains archived neural reference
vectors, while this second replay independently regenerates them from
weights. Neither constitutes on-chip physical timing proof.

## Remaining acceptance boundaries

- CPU read edges and 16-bit Timer1 TEMP register high-byte staging
  remain outside this gate.
- This fixture does not exercise simultaneous event/write at the
  *same* cycle; the tie-break is chosen, not measured on silicon.
- Watchdog resets, CTC/phase-correct PWM, Timer2 AS2 and priority
  overlap require independent hardware-derived fixtures.
- Full ATmega328P/Arduino Uno conformance and ONNX runtime replay
  remain unclaimed.
- The implementation is opt-in for ATmega328/328P and changes libsimavr's
  C ABI through the preceding opt-in timing-feature field.
- No GitHub Actions or upstream PR were used; master and the original
  prescaler-fix branch were not changed.

**Decision:** accept this experimental causal order repair and
regression fixture for the currently tested subset; do not merge to
production or open an upstream PR until wider timing modes and silicon
edges have been independently tested.
