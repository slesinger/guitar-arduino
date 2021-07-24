// #include <Arduino.h>

#include "src/guitar_v2.h"
#include "src/chords.h"
#include "src/profile_default.h"
#include "MIDIUSB.h" //https://github.com/arduino-libraries/MIDIUSB
#include "SPI.h"

//profiles -> profile (which accords are mapped to fret fields) -> accord (definition of notes)
/*
typedef struct
{
  uint8_t header;  //status
  uint8_t byte1;   //data1
  uint8_t byte2;
  uint8_t byte3;
}midiEventPacket_t;
*/
//Linux setup http://tedfelix.com/linux/linux-midi.html
//MIDI reference https://users.cs.cf.ac.uk/Dave.Marshall/Multimedia/node155.html
//MIDI reference http://www.gweep.net/~prefect/eng/reference/protocol/midispec.html#Off
//MIDI message specs https://www.midi.org/specifications/item/table-1-summary-of-midi-message

//guitarpack.sf2 has tuning -12
//accounsting.sf2 has 0
#define TUNING -12


//const int LASER_ANALOG_TH[] = {960, 920, 975, 711, 900, 975}; //for analog read of bpw34. 900-1023 means laser is blocked, 0-600 means laser hit diode
int LASER_ANALOG_TH[] = {1024, 1024, 1024, 1024, 1024, 1024}; //for analog read of bpw34. 900-1023 means laser is blocked, 0-600 means laser hit diode
#define LASER_ANALOG_EMPH 975 //for analog read of bpw34. 900-1023 means laser is blocked, 0-600 means laser hit diode
#define LASER_ANALOG_THOLD_US 500 //[us] fastest accepted laser state change to prevent noise, fast strum may not be accepted 
//off topic: measured fastest cadence of strumming is 120ms(120000us)
#define LASER_THOLD 1000 //[us]
#define STOP_THOLD 500000 //[us]
#define FRET_SLIDING_THOLD 500000 //[us]
#define PITCH_HALFTONE 800
#define VEL_TIME_OUT_OF_RANGE 200000 //[us]
#define VEL_TIME_MIN 6000
#define VEL_TIME_MAX 25000
#define VELOCITY_MAX 127
#define DEFAULT_VELOCITY 84 //bylo 96 ale byl to maly rozdil
#define VELOCITY_MIN 74 //bylo 80
#define EFFECT_THOLD_MS 5000 //[ms] how often to effect setting
#define VIBRATO_THOLD_MS 5 //[ms] how often send vibrato events, real event sent only if not 0
#define NO_FRETS 13
//#define DBG 1
//#define DBG2 1

int fret_pins[NO_FRETS] = {47,43,25,41,27,23,35,37,45,29,31,33,39};
int string_pins[6]= {32,30,28,26,24,22};
int laser_pins[6] = {A0,A2,A4,A3,A6,A5};  //bpw34 photodiode
int emphasis_pin  = A1;
int stop_tone_pin = 53;
int vibrato_pin = A9;
int effect_pin = A8;
uint8_t pitch = 0;

boolean fret_status[NO_FRETS][6];
int fret_status_last_f = UNDEF;
int frets0_when_noteon = UNDEF;
unsigned long fret_status_last_f_us = 0;
int fret_status_last_s = UNDEF;

byte laser_status[6];
uint8_t played_notes[6] = {60,60,60,60,60,60};
unsigned long laser_status_last_update_us[6];
boolean last_alaser_status[7]; //needed just for analog read, including emphasis
long effect_last_update_ms = 0;
long vibrato_last_update_ms = 0;
unsigned long stop_status_last_update_us = 0;

byte emphasis_status = 0;
unsigned long emphasis_status_last_update_us = 0;
unsigned long emphasis_last_blocked_us = 0;

uint8_t velocity = DEFAULT_VELOCITY;
uint8_t selected_capo_id = 0;

inline void set_fret(int f_idx) {
  digitalWrite(fret_pins[f_idx], HIGH);
}
inline void unset_fret(int f_idx) {
  digitalWrite(fret_pins[f_idx], LOW);
}

inline boolean is_string_pressed_on_active_fret(int s_idx) {
  return digitalRead(string_pins[s_idx]);
}

void send_midi_debug(uint8_t v1, uint8_t v2) {
  midiEventPacket_t packet0 = {0x09, 0x90 | 1, v1, v2};
  MidiUSB.sendMIDI(packet0);
  MidiUSB.flush();
  delay(2);
}

uint8_t get_note(int l_idx) {
  //TODO switch to profile pointer to allow profile switching
  //accord is of type uint8_t[6]
  uint8_t *accord = default_profile[fret_status_last_s][fret_status_last_f];
  uint8_t note = *(accord + l_idx);
  uint8_t capo_note = capo[selected_capo_id][l_idx];
  //calculate capo
  if ( (note == XX) || (note < capo_note) ) {
    note = capo_note + TUNING;
  }
  else {
    note = note + TUNING; //note not affected by capo, just proceed
  }
  return note;
}

//------ handle events --------

inline void send_fret_sliding_event(int delta, int f) {
  if (delta != 0) {
    send_pitch_event(delta * PITCH_HALFTONE);  //-4096 <> 4096
  }
}


inline void send_fret_event(boolean pressed, int s, int f) {

  //check for sliding
  if ( pressed && (s == 0) ) {  //jen pro vybrnkavani
    unsigned long diff_us = micros() - fret_status_last_f_us;
    if (diff_us < FRET_SLIDING_THOLD) {
      int delta = f - ( (frets0_when_noteon != UNDEF) ? frets0_when_noteon : 0 ); //0: no sliding, -1:sliding outward: 1: sliding inward
      send_midi_debug(delta +100, frets0_when_noteon);
      send_fret_sliding_event(delta, f);
    }
    fret_status_last_f_us = micros();
  }

  //set status
  if (pressed) {
    fret_status_last_f = f;
    fret_status_last_s = s;
  }
  else {
    fret_status_last_f = UNDEF;
    fret_status_last_s = UNDEF;
  }
}


inline void send_laser_event(boolean laser_blocked, int l_idx) {

  // Mute strings
  if (laser_blocked) {
    int s0p = 0;
    for (int fr = 0; fr < NO_FRETS; fr++) {
      s0p += (fret_status[fr][0]) ? 1 : 0;
    }
    if (s0p > 0) {
      for (int i = 0; i < 6; i++) { //mute all strings if vybrnkavaci struna is pressed
        midiEventPacket_t packet0 = {0x08, 0x80 | 0, played_notes[i], 0};
        MidiUSB.sendMIDI(packet0);
        MidiUSB.flush();
        delay(2);
      }
    }
    if (s0p == 0) {  // finger is muting current laser
      midiEventPacket_t packet0 = {0x08, 0x80 | 0, played_notes[l_idx], 0};
      MidiUSB.sendMIDI(packet0);
      MidiUSB.flush();
      delay(2);
    }
  }  //end of laser_blocked

  // Play strings
  if (!laser_blocked) {  //finger is releasing string(laser)
    if (pitch != 0) {
      pitch = 0;
      send_fret_sliding_event(0, 0);  //reset pitch
    }
    //calculate velocity
    unsigned long diff_us = micros() - emphasis_last_blocked_us;
    if (l_idx == 0) {
      //out of range - set default velocity because emphasis laser was likely not blocked right before laser 0
      if ( (diff_us < 0) || (diff_us > VEL_TIME_OUT_OF_RANGE) ) {
        velocity = DEFAULT_VELOCITY;
      }
      else {
        if (diff_us < VEL_TIME_MIN) {
          velocity = VELOCITY_MAX;
        }
        else {
          if (diff_us > VEL_TIME_MAX) {
            velocity = VELOCITY_MIN;
          }
          else {
            velocity = map(diff_us, VEL_TIME_MIN, VEL_TIME_MAX, VELOCITY_MAX, VELOCITY_MIN);
          }
        }
      }
    }
    else {  //for other laser check if time difference is out of range
      if ( (diff_us < 0) || (diff_us > VEL_TIME_OUT_OF_RANGE) ) {
        velocity = DEFAULT_VELOCITY;
      }
    } //end of calculate velocity

    //noteOn
    uint8_t note = get_note(l_idx);
    if (note != 0) {
      frets0_when_noteon = fret_status_last_f;
      played_notes[l_idx] = note;
      midiEventPacket_t packet = {0x09, 0x90 | 0, note, velocity};  //channel, pitch, velocity
      MidiUSB.sendMIDI(packet);
      MidiUSB.flush();
    }
  }
}

inline void send_emphasis_event(boolean emphasis_blocked) {
  if (!emphasis_blocked) { //set time stamp start to measure time between emphasis and laser 0 duration for calculation of velocity
    emphasis_last_blocked_us = micros();
  }
}

inline void send_stop_tone_event(boolean stop_pressed, boolean skip_setup) {

  if (!skip_setup) {
    //find if instrument selection is pressed (top string), fret selects instrument
    for (int i = 0; i < NO_FRETS; i++) {
      if ((fret_status[i][5] == true)) {
        midiEventPacket_t special = {0x0C, 0xC0, instruments[i], 0};
        MidiUSB.sendMIDI(special);
        MidiUSB.flush();
      }
    }

    //find if capo selection is requested (bottom string), max 8th fret, 9-13 fret means remove capo
    for (int i = 0; i < NO_FRETS; i++) {
      if ((fret_status[i][0] == true)) {
        selected_capo_id = (i < 8) ? i + 1 : 0;  //frets 9 and more removes capo
      }
    }
  }

  if (stop_pressed) {
    //noteOff for all lasers
    midiEventPacket_t packet = {0x0B, 0xB0 | 0, 120, 0};  //channel, pitch, velocity
    MidiUSB.sendMIDI(packet);
    MidiUSB.flush();
  }
}

//val <0; 1023>
inline void send_vibrato_event(int val) {
  //300-360    0
  //<300-0     -
  //>360-1023  +

  unsigned int val2 = 0;
  if (val < 300) {
    val2 = map(val, 0, 300,  8192-4096, 8192);
  }
  else if (val > 400) {
    val2 = map(val, 400, 1024, 8192, 8192+4096);
  }
  else return;  //ignore center position

  uint8_t val_lsb = val2 & 0x7F;
  uint8_t val_msb = val2 >> 7;
  midiEventPacket_t packet = {0x0E, 0xE0 | 0, val_lsb, val_msb};
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();
}

//val <-4096; 4096>
inline void send_pitch_event(int val) {
  unsigned int val2 = 8192 + val;
  uint8_t val_lsb = val2 & 0x7F;
  uint8_t val_msb = val2 >> 7;
  midiEventPacket_t packet = {0x0E, 0xE0 | 0, val_lsb, val_msb};
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();
}

inline void send_effect_event(int val) { //0-1023
  uint8_t val_norm = map(val, 0, 1023, 0, 127);
  midiEventPacket_t packet = {0x0B, 0xB0 | 0, MIDI_CONTROL_SUSTAIN, val_norm};
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();
}

// ---- ISR handling -----
/*
inline void laser_common_ISR(int l_idx) {
  unsigned long us = micros();
  int laser_blocked = digitalRead(laser_pins[l_idx]); //LOW for laser radiates, HIGH laser blocked

  if (laser_blocked != laser_status[l_idx]) { //ISR invoked but status did not change from current, do not know why ISR is called -> ignore

    if (us > (laser_status_last_update_us[l_idx] + LASER_THOLD)) {
      laser_status[l_idx] = laser_blocked;
      laser_status_last_update_us[l_idx] = us;
      send_laser_event((boolean)laser_blocked,  l_idx);
    } //else ignore, change was due to jitter, likely

  }
}
*/
inline boolean all_lasers_blocked() {
  if (laser_status[0] == true &&
      laser_status[1] == true &&
      laser_status[2] == true &&
      laser_status[3] == true &&
      laser_status[4] == true &&
      laser_status[5] == true
  )
    return true;
  else
    return false;
}

inline void set_all_laser_unblocked() {
  laser_status[0] = false;
  laser_status[1] = false;
  laser_status[2] = false;
  laser_status[3] = false;
  laser_status[4] = false;
  laser_status[5] = false;
}

inline void laser_common_analog(int l_idx, boolean laser_blocked) {
  unsigned long us = micros();
//  if (laser_blocked != laser_status[l_idx]) { //ISR invoked but status did not change from current, do not know why ISR is called -> ignore

    if (us > (laser_status_last_update_us[l_idx] + LASER_ANALOG_THOLD_US)) {
      laser_status_last_update_us[l_idx] = us;

      laser_status[l_idx] = laser_blocked;

      if (all_lasers_blocked()) {  //detect hand over all lasers, stop sound
        set_all_laser_unblocked();
        send_stop_tone_event(true, true);
      }

      send_laser_event((boolean)laser_blocked, l_idx);
    } //else ignore, change was due to jitter, likely

//  }
}

void laseremphasis_ISR()   {
  unsigned long us = micros();
  int laser_blocked = digitalRead(emphasis_pin); //LOW for laser radiates, HIGH laser blocked

  if (laser_blocked != emphasis_status) {

    if (us > (emphasis_status_last_update_us + LASER_THOLD)) {
      emphasis_status = laser_blocked;
      emphasis_status_last_update_us = us;
      send_emphasis_event((boolean)laser_blocked);
    } //else ignore, change was due to jitter, likely
  }
}

void laseremphasis_analog(boolean laser_blocked)   {
  unsigned long us = micros();

  if (laser_blocked != emphasis_status) {

    if (us > (emphasis_status_last_update_us + LASER_THOLD)) {
      emphasis_status = laser_blocked;
      emphasis_status_last_update_us = us;
      send_emphasis_event((boolean)laser_blocked);
    } //else ignore, change was due to jitter, likely
  }
}

void stop_ISR()   {
  int stop = digitalRead(stop_tone_pin); //LOW pressed; HIGH unpressed
  if (stop == LOW) { //confirm the actual value on pin
    unsigned long us = micros();
    if (us > (stop_status_last_update_us + STOP_THOLD)) {
      send_stop_tone_event(true, false);
      stop_status_last_update_us = us;
    }
  }
}

// TODO je toto potreba na analog?
//void laser0_ISR()   { laser_common_ISR(0); }
//void laser1_ISR()   { laser_common_ISR(1); }
//void laser2_ISR()   { laser_common_ISR(2); }
//void laser3_ISR()   { laser_common_ISR(3); }
//void laser4_ISR()   { laser_common_ISR(4); }
//void laser5_ISR()   { laser_common_ISR(5); }

//-------------------------
void setup() {

  init_default_profile();

  //zero fret_status and set frets as source of 3.3V (in scan mode)
  for (int f = 0; f <= 12; f++) {
    pinMode(fret_pins[f], OUTPUT);
    unset_fret(f);
    for (int s = 0; s <= 5; s++) {
      fret_status[f][s] = false;
    }
  }

  //string are connected with explicit 100K resistor to gnd (pull down)
  for (int s = 0; s <= 5; s++) {
    pinMode(string_pins[s], INPUT);
  }

  //zero laser_status
  for (int l = 0; l <= 5; l++) {
    laser_status[l] = 0;
    laser_status_last_update_us[l] = 0;
    last_alaser_status[l] = false;
  }

  last_alaser_status[6] = false;  //emhasis

  //init input pins with internal pullup resistors
  pinMode(stop_tone_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(stop_tone_pin), stop_ISR, FALLING);
  delay(1000); //because analogRead returns strange values at start
}



//-------------------------
void loop()
{

/* Debugging laser transistor levels
      int a = analogRead(laser_pins[0]);
      send_midi_debug(0, (int)((float)a / 8));
      a = analogRead(laser_pins[1]);
      send_midi_debug(1, (int)((float)a / 8));
      a = analogRead(laser_pins[2]);
      send_midi_debug(2, (int)((float)a / 8));
      delay(200);
      a = analogRead(laser_pins[3]);
      send_midi_debug(3, (int)((float)a / 8));
      a = analogRead(laser_pins[4]);
      send_midi_debug(4, (int)((float)a / 8));
      a = analogRead(laser_pins[5]);
      send_midi_debug(5, (int)((float)a / 8));
      delay(200);
      return;
*/

  
  bool narazil_na_prst = false;
  //scan fret by fret
  for (int f = 0; f <= (NO_FRETS-1); f++) { //f<=12

    set_fret(f);
    //scan strings for this fret
    for (int s = 5; s >= 0; s--) {
      boolean pressed = is_string_pressed_on_active_fret(s);
      //emit event if status changed
      if (pressed ^ fret_status[f][s]) {
        send_fret_event(pressed, s, f);
        fret_status[f][s] = pressed;
      }
      if (pressed == true) {
        narazil_na_prst = true;
        break;
      }
    }
    unset_fret(f);
    if (narazil_na_prst == true) {
        break;
    }
  }

  //read analog emphasis
  boolean blocked = false;
  if (analogRead(emphasis_pin) > LASER_ANALOG_EMPH)
    blocked = true;
  if (last_alaser_status[6] != blocked) { //status changed
    last_alaser_status[6] = blocked;
    laseremphasis_analog(blocked);
  }

  //read lasers
  for (int i = 0; i < 6; i++) {
    boolean blocked = false;

    int a = analogRead(laser_pins[i]);
//    if (i==1){
//    send_midi_debug(i, (int)((float)a / 8));
//    send_midi_debug(i, (int)((float)LASER_ANALOG_TH[i] / 8));
//    delay(100);
//    }
    if (a > LASER_ANALOG_TH[i]+5)
      blocked = true;
    else {
      int delta = (int)((a - LASER_ANALOG_TH[i]) / 2);
      if (delta < -1) delta = -1;
      if (delta > 1) delta = 1;
      LASER_ANALOG_TH[i] += delta; //update level of laser sensors dynamically
    }
      
    if (last_alaser_status[i] != blocked) { //status changed
//      send_midi_debug(i, (int)((float)a / 8));
      last_alaser_status[i] = blocked;
      laser_common_analog(i, blocked);
    }
  }

  long ms = millis();
  if (ms > (vibrato_last_update_ms + VIBRATO_THOLD_MS)) {
    vibrato_last_update_ms = ms;
    send_vibrato_event(analogRead(vibrato_pin));
  }

  if (ms > (effect_last_update_ms + EFFECT_THOLD_MS)) {
    effect_last_update_ms = ms;
    send_effect_event(analogRead(effect_pin));
  }
}
