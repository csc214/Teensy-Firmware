/* wait half a second to turn dac on well vsub(through pot)
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


#include <avr/io.h> // Interrupt Vector Definitions
#include <avr/interrupt.h> // Interrupt Function Definitions
#include <util/atomic.h>

#include "proto.h"

#define N05DELAY asm volatile( "nop\n" )                                               //noisey, too quick
#define N10DELAY asm volatile( "nop\nnop\n" )                                          //still noisey
#define N20DELAY asm volatile( "nop\nnop\nnop\nnop\n" )
#define N50DELAY asm volatile( "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" )
#define NXDELAY(t) { int x = ((t*100)/521 + ARM_DWT_CYCCNT); while (x > ARM_DWT_CYCCNT) {}}

#define V_TOGGLE GPIOD_PTOR = 0b0100
#define HR_TOGGLE GPIOD_PTOR = 0b1001
#define H_TOGGLE GPIOD_PTOR = 0b1000
#define R_TOGGLE GPIOD_PTOR = 0b0001
#define FT_TOGGLE GPIOC_PTOR = 0b10000
#define C_TOGGLE GPIOC_PTOR = 0b1000

#define R_LOW GPIOD_PCOR = 0b1
#define R_HIGH GPIOD_PSOR = 0b1
#define V1_LOW GPIOD_PSOR = 0b100
#define V1_HIGH GPIOD_PCOR = 0b100
#define H1_LOW GPIOD_PCOR = 0b1000
#define H1_HIGH GPIOD_PSOR = 0b1000
#define FT_LOW GPIOC_PCOR = 0b10000
#define C_LOW GPIOC_PCOR = 0b1000

#define POLL_ENDRUN (GPIOA_PDIR & 0x10000)

int xsize, ysize, xoffs, yoffs; // AOI vars
int xbin, ybin;                 // Binning vars
int xsec, xmsec;                // Exposure vars
int temp_mon = 0;               // Monitor temperature?
int video = 38;
const int pinIN = 0;
const int pinSCK = 1;
const int pinCSLD= 2;
const int pinCLR = 3;

uint16_t img[780];

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
                      {.name="TEMP?",  .id=145},
                      {.name="XSEC",   .id=150},
                      {.name="XMSEC",  .id=160},
                      {.name="XSIZE",  .id=170},
                      {.name="YSIZE",  .id=180},
                      {.name="XOFFS",  .id=190},
                      {.name="YOFFS",  .id=200},
                      {.name="XBIN",   .id=210},
                      {.name="YBIN",   .id=220},
                      {.name="TTEMP",  .id=225},
                      {.name="MTEMP",  .id=227},
                      {.name="EXPOSE", .id=230},
                      {.name="FLUSH",  .id=240},
                      {.name="GRIMG",  .id=250},
                      {.name="TEST",   .id=260},
                      {.name="THSHIFT",.id=261},
                      {.name="TVSHIFT",.id=262},
                      {.name="DACOFF", .id=270}
                     };

char  input_string[1024];
char* stend = input_string;

float get_temp();
void set_temp(int duty);

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
    case TEMPQ:
      Serial.print(get_temp());
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
    case TTEMP:
      set_temp(atoi(param));
      break;
    case MTEMP:
      temp_mon ^= 1;
      break;
    case EXPOSE:
      expose();
      break;
    case FLUSH:
      flusher();
      break;
    case GRIMG:
      grimg();
      break;
    case DACOFF:
      dac_programmer(0b11111111, 0);
      //digitalWrite(pinCLR, LOW); //doesn't work well
      break;
    case TEST:
      if (*param) Serial.write(param);
      else Serial.write("42");
      break;
    case THSHIFT:
      thshift();
      break;
    case TVSHIFT:
      tvshift();
      break;
    default:
      Serial.write(EDELIM);
      return;
  }
  Serial.write(DELIM);
  input_string[0] = 0x00;
}

void thshift() {
  noInterrupts();
  H1_HIGH;
  R_LOW;
  while (1) {
    HR_TOGGLE;
    delayMicroseconds(1);
    R_LOW;
    delayMicroseconds(4);
    H1_HIGH;
    delayMicroseconds(5);
    if (POLL_ENDRUN){
      interrupts();
      Serial.write(HALT);
      return;
    } 
  }
}

void tvshift() {
  noInterrupts();
  while (1) {
    FT_LOW;
    delayMicroseconds(10);
    V1_LOW;
    delayMicroseconds(10);
    FT_TOGGLE;
    delayMicroseconds(5);
    FT_TOGGLE;
    delayMicroseconds(10);
    V_TOGGLE;
    delayMicroseconds(10);
    FT_TOGGLE;
    delayMicroseconds(5);
    if (POLL_ENDRUN){
      interrupts();
      Serial.write(HALT);
      return;
    } 
  }
}

void expose() {
  flusher();
  noInterrupts();
  GPIOD_PSOR |= 0b100; //v1 low
  int xtime = (xsec * 1000) + xmsec;
  while(!POLL_ENDRUN && xtime){
      delayMicroseconds(1000);
      xtime -= 1;
    }
  grimg();
  interrupts(); 
}

void flusher() {
  noInterrupts();
  R_LOW;                        //initial states
  C_LOW;
  V1_LOW;
  H1_HIGH;
  FT_LOW;
  
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
    
    if (POLL_ENDRUN){
      interrupts();
      Serial.write(HALT);
      V_TOGGLE;
      return;
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
    
    if (POLL_ENDRUN){
      interrupts();
      Serial.write(HALT);
      V_TOGGLE;
      return;
    }
  }
  V_TOGGLE;
  interrupts();
}

void grimg() {
  noInterrupts();
  R_LOW;                        //initial states
  C_LOW;
  V1_LOW;
  H1_HIGH;
  FT_LOW;
  int x = 0;

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
      img[b] = c^b; //analogRead(video);
      //delayMicroseconds(4);
      x++;       
    }//end row shifting
    for (x = 0; x < 780; x++) {
      Serial.println(img[x]);
    }
    if (POLL_ENDRUN){
      interrupts();
      Serial.write(HALT);
      V_TOGGLE;
      return;
    }
    x = 0;
  }//end vertical shifting
  
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
      img[b] = c^b;//analogRead(video); 
      //delayMicroseconds(4);
      x++;
    }
    for (x = 0; x < 780; x++) {
      Serial.println(img[x]);
    }
    x = 0;
  }
  if (POLL_ENDRUN){
    interrupts();
    Serial.write(HALT);
    V_TOGGLE;
    return;
  }
  V_TOGGLE;
  interrupts();
}

void dac_programmer(uint8_t chn, double val)
{
  unsigned long num = (unsigned long)(val * 255.0 / 12.0);
  //int num = val;
  //the control word is 16 bits
  //the high 8 bits defines the output channel
  
  unsigned long t = chn << 8;
  t = t | num;
   
  digitalWrite(pinCSLD, LOW);
  for (long i = 15; i >= 0; i--)
  { 
    long b = (t >> i) & 1;
    digitalWrite(pinIN, b);
    digitalWrite(pinSCK, HIGH);
    delayMicroseconds(5);
    digitalWrite(pinSCK, LOW);
  }
   
  digitalWrite(pinCSLD, HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT); // On-board LED
  
  pinMode(8,  OUTPUT); // H_CTL, D3
  pinMode(7,  OUTPUT); // V1_V2, D2
  pinMode(9,  OUTPUT); // CLAMP, C3
  pinMode(10, OUTPUT); // FT,    C4
  pinMode(2,  OUTPUT); // RESET, D0
  //pinMode(A2, OUTPUT); // VIDEO, C11
  pinMode(A0, OUTPUT); // TEMP,  C9
  pinMode(32, OUTPUT); // DAC0,  B11
  pinMode(33, OUTPUT); // DAC1,  E24
  pinMode(pinIN, OUTPUT); // D_N,   B0
  pinMode(pinSCK, OUTPUT); // D_SCK, B1
  pinMode(pinCSLD, OUTPUT); // D_CSLD,B3
  pinMode(pinCLR, OUTPUT); // D_CLR, B2
  pinMode(28,  INPUT); // ENDRUN,A16
   
  digitalWrite(pinCLR, HIGH);
  digitalWrite(pinSCK, LOW);
  digitalWrite(pinCSLD, HIGH);

  analogReadResolution(10);
  analogWriteResolution(10);

  analogWrite(A2, 1023);
  
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

  ARM_DEMCR |= ARM_DEMCR_TRCENA; //CPU cycles, might not be necessary
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  
  /*dac_programmer(0b1, 9);         //DAC0 - 9v     reads 8
  dac_programmer(0b10, 8);        //DAC1 - 8v     7.1
  dac_programmer(0b100, 3);       //DAC2 - 3v     2.6
  dac_programmer(0b1000, 8.5);    //DAC3 - 8.5v   reads7.5
  dac_programmer(0b10000, 4);     //DAC4 - 4v     3.5
  dac_programmer(0b100000, 11);   //DAC5 - 11v    9.8
  dac_programmer(0b1000000, 2);   //DAC6 - 2v     1.7
  dac_programmer(0b10000000, 7.5);//DAC7 - 7.5v   6.6*/
  dac_programmer(0b11111111, 6);
  GPIOC_PTOR = 0b100000;
  delay(300);
  GPIOC_PTOR = 0b100000;

}

float get_temp(){
  float temp = analogRead(A0)*3.3/1024.0;
  temp = temp - 0.5;
  temp = temp / 0.01;
  return temp;
  }

// Set the duty cycle for the TEC, where duty is out of 1023
void set_temp(int duty){
  analogWrite(A2, 1023 - duty);
  }

void loop() {
  while(1){
    if(temp_mon) Serial.println(get_temp());
    yield();
    delay(5);
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
