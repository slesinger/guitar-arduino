#ifndef GUITAR_V2_H
#define GUITAR_V2_H


#define UNDEF 78
#define MIDI_CONTROL_CHORUS 0x5D

#define XX 130 //string is played but no fret is pressed
#define ZZ 0  //string is not played

uint8_t instruments[13] = {0x19,0x1a,0x1b,0x1c, 0x00,0x01,0x02,0x03, 0x15,0x16,0x17,0x18, 0x19};

#endif
