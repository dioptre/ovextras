#ifndef __ovExternalP300PropertyReader__
#define __ovExternalP300PropertyReader__


#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <xml/IReader.h>

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stack>

#include "../visualisation/glGObject.h"

namespace OpenViBEApplications
{
	enum SpellingMode {
		ODDBALL,
		CALIBRATION_MODE,
		COPY_MODE,
		FREE_MODE
	};
		
	class ExternalP300PropertyReader : public XML::IReaderCallback
	{
				
	public:	
		ExternalP300PropertyReader(OpenViBE::Kernel::IKernelContext* kernelContext) : m_pKernelContext(kernelContext) {}

		virtual void readPropertiesFromFile(OpenViBE::CString propertyFile);
		
		virtual OpenViBE::Kernel::IKernelContext* getKernelContext()
		{
			return m_pKernelContext;
		}

	protected:
		virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount){} 
		virtual void processChildData(const char* sData){} 
		virtual void closeChild(void){}
		
		void writeAttribute(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount);
		void writeElement(const char* sName, const char* sData);
		
	protected:	

		OpenViBE::Kernel::IKernelContext* m_pKernelContext;
		std::stack<OpenViBE::CString> m_vNode;
	};
};

#endif
