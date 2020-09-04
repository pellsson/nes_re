function on_end_of_frame()
	local gdv = 0
	local page_flags = memory.readbyte(0x78)
	local solomon_seals = memory.readbyte(0x79)
	local game_flags = memory.readbyte(0x7c)
	local hard_levels = memory.readbyte(0x84)
	local beaten_levels = memory.readbyte(0x85)
	local fairy_bonus = memory.readbyte(0x86)
	local digit_10m = memory.readbyte(0x44a)
	local digit_1m = memory.readbyte(0x44b)
	local digit_100k = memory.readbyte(0x44c)
	local debug_out = {}

	-- Has Page of Space?
	if 0x40 == bit.band(page_flags, 0x40) then
		 gdv = gdv + 1
		 debug_out[#debug_out+1] = 'SP'
	end
	-- Has Page of Time?
	if 0x80 == bit.band(page_flags, 0x80) then
		gdv = gdv + 1
		debug_out[#debug_out+1] = 'TM'
	end
	gdv = gdv + 1 -- Add with carry set here in the NES code. Just add one.
	gdv = gdv + fairy_bonus
	debug_out[#debug_out+1] = 'Fb:' .. tostring(fairy_bonus)
	-- Has collected Princess?
	if 0x04 == bit.band(game_flags, 0x04) then
		gdv = gdv + 1
		debug_out[#debug_out+1] = 'PR'
	end
	gdv = gdv * 2
	gdv = gdv + beaten_levels
	gdv = gdv + solomon_seals
	gdv = gdv * 2
	gdv = gdv + hard_levels
	gdv = math.floor(gdv / 8)

	debug_out[#debug_out+1] = 'L:' .. tostring(beaten_levels) .. ',' .. tostring(hard_levels)
	debug_out[#debug_out+1] = 'S:' .. tostring(solomon_seals)

	local from_score
	-- If 10 million digit or 1 million digit is non-zero, award 5 pts
	if 0 ~= digit_10m or 0 ~= digit_1m then
		from_score = 5
	else
		-- ...otherwise award 5pts for each 100k (up to a max of 5 pts)
		from_score = math.min(5, digit_100k)
	end
	gdv = gdv + from_score
	debug_out[#debug_out+1] = 'X:' .. tostring(from_score)
	-- Did collect Solomons Key?
	if 0x08 == bit.band(game_flags, 0x08) then
		gdv = gdv + 1
		debug_out[#debug_out+1] = 'KEY'
	end
	-- Base gdv is 47
	gdv = gdv + 47
	gui.drawtext(8, 8, 'GDV: '.. tostring(gdv)  .. '  ' .. table.concat(debug_out, ' '))
end

emu.registerbefore(on_end_of_frame)