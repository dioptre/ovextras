classes = nil

current_target = nil
start_time = 0;
stop_time = 0;

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
		
		for i = 1, classes do
			output = output .. string.format("%d : %3d, ", i - 1, score[j][i])

			if j ~= 1 and i == 1 then
				incorrect = incorrect + score[j][i]
			elseif i == j then
				correct = correct + score[j][i]
			end
		end

		box:log("Info", string.format("%s", output))
	end

	box:log("Info", string.format("Correct   %4d -> %f2.1%%", correct, 100*correct/(correct+incorrect)))
	box:log("Info", string.format("Incorrect %4d -> %f2.1%%", incorrect, 100*incorrect/(correct+incorrect)))

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
				stop_time = s_date
				
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

			-- box:log("Info", string.format("Received prediction %d", s_code))
			
			if s_date >= start_time and s_date < stop_time and (s_code >= OVTK_StimulationId_Label_00 and s_code <= OVTK_StimulationId_Label_1F) then

				if do_debug then box:log("Info", string.format("Accepted prediction %d at ", s_code) .. s_date) end

				real_target = current_target + 1
				prediction = s_code - OVTK_StimulationId_Label_00 + 1
				score[real_target][prediction] = score[real_target][prediction] + 1

			end
		end

		box:sleep()

	end

end
