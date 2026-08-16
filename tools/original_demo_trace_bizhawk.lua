-- tools/original_demo_trace_bizhawk.lua
-- Robust, self-validating BizHawk / EmuHawk Lua script for capturing original Banjo-Tooie (US) attract demo ground truth.
-- Verified ROM SHA-1: af1a89e12b638b8d82cc4c085c8e01d4cba03fb3

local log_file = io.open("original_wooded_hollow_demo_sync.csv", "w")
log_file:write("consumed_glidx,post_glidx,vi_count,mode,map,pad_x,pad_y,pad_btn,px,py,pz,vx,vy,vz,velxz,tspd,yaw,ideal_yaw,phys_yaw,mult,accel,cam_x,cam_y,cam_z,cam_yaw,digger_x,digger_y,digger_z\n")

local prev_post_glidx = -1
local captured_samples = 0
local landmark_checks = {
    [70] = {x = -2, y = 80, btn = 0x0000},
    [74] = {x = -18, y = 82, btn = 0x0000},
    [75] = {x = -47, y = 78, btn = 0x0000},
    [76] = {x = -63, y = 74, btn = 0x0000},
    [322] = {x = -9, y = 84, btn = 0x2008}
}

local function is_valid_kseg0(addr)
    return addr >= 0x80000000 and addr < 0x80800000 and (addr % 4 == 0)
end

local function kseg0_to_phys(addr)
    return addr - 0x80000000
end

event.onframeend(function()
    -- Read mode (D_80127630 + 0x2) and map (D_80127630 + 0x10)
    local mode = mainmemory.read_s16_be(0x127632)
    local map = mainmemory.read_s16_be(0x127640)
    local vi_count = emu.framecount()

    if mode == 5 then -- Gameplay attract demo
        -- D_8012D5B0: GlRecord control structure
        local gl_base = mainmemory.read_u32_be(0x12D5B0)
        local post_glidx = mainmemory.read_s32_be(0x12D5B8)
        local gl_cnt = mainmemory.read_s32_be(0x12D5BC)

        if post_glidx ~= prev_post_glidx and is_valid_kseg0(gl_base) and post_glidx > 0 then
            prev_post_glidx = post_glidx
            captured_samples = captured_samples + 1

            -- Crucial semantic fix: glrecord_entrypoint_0 increments index on consumption.
            -- Therefore post_glidx is the NEXT index; consumed_glidx is post_glidx - 1.
            local consumed_glidx = post_glidx - 1

            -- Read recorded pad sample (6 bytes per sample: stride = 6)
            local sample_addr = kseg0_to_phys(gl_base) + consumed_glidx * 6
            local pad_x = mainmemory.read_s8(sample_addr)
            local pad_y = mainmemory.read_s8(sample_addr + 1)
            local pad_btn = mainmemory.read_u16_be(sample_addr + 2)

            -- Landmark self-test
            local expected = landmark_checks[consumed_glidx]
            if expected then
                local pass_x = (pad_x == expected.x)
                local pass_y = (pad_y == expected.y)
                local pass_btn = (pad_btn == expected.btn)
                if pass_x and pass_y and pass_btn then
                    print(string.format("[SELF-TEST PASS] consumed_glidx=%d post=%d pad=(%d,%d,0x%04x)",
                        consumed_glidx, post_glidx, pad_x, pad_y, pad_btn))
                else
                    print(string.format("[SELF-TEST FAIL] consumed_glidx=%d post=%d pad=(%d,%d,0x%04x) expected=(%d,%d,0x%04x)",
                        consumed_glidx, post_glidx, pad_x, pad_y, pad_btn, expected.x, expected.y, expected.btn))
                end
            end

            -- D_80135490: Player struct pointer table
            local ps_addr = mainmemory.read_u32_be(0x135490)
            local px, py, pz = 0, 0, 0
            local vx, vy, vz, tspd = 0, 0, 0, 0
            local yaw, ideal_yaw, phys_yaw = 0, 0, 0
            local mult, accel = 0, 0

            if is_valid_kseg0(ps_addr) then
                local ps = kseg0_to_phys(ps_addr)
                local pos_ptr = mainmemory.read_u32_be(ps + 0xE4)
                if is_valid_kseg0(pos_ptr) then
                    local ppos = kseg0_to_phys(pos_ptr)
                    px = mainmemory.readfloat(ppos, true)
                    py = mainmemory.readfloat(ppos + 4, true)
                    pz = mainmemory.readfloat(ppos + 8, true)
                end

                local yaw_ptr = mainmemory.read_u32_be(ps + 0xF8)
                if is_valid_kseg0(yaw_ptr) then
                    local pyaw = kseg0_to_phys(yaw_ptr)
                    yaw = mainmemory.readfloat(pyaw, true)
                    ideal_yaw = mainmemory.readfloat(pyaw + 4, true)
                end

                local phys_ptr = mainmemory.read_u32_be(ps + 0xC8)
                if is_valid_kseg0(phys_ptr) then
                    local pphys = kseg0_to_phys(phys_ptr)
                    vx = mainmemory.readfloat(pphys + 0x10, true)
                    vy = mainmemory.readfloat(pphys + 0x14, true)
                    vz = mainmemory.readfloat(pphys + 0x18, true)
                    tspd = mainmemory.readfloat(pphys + 0x3C, true)
                    phys_yaw = mainmemory.readfloat(pphys + 0x40, true)
                    accel = mainmemory.readfloat(pphys + 0x44, true)
                    mult = mainmemory.readfloat(pphys + 0x50, true)
                end
            end

            -- D_8012D500: Active Viewport/Camera pointer
            local cam_addr = mainmemory.read_u32_be(0x12D500)
            local cx, cy, cz, cam_yaw = 0, 0, 0, 0
            if is_valid_kseg0(cam_addr) then
                local cam = kseg0_to_phys(cam_addr)
                cx = mainmemory.readfloat(cam, true)
                cy = mainmemory.readfloat(cam + 4, true)
                cz = mainmemory.readfloat(cam + 8, true)
                local m00 = mainmemory.readfloat(cam + 0xA4, true)
                local m02 = mainmemory.readfloat(cam + 0xA4 + 8, true)
                cam_yaw = math.atan2(-m02, m00) * (180.0 / math.pi)
                if cam_yaw < 0 then cam_yaw = cam_yaw + 360 end
            end

            -- D_80136EE0: World actors vector pointer
            local vec_addr = mainmemory.read_u32_be(0x136EE0)
            local dgx, dgy, dgz = 0, 0, 0
            if is_valid_kseg0(vec_addr) then
                local vec = kseg0_to_phys(vec_addr)
                local elem_size = mainmemory.read_u32_be(vec + 0x00)
                local begin_addr = mainmemory.read_u32_be(vec + 0x04)
                local end_addr = mainmemory.read_u32_be(vec + 0x08)
                if elem_size == 0x9C and is_valid_kseg0(begin_addr) and is_valid_kseg0(end_addr) and end_addr >= begin_addr then
                    local count = math.floor((end_addr - begin_addr) / elem_size)
                    for i = 0, math.min(count - 1, 63) do
                        local act = kseg0_to_phys(begin_addr) + i * 0x9C
                        local idw = mainmemory.read_u32_be(act + 0x6C)
                        local mid = bit.band(idw, 0x1FF)
                        if mid == 337 then -- chdiggerground
                            dgx = mainmemory.readfloat(act + 0x04, true)
                            dgy = mainmemory.readfloat(act + 0x08, true)
                            dgz = mainmemory.readfloat(act + 0x0C, true)
                            break
                        end
                    end
                end
            end

            local velxz = math.sqrt(vx * vx + vz * vz)
            log_file:write(string.format("%d,%d,%d,%d,%d,%d,%d,%04x,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f,%.1f,%.2f,%.2f,%.2f\n",
                consumed_glidx, post_glidx, vi_count, mode, map, pad_x, pad_y, pad_btn,
                px, py, pz, vx, vy, vz, velxz, tspd, yaw, ideal_yaw, phys_yaw, mult, accel,
                cx, cy, cz, cam_yaw, dgx, dgy, dgz))
            log_file:flush()

            if consumed_glidx % 50 == 0 or landmark_checks[consumed_glidx] then
                print(string.format("[DEMO-TRACE] consumed=%d post=%d vi=%d pos=(%.1f,%.1f,%.1f) velxz=%.1f yaw=%.1f cam_yaw=%.1f",
                    consumed_glidx, post_glidx, vi_count, px, py, pz, velxz, yaw, cam_yaw))
            end
        end
    end
end)
