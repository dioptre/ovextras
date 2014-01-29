#include "ovp_defines.h"

/*
#include <openvibe/ov_all.h>
#include <openvibe-toolkit/ovtk_all.h>
*/

#include "box-algorithms/ovpCBoxAlgorithmTwoSampleTTest.h"
#include "box-algorithms/ovpCBoxAlgorithmLikelinessDistributor.h"
#include "box-algorithms/basic/ovpCBoxAlgorithmEpochVariance.h"
#include "box-algorithms/ovpCBoxAlgorithmXDAWNSpatialFilterTrainer.h"
#include "box-algorithms/ovpCBoxAlgorithmSpatialFilter.h"
#include "box-algorithms/ovpCBoxAlgorithmMultipleSpatialFilters.h"
#include "box-algorithms/ovpCBoxAlgorithmConditionalIdentity.h"
#include "algorithms/basic/ovpCMatrixVariance.h"

OVP_Declare_Begin();
	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_EpochAverageMethod, "Epoch Average method");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Moving epoch average", OVP_TypeId_EpochAverageMethod_MovingAverage.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Moving epoch average (Immediate)", OVP_TypeId_EpochAverageMethod_MovingAverageImmediate.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Epoch block average", OVP_TypeId_EpochAverageMethod_BlockAverage.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Cumulative average", OVP_TypeId_EpochAverageMethod_CumulativeAverage.toUInteger());

	OVP_Declare_New(OpenViBEPlugins::SignalProcessingStatistics::CBoxAlgorithmTwoSampleTTestDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingStatistics::CBoxAlgorithmLikelinessDistributorDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessing::CEpochVarianceDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessing::CMatrixVarianceDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingCoAdapt::CBoxAlgorithmXDAWNSpatialFilterTrainerDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingCoAdapt::CBoxAlgorithmSpatialFilterDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingCoAdapt::CConditionalIdentityDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingCoAdapt::CBoxAlgorithmMultipleSpatialFiltersDesc);
OVP_Declare_End();
