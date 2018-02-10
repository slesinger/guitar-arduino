#ifndef GUITAR_V2_H
#define GUITAR_V2_H


#define UNDEF 78
#define MIDI_CONTROL_CHORUS 0x5D
#define MIDI_CONTROL_SUSTAIN 0x40

#define XX 130 //string is played but no fret is pressed
#define ZZ 0  //string is not played

uint8_t instruments[13] = {0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08, 0x09,0x0a,0x0b,0x0c, 0x0d};

#endif
