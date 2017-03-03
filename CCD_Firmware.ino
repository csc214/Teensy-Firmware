
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

// I'll find a better way to do this... probably
char* comm_list[] = {"XFRAME", (char*) 10,
                     "YFRAME", (char*) 20,
                     "CCDPX",  (char*) 30,
                     "CCDPY",  (char*) 40,
                     "CCDT",   (char*) 50,
                     "VER?",   (char*) 60,
                     "XSEC?",  (char*) 70,
                     "XMSEC?", (char*) 80,
                     "XSIZE?", (char*) 90,
                     "YSIZE?", (char*) 100,
                     "XOFFS?", (char*) 110,
                     "YOFFS?", (char*) 120,
                     "XBIN?",  (char*) 130,
                     "YBIN?",  (char*) 140,
                     "XSEC",   (char*) 150,
                     "XMSEC",  (char*) 160,
                     "XSIZE",  (char*) 170,
                     "YSIZE",  (char*) 180,
                     "XOFFS",  (char*) 190,
                     "YOFFS",  (char*) 200,
                     "XBIN",   (char*) 210,
                     "YBIN",   (char*) 220,
                     "EXPOSE", (char*) 230,
                     "FLUSH",  (char*) 240,
                     "GRIMG",  (char*) 250,
                     "TEST",   (char*) 260};

char  input_string[200];
char* stend = input_string;

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  GPIOC_PDOR = 1;         // Set PORTD to output

}

void loop() {
  GPIOC_PDOR ^= 0b00100000;
  delay(500);
  GPIOC_PDOR ^= 0b00100000;
  
  while(1){
      if (input_string[0]){
          handle_comms();
        }
    }

}

void handle_comms(){
      int comm_id;
      for (int i = 0; i < 26; i += 2){
          if (comm_list[i] == input_string) comm_id = (int)(comm_list[i + 1]);
        }
      switch(comm_id){
          case 260:
            Serial.write("42");
            Serial.write(DELIM);
            break;
          default:
            Serial.write(EDELIM);
        }
    input_string[0] = 0x00;
  }

void serialEvent(){
    while (Serial.available()){
        *stend++ = (char)Serial.read();
      }
      stend = input_string;
  }
