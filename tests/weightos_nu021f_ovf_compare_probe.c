/* NU-021F independent executing-guest Timer1 OVF/COMPA/COMPB.
 * STRICT: firmware-generated events only, complete cycle/stack/GPR trace.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STEPS 1000

static int test_case(const char *path, unsigned mask) {
    uint8_t image[32768] = {0};
    FILE *f = fopen(path,"rb");
    if (!f) {perror(path);return 2;}
    size_t n = fread(image,1,sizeof(image),f);
    fclose(f);
    if (n < 0xb0 || n >= sizeof(image)) return 2;
    avr_t *a = avr_make_mcu_by_name("atmega328p");
    if (!a || avr_init(a)) return 3;
    a->frequency=16000000;
    avr_loadcode(a,image,(uint32_t)n,0);
    unsigned sp_init = (unsigned)a->data[0x5d] + 256U*a->data[0x5e];
    unsigned last_pc=0xffff,last_sp=sp_init,vec_a=0,vec_b=0,vec_o=0,reti=0;
    unsigned done=0,steps=0;
    for (unsigned i=0;i<MAX_STEPS;++i){
        int status=avr_run(a);
        if(status!=cpu_Running && status!=cpu_Sleeping){
            fprintf(stderr,"NU021F_HALTED mask=%02x step=%u status=%d\n",mask,i,status);
            avr_terminate(a);return 4;
        }
        unsigned pc=(unsigned)a->pc/2U;
        unsigned sp=(unsigned)a->data[0x5d] + 256U*a->data[0x5e];
        unsigned saved = (sp_init>=0x100 && sp_init<0x900)
            ? (unsigned)a->data[sp_init] + 256U*(unsigned)a->data[sp_init-1]
            : 0;
        if(pc==0x16 && last_pc!=0x16) ++vec_a;
        if(pc==0x18 && last_pc!=0x18) ++vec_b;
        if(pc==0x1a && last_pc!=0x1a) ++vec_o;
        if(last_sp<sp_init && sp==sp_init) ++reti;
        last_sp=sp;last_pc=pc;steps=i+1;
        printf("NU021F_TRACE mask=%02x step=%u pc_word=%04x cycle=%llu "
               "r21=%02x r22=%02x r23=%02x tifr1=%02x timsk1=%02x "
               "I=%u sp_delta=%d saved_pc=%04x\n",
               mask,steps,pc,(unsigned long long)a->cycle,
               (unsigned)a->data[21],(unsigned)a->data[22],
               (unsigned)a->data[23],(unsigned)a->data[0x36],
               (unsigned)a->data[0x6f],(unsigned)a->sreg[S_I],
               (int)sp_init-(int)sp,saved);
        if(a->data[23]>=3 &&
           a->data[21]>=((mask&2)?3U:0U) &&
           a->data[22]>=((mask&4)?3U:0U) &&
           sp==sp_init && a->sreg[S_I]){done=1;break;}
    }
    int ok=done && a->data[23]>=3 && vec_o>=3 &&
        a->data[21]==((mask&2)?3U:0U) &&
        a->data[22]==((mask&4)?3U:0U) &&
        vec_a>=((mask&2)?3U:0U) && vec_b>=((mask&4)?3U:0U) &&
        reti>=((mask&2)?6U:3U)+((mask&4)?3U:0U);
    printf("NU021F_SUMMARY mask=%02x steps=%u ovf=%u compa=%u compb=%u "
           "vectors_ovf=%u vectors_a=%u vectors_b=%u reti=%u pass=%d\n",
           mask,steps,(unsigned)a->data[23],(unsigned)a->data[21],
           (unsigned)a->data[22],vec_o,vec_a,vec_b,reti,ok);
    avr_terminate(a);
    return ok?0:1;
}
int main(int argc,char**argv){
    if(argc!=3)return 2;
    unsigned mask=(unsigned)strtoul(argv[2],NULL,0);
    if(mask!=1 && mask!=3 && mask!=7)return 2;
    return test_case(argv[1],mask);
}
