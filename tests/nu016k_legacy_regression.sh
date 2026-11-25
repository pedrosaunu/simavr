#!/bin/sh
# Reuse legacy Dockerfile probe commands; continue across groups to record failures.
set -u
fail=0

echo NU016K_LEGACY_BEGIN ctc
(
set -eux; cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu015a_ctc_probe.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe; \
 avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp tests/weightos_nu015a_ctc.S -o /tmp/spm.elf; \
 avr-objcopy -O binary -j .text /tmp/spm.elf /tmp/spm.bin; sha256sum /tmp/spm.bin; \
 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/spm.bin
) >/tmp/legacy-ctc.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT ctc $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-ctc.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-ctc.log; fi

echo NU016K_LEGACY_BEGIN capture
(
set -eux; \
 cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim \
  tests/weightos_nu021g_capture_compare_probe.c \
  -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/nu021g_probe; \
 for mask in 32 34 38; do \
  avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DNU021G_MASK=$mask \
   -x assembler-with-cpp tests/weightos_nu021g_capture_compare.S \
   -o /tmp/nu021g-$mask.elf; \
  avr-objcopy -O binary -j .text /tmp/nu021g-$mask.elf /tmp/nu021g-$mask.bin; \
  sha256sum /tmp/nu021g-$mask.bin; \
  echo NU021G_WORDS_BEGIN_$mask; \
  od -An -tu2 -v /tmp/nu021g-$mask.bin | xargs; \
  echo NU021G_WORDS_END_$mask; \
  LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
   /tmp/nu021g_probe /tmp/nu021g-$mask.bin $mask; \
 done; \
 echo NU021G_REFERENCE_ALL_MASKS_PASS
) >/tmp/legacy-capture.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT capture $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-capture.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-capture.log; fi

echo NU016K_LEGACY_BEGIN prescaler
(
set -eux; \
 cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim \
   tests/weightos_nu021i_prescaler_probe.c \
   -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/nu021i_probe; \
 fail=0; \
 for case in 0 1 2 3 4 5 6 7 8 9 10 11; do \
  avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DNU021I_CASE=$case \
    -x assembler-with-cpp tests/weightos_nu021i_prescaler.S \
    -o /tmp/nu021i-$case.elf; \
  avr-objcopy -O binary -j .text /tmp/nu021i-$case.elf /tmp/nu021i-$case.bin; \
  sha256sum /tmp/nu021i-$case.bin; \
  echo NU021I_WORDS_BEGIN_$case; \
  od -An -tu2 -v /tmp/nu021i-$case.bin | xargs; \
  echo NU021I_WORDS_END_$case; \
  LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
    /tmp/nu021i_probe /tmp/nu021i-$case.bin $case || fail=$((fail+1)); \
 done; \
 echo NU021I_REFERENCE_SUMMARY case_count=12 failed=$fail; \
 test "$fail" -eq 0; \
 echo NU021I_TWELVE_REAL_FIRMWARE_REFERENCE_PASS
) >/tmp/legacy-prescaler.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT prescaler $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-prescaler.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-prescaler.log; fi

echo NU016K_LEGACY_BEGIN contention
(
set -eux; cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim tests/weightos_nu021o_contention_probe.c -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf -o /tmp/probe; \
 avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -x assembler-with-cpp tests/weightos_nu021o_contention.S -o /tmp/o.elf; \
 avr-objcopy -O binary -j .text /tmp/o.elf /tmp/o.bin; sha256sum /tmp/o.bin; \
 echo NU021O_WORDS_BEGIN; od -An -tu2 -v /tmp/o.bin | xargs; echo NU021O_WORDS_END; \
 LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu /tmp/probe /tmp/o.bin; echo NU021O_REFERENCE_PASS
) >/tmp/legacy-contention.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT contention $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-contention.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-contention.log; fi

echo NU016K_LEGACY_BEGIN w1c
(
set -eux; \
 cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim \
   tests/weightos_timer1_nu020_access_probe.c \
   -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
   -o /tmp/nu020_access_probe; \
 for running in 0 1; do \
  for style in 0 1 2 3 4 5 6; do \
   avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start \
      -DCLEAR_STYLE=$style -DRUNNING_TIMER=$running \
      -x assembler-with-cpp tests/weightos_timer1_nu020_access.S \
      -o /tmp/nu020-$running-$style.elf; \
   avr-objcopy -O binary -j .text /tmp/nu020-$running-$style.elf /tmp/nu020-$running-$style.bin; \
   echo NU020_BINARY_BEGIN; \
   sha256sum /tmp/nu020-$running-$style.bin; \
   od -An -tu2 -v /tmp/nu020-$running-$style.bin | xargs; \
   echo NU020_BINARY_END; \
   LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
    /tmp/nu020_access_probe /tmp/nu020-$running-$style.bin $style $running; \
  done; \
 done; \
 echo NU020_INDEPENDENT_14_GUEST_MATRIX_COMPLETE; \
 echo NU020_TIMER1_STOP_RETAINED_COUNTER_REFERENCE_PASS
) >/tmp/legacy-w1c.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT w1c $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-w1c.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-w1c.log; fi

echo NU016K_LEGACY_BEGIN gtccr
(
set -eux; \
  for n in 3 11; do \
    avr-gcc -mmcu=atmega328p -nostdlib -Wl,-e,start -DNOP_COUNT=$n \
      -x assembler-with-cpp tests/weightos_gtccr_hold_release.S \
      -o /tmp/gtccr-$n.elf; \
    avr-objcopy -O binary -j .text /tmp/gtccr-$n.elf /tmp/gtccr-$n.bin; \
    sha256sum /tmp/gtccr-$n.bin; \
  done; \
  cc -std=c11 -O2 -Wall -Wextra -Werror -Isimavr/sim \
    tests/weightos_gtccr_hold_release_probe.c \
    -Lsimavr/obj-x86_64-linux-gnu -lsimavr -lelf \
    -o /tmp/gtccr_probe; \
  LD_LIBRARY_PATH=simavr/obj-x86_64-linux-gnu \
    /tmp/gtccr_probe /tmp/gtccr-3.bin /tmp/gtccr-11.bin --strict; \
  echo GTCCR_TSM_PSR_SYNC_EXTERNAL_STRICT_PASS
) >/tmp/legacy-gtccr.log 2>&1
rc=$?
echo NU016K_LEGACY_RESULT gtccr $rc
if [ "$rc" -ne 0 ]; then tail -n 30 /tmp/legacy-gtccr.log; fail=$((fail+1)); else tail -n 3 /tmp/legacy-gtccr.log; fi
echo "NU016K_LEGACY_SUMMARY failed=$fail total=6"
# Preserve all group results; acceptance is determined from explicit result records.
exit 0
