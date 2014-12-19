#include "ovpCChannelUnitsEncoder.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::StreamCodecs;

boolean CChannelUnitsEncoder::initialize(void)
{
	CStreamedMatrixEncoder::initialize();

	ip_bDynamic.initialize(getInputParameter(OVP_Algorithm_ChannelUnitsStreamEncoder_InputParameterId_Dynamic));

	return true;
}

boolean CChannelUnitsEncoder::uninitialize(void)
{
	ip_bDynamic.uninitialize();

	CStreamedMatrixEncoder::uninitialize();

	return true;
}

// ________________________________________________________________________________________________________________
//

boolean CChannelUnitsEncoder::processHeader(void)
{
	CStreamedMatrixEncoder::processHeader();

//	const IMatrix* l_pUnits=ip_pMeasurementUnits;


	m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits);
	 m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits_Dynamic);
	  m_pEBMLWriterHelper->setUIntegerAsChildData(ip_bDynamic?1:0);
	 m_pEBMLWriterHelper->closeChild();

	 /*
	m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits_MeasurementUnit);
	m_pEBMLWriterHelper->setBinaryAsChildData(l_pUnits->getBuffer(), l_pUnits->getBufferElementCount()*sizeof(float64));
	m_pEBMLWriterHelper->closeChild();
	*/

	 /*
	 // Convention: Only encode units if they have changed from the default (unit=0,factor=0)
	 bool l_bGotUnits = false;
	 for(uint32 i=0;i<l_pUnits->getBufferElementCount();i++) 
	 {
		 if(l_pUnits->getBuffer()[i]!=0) 
		 {
			 l_bGotUnits = true;
			 break;
		 }
	 }
	 if(l_bGotUnits)
	 {
		for(uint32 i=0; i<l_pUnits->getDimensionSize(0); i++)
		{
			m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits_MeasurementUnit);
			m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits_MeasurementUnit_Unit);
			    m_pEBMLWriterHelper->setFloat64AsChildData(l_pUnits->getBuffer()[i*2+0]);
			m_pEBMLWriterHelper->closeChild();
			m_pEBMLWriterHelper->openChild(OVTK_NodeId_Header_ChannelUnits_MeasurementUnit_Factor);
		        m_pEBMLWriterHelper->setFloat64AsChildData(l_pUnits->getBuffer()[i*2+1]);
			m_pEBMLWriterHelper->closeChild();
			m_pEBMLWriterHelper->closeChild();
		 }	
	 }
	 */

	m_pEBMLWriterHelper->closeChild();

	return true;
}
