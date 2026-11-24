#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[32768];for(unsigned i=0;i<sizeof image;i++)image[i]=255;
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(image,1,sizeof image,f);fclose(f);if(n<0x7e00)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,sizeof image,0);
 printf("NU012A_ROM hex=");for(unsigned i=0;i<192;i++)printf("%04x",word(image,2*(i<128?i:0x3f00+i-128)));putchar('\n');
 unsigned spm=0,done=0;unsigned last=0;
 for(unsigned i=0;i<80;i++){
  unsigned old=a->pc;unsigned opcode=word(a->flash,old);
  a->run_cycle_limit=1;a->run_cycle_count=1;
  int rc=avr_run(a);
  if(rc!=cpu_Running && !(rc==cpu_Done && a->data[24]==0x5a && a->pc/2==65 && opcode==0xe58a)){printf("NU012A_STOP rc=%d pc=%u old=%u opcode=%u r24=%u cycle=%llu\n",rc,a->pc/2,old/2,opcode,a->data[24],(unsigned long long)a->cycle);return 4;}
  if(opcode==0x95e8)spm++;
  printf("NU012A_ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",i+1,(unsigned long long)a->cycle,a->pc/2,opcode,a->data[0x57],word(a->flash,0x80),word(a->flash,0x82),word(a->flash,0x84),a->data[0],a->data[1],a->data[20],a->data[21],a->data[24],word(a->data,30),spm);
  if(a->data[24]==0x5a&&a->pc/2==65){done=1;last=i+1;break;}
 }
 unsigned errors=0;
 if(word(a->flash,0x80)!=0xe58a||word(a->flash,0x82)!=0xcfff||word(a->flash,0x84)!=0xffff||a->data[20]!=0x8a||a->data[21]!=0xe5||spm!=5||!done)errors++;
 for(unsigned i=0;i<128;i++)if(a->flash[i]!=image[i])errors++;
 for(unsigned i=0x7e00;i<0x7e80;i++)if(a->flash[i]!=image[i])errors++;
 printf("NU012A_SUMMARY steps=%u spm=%u done=%u errors=%u\n",last,spm,done,errors);
 avr_terminate(a);return errors?1:0;
}
