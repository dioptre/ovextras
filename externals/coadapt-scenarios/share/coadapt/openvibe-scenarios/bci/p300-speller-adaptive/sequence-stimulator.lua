#!/bin/lua

-- This Lua script generates sequence stimulations for the P300 visualisation
--
-- Author : Loïc MAHE, Ecm
-- Date   : 2012-06-11

dofile("../share/openvibe-plugins/stimulation/lua-stimulator-stim-codes.lua")

index = 0


-- time management functions ------------------------------------------------------------------------------------

function wait_until(box, time)
  while box:get_current_time() < time do
    box:sleep()
  end
end

function wait_for(box, duration)
  wait_until(box, box:get_current_time() + duration)
end

-- sequence generation functions ------------------------------------------------------------------------------------
function permutation(tab)
	n = table.getn(tab)
	permuted_tab = {}
	for i=1,n do
		j = math.random(1,table.getn(tab))
		permuted_tab[i] = tab[j]
		table.remove(tab,j)
	end
	return permuted_tab
end


function generate_sequence()
	row_list = {1,2,3,4,5,6}
	column_list = {7,8,9,10,11,12}
	list = {1,2,3,4,5,6,7,8,9,10,11,12}
	row_sequence = permutation(row_list)
	column_sequence = permutation(column_list)
	-- for i=1,6 do
	--	print(row_sequence[i])
	-- end
	sequence = permutation(list)
	for i=1,6 do
		sequence[(i-1)*2+1] = row_sequence[i]
		sequence[(i-1)*2+2] = column_sequence[i]
	end
	-- return sequence
	return sequence
	
end


-- this function is called when the box is initialized --------------------------------------------------------------------
function initialize(box)
	--box:log("Fatal","init")
	state = "TrialRest"
	last_state = "Init"
	
	rows = box:get_setting(5)+0
	columns = box:get_setting(6)+0

	Rmin = box:get_setting(7)+0
	Rmax = box:get_setting(8)+0

	ltarget = box:get_setting(9)+0


	flash_duration = box:get_setting(10)+0
	no_flash_duration = box:get_setting(11)+0
	inter_repetition_delay = box:get_setting(12)+0
	inter_trial_delay = box:get_setting(13)+0

	EarlyStopping = 0
	F = 12
end

-- this function is called when the box is uninitialized
function uninitialize(box)
	box:log("Fatal","uninit")
end

-- this function is called once by the box
function process(box)
	--box:log("Fatal","process")
	t = box:get_current_time()
	active = true
	-- enters infinite loop
	-- cpu will be released with a call to sleep
	-- at the end of the loop
	while active do

		-- gets current simulated time
		t = box:get_current_time()

		-- loops on all inputs of the box
		for input = 1, box:get_input_count() do

		    -- loops on every received stimulation for a given input
		    	for stimulation = 1, box:get_stimulation_count(input) do

				-- gets the received stimulation
				identifier, date, duration = box:get_stimulation(input, 1)

				-- logs the received stimulation
				--box:log("Error", string.format("At time %f on input %i got stimulation id:%s date:%s duration:%s", t, input, identifier, date, duration))

				if (identifier==OVTK_StimulationId_Target) then 
					EarlyStopping = 1
				end

				-- discards it
				box:remove_stimulation(input, 1)
			end
		end


		if (state == "TrialRest") then
			if (last_state == "Init") then
				box:send_stimulation(1,OVTK_StimulationId_ExperimentStart, t+1, 0)
				i = 0
			end
			box:send_stimulation(1,OVTK_StimulationId_RestStart, t+1, 0)
			i = i+1
			last_state = state
			state = "Flash"
			wait_for(box,inter_trial_delay)

		elseif (state == "Flash") then 
			if (last_state == "TrialRest" or last_state == "EndOfSegment") then
				if (last_state == "TrialRest") then
					box:send_stimulation(1,OVTK_StimulationId_RestStop, t+1, 0)
					box:send_stimulation(1,OVTK_StimulationId_TrialStart, t+1, 0)
					r = 0			
				end
				r = r+1
				if (r<=Rmax) then
					box:send_stimulation(1,OVTK_StimulationId_SegmentStart, t+1, 0)
				end
				f = 0	
				-- current_sequence = generate_sequence()
				current_sequence = generate_sequence()	
			end
			
			if r<=Rmax then
				x = current_sequence[f+1]
				box:send_stimulation(1,OVTK_StimulationId_Label_00+x, t+1, 0)			
				box:send_stimulation(1,OVTK_StimulationId_VisualStimulationStart, t+1, 0)
				-- box:log("Fatal",string.format("repetition %i		flash %i",r,f))
			end
			f = f+1
			last_state = state
			state = "NoFlash"
			wait_for(box,flash_duration)

		elseif (state == "NoFlash") then 
			box:send_stimulation(1,OVTK_StimulationId_VisualStimulationStop, t+1, 0)
			last_state = state
			if (f<F) then
				state = "Flash"
			else
				state = "EndOfSegment"
			end
			wait_for(box,no_flash_duration)
		elseif (state == "EndOfSegment") then 
			box:send_stimulation(1,OVTK_StimulationId_SegmentStop, t+1, 0)
			last_state = state
			--if (r<Rmin) then
			--	state = "Flash"
			if (EarlyStopping==1) then
				state = "EndOfTrial" 
				--box:log("Error",string.format("End of trial %i %i %i",Rmin,r,Rmax))
			else
				state = "Flash"
			end
			wait_for(box,inter_repetition_delay)
		elseif (state == "EndOfTrial") then 
			EarlyStopping = 0
			box:send_stimulation(1,OVTK_StimulationId_TrialStop, t+1, 0)
			last_state = state
			if (i<ltarget) then
				state = "TrialRest"
			else
				state = "End"
			end
		elseif (state == "End") then 
			box:send_stimulation(1,OVTK_StimulationId_ExperimentStop, t+1, 0)
			active = false

		end

		box:sleep()
	end

	box:sleep()
end
