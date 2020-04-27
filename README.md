# Spleeter realtime

This project runs spleeter as a pulseaudio module to filter all the sound going out of the computer.

## Installation

```
git clone https://github.com/mie00/spleeter-realtime
cd spleeter-realtime

# install pulseaudio plugin
cd pulseaudio
make
sudo make install
cd ..

# install python dependencies
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt

# test if tensorflow works on gpu
python test_gpu.py
```

## Usage

1. Run the server
```
cd spleeter-realtime
source .venv/bin/activate
python spleeter_realtime.py
```

2. Get the master sink
```
~ pactl list sinks short
0       alsa_output.pci-0000_00_1f.3.analog-stereo      module-alsa-card.c      s16le 2ch 44100Hz       RUNNING
~ export MASTER=alsa_output.pci-0000_00_1f.3.analog-stereo
```
In this case `alsa_output.pci-0000_00_1f.3.analog-stereo` is the sink name that is going to be used.

3. Load the plugin
```
~ pactl load-module module-ladspa-sink plugin=spleeter_ladspa label=vocals sink_master=$MASTER
26
```
26 is the module id used to unload the plugin run `pactl unload-module 26` to unload it.

4. Set default sink to the `LADSPA Plugin Vocals on ...` sink, using either `pavucontrol`, `pactl set-default-sink` or any other sound control software.
