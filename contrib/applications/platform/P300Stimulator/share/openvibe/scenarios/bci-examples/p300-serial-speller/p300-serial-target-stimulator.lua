#!/bin/lua

-- This Lua script generates target stimulations for the P300 visualisation
-- box based on the matrix of letters / numbers a P300 speller has
--
-- Author : Dieter Devlaminck, INRIA
-- Date   : 2012-10-23

require "lxp"

dofile("../share/openvibe-plugins/stimulation/lua-stimulator-stim-codes.lua")

index = 0
labelElementOpen = false
symbolList = {}
symbolCounter = 0

callbacks = {
	StartElement = function (parser, name, attributes)
		if name=="Label" then
			symbolCounter = symbolCounter+1
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
			symbolList[string] = symbolCounter-1
			--print(string," ", symbolList[string])
		end
	end
}

-- this function is called when the box is initialized
function initialize(box)
	math.randomseed(os.time())
	target = box:get_setting(2)
	stimBase = _G[box:get_setting(3)]
	delay = box:get_setting(4)
	if target == "" then
		--TODO
	end

	--parsing symbol list
	fh = io.open(box:get_setting(5),"r")
	p = lxp.new(callbacks)
	while true do
	    line = fh.read(fh)
	    if not line then break end  -- iterate lines
	    p:parse(line)          -- parses the line
	    p:parse("\n")       -- parses the end of line
	end
	p:parse()               -- finishes the document
	p:close() 

	print(symbolList["A"])
end

-- this function is called when the box is uninitialized
function uninitialize(box)
end

-- this function is called once by the box
function process(box)

	-- enters infinite loop
	-- cpu will be released with a call to sleep
	-- at the end of the loop
	while true do

		-- gets current simulated time
		t = box:get_current_time()

		-- loops on every received stimulation for a given input
		for stimulation = 1, box:get_stimulation_count(1) do

			-- gets stimulation
			stimulation_id, stimulation_time, stimulation_duration = box:get_stimulation(1, 1)

			if stimulation_id == OVTK_StimulationId_RestStart then

				-- finds a new target
				index = index + 1
				symbolIndex = symbolList[string.sub(target, index, index)]

				-- triggers the target
				box:send_stimulation(1, stimBase+symbolIndex, t+delay, 0)

			elseif stimulation_id == OVTK_StimulationId_ExperimentStop then

				-- triggers train stimulation
				-- box:send_stimulation(1, OVTK_StimulationId_Train, t+delay+1, 0)
				box:send_stimulation(1, OVTK_StimulationId_ExperimentStop, t+delay+1, 0)

			end

			-- discards it
			box:remove_stimulation(1, 1)

		end

		-- releases cpu
		box:sleep()
	end
end
