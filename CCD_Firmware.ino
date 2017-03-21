//GPIOx_PSOR - set output reg
//GPIOx_PDDR - data direction reg
//GPIOx_PTOR - toggle output reg

#include "proto.h"

//when clocked to 192 MHz, 1 cycle is 5.21 nanoseconds
#define N5DELAY asm volatile( "nop\n" )

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

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  GPIOC_PDOR = 1;         // Set PORTD to output

  // Setup operating variables.
  xsize = WIDTH; ysize = HEIGHT;
  xoffs = yoffs = 0;
  xsec = 30;
  xmsec = 0;
  xbin = ybin = 1;
}

void expose() {
  noInterrupts();
  flusher();
  //v1_v2 = 0
  delay(xsec*1000 + xmsec);
  interrupts(); 
}
void flusher() {
  GPIOD_PSOR |= 0b100; //v1_v2
  delayMicroseconds(63);
  GPIOC_PSOR |= 0b10000;
  delayMicroseconds(17);
  GPIOC_PTOR = 0b10000;
  delayMicroseconds(63);//random choice
  for (int c = 0; c < 33; c++) {
    for (int a = 0; a < 8; a++) { GPIOD_PTOR = 0b100; delayMicroseconds(3); GPIOD_PTOR = 0b100; delayMicroseconds(60); }
    for (int b = 0; b < 99; b++) {
      for (int a = 0; a < 8; a++) { /*h toggle*/ }
      //clamp toggle
    }
  }
  //v1v2 = 1
  //ft = 1
  //wait
  //ft = 0
  for (int c = 0; c < 33; c++) {
    for (int a = 0; a < 8; a++) { /*v1v2 toggle*/ }
    for (int b = 0; b < 99; b++) {
      for (int a = 0; a < 8; a++) { /*h toggle*/ }
      //clamp toggle
    }
  }
}
void grimg() {
  noInterrupts();
  //791 columns, 262 odd rows, 263 even rows
  interrupts();
}

void loop() {
  while (1) {
    GPIOC_PTOR = 32;  // Pointlessly toggle LED
    delay(500);
    GPIOC_PTOR = 32;
    delay(500);
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
