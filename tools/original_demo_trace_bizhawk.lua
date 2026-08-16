-- tools/original_demo_trace_bizhawk.lua
-- Ready-to-run BizHawk / EmuHawk Lua script for capturing original Banjo-Tooie (US) attract demo ground truth.
-- Place in BizHawk Lua console while running Banjo-Tooie (US) (ROM SHA-1: af1a89e12b638b8d82cc4c085c8e01d4cba03fb3).

local log_file = io.open("original_wooded_hollow_demo_sync.csv", "w")
log_file:write("glidx,tick,mode,map,pad_x,pad_y,pad_btn,px,py,pz,vx,vy,vz,velxz,tspd,yaw,ideal_yaw,phys_yaw,mult,accel,cam_x,cam_y,cam_z,cam_yaw,digger_x,digger_y,digger_z\n")

local prev_glidx = -1
local logic_tick = 0

event.onframeend(function()
    -- Read mode (D_80127630 + 0x2) and map (D_80127630 + 0x10)
    local mode = mainmemory.read_s16_be(0x127632)
    local map = mainmemory.read_s16_be(0x127640)

    if mode == 5 then -- Gameplay attract demo
        -- Read glrecord base and current index (D_8012D5B0)
        local gl_base = mainmemory.read_u32_be(0x12D5B0)
        local gl_idx = mainmemory.read_s32_be(0x12D5B8)
        local gl_cnt = mainmemory.read_s32_be(0x12D5BC)

        if gl_idx ~= prev_glidx and gl_base >= 0x80000000 then
            prev_glidx = gl_idx
            logic_tick = logic_tick + 1

            -- Read recorded pad sample (6 bytes)
            local sample_addr = (gl_base - 0x80000000) + gl_idx * 6
            local pad_x = mainmemory.read_s8(sample_addr)
            local pad_y = mainmemory.read_s8(sample_addr + 1)
            local pad_btn = mainmemory.read_u16_be(sample_addr + 2)

            -- Read player struct D_80135490[0]
            local ps_addr = mainmemory.read_u32_be(0x135490)
            local px, py, pz = 0, 0, 0
            local vx, vy, vz, tspd = 0, 0, 0, 0
            local yaw, ideal_yaw, phys_yaw = 0, 0, 0
            local mult, accel = 0, 0

            if ps_addr >= 0x80000000 then
                local ps = ps_addr - 0x80000000
                local pos_ptr = mainmemory.read_u32_be(ps + 0xE4)
                if pos_ptr >= 0x80000000 then
                    local ppos = pos_ptr - 0x80000000
                    px = mainmemory.readfloat(ppos, true)
                    py = mainmemory.readfloat(ppos + 4, true)
                    pz = mainmemory.readfloat(ppos + 8, true)
                end

                local yaw_ptr = mainmemory.read_u32_be(ps + 0xF8)
                if yaw_ptr >= 0x80000000 then
                    local pyaw = yaw_ptr - 0x80000000
                    yaw = mainmemory.readfloat(pyaw, true)
                    ideal_yaw = mainmemory.readfloat(pyaw + 4, true)
                end

                local phys_ptr = mainmemory.read_u32_be(ps + 0xC8)
                if phys_ptr >= 0x80000000 then
                    local pphys = phys_ptr - 0x80000000
                    vx = mainmemory.readfloat(pphys + 0x10, true)
                    vy = mainmemory.readfloat(pphys + 0x14, true)
                    vz = mainmemory.readfloat(pphys + 0x18, true)
                    tspd = mainmemory.readfloat(pphys + 0x3C, true)
                    phys_yaw = mainmemory.readfloat(pphys + 0x40, true)
                    accel = mainmemory.readfloat(pphys + 0x44, true)
                    mult = mainmemory.readfloat(pphys + 0x50, true)
                end
            end

            -- Read camera (D_8012D500)
            local cam_addr = mainmemory.read_u32_be(0x12D500)
            local cx, cy, cz, cam_yaw = 0, 0, 0, 0
            if cam_addr >= 0x80000000 then
                local cam = cam_addr - 0x80000000
                cx = mainmemory.readfloat(cam, true)
                cy = mainmemory.readfloat(cam + 4, true)
                cz = mainmemory.readfloat(cam + 8, true)
                local m00 = mainmemory.readfloat(cam + 0xA4, true)
                local m02 = mainmemory.readfloat(cam + 0xA4 + 8, true)
                cam_yaw = math.atan2(-m02, m00) * (180.0 / math.pi)
                if cam_yaw < 0 then cam_yaw = cam_yaw + 360 end
            end

            -- Read digger (0x80136EE0 vector)
            local vec_addr = mainmemory.read_u32_be(0x136EE0)
            local dgx, dgy, dgz = 0, 0, 0
            if vec_addr >= 0x80000000 then
                local vec = vec_addr - 0x80000000
                local begin_addr = mainmemory.read_u32_be(vec + 4)
                local end_addr = mainmemory.read_u32_be(vec + 8)
                if begin_addr >= 0x80000000 and end_addr >= begin_addr then
                    local count = math.floor((end_addr - begin_addr) / 0x9C)
                    for i = 0, math.min(count - 1, 63) do
                        local act = (begin_addr - 0x80000000) + i * 0x9C
                        local mid = bit.band(mainmemory.read_u32_be(act + 0x6C), 0x1FF)
                        if mid == 337 then
                            dgx = mainmemory.readfloat(act + 4, true)
                            dgy = mainmemory.readfloat(act + 8, true)
                            dgz = mainmemory.readfloat(act + 0xC, true)
                            break
                        end
                    end
                end
            end

            local velxz = math.sqrt(vx*vx + vz*vz)
            log_file:write(string.format("%d,%d,%d,%d,%d,%d,%04x,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f,%.1f,%.2f,%.2f,%.2f\n",
                gl_idx, logic_tick, mode, map, pad_x, pad_y, pad_btn,
                px, py, pz, vx, vy, vz, velxz, tspd, yaw, ideal_yaw, phys_yaw, mult, accel,
                cx, cy, cz, cam_yaw, dgx, dgy, dgz))
            log_file:flush()
        end
    end
end)
