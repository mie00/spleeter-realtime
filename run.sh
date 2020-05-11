#!/bin/bash

set -emx

[ -f /usr/lib/ladspa/spleeter_ladspa.so ] || (echo "plugin not found please install the plugin in pulseaudio directory first" && exit 1)
[ -e .venv/ ] || (echo "virtual env not found, are you sure you installed it?" && exit 1)

trap "" SIGINT

source .venv/bin/activate

# local env
[ -f .env ] && source .env

.venv/bin/python spleeter_realtime.py &
PID=$!

for i in {1..40}; do
	echo "trying to connect to server"
	nc -vz -w 1 127.0.0.1 18083 && echo "connected to server" && break
	sleep 1
done

# cleanup
pactl list modules short | grep ladspa | awk '{print $1}' | xargs -I@ pactl unload-module @
MASTER=`pactl info | grep 'Default Sink' | sed 's/Default Sink: //g'`
pactl load-module module-ladspa-sink plugin=spleeter_ladspa label=vocals sink_master="$MASTER"

NEW_SINK=`pactl list sinks short | grep ladspa | awk '{print $2}'`

pactl set-default-sink $NEW_SINK
pactl list sink-inputs short | grep -v ladspa | awk '{print $1}' | xargs -I@ pactl move-sink-input @ $NEW_SINK

echo '================================================================================='
echo 'spleeter-realtime started, press q or ctrl+c to quit, any key to toggle it on/off'
echo '================================================================================='

function cleanup {
	set +e
	echo "closing"
	pactl list sink-inputs short | grep -v ladspa | awk '{print $1}' | xargs -I@ pactl move-sink-input @ $MASTER
	pactl set-default-sink "$MASTER"
	pactl list modules short | grep ladspa | awk '{print $1}' | xargs -I@ pactl unload-module @
	kill $PID
	sleep 1
	ps -p $PID || (sleep 5 && kill -9 $PID)
	exit 0
}

trap cleanup SIGINT
trap cleanup EXIT

jobs
while IFS= read -rn1 a; do [ "$a" == "q" ] && exit 0; kill -SIGUSR1 $PID; done
