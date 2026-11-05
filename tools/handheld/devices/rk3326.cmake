# Rockchip RK3326: 4x Cortex-A35, Mali-G31 (MagicX XU10, R36S and other RK3326 handhelds).
# Cortex-A35 has no LSE atomics, so the outline-atomics helpers are not needed.
# Tested: XU10 with ROCKNIX. The R36S uses the same chip and binary but has not run it.
set(HANDHELD_CPU_FLAGS "-mcpu=cortex-a35 -mno-outline-atomics")
