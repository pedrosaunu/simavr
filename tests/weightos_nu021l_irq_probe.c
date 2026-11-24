/* NU-021L independent software oracle. Only external PB0 is driven.
 * Capture actual stack bytes for all three active frames; compare return
 * PC against the observed top frame on each unwind, never write state. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_STEPS 450
#define FIELD_BYTES 28
static unsigned word(uint8_t *p,unsigned a){return p[a]+256U*p[a+1];}
static int run(const char *file,unsigned c){
 uint8_t image[32768]={0};
 FILE *f=fopen(file,"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);fclose(f);
 if(n<0x250||n>=sizeof(image)||(n&1))return 2;
 avr_t *a=avr_make_mcu_by_name("atmega328p");
 if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
 if(!pin){avr_terminate(a);return 3;}
 avr_raise_irq(pin,0);avr_loadcode(a,image,(uint32_t)n,0);
 uint8_t rows[MAX_STEPS][FIELD_BYTES]={{0}};
 unsigned initial=word(a->data,0x5d),prevdepth=0,maxdepth=0,steps=0;
 unsigned frames[4]={0},seq[8]={0},depths[8]={0},entries=0,returns=0,errors=0;
 unsigned ready=0,pending=0,mask_seen=0,done=0;
 uint64_t ready_cycle=0;
 int level=0;
 for(unsigned i=1;i<=MAX_STEPS;i++){
   int rc=avr_run(a);
   if(rc!=cpu_Running&&rc!=cpu_Sleeping){errors++;break;}
   unsigned pc=a->pc/2U,sp=word(a->data,0x5d),flags=a->data[0x36],mask=a->data[0x6f];
   if(sp>initial||initial-sp>6||(initial-sp)%2){errors++;break;}
   unsigned depth=(initial-sp)/2;
   if(depth>maxdepth)maxdepth=depth;
   if(depth==prevdepth+1){
     if(entries>=8||pc<0x14||pc>0x1a||(pc&1)||a->sreg[S_I])errors++;
     else {seq[entries]=pc;depths[entries++]=depth;}
     unsigned slot=initial-2*(depth-1);
     frames[depth]=a->data[slot]+256U*a->data[slot-1];
   }else if(depth+1==prevdepth){
     if(pc!=frames[prevdepth]||!a->sreg[S_I])errors++;
     returns++;
   }else if(depth!=prevdepth)errors++;
   for(unsigned d=1;d<=depth;d++){
     unsigned slot=initial-2*(d-1);
     if(frames[d]!=(unsigned)a->data[slot]+256U*a->data[slot-1])errors++;
   }
   prevdepth=depth;
   if(!ready&&a->data[0x100]==0xa5){ready=1;ready_cycle=a->cycle;}
   if(!mask_seen&&mask==0x20){mask_seen=1;pending=flags&0x27;}
   uint8_t *v=rows[i-1];
   v[0]=i;v[1]=i>>8;v[2]=pc;v[3]=pc>>8;v[4]=a->cycle;v[5]=a->cycle>>8;
   for(unsigned k=0;k<8;k++)v[6+k]=a->data[20+k];
   v[14]=flags;v[15]=mask;v[16]=a->sreg[S_I];v[17]=initial-sp;
   for(unsigned k=0;k<6;k++)v[18+k]=a->data[initial-k];
   v[24]=level;v[25]=a->data[0x86];v[26]=a->data[0x87];v[27]=a->data[0x81];
   steps=i;
   uint64_t age=ready?a->cycle-ready_cycle:0;
   int desired=ready&&age>=8&&age<17;
   if(desired!=level){avr_raise_irq(pin,desired);level=desired;
     printf("NU021L_PIN case=%u step=%u cycle=%llu level=%d\n",c,i,(unsigned long long)a->cycle,level);}
   if(!depth&&a->sreg[S_I]&&a->data[20]==1&&a->data[21]==1&&
      a->data[22]==(c!=1)&&a->data[23]==1&&a->data[24]==1&&a->data[25]==(c==0)){
     done=1;break;
   }
 }
 unsigned count=c==1?3:4,expected_depth=c==0?3:c==1?2:1;
 unsigned expected_seq[4]={0x14,0x16,c==1?0x1a:0x18,0x1a};
 unsigned expected_depths[4]={1,c==2?1:2,c==0?3:1,1};
 for(unsigned i=0;i<count;i++)if(seq[i]!=expected_seq[i]||depths[i]!=expected_depths[i])errors++;
 unsigned final_flags=a->data[0x36]&0x27;
 int pass=done&&ready&&pending==0x27&&entries==count&&returns==count&&
          maxdepth==expected_depth&&final_flags==(c==1?4:0)&&!errors;
 for(unsigned first=0;first<steps;first+=6){
   unsigned k=steps-first<6?steps-first:6;
   for(unsigned copy=0;copy<2;copy++){
     printf("NU021L_PACK case=%u first=%u rows=%u hex=",c,first+1,k);
     for(unsigned j=0;j<k;j++)for(unsigned b=0;b<FIELD_BYTES;b++)printf("%02x",rows[first+j][b]);
     putchar('\n');
   }
 }
 printf("NU021L_SUMMARY case=%u steps=%u pending=%02x depth=%u entries=%u returns=%u final_flags=%02x errors=%u pass=%d\n",c,steps,pending,maxdepth,entries,returns,final_flags,errors,pass);
 avr_terminate(a);return pass?0:1;
}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 unsigned c=(unsigned)strtoul(argv[2],NULL,0);if(c>2)return 2;
 return run(argv[1],c);
}
