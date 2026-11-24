/* NU-021K software reference: observe real guest AVR, apply only
 * asynchronous PB0 after a retired instruction. The offsets are against
 * FIRST CPU-visible TOV1 rising edge. A drive at offset=0 occurs AFTER
 * that retired instruction's timer updates, NOT proven simultaneous
 * with an actual silicon timer clock edge. This limitation is explicit.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_STEPS 160
#define FIELD_BYTES 22

static int run(const char *file,int offset,int preflight,uint64_t reference_tov){
  uint8_t img[32768]={0};
  FILE *f=fopen(file,"rb");if(!f)return 2;
  size_t bytes=fread(img,1,sizeof(img),f);fclose(f);
  if(bytes<0xa0 || bytes>=sizeof(img) || (bytes&1))return 2;
  avr_t *a=avr_make_mcu_by_name("atmega328p");
  if(!a||avr_init(a))return 3;
  a->frequency=16000000;
  avr_irq_t *pb0=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
  if(!pb0){avr_terminate(a);return 3;}
  avr_raise_irq(pb0,0);avr_loadcode(a,img,(uint32_t)bytes,0);
  uint8_t packed[MAX_STEPS][FIELD_BYTES]={{0}};
  uint64_t tov_cycle=0,compa_cycle=0,compb_cycle=0,ready_cycle=0,actual_edge_cycle=0;
  unsigned prev_flags=0,ready=0,observations=0,done=0,icf_seen=0,pre_guard=0;
  int level=0;unsigned final_flags=0,final_icr=0;
  unsigned init_sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
  uint64_t requested=reference_tov+(int64_t)offset;
  if(preflight)requested=UINT64_MAX;
  for(unsigned i=1;i<=MAX_STEPS;i++){
    int rc=avr_run(a);
    if(rc!=cpu_Running&&rc!=cpu_Sleeping){
      fprintf(stderr,"NU021K_STOP offset=%d step=%u rc=%d\n",offset,i,rc);
      avr_terminate(a);return 4;
    }
    unsigned pc=(unsigned)a->pc/2U;
    unsigned flags=a->data[0x36];
    unsigned sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
    unsigned icr=(unsigned)a->data[0x86]+256U*(unsigned)a->data[0x87];
    uint64_t cycle=a->cycle;
    if(!ready && a->data[0x100]==0xa5){ready=1;ready_cycle=cycle;}
    if(!tov_cycle && (flags&1U) && !(prev_flags&1U))tov_cycle=cycle;
    if(!compa_cycle && (flags&2U) && !(prev_flags&2U))compa_cycle=cycle;
    if(!compb_cycle && (flags&4U) && !(prev_flags&4U))compb_cycle=cycle;
    if(flags&0x20U)icf_seen=1;
    prev_flags=flags;
    if(!preflight){
      uint8_t *v=packed[i-1];
      v[0]=(uint8_t)i;v[1]=(uint8_t)(i>>8);
      v[2]=(uint8_t)pc;v[3]=(uint8_t)(pc>>8);
      v[4]=(uint8_t)cycle;v[5]=(uint8_t)(cycle>>8);
      for(unsigned k=0;k<7;k++)v[6+k]=a->data[20+k];
      v[13]=(uint8_t)flags;
      v[14]=a->data[0x6f];
      v[15]=a->sreg[S_I];
      v[16]=(uint8_t)(init_sp-sp);
      v[17]=(uint8_t)level;
      v[18]=(uint8_t)icr;v[19]=(uint8_t)(icr>>8);
      v[20]=a->data[0x81];
      v[21]=0;
      observations=i;
    }
    if(!preflight && !level && ready && cycle>=requested){
      if(cycle!=requested){
         fprintf(stderr,"NU021K_BAD_BOUNDARY offset=%d expected=%llu observed=%llu\n",
                 offset,(unsigned long long)requested,(unsigned long long)cycle);
         avr_terminate(a);return 5;
      }
      /* The pin, not TIFR1 or CPU state, is the sole external input. */
      avr_raise_irq(pb0,1);level=1;actual_edge_cycle=cycle;
      printf("NU021K_PIN offset=%d step=%u cycle=%llu level=1\n",
             offset,i,(unsigned long long)cycle);
    }
    if(pc>=0x3b && ready && a->data[0x81]==0x40) {
      /* Stop written by guest; confirm ICR1 and flags then finish. */
      if(a->data[26]==flags && a->data[24]==(icr&255U) &&
          a->data[25]==((icr>>8)&255U)){done=1;final_flags=flags;final_icr=icr;break;}
    }
  }
  if(preflight){
    int good=ready && tov_cycle && compa_cycle && compb_cycle &&
             compa_cycle<tov_cycle && compb_cycle>=tov_cycle &&
             (tov_cycle>ready_cycle+4);
    printf("NU021K_PREFLIGHT ready=%llu compa=%llu tov=%llu compb=%llu passed=%d\n",
      (unsigned long long)ready_cycle,
      (unsigned long long)compa_cycle,
      (unsigned long long)tov_cycle,
      (unsigned long long)compb_cycle,good);
    avr_terminate(a);return good?0:1;
  }
  if(!done){ /* Still export exactly what was observed; never false-green. */
    fprintf(stderr,"NU021K_NOT_DONE offset=%d rows=%u flags=%02x\n",
            offset,observations,(unsigned)prev_flags);
  }
  for(unsigned first=0;first<observations;first+=8){
    unsigned count=(observations-first<8)?observations-first:8;
    for(unsigned copy=0;copy<2;copy++){
      printf("NU021K_PACK offset=%d first=%u rows=%u hex=",
             offset,first+1U,count);
      for(unsigned j=0;j<count;j++)for(unsigned k=0;k<FIELD_BYTES;k++)
        printf("%02x",(unsigned)packed[first+j][k]);
      putchar('\n');
    }
  }
  int good=done && ready && icf_seen && actual_edge_cycle==requested &&
           tov_cycle==reference_tov && compa_cycle && compb_cycle &&
           (final_flags & 0x27U)==0x27U &&
           final_icr!=0 && a->data[0x6f]==0 &&
           a->sreg[S_I]==0 && init_sp==((unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e]) ;

  printf("NU021K_SUMMARY offset=%d rows=%u edge=%llu compa=%llu tov=%llu "
         "compb=%llu icr=%04x flags=%02x ready=%u pass=%d\n",
         offset,observations,(unsigned long long)actual_edge_cycle,
         (unsigned long long)compa_cycle,(unsigned long long)tov_cycle,
         (unsigned long long)compb_cycle,final_icr,final_flags,ready,good);
  avr_terminate(a);return good?0:1;
}
int main(int argc,char**argv){
  if(argc!=3)return 2;
  const char *file=argv[1];
  uint64_t tov=(uint64_t)strtoull(argv[2],NULL,0);
  if(run(file,0,1,0))return 1;
  int failures=0;
  for(int off=-3;off<=3;off++){
    if(run(file,off,0,tov))failures++;
  }
  printf("NU021K_CAMPAIGN_SUMMARY cases=7 failures=%d\n",failures);
  return failures?1:0;
}
