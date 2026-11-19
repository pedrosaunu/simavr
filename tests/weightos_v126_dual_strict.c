/* WeightOS v1.26 dual-timer differential conformance gate.
 * This is a PINNED NEURAL MODEL differential test, NOT a datasheet oracle.
 * Predicted vector cycles, return PCs, registers and GPIO are copied from
 * the archived v1.26 neural evaluator evidence. The independent scalar
 * oracle is used separately for the Timer1-only test.
 * Firmware SHA-256 ad15fee7130e39a7a1fc87d48a0eb95bd15709f85c3a7e6708fa9bf799b698c2
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t firmware[320] = {
  [0]=0x7f, [1]=0xc0, [52]=0x79, [53]=0xc0, [64]=0x7c, [65]=0xc0,
  [256]=0x0f, [257]=0xef, [258]=0x04, [259]=0xb9, [260]=0x01, [261]=0xe0,
  [263]=0x93, [264]=0x6f, [267]=0x93, [268]=0x6e, [270]=0x05, [271]=0xbd,
  [273]=0x93, [274]=0x81, [276]=0x0f, [277]=0xef, [279]=0x93, [280]=0x85,
  [283]=0xe0, [285]=0x93, [286]=0x84, [288]=0x06, [289]=0xbd,
  [290]=0x78, [291]=0x94, [294]=0xfe, [295]=0xcf,
  [296]=0x53, [297]=0x95, [298]=0x55, [299]=0xb9,
  [300]=0x0f, [301]=0xef, [303]=0x93, [304]=0x85,
  [307]=0xe0, [309]=0x93, [310]=0x84, [312]=0x18, [313]=0x95,
  [314]=0x63, [315]=0x95, [316]=0x65, [317]=0xb9,
  [318]=0x18, [319]=0x95
};
static const uint64_t reference_cycles[13] = {
    279, 298, 535, 549, 792, 819, 1047, 1089, 1303, 1359, 1560, 1629, 1815
};
static const uint8_t reference_vectors[13] = {
    26, 32, 32, 26, 32, 26, 32, 26, 32, 26, 32, 26, 32
};
static const uint8_t reference_saved_pc[13] = {
    146, 147, 147, 146, 146, 146, 146, 146, 147, 146, 146, 146, 146
};
static const uint8_t reference_portb[13] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6
};
/* FNV-1a of all 32 guest architectural register bytes, in AVR R0..R31 order.
 * This is an additional noncryptographic regression check, not a replacement
 * for byte-for-byte register comparisons or independent silicon testing.
 */
static const uint32_t reference_register_hash[13] = {
    0x0b2ae445U,0x2479f29aU,0x7d01b123U,0x736a7588U,0x8cb983ddU,
    0x3431c554U,0x60af4adfU,0xf4f91520U,0xd28617bdU,0x79fe5934U,
    0x801ef053U,0x7687b4b8U,0x5414b755U
};
static uint32_t hash_regs(const avr_t *a) {
    uint32_t h = 2166136261U;
    for (unsigned i = 0; i < 32; ++i) h = (h ^ a->data[i]) * 16777619U;
    return h;
}
int main(int argc, char **argv) {
    const int inject_cycle = argc == 2 && !strcmp(argv[1], "--negative-cycle");
    const int inject_pc = argc == 2 && !strcmp(argv[1], "--negative-return-pc");
    if (argc > 2 || (argc == 2 && !inject_cycle && !inject_pc)) return 2;
    avr_t *a = avr_make_mcu_by_name("atmega328p");
    if (!a || avr_init(a)) return 2;
    a->frequency = 16000000;
    avr_loadcode(a, (uint8_t *)firmware, sizeof firmware, 0);
    unsigned events = 0, mismatches = 0;
    for (unsigned step=1; step<=1200; ++step) {
        const int status = avr_run(a);
        if (status != cpu_Running) { mismatches++; break; }
        if (a->pc != 0x34 && a->pc != 0x40) continue;
        if (events >= 13) { mismatches++; break; }
        unsigned i=events++;
        const unsigned vector=(unsigned)(a->pc >> 1);
        const unsigned saved_pc=(unsigned)a->data[0x8ff] | ((unsigned)a->data[0x8fe] << 8);
        const unsigned sp=(unsigned)a->data[0x5d] | ((unsigned)a->data[0x5e] << 8);
        const unsigned sreg = ((unsigned)!!a->sreg[0] << 0) |
           ((unsigned)!!a->sreg[1] << 1) | ((unsigned)!!a->sreg[2] << 2) |
           ((unsigned)!!a->sreg[3] << 3) | ((unsigned)!!a->sreg[4] << 4) |
           ((unsigned)!!a->sreg[5] << 5) | ((unsigned)!!a->sreg[6] << 6) |
           ((unsigned)!!a->sreg[7] << 7);
        const uint64_t cycle_expected = reference_cycles[i] + (inject_cycle && i==0);
        const unsigned pc_expected = reference_saved_pc[i] + (inject_pc && i==0);
        const uint32_t regs = hash_regs(a);
        int ok = a->cycle == cycle_expected &&
            vector == reference_vectors[i] &&
            saved_pc == pc_expected && sp == 2301 && sreg == 0 &&
            a->data[0x24] == 255 && a->data[0x25] == reference_portb[i] &&
            regs == reference_register_hash[i];
        if (!ok) mismatches++;
        printf("WEIGHTOS_DUAL_STRICT event=%u vector=%u cycle=%llu expected=%llu "
               "saved_pc=%u expected_pc=%u sp=%u sreg=%u portb=%u "
               "reg_hash=%08x expected_hash=%08x equal=%d\n",
               events,vector,(unsigned long long)a->cycle,
               (unsigned long long)cycle_expected,saved_pc,pc_expected,sp,
               sreg,(unsigned)a->data[0x25],regs,reference_register_hash[i],ok);
    }
    avr_terminate(a);
    printf("WEIGHTOS_DUAL_STRICT_SUMMARY events=%u mismatches=%u\n",events,mismatches);
    return events == 13 && !mismatches ? 0 : 1;
}
