#!/bin/sh
# Fast memory watchdog: records the timeline, and below the limit records who is working, then closes the game.
#   mem-probe.sh <name> <limit MB> ; stop with kill
N=$1; LIMIT=$2
OUT=/tmp/probe-$N.txt
: > "$OUT"
LAST=0
snap() {
    P=$(pidof TooieRecompiled)
    [ -n "$P" ] || return
    echo "--- $1 up=$(cut -d" " -f1 /proc/uptime) avail=$A" >> "$OUT"
    grep -E "VmRSS|VmHWM|RssAnon|VmData|Threads" /proc/$P/status | tr -s ' \t' ' ' | tr '\n' ';' >> "$OUT"; echo >> "$OUT"
    for t in /proc/$P/task/*; do
        sed -E 's/^[0-9]+ \((.*)\) ([A-Za-z]) /\1|\2|/' "$t/stat" 2>/dev/null | awk -F'|' '{n=split($3,a," "); printf "%6d %s %s\n", a[11]+a[12], $2, $1}'
    done | sort -rn | head -6 >> "$OUT"
    tail -3 /storage/roms/ports/tooie/log.txt | cut -c1-140 >> "$OUT"
}
while :; do
    A=$(awk '/MemAvailable/ {print int($2/1024)}' /proc/meminfo)
    NOW=$(cut -d. -f1 /proc/uptime)
    if [ "$NOW" != "$LAST" ]; then
        LAST=$NOW
        P=$(pidof TooieRecompiled)
        R=0; [ -n "$P" ] && R=$(awk '/VmRSS/ {print int($2/1024)}' /proc/$P/status 2>/dev/null)
        echo "$NOW avail=$A rss=$R" >> "$OUT"
    fi
    if [ "$A" -lt 320 ] && [ -z "$S320" ]; then S320=1; snap below320; fi
    if [ "$A" -lt 270 ] && [ -z "$S270" ]; then S270=1; snap below270; fi
    if [ "$A" -lt "$LIMIT" ]; then
        snap limit
        echo "WATCHDOG avail=$A" >> "$OUT"
        killall -9 TooieRecompiled
        sleep 3
    fi
    usleep 200000 2>/dev/null || sleep 1
done
