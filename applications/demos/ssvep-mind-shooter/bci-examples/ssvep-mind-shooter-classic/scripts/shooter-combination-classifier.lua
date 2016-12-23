class_count = 0

function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")
	class_count = box:get_setting(2)
end

function uninitialize(box)
end

function process(box)

	while box:keep_processing() do

		if box:get_stimulation_count(1) > 0 then

			local decision = 0
			local conflict = false
			
			-- check each input
			for i = 1, class_count do
				-- if the frequency is considered as stimulated
				stim_id = box:get_stimulation(i, 1)
				-- box:log("Info", string.format("Received %d from %d", stim_id, i))	
				if stim_id == OVTK_StimulationId_Label_01 then
					if decision ~= 0 then
						conflict = true
					end
					decision = i
				end
				box:remove_stimulation(i, 1)
			end

			-- If several classifiers predict 'in class', predict no action class 0 instead.
			if conflict then
				decision = 0
			end
			
			box:send_stimulation(1, OVTK_StimulationId_Label_00 + decision, box:get_current_time() + 0.01, 0)		
		end
		
		box:sleep()
	end
end
