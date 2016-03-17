
sent = false

function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")
	sent = false;
end

function uninitialize(box)
end

function process(box)

	while box:keep_processing() and sent == false do
	
		current_time = box:get_current_time() + 1

		box:send_stimulation(1, OVTK_StimulationId_Label_01,       current_time, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_02,       current_time+4, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_03,       current_time+8, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_02,       current_time+12, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_01,       current_time+16, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_03,       current_time+20, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_01,       current_time+24, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_02,       current_time+28, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_03,       current_time+32, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_03,       current_time+36, 0)
		box:send_stimulation(1, OVTK_StimulationId_Label_02,       current_time+40, 0)		
		box:send_stimulation(1, OVTK_StimulationId_Label_01,       current_time+44, 0)
		box:send_stimulation(1, OVTK_StimulationId_ExperimentStop, current_time+48, 0)	

		sent = true
		
		box:sleep()
		
	end
	
end

