/*
 * GPIOx_PSOR - set output reg
 * GPIOx_PDDR - data direction reg
 * GPIOx_PTOR - toggle output reg
 * 
 * when clocked to 192 MHz, 1 cycle is 5.21 nanoseconds
 *
 *  label     8   7  14   2   |  10  9
 *  actual   D3  D2  D1  D0   |  C4 C3
 *            H   V   x   R   |   F  C
 */


#include "proto.h"

#define N5DELAY asm volatile( "nop\n" )

#define V_TOGGLE GPIOD_PTOR = 0b0100
#define HR_TOGGLE GPIOD_PTOR = 0b1001
#define H_TOGGLE GPIOD_PTOR = 0b1000
#define R_TOGGLE GPIOD_PTOR = 0b0001
#define FT_TOGGLE GPIOC_PTOR = 0b10000
#define C_TOGGLE GPIOC_PTOR = 0b1000

#define R_LOW GPIOD_PSOR &= 0xFFFFFFFE //1110
#define R_HIGH GPIOD_PSOR |= 0b1
#define V1_LOW GPIOD_PSOR |= 0b100
#define V1_HIGH GPIOD_PSOR &= 0xFFFFFFFB //1011
#define H1_LOW GPIOD_PSOR &= 0xFFFFFFF7 //0111
#define H1_HIGH GPIOD_PSOR |= 0b1000
#define FT_LOW GPIOC_PSOR &= 0xFFFFFFEF //1110
#define C_LOW GPIOC_PSOR &= 0xFFFFFFF7 //0111

int xsize, ysize, xoffs, yoffs; // AOI vars
int xbin, ybin;                 // Binning vars
int xsec, xmsec;                // Exposure vars
int video = 38;
int img[780][495];

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
  GPIOD_PSOR |= 0b100; //v1 low
  delay(xsec*1000 + xmsec);
  grimg();
  interrupts(); 
}
void flusher() {
  GPIOD_PSOR &= 0xFFFFFFFE;       //initial states
  GPIOD_PSOR |= 0b1100;           //r low v1 low h1 high
  GPIOC_PSOR &= 0xFFFFFFE7;       //c low ft low
  
  delayMicroseconds(63);          
  FT_TOGGLE;                      //even shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;           
  delayMicroseconds(126);         //arbitrary delay before shift
  
  for (int c = 0; c < 248; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V 
    V_TOGGLE; 
    delayMicroseconds(10);        //t_HD

    for (int b = 0; b < 780; b++) {
      HR_TOGGLE;                  //horizontal shift
      delayMicroseconds(1);       //t_R
      R_TOGGLE;                   //reset off
      delayMicroseconds(4);       // + t_R = tcd
      H_TOGGLE;
      delayMicroseconds(5);       
    }
  }
  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //odd shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(63);          //arbitrary delay before shift
  V_TOGGLE;
  delayMicroseconds(63);          //another arbitrary delay
    
  for (int c = 0; c < 247; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V 
    V_TOGGLE; 
    delayMicroseconds(10);        //t_HD 

    for (int b = 0; b < 780; b++) {
      HR_TOGGLE;                  //horizontal shift
      delayMicroseconds(1);       //t_R
      R_TOGGLE;                   //reset off
      delayMicroseconds(4);       // + t_R = tcd
      H_TOGGLE;
      delayMicroseconds(5);
    }
  }
}
void grimg() {
  noInterrupts();
  GPIOD_PSOR &= 0xFFFFFFFE;       //initial states
  GPIOD_PSOR |= 0b1100;           //r low v1 low h1 high
  GPIOC_PSOR &= 0xFFFFFFE7;       //c low ft low
  int x = 0;
  int y = 0;

  delayMicroseconds(63);          
  FT_TOGGLE;                      //even shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;           
  delayMicroseconds(126);         //arbitrary delay before shift
  
  for (int c = 0; c < 248; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V 
    V_TOGGLE; 
    delayMicroseconds(10);        //t_HD

    for (int b = 0; b < 780; b++) {
      HR_TOGGLE;                  //horizontal shift
      delayMicroseconds(1);       //t_R
      R_TOGGLE;                   //reset off
      delayMicroseconds(4);       // + t_R = tcd
      C_TOGGLE;
      delayMicroseconds(1);       //t_C
      C_TOGGLE;
      H_TOGGLE;
      delayMicroseconds(2);       //t_sd
      img[x][y] = analogRead(video);
      delayMicroseconds(4);
      x++;       
    }
    x = 0;
    y++;
  }
  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //odd shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(63);          //arbitrary delay before shift
  V_TOGGLE;
  delayMicroseconds(63);          //another arbitrary delay
    
  for (int c = 0; c < 247; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V 
    V_TOGGLE; 
    delayMicroseconds(10);        //t_HD 

    for (int b = 0; b < 780; b++) {
      HR_TOGGLE;                  //horizontal shift
      delayMicroseconds(1);       //t_R
      R_TOGGLE;                   //reset off
      delayMicroseconds(4);       // + t_R = tcd
      C_TOGGLE;
      delayMicroseconds(1);       //t_C
      C_TOGGLE;
      H_TOGGLE;
      delayMicroseconds(2);       //t_sd
      img[x][y] = analogRead(video);
      delayMicroseconds(4);
      x++;
    }
    x = 0;
    y++;
  }
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
