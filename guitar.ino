/*
N = pick string
   - block laser beam -> plays tone
   - unblock laser beam -> do nothing

len=release tab
   - stop tone playing

stop all tones button
   - stop all tones

/ = slide up
   - needs to be tabbed with two finger next to each other and slide
   - pitch of tone changes when following change on tabs is detected
     - t1 is pressed > t1+t2 is pressed > t2 is pressed
       where t1 can be > or < t2 with max distance 1

\ = slide down
   - see slide up

h = hammer-on
   - if more far tab is pressed and closer (max 4 tabs away) tab is pressed and depressed -> new tone is played
     - if tone was played -> play new tone louder
     - if tone did not play -> play new tone relativeli quiet

p = pull-off
   - not supported due to gesture not possible

~ = vibrato
   - using long lever
   
+ = harmonic
   - no need to suppot it yet
   
x = Dead note
   - disabled in chord library
   
b = Bend
   - using long vibrato lever
pb = Pre-bend
br = Bend release
pbr = Pre-bend release
brb = Bend release bend
*/

int fret_pins[12] = {22,23,24,25,26,27,28,29,30,31,32,33};
int string_pins[6]= {36,37,38,39,40,41};
int laser_pins[6] = {46,47,48,49,50,51};
int emphasis_pin  = 52;
int stop_tone_pin = 53;

boolean fret_status[12][6];
boolean laser_status[6];
boolean emphasis_status = false;
boolean stop_tone_status = false;

inline void init_digital_in(int pin) {
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}

inline boolean is_fret_pressed(int f) {
  return digitalRead(fret_pins[f]);
}

//put hesitation delay in this function to prevent jitter when laser pressed half-way
inline boolean is_laser_blocked(int l) {
  //TODO prevent jitter
  return digitalRead(laser_pins[l]);
}

inline boolean is_emphasis_blocked() {
  return digitalRead(emphasis_pin);
}

inline boolean is_stop_tone_pressed() {
  //TODO debounce
  return digitalRead(stop_tone_pin);
}

inline void set_string(int s) {
  digitalWrite(string_pins[s], HIGH);
}

inline void unset_string(int s) {
  digitalWrite(string_pins[s], LOW);
}

inline void send_fret_event(boolean pressed, int f, int s) {
  if (Serial.availableForWrite() >= 1)
    Serial.write(s * 12 + f + 1);
}

inline void send_laser_event(boolean laser_blocked, int l) {
  if (Serial.availableForWrite() >= 1)
    Serial.write(78 + l + 128 * laser_blocked);
}

inline void send_emphasis_event(boolean emphasis_blocked) {
  if (Serial.availableForWrite() >= 1)
    Serial.write(84 + 128 * emphasis_blocked);
}

inline void send_stop_tone_event() {
  if (Serial.availableForWrite() >= 1)
    Serial.write(85);
}

//-------------------------
void setup()
{
  Serial.begin(115200);

  //zero fret_status
  for (int f = 0; f <= 11; f++) {
    for (int s = 0; s <= 5; s++) {
      fret_status[f][s] = 0;
    }
  }
  //zero laser_status
  for (int l = 0; l <= 5; l++) {
    laser_status[l] = 0;
    init_digital_in(laser_pins[l]);
  }

  //init input pins with internal pullup resistors
  init_digital_in(emphasis_pin);
  init_digital_in(stop_tone_pin);
}

//-------------------------
void loop()
{
  //scan string by string
  for (int s = 0; s <= 5; s++) {
    set_string(s);

    //scan frets for this string
    for (int f = 0; f <= 11; f++) {
      boolean pressed = is_fret_pressed(f);
      //emit event if status changed
      if (pressed ^ fret_status[f][s]) {
        send_fret_event(pressed, f, s);
      }
      fret_status[f][s] = pressed;
    }
    unset_string(s);

    //read emphasis laser
    boolean emphasis_blocked = is_emphasis_blocked();
    if (emphasis_blocked ^ emphasis_status) {
      send_emphasis_event(emphasis_blocked);
    }
    emphasis_status = emphasis_blocked;

    //scan lasers (in time-multiplex to keep high scan freq)
    for (int l = 0; l <= 5; l++) {
      boolean laser_blocked = is_laser_blocked(l);
      if (laser_blocked ^ laser_status[l]) {
        send_laser_event(laser_blocked, l);
      }
      laser_status[l] = laser_blocked;
    }
    
    //read stop_tone button
    boolean st = is_stop_tone_pressed();
    if (st ^ stop_tone_status) {
      send_stop_tone_event();
    }
    stop_tone_status = st;


  }

}
