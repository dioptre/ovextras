classes = nil

current_target = nil
start_time = 0;
stop_time = -1;

do_debug = false;

score = {}

function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")
	classes = box:get_setting(2)

	for j = 1, classes do
		score[j] = {}
		for i = 1, classes do
			score[j][i] = 0
		end
	end
end

function uninitialize(box)

	correct = 0
	incorrect = 0

	for j = 1, classes do
		
		output = string.format("Target %d : ", j - 1)
		inClass = 0
		
		for i = 1, classes do
			inClass = inClass + score[j][i]
		end

		for i = 1, classes do
			output = output .. string.format("%d : %5.1f%%, ", i - 1, 100*score[j][i]/inClass)

			if i == j then
				correct = correct + score[j][i]	
			else
				incorrect = incorrect + score[j][i]
			end
		end

		box:log("Info", string.format("%s total %d", output, inClass))
	end

	box:log("Info", string.format("Correct   %4d -> %5.1f%%", correct, 100*correct/(correct+incorrect)))
	box:log("Info", string.format("Incorrect %4d -> %5.1f%%", incorrect, 100*incorrect/(correct+incorrect)))

end

function process(box)

	finished = false

	while not finished do

		-- time = box:get_current_time()

		while box:get_stimulation_count(1) > 0 do

			s_code, s_date, s_duration = box:get_stimulation(1, 1)
			box:remove_stimulation(1, 1)
			
			if s_code >= OVTK_StimulationId_Label_00 and s_code <= OVTK_StimulationId_Label_1F then
				if do_debug then box:log("Info", string.format("Received target %d at ", s_code) .. s_date) end
				current_target = s_code - OVTK_StimulationId_Label_00

			elseif s_code == OVTK_StimulationId_VisualStimulationStart then
				if do_debug then box:log("Info", "Trial started at " .. s_date) end
				start_time = s_date
				-- We don't know the stop time yet, so for now accept anything in range [start_time, infty]
				stop_time = -1 
				
			elseif s_code == OVTK_StimulationId_VisualStimulationStop then
				if do_debug then box:log("Info", "Trial ended at " .. s_date) end
				stop_time = s_date

			elseif s_code == OVTK_StimulationId_ExperimentStop then
				finished = true
			end
		end

		while box:get_stimulation_count(2) > 0 do

			s_code, s_date, s_duration = box:get_stimulation(2, 1)
			box:remove_stimulation(2, 1)

			if do_debug then box:log("Info", string.format("Received prediction %d at ", s_code) .. s_date) end
			
			if s_date >= start_time and (stop_time < 0 or s_date < stop_time) and (s_code >= OVTK_StimulationId_Label_00 and s_code <= OVTK_StimulationId_Label_1F) then

				if do_debug then box:log("Info", string.format("Accepted prediction %d at ", s_code) .. s_date) end

				real_target = current_target + 1
				prediction = s_code - OVTK_StimulationId_Label_00 + 1
				score[real_target][prediction] = score[real_target][prediction] + 1

			end
		end

		box:sleep()

	end

end
