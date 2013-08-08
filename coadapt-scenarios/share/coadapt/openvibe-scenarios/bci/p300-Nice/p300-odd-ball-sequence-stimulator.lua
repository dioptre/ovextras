#!/bin/lua

-- This Lua script generates sequence stimulations for the serial P300 visualisation
--
-- Author : Dieter Devlaminck, INRIA
-- Date   : 2012-10-22

lxp = require "lxp"

dofile("../share/openvibe-plugins/stimulation/lua-stimulator-stim-codes.lua")

--repCounter = 0
labelElementOpen = false
symbolProbabilities = {}
symbolList = {}
symbolCounter = 0
uniformDistribution = true

callbacks = {
	StartElement = function (parser, name, attributes)
		if attributes[1] == "probability" then
			symbolCounter = symbolCounter+1
			if symbolCounter == 1 then
				symbolProbabilities[symbolCounter] = tonumber(attributes["probability"])
			else
				symbolProbabilities[symbolCounter] = symbolProbabilities[symbolCounter-1] + tonumber(attributes["probability"])	
				if symbolProbabilities[symbolCounter]~=symbolProbabilities[symbolCounter-1] then
					uniformDistribution = false				
				end		
			end
		end

		if name=="Label" then
			labelElementOpen = true
		end
	end,
	EndElement = function (parser, name)
		if name=="Label" then
			labelElementOpen = false
		end
	end,
	CharacterData = function(parser, string)
		if labelElementOpen then
			symbolList[symbolCounter] = string
		end
	end
}

-- time management functions ------------------------------------------------------------------------------------

function wait_until(box, time)
  while box:get_current_time() < time do
    box:sleep()
  end
end

function wait_for(box, duration)
  wait_until(box, box:get_current_time() + duration)
end

function find_symbol_index(prob)
	for i=1, #symbolProbabilities do
		--io.write(string.format("symbol prob %f, prob %f\n", symbolProbabilities[i],prob))
		if prob<symbolProbabilities[i] then
			return i		
		end
	end
end

-- sequence generation functions ------------------------------------------------------------------------------------
function permutation(tab)
	n = table.getn(tab)
	permuted_tab = {}
	if uniformDistribution then
		for i=1,n do
			j = math.random(1,table.getn(tab))
			permuted_tab[i] = tab[j]
			table.remove(tab,j)
		end
	else
		for i=1,n do
			j = math.random()
			permuted_tab[i] = find_symbol_index(j)		
		end	
	end
	return permuted_tab
end


function generate_sequence()
	list = {}    -- new array
	for i=1, symbolCounter do
		list[i] = i
	end

	list = permutation(list)

	-- return sequence
	return list	
end


-- this function is called when the box is initialized --------------------------------------------------------------------
function initialize(box)
	--box:log("Fatal","init")
	state = "Init"
	last_state = "Init"

	stimBase = _G[box:get_setting(3)]
	Rmin = box:get_setting(4)+0
	Rmax = box:get_setting(5)+0
	ltarget = box:get_setting(6)+0
	flash_duration = box:get_setting(7)+0
	no_flash_duration = box:get_setting(8)+0
	inter_repetition_delay = box:get_setting(9)+0
	inter_trial_delay = box:get_setting(10)+0
	
	--parsing symbol list
	fh = io.open(box:get_setting(11),"r")
	p = lxp.new(callbacks)
	while true do
	    line = fh.read(fh)
	    if not line then break end  -- iterate lines
	    p:parse(line)          -- parses the line
	    p:parse("\n")       -- parses the end of line
	end
	p:parse()               -- finishes the document
	p:close() 

	EarlyStopping = 0
end

-- this function is called when the box is uninitialized
function uninitialize(box)
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

				if (identifier == OVTK_StimulationId_ExperimentStart) then
					state = "TrialRest"
					last_state = "Init"
					box:send_stimulation(1,OVTK_StimulationId_ExperimentStart, t+1, 0)
					repCounter = 0
				end

				-- discards it
				box:remove_stimulation(input, 1)
			end
		end


		if (state == "TrialRest") then
			--if (last_state == "Init") then
			--	box:send_stimulation(1,OVTK_StimulationId_ExperimentStart, t+1, 0)
			--	i = 0
			--end
			repCounter = repCounter + 1
			box:send_stimulation(1,OVTK_StimulationId_RestStart, t+1, 0)
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
				if (r==Rmax) then
					EarlyStopping = 1				
				end
				f = 0	
				-- current_sequence = generate_sequence()
				current_sequence = generate_sequence()	
			end
			
			if r<=Rmax then
				x = current_sequence[f+1]
				box:send_stimulation(1,stimBase+x-1, t+1, 0)			
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
			if (f<symbolCounter) then
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
			if (repCounter<ltarget) then
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
