/* NU-021I: independent simavr executes real 12 AVR guest programs.
 * No host state/IRQ/flag seeding. A fail-closed reference-vs-datasheet gate
 * checks the post-TCNT1 write compare inhibition in WGM0 /1 and /8.
 */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MAX_STEPS 350
static unsigned expected(unsigned case_id) {
  return (case_id==7 || case_id==11)?6U:4U;
}
static int probe(const char *filename, unsigned case_id) {
  uint8_t image[32768]={0};
  FILE *f=fopen(filename,"rb");if(!f){perror(filename);return 2;}
  size_t bytes=fread(image,1,sizeof(image),f);fclose(f);
  if(bytes<0x80 || bytes>=sizeof(image) || (bytes&1))return 2;
  avr_t *avr=avr_make_mcu_by_name("atmega328p");
  if(!avr || avr_init(avr))return 3;
  avr->frequency=16000000;avr_loadcode(avr,image,(uint32_t)bytes,0);
  unsigned done=0,steps=0,write_mark_cycle=0;
  for(unsigned i=1;i<=MAX_STEPS;i++){
    int status=avr_run(avr);
    if(status!=cpu_Running && status!=cpu_Sleeping){
      fprintf(stderr,"NU021I_HALTED case=%u step=%u status=%d\n",case_id,i,status);
      avr_terminate(avr);return 4;
    }
    unsigned pc=(unsigned)avr->pc/2U;
    unsigned flags=(unsigned)avr->data[0x36];
    unsigned cs=(unsigned)avr->data[0x81]&7U;
    if(!write_mark_cycle && avr->data[24]==0xa5 && avr->data[25]==0x5a)
      write_mark_cycle=(unsigned)avr->cycle;
    printf("NU021I_TRACE case=%u step=%u pc_word=%04x cycle=%llu "
      "r20=%02x r21=%02x r24=%02x r25=%02x "
      "r26=%02x r27=%02x r28=%02x r29=%02x "
      "tifr1=%02x tccr1b=%02x\n",
      case_id,i,pc,(unsigned long long)avr->cycle,
      (unsigned)avr->data[20],(unsigned)avr->data[21],
      (unsigned)avr->data[24],(unsigned)avr->data[25],
      (unsigned)avr->data[26],(unsigned)avr->data[27],
      (unsigned)avr->data[28],(unsigned)avr->data[29],flags,cs);
    steps=i;
    if(pc>=bytes/2U-3U && avr->data[25]==0x5a){done=1;break;}
  }
  unsigned final=(unsigned)avr->data[29]&6U;
  unsigned want=expected(case_id);
  int ok=done && final==want && write_mark_cycle>0;
  printf("NU021I_SUMMARY case=%u steps=%u final_tifr1=%02x expected_tifr1=%02x "
    "early=%02x mid=%02x late=%02x tcnt_read=%02x%02x "
    "write_marker_cycle=%u passed=%d\n",
    case_id,steps,final,want,(unsigned)avr->data[26],
    (unsigned)avr->data[27],(unsigned)avr->data[28],
    (unsigned)avr->data[21],(unsigned)avr->data[20],
    write_mark_cycle,ok);
  avr_terminate(avr);return ok?0:1;
}
int main(int argc,char **argv){
  if(argc!=3)return 2;
  unsigned c=(unsigned)strtoul(argv[2],NULL,0);
  if(c>=12)return 2;
  return probe(argv[1],c);
}
