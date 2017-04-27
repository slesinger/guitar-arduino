#include <Arduino.h>

#include "guitar_v2.h"
#include "chords.h"
#include "profile_default.h"
#include "MIDIUSB.h" //https://github.com/arduino-libraries/MIDIUSB

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


#define LASER_ANALOG_TH 900 //for analog read of bpw34. 900-1023 means laser is blocked, 0-600 means laser hit diode
#define LASER_ANALOG_THOLD_US 5000 //[us]
#define LASER_THOLD 1000 //[us]
#define STOP_THOLD 500000 //[us]
#define FRET_SLIDING_THOLD 500000 //[us]
#define DEFAULT_VELOCITY 96
#define VEL_TIME_OUT_OF_RANGE 200000 //[us]
#define VEL_TIME_MIN 6000
#define VEL_TIME_MAX 25000
#define VELOCITY_MAX 127
#define VELOCITY_MIN 80
#define EFFECT_THOLD_MS 1000 //[ms] how often to effect setting
#define VIBRATO_THOLD_MS 5 //[ms] how often send vibrato events, real event sent only if not 0
//#define DBG 1
#define DBG2 1

int fret_pins[13] = {47,43,25,41,27,23,35,37,45,29,31,33,39};
int string_pins[6]= {32,30,28,26,24,22};
int laser_pins[6] = {A0,A2,A4,A3,A6,A5};  //bpw34 photodiode
int emphasis_pin  = A1;
int stop_tone_pin = 53;
int vibrato_pin = A9;
int effect_pin = A8;
uint8_t pitch = 0;

boolean fret_status[13][6];
int fret_status_last_f = UNDEF;
int fret_status_last_used = UNDEF;
unsigned long fret_status_last_f_us = 0;
int fret_status_last_s = UNDEF;

byte laser_status[6];
unsigned long laser_status_last_update_us[6];
boolean last_alaser_status[7]; //needed just for analog read, including emphasis
long effect_last_update_ms = 0;
long vibrato_last_update_ms = 0;
unsigned long stop_status_last_update_us = 0;

byte emphasis_status = 0;
unsigned long emphasis_status_last_update_us = 0;
unsigned long emphasis_last_blocked_us = 0;

uint8_t velocity = DEFAULT_VELOCITY;


inline void set_fret(int f_idx) {
  digitalWrite(fret_pins[f_idx], HIGH);
}
inline void unset_fret(int f_idx) {
  digitalWrite(fret_pins[f_idx], LOW);
}

inline boolean is_string_pressed_on_active_fret(int s_idx) {
  return digitalRead(string_pins[s_idx]);
}

uint8_t get_note(int l_idx) {
  //TODO switch to profile pointer to allow profile switching
  //accord is of type uint8_t[6]
  uint8_t *accord = default_profile[fret_status_last_s][fret_status_last_f];
  uint8_t note = *(accord + l_idx);
#ifdef DBG
  Serial.print(fret_status_last_s);
  Serial.print("n");
  Serial.println(fret_status_last_f);
#endif
#ifdef DBG2
  Serial.print(l_idx);
  Serial.print(" ");
  Serial.print(fret_status_last_s);
  Serial.print(" ");
  Serial.print(fret_status_last_f);
  Serial.print(" ");
  Serial.println(note);
#endif
  return note;
}

//------ handle events --------

inline void send_fret_sliding_event(int dir, int f) {
  pitch += dir * 10;

  if (dir == 0)
    pitch = 0;

#ifdef DBG
  Serial.print(f);
  if (dir > 0)
    Serial.print(">");
  else
    Serial.print("<");
  Serial.print(dir);
  Serial.print("|");
  Serial.println(pitch);
#endif

  midiEventPacket_t packet = {0x0E, 0xE0 | 0, 0, pitch};  //channel, receiver sensitivity, velocity
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();
}


inline void send_fret_event(boolean pressed, int s, int f) {
#ifdef DBG
  if (pressed)
    Serial.print("F");
  else
    Serial.print("f");
  Serial.println(f * 10 + s);
#endif

  unsigned long diff_us = micros() - fret_status_last_f_us;

  //check for sliding
  int dir = f - fret_status_last_used; //0: no sliding, -1:sliding outward: 1: sliding inward
  if (diff_us < FRET_SLIDING_THOLD) {
    send_fret_sliding_event(dir, f);
  }

  fret_status_last_used = f;
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
#ifdef DBG
  if (laser_blocked)
    Serial.print("L");
  else
    Serial.print("l");
  Serial.println(l_idx);
#endif

  if (laser_blocked) {
    if (pitch != 0) {
      send_fret_sliding_event(0, 0);  //reset pitch
    }
    //calculate velocity
    if (l_idx == 0) {
      unsigned long diff_us = micros() - emphasis_last_blocked_us;

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
#ifdef DBG
  Serial.print(diff_us);
  Serial.print(">");
  Serial.println(velocity);
#endif
    }
    else {  //for other laser check if time difference is out of range
      unsigned long diff_us = micros() - emphasis_last_blocked_us;
      if ( (diff_us < 0) || (diff_us > VEL_TIME_OUT_OF_RANGE) ) {
        velocity = DEFAULT_VELOCITY;
#ifdef DBG
  Serial.print(diff_us);
  Serial.print("]");
  Serial.println(velocity);
#endif
      }
    }

    //noteOn
    uint8_t note = get_note(l_idx);
    if (note != 0) {
      midiEventPacket_t packet = {0x09, 0x90 | 0, note, velocity};  //channel, pitch, velocity
      MidiUSB.sendMIDI(packet);
      MidiUSB.flush();
    }
  }
}

inline void send_emphasis_event(boolean emphasis_blocked) {
#ifdef DBG
  if (emphasis_blocked)
    Serial.println("E");
  else
    Serial.println("e");
#endif

  if (emphasis_blocked) { //set time stamp start to measure time between emphasis and laser 0 duration for calculation of velocity
    emphasis_last_blocked_us = micros();
  }
}

inline void send_stop_tone_event(boolean stop_pressed) {
#ifdef DBG
  if (stop_pressed)
    Serial.println("S");
  else
    Serial.println("s");
#endif

  if (stop_pressed) {

    //find if special function is pressed
    uint8_t program = -1;
    if ((fret_status[0][5] == true)) {
      program = 0x19;
    }
    if ((fret_status[1][5] == true)) {
      program = 0x1B;
    }

    //noteOff for all lasers
    if (program != -1) {
      midiEventPacket_t special = {0x0C, 0xC0 | 0, program, 0};  //channel, pitch, velocity
      MidiUSB.sendMIDI(special);
      MidiUSB.flush();
    }
    midiEventPacket_t packet = {0x0B, 0xB0 | 0, 120, 0};  //channel, pitch, velocity
    MidiUSB.sendMIDI(packet);
    MidiUSB.flush();
  }
}

inline void send_vibrato_event(int val) {
  //300-360    0
  //<300-0     -
  //>360-1023  +
  int val2 = map(val, -1024, 1024, 0, 32768);
  uint8_t val_lsb = val2 && 0xFF;
  uint8_t val_msb = val2 >> 8;
  midiEventPacket_t packet = {0x0E, 0xE0 | 0, val_lsb, val_msb};
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();

}

inline void send_effect_event(int val) { //0-1023
  uint8_t val_norm = map(val, 0, 1023, 0, 127);
  midiEventPacket_t packet = {0x0B, 0xB0 | 0, MIDI_CONTROL_CHORUS, val_norm};
  MidiUSB.sendMIDI(packet);
  MidiUSB.flush();
}

// ---- ISR handling -----
inline void laser_common_ISR(int l_idx) {
  /*Serial.print(digitalRead(laser_pins[0]));
  Serial.print(digitalRead(laser_pins[1]));
  Serial.print(digitalRead(laser_pins[2]));
  Serial.print(digitalRead(laser_pins[3]));
  Serial.print(digitalRead(laser_pins[4]));
  Serial.println(digitalRead(laser_pins[5]));*/
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

inline void laser_common_analog(int l_idx, boolean laser_blocked) {
  unsigned long us = micros();
  if (laser_blocked != laser_status[l_idx]) { //ISR invoked but status did not change from current, do not know why ISR is called -> ignore

    if (us > (laser_status_last_update_us[l_idx] + LASER_ANALOG_THOLD_US)) {

      laser_status[l_idx] = laser_blocked;
      laser_status_last_update_us[l_idx] = us;
      send_laser_event((boolean)laser_blocked,  l_idx);
    } //else ignore, change was due to jitter, likely

  }
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
      send_stop_tone_event(true);
      stop_status_last_update_us = us;
    }
  }
}

void laser0_ISR()   { laser_common_ISR(0); }
void laser1_ISR()   { laser_common_ISR(1); }
void laser2_ISR()   { laser_common_ISR(2); }
void laser3_ISR()   { laser_common_ISR(3); }
void laser4_ISR()   { laser_common_ISR(4); }
void laser5_ISR()   { laser_common_ISR(5); }

//-------------------------
void setup()
{
#ifdef DBG
  Serial.begin(115200);
  Serial.println("Starting guitar_v2");
#endif
#ifdef DBG2
  Serial.begin(115200);
#endif

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
//    pinMode(laser_pins[l], INPUT);  //neni treba pro analog read
  }

//  attachInterrupt(digitalPinToInterrupt(laser_pins[0]), laser0_ISR, RISING);
//  attachInterrupt(digitalPinToInterrupt(laser_pins[1]), laser1_ISR, CHANGE);  //there is some HW difference, does not jitter that much
//  attachInterrupt(digitalPinToInterrupt(laser_pins[2]), laser2_ISR, RISING);
//  attachInterrupt(digitalPinToInterrupt(laser_pins[3]), laser3_ISR, RISING);
//  attachInterrupt(digitalPinToInterrupt(laser_pins[4]), laser4_ISR, RISING);
//  attachInterrupt(digitalPinToInterrupt(laser_pins[5]), laser5_ISR, RISING);

//  pinMode(emphasis_pin, INPUT);  //neni treba pro analog read
//  attachInterrupt(digitalPinToInterrupt(emphasis_pin), laseremphasis_ISR, RISING);
  last_alaser_status[6] = false;  //emhasis

  //init input pins with internal pullup resistors
  pinMode(stop_tone_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(stop_tone_pin), stop_ISR, FALLING);

}



//-------------------------
void loop()
{
  //scan fret by fret
  for (int f = 0; f <= 12; f++) { //f<=12

    set_fret(f);
    //scan strings for this fret
    for (int s = 0; s <= 5; s++) {
      boolean pressed = is_string_pressed_on_active_fret(s);
      //emit event if status changed
      if (pressed ^ fret_status[f][s]) {
        send_fret_event(pressed, s, f);
        fret_status[f][s] = pressed;
      }
    }
    unset_fret(f);
  }


  //read analog emphasis
  boolean blocked = false;
  if (analogRead(emphasis_pin) > LASER_ANALOG_TH)
    blocked = true;
  if (last_alaser_status[6] != blocked) { //status changed
    last_alaser_status[6] = blocked;
    laseremphasis_analog(blocked);
  }

  for (int i = 0; i < 6; i++) {
    boolean blocked = false;
    if (analogRead(laser_pins[i]) > LASER_ANALOG_TH)
      blocked = true;

    if (last_alaser_status[i] != blocked) { //status changed
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
