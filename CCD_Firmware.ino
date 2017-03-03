
// Increments each command by 10 so that new commands can be added in.
#define XFRAME   10   // Returns installed CCD Sensor's frame width
#define YFRAME   20   // Returns the installed CCD Sensor's frame height
#define CCDPX    30   // Returns X axis CCD Pixel Pitch
#define CCDPY    40   // Returns Y axis CCD Pixel Pitch
#define CCDT     50   // Returns the ccd type; 2 = interline full/frame frame-transfer

// Commands ending in a 'Q' mean a query. As far as I know, '?' isn't allowed in macros.

#define VERQ     60   // Returns current version of firmware
#define XSECQ    70   // Returns current exposure time seconds
#define XMSECQ   80   // Returns current exposure time milliseconds
#define XSIZEQ   90   // Returns width of current AOI (Area of Interest)
#define YSIZEQ  100   // Returns height of current AOI
#define XOFFSQ  110   // Returns x-offset of current AOI
#define YOFFSQ  120   // Returns y-offset of current AOI
#define XBINQ   130   // Returns current x-bin value
#define YBINQ   140   // Returns current y-bin value

#define XSEC    150   // Set exposure time seconds
#define XMSEC   160   // Set exposure time milliseconds
#define XSIZE   170   // Set width of AOI
#define YSIZE   180   // Set height of AOI
#define XOFFS   190   // Set x-offset of AOI
#define YOFFS   200   // Set y-offset of AOI
#define XBIN    210   // Set the binning in the horizontal direction
#define YBIN    220   // Set the binning in the vertical direction

#define EXPOSE  230   // Opens the shutter (metaphorically) and exposes sensor for XSEC + XMSEC
#define FLUSH   240   // Scans entire CCD frame with BIN 8 to assure empty pixel wells
#define GRIMG   250   // Acquire image pixels (~10us per pixel) in AOI specified above
#define TEST    260   // Test function to verify comms, return 42

#define DELIM   " OK\n"  // Delimiter for returning messages from teensy
#define EDELIM  " ERR\n" // Delimiter for returning an error message

#define NCOMMS 26  // Number of commands available

typedef struct {
  String name;
  short id;
} comm_t;

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

char  input_string[200];
char* stend = input_string;

void handle_comms() {
  int comm_id;
  for (int i = 0; i < NCOMMS; i++) {
    // When comparing a ``String`` to a ``char*`` put the string first to use the compare operator (``==``)
    if (comm_list[i].name == input_string){
      comm_id = comm_list[i].id;
      break;
    }
  }
  switch (comm_id) {
    case 260:
      Serial.write("42");
      Serial.write(DELIM);
      break;
    default:
      Serial.write(EDELIM);
  }
  input_string[0] = 0x00;
}

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  GPIOC_PDOR = 1;         // Set PORTD to output

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
  *(stend++) = 0x00; // 0-delimiter
  stend = input_string;
  handle_comms();
}
