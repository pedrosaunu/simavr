#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[384]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);if(n%2||n>256)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,n,0);
 printf("CTC_ROM words=%zu hex=",n/2);for(unsigned i=0;i<n;i+=2)printf("%04x",word(image,i));puts("");
 for(unsigned i=0;i<110;i++){
  a->run_cycle_limit=1;a->run_cycle_count=1;
  int rc=avr_run(a);
  unsigned wgm=(a->data[0x80]&3)|((a->data[0x81]>>1)&12);
  unsigned cnt=word(a->data,0x84);
  printf("CTC_ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",i+1,(unsigned long long)a->cycle,a->pc/2,wgm,a->data[0x81]&7,cnt,word(a->data,0x88),word(a->data,0x8a),word(a->data,0x86),a->data[0x36],a->data[25]);
  if(a->data[25]==90){puts("CTC_REF_PASS");avr_terminate(a);return 0;}
  if(rc!=cpu_Running)return 4;
 }
 return 5;
}
