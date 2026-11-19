/* Byte-exact WeightOS Neural Uno v1.28 guest fixture, 15 instructions.
 * Source: pedrosaunu/weightos scripts/accept_onnx_recurrent_v127.py
 * WORDS_SHARED_TEMP. SHA-256 is enforced by Dockerfile.shared-temp.
 * This is a comparison input, not a substituted simulator oracle.
 */
.text
.global start
start:
    .word 0xe108, 0x9300, 0x0081
    .word 0xe002, 0x9300, 0x0080
    .word 0xe102, 0x9300, 0x0089
    .word 0xe304, 0x9300, 0x008a
    .word 0xe506, 0x9300, 0x0085
    .word 0xe708, 0x9300, 0x0086
    .word 0x9110, 0x0089
    .word 0x9120, 0x0087
    .word 0x9598
