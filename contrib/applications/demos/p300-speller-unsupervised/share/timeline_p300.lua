
function initialize(box)

	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")

	number_of_trials = box:get_setting(2)
	flashes_per_trial = box:get_setting(3)
	flash_duration = box:get_setting(4)
	text_to_spell = box:get_setting(5)
	
	baseline_duration = 1
	post_flash_duration = 0.1;
	
	keyboard_config = 'ab#c#de#fghij#klm#nopq#rstu#vwxy#z_#.#,!?<'
	keyboard_cols = 7
	
	-- strlen()
	-- strlib_open()
	
end

function process(box)

	local t=0

	-- manages baseline

	box:send_stimulation(1, OVTK_StimulationId_ExperimentStart, t, 0)
	t = t + 1

	-- send out the text to spell in the beginning
	box:send_stimulation(1, OVTK_StimulationId_Target, t, 0)
	t = t + 0.1;
	
	-- io.write(string.format("str is %s kb is %s\n", text_to_spell, keyboard_config))
	
	text_to_spell = text_to_spell:lower()
	text_to_spell = text_to_spell:gsub(' ','_')

	for i = 1, #text_to_spell do
	    idx = -1
		
		for j = 1, #keyboard_config do
			if keyboard_config:sub(j,j) == text_to_spell:sub(i,i) then
				-- io.write(string.format("match %s %s\n", text_to_spell:sub(i,i), keyboard_config:sub(j,j)))
				idx = j-1
				break
			end
		end
		if idx > -1 then
			-- adhoc rounding without math lib
			rowidx = tonumber(string.format("%d", idx / (keyboard_cols)))
			-- colidx = idx - rowidx * keyboard_cols
			colidx = idx % keyboard_cols
			io.write(string.format("for %d %s idx is %d -> %d,%d\n", i, text_to_spell:sub(i,i), idx, rowidx, colidx))
			box:send_stimulation(1, OVTK_StimulationId_Label_01+rowidx, t, 0);
			box:send_stimulation(1, OVTK_StimulationId_Label_08+colidx, t, 0);	
		end
	end
	t = t + 0.1;
	box:send_stimulation(1, OVTK_StimulationId_NonTarget, t, 0)
	t = t + 0.1;
	
	box:send_stimulation(1, OVTK_StimulationId_BaselineStart, t, 0)
	t = t + baseline_duration

	box:send_stimulation(1, OVTK_StimulationId_BaselineStop, t, 0)
	t = t + 1

	if #text_to_spell > 0 then
		-- if text is specified, overrides
		number_of_trials = #text_to_spell
	end
	
	-- manages trials
	for i = 1, number_of_trials do

		box:send_stimulation(1, OVTK_GDF_Start_Of_Trial, t, 0)
		t = t + 1
		
		for j = 1, flashes_per_trial do
		
			-- flash
			box:send_stimulation(1, OVTK_StimulationId_VisualStimulationStart, t, 0)
			t = t + flash_duration
		
			box:send_stimulation(1, OVTK_StimulationId_VisualStimulationStop, t, 0.10)
			t = t + post_flash_duration
			
		end

		-- testing
		box:send_stimulation(1, OVTK_StimulationId_Label_01 + (i % 6) -1, t, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_08 + (i % 7) -1, t, 0)	
		t = t + 1

		box:send_stimulation(1, OVTK_GDF_End_Of_Trial, t, 0)
		t = t + 1
		
	end

	-- send end for completeness	
	box:send_stimulation(1, OVTK_GDF_End_Of_Session, t, 0)
	t = t + 5
	
	-- used to cause the acquisition scenario to stop
	box:send_stimulation(1, OVTK_StimulationId_ExperimentStop, t, 0)
	
end
