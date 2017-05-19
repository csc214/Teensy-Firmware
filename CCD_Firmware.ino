/* wait half a second to turn dac on well vsub(through pot)
   GPIOx_PSOR - set output reg
   GPIOx_PDDR - data direction reg
   GPIOx_PTOR - toggle output reg

   when clocked to 192 MHz, 1 cycle is 5.21 nanoseconds **

    label     6   5  14   2   |   8  7
    actual   D4  D7  D1  D0   |  D3 D2
              H   V   x   R   |   F  C
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

#define V_TOGGLE (GPIOD_PTOR = 0b10000000)
#define HR_TOGGLE (GPIOD_PTOR = 0b10001)
#define HC_TOGGLE (GPIOD_PTOR = 0b10100)
#define H_TOGGLE (GPIOD_PTOR = 0b10000)
#define R_TOGGLE (GPIOD_PTOR = 0b0001)
#define FT_TOGGLE (GPIOD_PTOR = 0b1000)
#define C_TOGGLE (GPIOD_PTOR = 0b100)

#define R_LOW GPIOD_PCOR = 0b1
#define R_HIGH GPIOD_PSOR = 0b1
#define V1_LOW GPIOD_PSOR = 0b10000000
#define V1_HIGH GPIOD_PCOR = 0b10000000
#define H1_LOW GPIOD_PCOR = 0b10000
#define H1_HIGH GPIOD_PSOR = 0b10000
#define FT_LOW GPIOD_PCOR = 0b1000
#define C_LOW GPIOD_PCOR = 0b100

#define POLL_ENDRUN (GPIOA_PDIR & 0x10000)

int xsize, ysize, xoffs, yoffs; // AOI vars
int xbin, ybin;                 // Binning vars
int xsec, xmsec;                // Exposure vars
int temp_mon = 0;               // Monitor temperature?
int pec_duty = 0;
int video = 16;
const int pinIN = 24;
const int pinSCK = 25;
const int pinCSLD = 26;
const int pinCLR = 27;

uint16_t img[WIDTH];
int lace1, lace2; // lace1 is the number of lines in the even lace, lace2 the odds

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
                      {.name="TTEMP?", .id=147},
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
                      {.name="HSHIFT", .id=255},
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
  if (input_string[0] == 0x00) {
    Serial.write(DELIM);
    return;
  }
  int comm_id;
  char* param = NULL; // Pointer to start of parameter.
  for (int i = 0; i < 200; i++) {
    if (input_string[i] == ' ') {
      input_string[i] = 0x00;     // Null-terminate command.
      param = input_string + i + 1;
    }
  }

  for (int i = 0; i < NCOMMS; i++) {
    // When comparing a ``String`` to a ``char*`` put the string first to use the compare operator (``==``)
    if (comm_list[i].name == input_string) {
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
    case TTEMPQ:
      Serial.print(pec_duty);
    case XSEC:
      xsec = atoi(param);
      break;
    case XMSEC:
      xmsec = atoi(param);
      break;
    case XSIZE:
      if (atoi(param) <= 0 || atoi(param) > WIDTH) {
        Serial.write(EDELIM);
        return;
      }
      xsize = atoi(param);
      break;
    case YSIZE:
      if (atoi(param) <= 0 || atoi(param) > HEIGHT) {
        Serial.write(EDELIM);
        return;
      }
      ysize = atoi(param);
      break;
    case XOFFS:
      if (atoi(param) < 0 || atoi(param) > WIDTH) {
        Serial.write(EDELIM);
        return;
      }
      xoffs = atoi(param);
      break;
    case YOFFS:
      if (atoi(param) < 0 || atoi(param) > HEIGHT) {
        Serial.write(EDELIM);
        return;
      }
      yoffs = atoi(param);
      break;
    case XBIN:
      if (atoi(param) < 0) {
        Serial.write(EDELIM);
        return;
      }
      xbin = atoi(param);
      break;
    case YBIN:
      if (atoi(param) < 0) {
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
    case HSHIFT:
      while(!POLL_ENDRUN) grimg();
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
    if (POLL_ENDRUN) {
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
    if (POLL_ENDRUN) {
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
  while (!POLL_ENDRUN && xtime) {
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
  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //even shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(381);         //arbitrary delay before shift

  for (int c = 0; c < lace2; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V
    V_TOGGLE;
    delayMicroseconds(10);        //t_HD

    hshift(0);
  }

  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //odd shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(381);          //arbitrary delay before shift
  V_TOGGLE;
  delayMicroseconds(508);
  
  for (int c = 0; c < lace1; c++) {
    V_TOGGLE;                     //vertical shift
    delayMicroseconds(50);        //t_V
    V_TOGGLE;
    delayMicroseconds(10);        //t_HD

    hshift(0);
  }
  V_TOGGLE;
  interrupts();
}

void hshift(int flag){
    int x = 0;
    H_TOGGLE;
    for (int b = 0; b < WIDTH; b++) {
      HR_TOGGLE;                  //horizontal shift
      N50DELAY;                   //t_R
      R_TOGGLE;                   //reset off
      NXDELAY(500);               // + t_R = tcd
      C_TOGGLE;
      NXDELAY(500);               //t_C
      C_TOGGLE;
      H_TOGGLE;
      NXDELAY(100);       //t_sd
      if (flag)  img[x] = analogRead(video);
      x++;
    }//end row shifting
    x = 0;
    if (flag) Serial.write((char*)img, WIDTH * 2); // Twice the height because every pixel is a 16-bit number
  }

void grimg() {
  noInterrupts();
  flusher();
  R_LOW;                        //initial states
  C_LOW;
  V1_LOW;
  H1_HIGH;
  FT_LOW;
  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //even shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(381);         //arbitrary delay before shift

  for (int c = 0; c < lace2; c++) {
    V_TOGGLE;
    delayMicroseconds(5);        //t_V
    V_TOGGLE;
    hshift(1);
  }//end vertical shifting

  V_TOGGLE;
  delayMicroseconds(63);
  FT_TOGGLE;                      //odd shift
  delayMicroseconds(17);          //t_VH
  FT_TOGGLE;
  delayMicroseconds(381);          //arbitrary delay before shift
  V_TOGGLE;
  delayMicroseconds(508);
  
  for (int c = 0; c < lace1; c++) {
    V_TOGGLE;
    delayMicroseconds(5);        //t_V
    V_TOGGLE;
    hshift(1);
    }
  V_TOGGLE;
  interrupts();
}

void dac_programmer(uint8_t chn, double val)
{
  unsigned long num = (unsigned long)(val * 255.0 / 11.75);
  //the control word is 16 bits
  //the high 8 bits defines the output channel

  unsigned long t = chn << 8;
  t = t | num;

  char out[16];
  //Serial.println(itoa(t, out, 2));

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
  delay(3000);
  pinMode(13, OUTPUT); // On-board LED

  pinMode(6,  OUTPUT); // H_CTL, D3
  pinMode(5,  OUTPUT); // V1_V2, D2
  pinMode(7,  OUTPUT); // CLAMP, C3
  pinMode(8, OUTPUT); // FT,    C4
  pinMode(2,  OUTPUT); // RESET, D0
  //pinMode(A2, OUTPUT); // VIDEO, C11
  pinMode(A0, OUTPUT); // TEMP,  C9
  pinMode(A21, OUTPUT); // DAC0,  B11
  pinMode(A22, OUTPUT); // DAC1,  E24
  pinMode(pinIN, OUTPUT); // D_N,   B0
  pinMode(pinSCK, OUTPUT); // D_SCK, B1
  pinMode(pinCSLD, OUTPUT); // D_CSLD,B3
  pinMode(pinCLR, OUTPUT); // D_CLR, B2
  pinMode(28,  INPUT); // ENDRUN,A16

  digitalWrite(pinCLR, HIGH);
  digitalWrite(pinSCK, LOW);
  digitalWrite(pinCSLD, HIGH);
  
  analogReadResolution(12);
  analogWriteResolution(12);

  analogWrite(A2, 1023);
  
  GPIOB_PDOR = 1; // Set ports B-E to outputs.
  GPIOC_PDOR = 1;
  GPIOD_PDOR = 1;
  GPIOE_PDOR = 1;

  // Setup operating variables.
  xsize = WIDTH; ysize = HEIGHT;
  lace1 = HEIGHT / 2; lace2 = (HEIGHT / 2) + 1;
  
  xoffs = yoffs = 0;
  xsec = 30;
  xmsec = 0;
  xbin = ybin = 1;

  ARM_DEMCR |= ARM_DEMCR_TRCENA; //CPU cycles, might not be necessary
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  analogReadResolution(10);
  analogWriteResolution(10);

  analogWrite(A21, 155);
  analogWrite(A22, 217);
  dac_programmer(0b1, 10.08);         //DAC0 - 9v     reads 8
  dac_programmer(0b10, 8.91);        //DAC1 - 8v     7.1
  dac_programmer(0b100, 3.5);       //DAC2 - 3v     2.6
  dac_programmer(0b1000, 9.42);    //DAC3 - 8.5v   reads7.5
  dac_programmer(0b10000, 4.60);     //DAC4 - 4v     3.5
  dac_programmer(0b100000, 11.75);   //DAC5 - 11v    9.8
  dac_programmer(0b1000000, 8.35); //DAC6 - 7.5v     1.7
  dac_programmer(0b10000000, 2.35);  //DAC7 - 2v   6.6
  GPIOC_PTOR = 0b100000;
  delay(300);
  GPIOC_PTOR = 0b100000;

}

float get_temp(){
  float temp = analogRead(A0);
  return temp;
  }

// Set the duty cycle for the TEC, where duty is out of 1023
void set_temp(int duty){
  analogWrite(A2, 1024 - duty);
  pec_duty = duty;
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
