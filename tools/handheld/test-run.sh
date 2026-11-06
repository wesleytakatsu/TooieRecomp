#!/bin/sh
# Runs the game over SSH for N seconds under the device's desktop session and samples CPU and memory.
#   test-run.sh <seconds> [VAR=value ...]
SECS="$1"; shift
GAMEDIR=/storage/roms/ports/tooie
cd "$GAMEDIR" || exit 1
. /etc/profile >/dev/null 2>&1
performance >/dev/null 2>&1
gpu_performance_level profile_peak >/dev/null 2>&1
export WAYLAND_DISPLAY=wayland-1 XDG_RUNTIME_DIR=/var/run/0-runtime-dir SDL_VIDEODRIVER=wayland SDL_AUDIODRIVER=pulseaudio
export TK64_GRAPHICS_API=opengles TK64_PROFILE=weak TOOIE_HANDHELD=1 TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0
export MESA_SHADER_CACHE_DIR="$GAMEDIR/shadercache" MESA_SHADER_CACHE_MAX_SIZE=64M
export SDL_GAMECONTROLLERCONFIG_FILE=/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt
for kv in "$@"; do export "$kv"; done
pkill -STOP emulationstation
./TooieRecompiled > log.txt 2>&1 &
PID=$!
: > stats.txt
i=0
while [ $i -lt "$SECS" ] && kill -0 $PID 2>/dev/null; do
    sleep 5
    i=$((i + 5))
    [ -z "$TEST_NO_SHOTS" ] && [ $((i % 10)) -eq 0 ] && grim /tmp/shot_$i.png 2>/dev/null
    rss=$(awk '/VmRSS/ {print int($2/1024)}' /proc/$PID/status 2>/dev/null)
    avail=$(awk '/MemAvailable/ {print int($2/1024)}' /proc/meminfo)
    temp=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)
    freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null)
    cpu=$(awk '{print $14 + $15}' /proc/$PID/stat 2>/dev/null)
    drm=""
    for f in /proc/$PID/fdinfo/*; do
        if grep -q "drm-driver:.*panfrost" "$f" 2>/dev/null; then
            drm=$(awk '/drm-(total|resident|purgeable|active|shared)-memory/ {sub("drm-","",$1); sub("-memory:","",$1); printf "%s=%s%s ", $1, $2, $3}' "$f")
            break
        fi
    done
    echo "t=$i rss=${rss}MB avail=${avail}MB temp=$temp freq=$freq cputicks=$cpu $drm" >> stats.txt
done
if kill -0 $PID 2>/dev/null; then
    top -H -b -n 1 -p $PID 2>/dev/null | head -24 >> stats.txt
    kill $PID; sleep 2; kill -9 $PID 2>/dev/null
    echo "ended=killed" >> stats.txt
else
    wait $PID
    echo "ended=exit:$?" >> stats.txt
fi
pkill -CONT emulationstation
ondemand >/dev/null 2>&1
