#include "sim_avr.h"
#include <stdio.h>
#include <stdint.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[1024]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);if(!n||n%2||n>=sizeof image)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,n,0);
 for(unsigned i=0;i<220;i++){
  a->run_cycle_limit=1;a->run_cycle_count=1;int rc=avr_run(a);
  printf("NU016Q ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",i+1,(unsigned long long)a->cycle,a->pc/2,a->data[24],a->data[25],a->data[17],a->data[19],a->data[21]+256U*a->data[22],a->data[23],a->data[0x43],a->data[0x45],a->data[0x81]);
  if(a->data[25]==90){puts("NU016Q PASS");avr_terminate(a);return 0;}
  if(rc!=cpu_Running)return 4;
 }
 return 5;
}
