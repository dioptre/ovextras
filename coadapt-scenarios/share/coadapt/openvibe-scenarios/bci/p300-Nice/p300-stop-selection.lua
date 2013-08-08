#!/bin/lua

-- This Lua script generates target stimulations for the P300 visualisation
-- box based on the matrix of letters / numbers a P300 speller has
--
-- Author : Dieter Devlaminck, INRIA
-- Date   : 2011-12-13

dofile("../share/openvibe-plugins/stimulation/lua-stimulator-stim-codes.lua")

-- this function is called when the box is initialized
function initialize(box)
	delay = box:get_setting(2)
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
			box:send_stimulation(1, stimulation_id, t, 0)
			box:send_stimulation(1, OVTK_StimulationId_VisualStimulationStop, t+delay, 0)

			-- discards it
			box:remove_stimulation(1, 1)

		end

		-- releases cpu
		box:sleep()
	end
end
