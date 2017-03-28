/*
 *GPIOx_PSOR - set output reg
 *GPIOx_PDDR - data direction reg
 *GPIOx_PTOR - toggle output reg
 *
 *  Pin     8  7     2     9 10
 *  Port   D3 D2 D1 D0    C3 C4
 *          H  V     R     C  F
 */


#include <avr/io.h> // Interrupt Vector Definitions
#include <avr/interrupt.h> // Interrupt Function Definitions
#include <util/atomic.h>

#include "proto.h"

//when clocked to 192 MHz, 1 cycle is 5.21 nanoseconds
#define N5DELAY asm volatile( "nop\n" )
#define VERTICLE_TOGGLE GPIOD_PTOR = 0b100


#define POLL_ENDRUN (GPIOA_PDIR & 0x10000)

int xsize, ysize, xoffs, yoffs; // AOI vars
int xbin, ybin;                 // Binning vars
int xsec, xmsec;                // Exposure vars


comm_t comm_list[NCOMMS] = {{.name="XFRAME", .id=10},
                      {.name="YFRAME", .id=20},
                      {.name="CCDPX",  .id=30},
                      {.name="CCDPY",  .id=40},
                      {.name="CCDT",   .id=50},
                      {.name="VER?",   .id=60},
                      {.name="XSEC?",  .id=70},
                      {.name="XMSEC?", .id=80},
                      {.name="XSIZE?", .id=90},
                      {.name="YSIZE?", .id=100},
                      {.name="XOFFS?", .id=110},
                      {.name="YOFFS?", .id=120},
                      {.name="XBIN?",  .id=130},
                      {.name="YBIN?",  .id=140},
                      {.name="XSEC",   .id=150},
                      {.name="XMSEC",  .id=160},
                      {.name="XSIZE",  .id=170},
                      {.name="YSIZE",  .id=180},
                      {.name="XOFFS",  .id=190},
                      {.name="YOFFS",  .id=200},
                      {.name="XBIN",   .id=210},
                      {.name="YBIN",   .id=220},
                      {.name="EXPOSE", .id=230},
                      {.name="FLUSH",  .id=240},
                      {.name="GRIMG",  .id=250},
                      {.name="TEST",   .id=260}
                     };

char  input_string[1024];
char* stend = input_string;

void handle_comms() {
  if (input_string[0] == 0x00){
      Serial.write(DELIM);
      return;
    }
  int comm_id;
  char* param = NULL; // Pointer to start of parameter.
  for (int i = 0; i < 200; i++){
      if (input_string[i] == ' '){
          input_string[i] = 0x00;     // Null-terminate command.
          param = input_string + i + 1;
        }
    }
  
  for (int i = 0; i < NCOMMS; i++) {
    // When comparing a ``String`` to a ``char*`` put the string first to use the compare operator (``==``)
    if (comm_list[i].name == input_string){
      comm_id = comm_list[i].id;
      break;
    }
  }
  char response[1024];
  switch (comm_id) {
    case XFRAME:
      itoa(WIDTH, response, 10);
      Serial.write(response);
      break;
    case YFRAME:
      itoa(HEIGHT, response, 10);
      Serial.write(response);
      break;
    case CCDPX:
      itoa(XPITCH, response, 10);
      Serial.write(response);
      break;
    case CCDPY:
      itoa(YPITCH, response, 10);
      Serial.write(response);
      break;
    case CCDT:
      itoa(CCDTYPE, response, 10);
      Serial.write(response);
      break;
    case VERQ:
      Serial.write(VERSION);
      break;
    case XSECQ:
      itoa(xsec, response, 10);
      Serial.write(response);
      break;
    case XMSECQ:
      itoa(xmsec, response, 10);
      Serial.write(response);
      break;
    case XSIZEQ:
      itoa(xsize, response, 10);
      Serial.write(response);
      break;
    case YSIZEQ:
      itoa(ysize, response, 10);
      Serial.write(response);
      break;
    case XOFFSQ:
      itoa(xoffs, response, 10);
      Serial.write(response);
      break;
    case YOFFSQ:
      itoa(yoffs, response, 10);
      Serial.write(response);
      break;
    case XBINQ:
      itoa(xbin, response, 10);
      Serial.write(response);
      break;
    case YBINQ:
      itoa(ybin, response, 10);
      Serial.write(response);
      break;
    case XSEC:
      xsec = atoi(param);
      break;
    case XMSEC:
      xmsec = atoi(param);
      break;
    case XSIZE:
      if (atoi(param) <= 0 || atoi(param) > WIDTH){
          Serial.write(EDELIM);
          return;
        }
      xsize = atoi(param);
      break;
    case YSIZE:
      if (atoi(param) <= 0 || atoi(param) > HEIGHT){
        Serial.write(EDELIM);
        return;
        }
      ysize = atoi(param);
      break;
    case XOFFS:
      if (atoi(param) < 0 || atoi(param) > WIDTH){
          Serial.write(EDELIM);
          return;
        }
      xoffs = atoi(param);
      break;
    case YOFFS:
      if (atoi(param) < 0 || atoi(param) > HEIGHT){
          Serial.write(EDELIM);
          return;
        }
      yoffs = atoi(param);
      break;
    case XBIN:
      if (atoi(param) < 0){
          Serial.write(EDELIM);
          return;
        }
      xbin = atoi(param);
      break;
    case YBIN:
      if (atoi(param) < 0){
          Serial.write(EDELIM);
          return;
        }
      ybin = atoi(param);
      break;

    //TODO: Fill in these functions
    case EXPOSE:
      expose();
      break;
    case FLUSH:
      flusher();
      break;
    case GRIMG:
      flusher();
      expose();
      grimg();
      break;
    case TEST:
      if (*param) Serial.write(param);
      else Serial.write("42");
      break;
    default:
      Serial.write(EDELIM);
      return;
  }
  Serial.write(DELIM);
  input_string[0] = 0x00;
}

void expose() {
  noInterrupts();
  flusher();
  GPIOD_PSOR |= 0b100; //v1 low
  int xtime = (xsec * 1000) + xmsec;
  while(!POLL_ENDRUN && xtime){
      delayMicroseconds(1000);
      xtime -= 1;
    }
    
  interrupts(); 
}
void flusher() {
  GPIOD_PSOR &= 0xFFFFFFFB;       //h1 low  1011
  GPIOD_PSOR |= 0b100;            //v2 high
  delayMicroseconds(63);          
  GPIOC_PTOR = 0b10000;           //even shift
  delayMicroseconds(17);          //t_VH
  GPIOC_PTOR = 0b10000;           
  delayMicroseconds(126);          //arbitrary delay before shift
  
  for (int c = 0; c < 248; c++) {
    VERTICLE_TOGGLE; //vertical shift
    delayMicroseconds(50);//t_V 
    VERTICLE_TOGGLE; 
    delayMicroseconds(10);//t_HD

    for (int b = 0; b < 780; b++) {
      GPIOD_PTOR = 0b1001; //horizontal shift
      delayMicroseconds(1);
      GPIOD_PTOR = 0b0001; //reset off
      delayMicroseconds(4); 
      GPIOD_PTOR = 0b1000;
      delayMicroseconds(5);
    }
    
  if (POLL_ENDRUN){
      VERTICLE_TOGGLE;
      return;
    }
  }
  VERTICLE_TOGGLE;
  delayMicroseconds(63);
  GPIOC_PTOR |= 0b10000; //odd shift
  delayMicroseconds(17);//t_VH
  GPIOC_PTOR = 0b10000;
  delayMicroseconds(63);          //arbitrary delay before shift
  VERTICLE_TOGGLE;
  delayMicroseconds(63);          //another arbitrary delay
    
  for (int c = 0; c < 247; c++) {
    VERTICLE_TOGGLE; //vertical shift
    delayMicroseconds(50);//t_v 
    VERTICLE_TOGGLE; 
    delayMicroseconds(10);//t_HD 

    for (int b = 0; b < 780; b++) {
      GPIOD_PTOR = 0b1001; //horizontal shift
      delayMicroseconds(1);
      GPIOD_PTOR = 0b0001; //reset off
      delayMicroseconds(4); 
      GPIOD_PTOR = 0b1000;
      delayMicroseconds(5);
    }
  if (POLL_ENDRUN){
      VERTICLE_TOGGLE;
      return;
    }
  }
  VERTICLE_TOGGLE;
  interrupts();
}
void grimg() {
  noInterrupts();
  //791 columns, 262 odd rows, 263 even rows
  interrupts();
}

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT); // On-board LED
  
  pinMode(8,  OUTPUT); // H_CTL, D3
  pinMode(7,  OUTPUT); // V1_V2, D2
  pinMode(9,  OUTPUT); // CLAMP, C3
  pinMode(10, OUTPUT); // FT,    C4
  pinMode(2,  OUTPUT); // RESET, D0
  pinMode(38, OUTPUT); // VIDEO, C11
  pinMode(36, OUTPUT); // TEMP,  C9
  pinMode(32, OUTPUT); // DAC0,  B11
  pinMode(33, OUTPUT); // DAC1,  E24
  pinMode(16, OUTPUT); // D_N,   B0
  pinMode(17, OUTPUT); // D_SCK, B1
  pinMode(18, OUTPUT); // D_CSLD,B3
  pinMode(19, OUTPUT); // D_CLR, B2
  pinMode(28,  INPUT); // ENDRUN,A16

  GPIOB_PDOR = 1; // Set ports B-E to outputs.
  GPIOC_PDOR = 1;
  GPIOD_PDOR = 1;
  GPIOE_PDOR = 1;

  // Setup operating variables.
  xsize = WIDTH; ysize = HEIGHT;
  xoffs = yoffs = 0;
  xsec = 30;
  xmsec = 0;
  xbin = ybin = 1;
  
  GPIOC_PTOR = 0b100000;
  delay(300);
  GPIOC_PTOR = 0b100000;
}

void loop() {
  
  while(1){
    yield();
    }
}

void serialEvent() {
  while (Serial.available()) {
    *(stend++) = (char)Serial.read();
  }
  *(--stend) = 0x00; // replace the newline with a zero-delimiter
  stend = input_string;
  handle_comms();
}
