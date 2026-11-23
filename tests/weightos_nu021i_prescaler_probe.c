/* NU-021I: independent simavr executes real 12 AVR guest programs.
 * No host state/IRQ/flag seeding. A fail-closed reference-vs-datasheet gate
 * checks the post-TCNT1 write compare inhibition in WGM0 /1 and /8.
 */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MAX_STEPS 350
#define TRACE_BYTES 15
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
  uint8_t trace[MAX_STEPS][TRACE_BYTES]={{0}};
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
    /* Record every retired AVR observation in a lossless binary table.
     * Streaming 1600 separate lines through Railway can lose periodic lines;
     * instead emit 16-row hex chunks twice AFTER guest execution.
     */
    uint8_t *rec=trace[i-1];
    const unsigned cyc=(unsigned)avr->cycle;
    rec[0]=(uint8_t)i;
    rec[1]=(uint8_t)pc;rec[2]=(uint8_t)(pc>>8);
    rec[3]=(uint8_t)cyc;rec[4]=(uint8_t)(cyc>>8);
    rec[5]=avr->data[20];rec[6]=avr->data[21];
    rec[7]=avr->data[24];rec[8]=avr->data[25];
    rec[9]=avr->data[26];rec[10]=avr->data[27];
    rec[11]=avr->data[28];rec[12]=avr->data[29];
    rec[13]=(uint8_t)flags;rec[14]=(uint8_t)cs;
    steps=i;
    if(pc>=bytes/2U-3U && avr->data[25]==0x5a){done=1;break;}
  }
  for(unsigned first=0;first<steps;first+=16U){
    unsigned count=steps-first<16U?steps-first:16U;
    for(unsigned copy=0;copy<2U;copy++){
      printf("NU021I_PACK case=%u first=%u rows=%u hex=",case_id,first+1U,count);
      for(unsigned j=0;j<count;j++)
        for(unsigned b=0;b<TRACE_BYTES;b++)
          printf("%02x",(unsigned)trace[first+j][b]);
      putchar('\n');
    }
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
