#include "sim_avr.h"
#include <stdio.h>
#include <stdint.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[1024]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);if(!n||n%2||n>=sizeof image)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,n,0);
 unsigned regs[]={17,19,26,27},old[4]={0};
 for(unsigned i=0;i<1200;i++){
  a->run_cycle_limit=1;a->run_cycle_count=1;int rc=avr_run(a);
  for(unsigned j=0;j<4;j++)if(a->data[regs[j]]!=old[j]){
    old[j]=a->data[regs[j]];
    printf("IRQ %u %llu %u %u\n",j,(unsigned long long)a->cycle,a->pc/2,a->data[0x84]+256U*a->data[0x85]);
  }
  if(a->data[25]==90){printf("DONE %u %u %u %u %u\n",old[0],old[1],old[2],old[3],a->data[24]);avr_terminate(a);return 0;}
  if(rc!=cpu_Running)return 4;
 }
 return 5;
}
