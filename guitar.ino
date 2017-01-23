PINS_IN = [18, 23,24, 2,3,4, 17,27,22, 10,9]  #vstup skrz prazce
PINS_OUT= [7,8,25, 11]  #vystup napeti do strun

zmacknuto = -1  #posledni zmacknuty prazec/struna. Adresuje se jako struna * 12 + prazec
lasers = 0 #ktere lasery jsou prerusene

def init_pins():
    GPIO.setmode(GPIO.BCM)
    for p in PINS_IN:
        GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    for p in PINS_OUT:
        GPIO.setup(p, GPIO.OUT)
    time.sleep(0.1)

def init_synth():
    fs = fluidsynth.Synth(gain=1.0)
    fs.start(driver='alsa')
    sfid = fs.sfload("/home/pi/guitar/sf2/acoustic.sf2")
    fs.program_select(0, sfid, 0, 0)
    fs.program_select(1, sfid, 0, 0)
    fs.program_select(2, sfid, 0, 0)
    fs.program_select(3, sfid, 0, 0)
    fs.program_select(4, sfid, 0, 0)
    fs.program_select(5, sfid, 0, 0)
    fs.noteon(0, 60, 90)
    time.sleep(0.5)
    return fs

def play_string(struna):
    global zmacknuto
    idx = {1:2, 2:3, 4:4, 8:5, 16:6, 32:7}

    #zadna struna se nezahrala? nic nedelej
    if struna == 0:
        return

    #kdyz neni nic zmacknuto zahraj ton daneho laseru. Definovano jako posledni zaznam s indexem 48
    accord = AK[ 48 if zmacknuto == -1 else zmacknuto ]
    tone = accord[idx[struna]]
    if tone != 0:
        fs.noteon(idx[struna]-1, tone, 80)


        #bit 0 - 5 lasery
        #bit 6 je rezervovan pro prazec 11
        #bit 7 je rezervovan pro nejake tlacitko, napr. pro zarazit zvuk vsech strun
        def read_lasers():
            #global lasers
            data = ser.read(20)
            #neprisla-li data nic nedelej
            #if len(data) == 0:
            #    return

            for d in data:
                l = ord(d) - 64
                if l > 63: continue  #zahod pripadne spatne byty
                play_string(l)

            #nastav posledni stav laseru na priste
            #lasers = l


        def play_accord(zmacknuto):
            a = AK[zmacknuto]
            #print "{} - {}".format(zmacknuto, a[1])
            MS = 0.001
            fs.noteon(0, a[2], 90)
            time.sleep(MS)
            fs.noteon(1, a[3], 90)
            time.sleep(MS)
            fs.noteon(2, a[4], 90)
            time.sleep(MS)
            fs.noteon(3, a[5], 90)
            time.sleep(MS)
            fs.noteon(4, a[6], 90)
            time.sleep(MS)
            fs.noteon(5, a[7], 90)
            time.sleep(0.15)


        #main program

        init_pins()
        fs = init_synth()
        ser = serial.Serial('/dev/ttyUSB0', 57600, timeout=0, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)

        play_accord(1)

        while True:
            #predpokladej, ze neni nic zmacknuto (jen pro tento cyklus)
            prazec = -1 #id prazce od 0 do 10 (12 prazec se musi cist pres arduino)
            struna = -1 #id struny 0=vybrnkavani, 1,2,3 jsou struny od spoda

            read_lasers()

            #nastav vsechny struny na zapnute
            GPIO.output(PINS_OUT[0], GPIO.LOW)
            GPIO.output(PINS_OUT[1], GPIO.LOW)
            GPIO.output(PINS_OUT[2], GPIO.LOW)
            GPIO.output(PINS_OUT[3], GPIO.LOW)

            #skenuj prazce dokud nenajdes zapnuty
            for p in range(len(PINS_IN)):
                if GPIO.input(PINS_IN[p]) == 0:
                    prazec = p
                    break

            if prazec == -1:
                #nic neni macknuto, zacneme od znova
                zmacknuto = -1
                continue

            read_lasers()

            #vypni vsechny struny
            GPIO.output(PINS_OUT[0], GPIO.HIGH)
            GPIO.output(PINS_OUT[1], GPIO.HIGH)
            GPIO.output(PINS_OUT[2], GPIO.HIGH)
            GPIO.output(PINS_OUT[3], GPIO.HIGH)

            #zjisti na ktere strune je prazec zmacknut
            for s in range(4):
                GPIO.output(PINS_OUT[s], GPIO.LOW)
                r = GPIO.input(PINS_IN[prazec])
                GPIO.output(PINS_OUT[s], GPIO.HIGH)
                if r == 0:
                    struna = s
                    break

            if struna == -1:
                #kdyby mezitim se struna pustila, zacneme od znova
                zmacknuto = -1
                continue

            #nastav zmacknuto
            zmacknuto = struna * 12 + prazec

            #hraj
            #play_accord(zmacknuto)  #vcetne sleep(0.2), toto se zrusi az budou lasery

//--------------------------------
/**
* fret 1-12 ~ pins 22-33
* strings 1-6 ~ 36-41
* emphasis laser ~ 42
* stop tone button ~ 43
**/


boolean fret_status[12][6];
boolean laser_status[6];
boolean emphasis;
boolean stop_tone;

void setup()
{
  Serial.begin(115200);

  //zero fret_status
  for (int f = 1; f <= 12; f++) {
    for (int s = 1; s <= 6; s++) {
      fret_status[f-1][s-1] = 0;
    }
  }
  //zero laser_status
  for (int l = 1; l <= 6; l++) {
    laser_status[l-1] = 0;
  }

  //init input pins
  pinMode(pin, INPUT);           // set pin to input
  digitalWrite(pin, HIGH);       // turn on pullup resistors
}

void loop()
{
  //scan frets
  setAllStrings(true);
  for (int f = 0; f <= 11; f++) {
    if (is_fret_pressed(f)) pressed_frets.append(f));
    //scan pressed frets for all strings in sequence
  }
  setAllStrings(false);

  //for each string find out what fret is pressed from list of active frets
  for (int s = 0; s <= 5; s++) {
    setString(s, true);
    for (int i = 0; i < pressed_frets.length; i++) {
      int f = pressed_frets[i];
      if (is_fret_pressed(f) && fret_status[f][s]) send_guitar_event(FRET_CHANGE

    }
    setString(s, false);
  }
  pressed_frets.clear();


}
