# guitar_arduino
laser guitar part of guitar controller arduino sketch

# MIDI installaton
```
sudo apt-get install fluidsynth
sudo apt-get install fluid-soundfont-gm
```
> See: http://tedfelix.com/linux/linux-midi.html

# Enable MIDI for arduino
Run fluidsynth server in one terminal
```
sudo fluidsynth --server --no-shell --audio-driver=alsa -o audio.alsa.device=hw:0 /home/honza/Documents/guitar/sf2/GS/gs.sf2
```

List midi devices to check if guitar is connected
```
aseqdump -l
```

Connect guitar controller with midi out sink
```
aconnect 20 128
```

Inspect midi commands
```
aseqdump -p 20
```
