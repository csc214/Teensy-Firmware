#ifndef PROTO_H
#define PROTO_H

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

#define THSHIFT 261   // Toggles horizontal register and reset
#define TVSHIFT 262   // Toggles vertical shift and frame transfer

#define DACOFF  270   // Sets DAC voltages to zero

#define DELIM   " OK\n"  // Delimiter for returning messages from teensy
#define EDELIM  " ERR\n" // Delimiter for returning an error message
#define HALT    " HALT\n"// Delimiter for pollendrun stop


#define WIDTH  768 // Resolution for KAI-0370C
#define HEIGHT 484
#define XPITCH  78 // Horizontal Pixel Pitch (px/mm)
#define YPITCH  63 // Vertical Pixel Pitch (px/mm)
#define CCDTYPE  2 // CCD Type for KAI-0370C

#define VERSION  "0.0.1"

#define NCOMMS 29  // Number of commands available

typedef struct {
  String name;
  short id;
} comm_t;
#endif
