/* NU-021J: independent real AVR software witness for capture and
 * overflow/OCF contention, masked pending bits, dynamic TIMSK1, priorities,
 * stack and post-SEI/RETI delivery. Host ONLY raises external PB0 level.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_STEPS 450
#define FIELD_BYTES 21
static int level_at(unsigned long long age) {
  return age>=8 && age<17;
}
static unsigned popcount(unsigned x) {
  unsigned n=0;while(x){n+=x&1U;x>>=1;}return n;
}
static int test(const char*filename,unsigned c){
  uint8_t image[32768]={0};
  FILE*f=fopen(filename,"rb");if(!f)return 2;
  size_t size=fread(image,1,sizeof(image),f);fclose(f);
  if(size<0x160||size>=sizeof(image)||(size%2U))return 2;
  avr_t *a=avr_make_mcu_by_name("atmega328p");
  if(!a || avr_init(a))return 3;
  a->frequency=16000000;
  avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
  if(!pin){avr_terminate(a);return 3;}
  avr_raise_irq(pin,0);
  avr_loadcode(a,image,(uint32_t)size,0);
  uint8_t rows[MAX_STEPS][FIELD_BYTES]={{0}};
  unsigned sp_init=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
  unsigned last_sp=sp_init,last_pc=0xffff;
  unsigned vec_cap=0,vec_a=0,vec_b=0,vec_ovf=0,reti=0;
  unsigned steps=0,ready=0,done=0,pending_mask_at_enable=0;
  unsigned expected_mask=c==0?0x07:(c==1?0x03:0x05);
  unsigned first_mask=c==0?0x27:(c==1?0x23:0x25);
  unsigned expected_total=1+popcount(expected_mask);
  unsigned mask_enabled=0,capture_mask_written=0,irq_sequence[5]={0},seq_n=0;
  int level=0;
  uint64_t ready_cycle=0;
  for(unsigned i=1;i<=MAX_STEPS;i++){
    int rc=avr_run(a);
    if(rc!=cpu_Running && rc!=cpu_Sleeping){
      fprintf(stderr,"NU021J_STOP case=%u step=%u rc=%d\n",c,i,rc);
      avr_terminate(a);return 4;
    }
    unsigned pc=(unsigned)a->pc/2U;
    unsigned sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
    unsigned saved=(unsigned)a->data[sp_init] + 256U*(unsigned)a->data[sp_init-1];
    if(pc==0x14&&last_pc!=0x14){vec_cap++;if(seq_n<5)irq_sequence[seq_n++]=0x14;}
    if(pc==0x16&&last_pc!=0x16){vec_a++;if(seq_n<5)irq_sequence[seq_n++]=0x16;}
    if(pc==0x18&&last_pc!=0x18){vec_b++;if(seq_n<5)irq_sequence[seq_n++]=0x18;}
    if(pc==0x1a&&last_pc!=0x1a){vec_ovf++;if(seq_n<5)irq_sequence[seq_n++]=0x1a;}
    if(last_sp<sp_init && sp==sp_init)reti++;
    last_sp=sp;last_pc=pc;steps=i;
    if(!ready && a->data[0x100]==0xa5){ready=1;ready_cycle=a->cycle;}
    unsigned timsk=a->data[0x6f],flags=a->data[0x36];
    if(!mask_enabled && timsk==first_mask){
      mask_enabled=1;pending_mask_at_enable=flags&0x27;
    }
    if(vec_cap && timsk==expected_mask)capture_mask_written=1;
    uint8_t *v=rows[i-1];
    uint64_t cycles=a->cycle;
    v[0]=i&255U;v[1]=(i>>8)&255U;
    v[2]=pc&255U;v[3]=(pc>>8)&255U;
    v[4]=cycles&255U;v[5]=(cycles>>8)&255U;
    for(unsigned k=0;k<6;k++)v[6+k]=a->data[20+k];
    v[12]=flags;v[13]=timsk;v[14]=a->sreg[S_I];
    v[15]=(uint8_t)(sp_init-sp);
    v[16]=saved&255U;v[17]=(saved>>8)&255U;
    v[18]=(uint8_t)level;
    v[19]=(uint8_t)a->data[0x86];v[20]=(uint8_t)a->data[0x87];
    /* Snapshot BEFORE the next PB0 transition, matching NU-021G v2. */
    int desired=ready?level_at(a->cycle-ready_cycle):0;
    if(desired!=level){
      avr_raise_irq(pin,(uint32_t)desired);
      level=desired;
      printf("NU021J_PIN case=%u step=%u cycle=%llu level=%d\n",
             c,i,(unsigned long long)a->cycle,level);
    }
    int done_counts=(a->data[20]==1 && a->data[21]==((expected_mask&2)?1:0) &&
                     a->data[22]==((expected_mask&4)?1:0) && a->data[23]==1);
    if(done_counts && a->sreg[S_I] && sp==sp_init && capture_mask_written){
      done=1;break;
    }
  }
  unsigned masks_ok=mask_enabled && pending_mask_at_enable==0x27;
  unsigned order_ok=seq_n==expected_total && irq_sequence[0]==0x14;
  unsigned k=1;
  if(expected_mask&2)order_ok &= irq_sequence[k++]==0x16;
  if(expected_mask&4)order_ok &= irq_sequence[k++]==0x18;
  order_ok &= irq_sequence[k++]==0x1a;
  unsigned flags_final=a->data[0x36];
  unsigned unmasked_remaining=(flags_final & expected_mask);
  int pass=done && masks_ok && order_ok && ready && capture_mask_written &&
           reti>=expected_total && !unmasked_remaining;
  for(unsigned first=0;first<steps;first+=8){
    unsigned n=steps-first<8?steps-first:8;
    for(unsigned copy=0;copy<2;copy++){
      printf("NU021J_PACK case=%u first=%u rows=%u hex=",c,first+1,n);
      for(unsigned j=0;j<n;j++)for(unsigned b=0;b<FIELD_BYTES;b++)
        printf("%02x",(unsigned)rows[first+j][b]);
      putchar('\n');
    }
  }
  printf("NU021J_SUMMARY case=%u steps=%u ready=%u pending=%02x "
         "mask_before=%02x mask_after=%02x cap=%u a=%u b=%u ovf=%u "
         "reti=%u seq=%02x,%02x,%02x,%02x final_tifr1=%02x pass=%d\n",
         c,steps,ready,pending_mask_at_enable,first_mask,expected_mask,
         vec_cap,vec_a,vec_b,vec_ovf,reti,
         irq_sequence[0],irq_sequence[1],irq_sequence[2],irq_sequence[3],
         flags_final,pass);
  avr_terminate(a);
  return pass?0:1;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 unsigned c=(unsigned)strtoul(argv[2],NULL,0);
 if(c>2)return 2;
 return test(argv[1],c);
}
