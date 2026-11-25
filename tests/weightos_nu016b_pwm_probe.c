#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdint.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[256]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);if(n%2||n>=256)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,n,0);
 printf("PWM_ROM words=%zu hex=",n/2);for(unsigned i=0;i<n;i+=2)printf("%04x",word(image,i));puts("");
 for(unsigned i=0;i<110;i++){
  a->run_cycle_limit=1;a->run_cycle_count=1;int rc=avr_run(a);
  avr_ioport_state_t port={0};if(avr_ioctl(a,AVR_IOCTL_IOPORT_GETSTATE('B'),&port)<0)return 3;
  unsigned mode=(a->data[0x80]&3)|((a->data[0x81]>>1)&12);
  printf("NU016B ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
   i+1,(unsigned long long)a->cycle,a->pc/2,mode,a->data[0x81]&7,word(a->data,0x88),word(a->data,0x8a),word(a->data,0x86),a->data[0x36],port.pin&6,a->data[20]&6,word(a->data,21),a->data[25]);
  if(a->data[25]==90){puts("PWM_REF_PASS");avr_terminate(a);return 0;}
  if(rc!=cpu_Running)return 4;
 }
 return 5;
}
