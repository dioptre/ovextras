#!/bin/lua


-- this function is called when the box is initialized
function initialize(box)
        box:log("Trace", "initialize has been called")
        dofile(box:get_config("${Path_Data}") .. "/openvibe-plugins/stimulation/lua-stimulator-stim-codes.lua")
        offset = box:get_setting(2)
end

-- this function is called when the box is uninitialized
function uninitialize(box)
        box:log("Trace", "uninitialize has been called")
end

-- this function is called once by the box
function process(box)

	-- enters infinite loop
	-- cpu will be released with a call to sleep
	-- at the end of the loop
	while true do

		time = box:get_current_time()
    
		-- loops on every received stimulation for a given input
		for stimulation = 1, box:get_stimulation_count(1) do

			stimulation_id, stimulation_time, stimulation_duration = box:get_stimulation(1, 1)
			-- discards it
			box:remove_stimulation(1, 1)
      
			-- change stimulation code and output stimulation
			box:send_stimulation(1, stimulation_id, time+offset)

		end

		-- releases cpu
		box:sleep()
	end
end
