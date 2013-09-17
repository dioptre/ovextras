function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")
	delay = box:get_setting(2)
end

function uninitialize(box)
end

function process(box)

	while (box:keep_processing()) do
		while box:get_stimulation_count(1) > 0 do

			stimulation = box:get_stimulation(1, index)
			box:remove_stimulation(1, 1)
			box:send_stimulation(1, stimulation, box:get_current_time() + delay, 0)
		end

		box:sleep()
	end
end
