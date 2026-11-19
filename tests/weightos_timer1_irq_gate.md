# WeightOS v1.26 / simavr — isolated Timer1 IRQ timing gate

**Status:** reproducible negative control, **not a fix** and **not CPU conformance acceptance**.

## Provenance

- Candidate simulator: `pedrosaunu/simavr`, branch `test/weightos-timer1-irq-latency-gate`, source build based on `fix/atmega328p-shared-prescaler`.
- Independent WeightOS artifact: `WEIGHTOS_V126_TIMER1_IRQ_BOUNDARY_ORACLE.md` and exact firmware `reports/timer_isolation_v125/timer1_only.S`.
- Pinned source: `tests/weightos_timer1_only_v126.S`, original source SHA-256 `bad0be45fe915249d5c1488a4ef7455ce5914b5a46c4c5a4a742d4129631989d`.
- Pinned compiled `.text` 318-byte image SHA-256 `02b3bb49a1ea0d535187c23473f2fa220b75a9e28f8597560df554ae80b91177`.
- Isolated Railway build deployment: `9160f514-cb4c-4386-84a5-7c11d1d33a90` in `WeightOS simavr TEMP 20261118`, GitHub source commit `3d97dc55ce1348765345cd362a8a710adb8d2daa`.
- Execution date: **2026-11-19 (UTC)**. No GitHub Actions were used.

## Reproduction

The Dockerfile compiles `libsimavr` and the complete native suite, assembles the pinned AVR firmware, checks the compiled bytes' SHA-256, and invokes `tests/weightos_timer1_irq_oracle_probe.c`. It evaluates actual guest AVR interrupt-vector transitions by observing `avr->pc`, `avr->cycle`, and pushed SRAM return-PC bytes after native `avr_run()` calls.

The `--expect-known-red` mode passes **only** if the historical failure has been preserved exactly: six Timer1 entries, zero cycle matches, and three return-PC matches. It is a **negative control**. Use `--strict` for a conformance check: unless all six cycle and saved-PC values match the independent oracle it returns non-zero.

| IRQ | Independent predicted vector cycle | Actual fork cycle | Independent predicted saved word PC | Actual fork saved word PC |
|---:|---:|---:|---:|---:|
| 1 | 278 | 271 | 0x0091 | 0x0091 |
| 2 | 548 | 535 | 0x0091 | 0x0092 |
| 3 | 818 | 800 | 0x0091 | 0x0091 |
| 4 | 1088 | 1064 | 0x0091 | 0x0092 |
| 5 | 1358 | 1329 | 0x0091 | 0x0091 |
| 6 | 1628 | 1593 | 0x0091 | 0x0092 |

### Actual Railway Docker build checks

- Native tests: **29 passed, 0 failed**.
- Shared /8 phase probes: `nop=3` => Timer0=1, Timer1=1; `nop=11` => Timer0=2, Timer1=2.
- Combined /1 v1.26 probe: **13 IRQ events**, 7 Timer0 and 6 Timer1.
- Isolated Timer1-only comparison: **6 events**, **0/6 cycle equal**, **3/6 saved return-PC equal**; **negative control passed**.
- Pinned firmware SHA-256: **OK**.
- Railway image build completed all Dockerfile RUN gates. These results are from the **image build**, not from a physical Arduino or from an interactive live container.

## Causal interpretation and boundaries

The fork modifies the shared synchronous Timer0/1 prescaler (and GTCCR TSM/PSRSYNC behavior), which is pertinent when division exceeds one. The isolated Timer1 experiment uses clock division /1. Therefore prescaler correctness cannot resolve this mismatch on its own.

The current `simavr/sim/sim_interrupts.c` services a pending IRQ by pushing the return address, clearing SREG.I, and switching the program counter to the vector, **without charging four simulated CPU cycles for entry at that point**; native `avr_callback_run_raw()` calls this routine after an instruction and cycle timer processing. The WeightOS scalar oracle independently models four interrupt-entry cycles and derives six predicted vector boundaries on this exact firmware. The neural engine matched those six cycles and saved PCs in the earlier v1.26 isolated experiment. This strongly motivates examining IRQ entry scheduling and Timer1 overflow/reload ordering in the reference, but **does not establish exact silicon timing**, nor does it imply that adding four cycles at one C statement will repair the simulation.

## Promotion gates

1. Implement any IRQ timing candidate on a **separate, disposable branch**, not in `fix/atmega328p-shared-prescaler` and not in WeightOS neural weights.
2. Drive **RED→GREEN** on the strict six-vector cycle/return-PC gate, using the same pinned firmware and preserving native 29/29 and shared phase behavior.
3. Repeat combined Timer0/Timer1, simultaneous pending IRQs, Timer1 /1, /8, /64, TCNT1 high/low staging, TSM hold/release, CTC/PWM, overflow, and other MCU families to catch regressions.
4. Obtain datasheet-grounded or measured **physical ATmega328P** timing for the disputed interrupt vector and TCNT1 reload boundary; keep simulation predictions and silicon measurements explicitly distinct.
5. Only then consider submitting a narrowly scoped, documented upstream simavr PR. Do **not** promote v1.26 to full ATmega328P compatibility or erase the pre-existing strict WeightOS gate.

**Decision:** prescaler branch is a useful candidate; isolated IRQ timing remains OPEN. This negative control is ready to reuse during root-cause repair.
