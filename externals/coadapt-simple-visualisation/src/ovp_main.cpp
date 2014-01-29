#include "ovp_defines.h"

#include "ovpCBoxAlgorithmP300SpellerVisualisationSplotch.h"
#include "ovpCBoxAlgorithmSerialP300SpellerVisualisation.h"
#include "ovpCBoxAlgorithmP300SpellerVisualisationLED.h"
#include "ovpCBoxAlgorithmErpPlot.h"
//#include "ovpCBoxAlgorithmTest.h"

OVP_Declare_Begin()
	OVP_Declare_New(OpenViBEPlugins::SimpleVisualisation::CBoxAlgorithmP300SpellerVisualisationSplotchDesc)
	OVP_Declare_New(OpenViBEPlugins::SimpleVisualisation::CBoxAlgorithmP300SpellerVisualisationLEDDesc)
	OVP_Declare_New(OpenViBEPlugins::SimpleVisualisation::CBoxAlgorithmSerialP300SpellerVisualisationDesc)
	OVP_Declare_New(OpenViBEPlugins::VisualisationPresentation::CBoxAlgorithmErpPlotDesc)
	//OVP_Declare_New(OpenViBEPlugins::VisualisationPresentation::CBoxAlgorithmTestDesc)
OVP_Declare_End()
