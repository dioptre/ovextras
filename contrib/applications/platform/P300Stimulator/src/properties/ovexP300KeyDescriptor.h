#ifndef __ovP300KeyDescriptor__
#define __ovP300KeyDescriptor__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <xml/IReader.h>

#if defined TARGET_HAS_ThirdPartyModulesForExternalStimulator
#include "../ova_defines.h"
#include "../visualisation/glGObject.h"

#include <exception>
#include <vector>
#include <cstring>
#include <map>

namespace OpenViBEApplications
{
	class NoSuchEventException : public std::exception
	{
		virtual const char* what() const throw()
		{
			return "There is no such event";
		}
	};	
	
	class P300KeyDescriptor
	{
	public:
		P300KeyDescriptor()
		{
			m_vActions = new std::vector<OpenViBE::CString>();
			m_vEventMapScaleSize = new std::map<VisualState, OpenViBE::float32>();
			m_vEventMapForegroundColor = new std::map<VisualState, GColor>();
			m_vEventMapBackgroundColor = new std::map<VisualState, GColor>();
			m_vEventMapSource = new std::map<VisualState, OpenViBE::CString>();
			m_vEventMapLabel = new std::map<VisualState, std::string>();
			m_vEventMapIsTextSymbol = new std::map<VisualState, OpenViBE::boolean>();
		}
		
		~P300KeyDescriptor()
		{
			m_vEventMapScaleSize->clear();
			m_vEventMapForegroundColor->clear();
			m_vEventMapBackgroundColor->clear();
			m_vEventMapSource->clear();
			m_vEventMapLabel->clear();
			m_vActions->clear();
			m_vEventMapIsTextSymbol->clear();
			delete m_vEventMapScaleSize;
			delete m_vEventMapForegroundColor;
			delete m_vEventMapBackgroundColor;
			delete m_vEventMapSource;
			delete m_vEventMapLabel;
			delete m_vActions;
			delete m_vEventMapIsTextSymbol;
		}
		
		void toString()
		{
			std::map<VisualState, OpenViBE::float32>::iterator iteratorScaleSize = m_vEventMapScaleSize->begin();
			//std::vector<OpenViBE::CString>* actions;
			std::map<VisualState, GColor>::iterator iteratorForeground = m_vEventMapForegroundColor->begin();
			std::map<VisualState, GColor>::iterator iteratorBackground = m_vEventMapBackgroundColor->begin();
			std::map<VisualState, OpenViBE::CString>::iterator iteratorSource = m_vEventMapSource->begin();
			std::map<VisualState, std::string>::iterator iteratorLabel = m_vEventMapLabel->begin();
			std::map<VisualState, OpenViBE::boolean>::iterator iteratorIsText = m_vEventMapIsTextSymbol->begin();
			
			std::cout << "Key descriptor\n";
			for (;iteratorIsText!=m_vEventMapIsTextSymbol->end(); iteratorIsText++)
				std::cout << "Is text label in state " << iteratorIsText->first << "? " << iteratorIsText->second << "\n";			
			for (;iteratorLabel!=m_vEventMapLabel->end(); iteratorLabel++)
				std::cout << "Label in state " << iteratorLabel->first << " is " << iteratorLabel->second.c_str() << "\n";
			for (;iteratorSource!=m_vEventMapSource->end(); iteratorSource++)
				std::cout << "Source file in state " << iteratorSource->first << " is " << iteratorSource->second.toASCIIString() << "\n";			
			for (;iteratorScaleSize!=m_vEventMapScaleSize->end(); iteratorScaleSize++)
				std::cout << "Scale size in state " << iteratorScaleSize->first << " is " << iteratorScaleSize->second << "\n";
			for (;iteratorBackground!=m_vEventMapBackgroundColor->end(); iteratorBackground++)
				std::cout << "Background color in state " << iteratorBackground->first << " is " << iteratorBackground->second.red <<
				", " << iteratorBackground->second.green << ", " << iteratorBackground->second.blue << "\n";			
			for (;iteratorForeground!=m_vEventMapForegroundColor->end(); iteratorForeground++)
				std::cout << "Foreground color in state " << iteratorForeground->first << " is " << iteratorForeground->second.red <<
				", " << iteratorForeground->second.green << ", " << iteratorForeground->second.blue << "\n";
			std::cout << "----------------\n";
		}
		
		//modifiers
		void addScaleSize(const VisualState event, const OpenViBE::float32& value);
		void addForegroundColor(const VisualState event, const GColor& value);
		void addBackgroundColor(const VisualState event, const GColor& value);
		void addSource(const VisualState event, const OpenViBE::CString& value);
		void addLabel(const VisualState event, const std::string& value);
		void setIfTextSymbol(const VisualState event, const OpenViBE::boolean value);	
		
		void addAction(const OpenViBE::CString& action) { m_vActions->push_back(action); }
		void setDimensions(BoxDimensions& dim) { m_oDimensions = dim; }
		
		//getters
		const OpenViBE::float32 getScaleSize(const VisualState event) const;
		const GColor& getForegroundColor(const VisualState event) const;
		const GColor& getBackgroundColor(const VisualState event) const;
		const OpenViBE::CString& getSource(const VisualState event) const;
		const std::string& getLabel(const VisualState event) const;
		const OpenViBE::boolean isTextSymbol(const VisualState event) const;
		const OpenViBE::boolean isActionEnabled(OpenViBE::CString action);
		const std::vector<OpenViBE::CString>* getActions() const { return m_vActions; }
		const OpenViBE::CString getAction() const { return m_vActions->at(0); }
		const BoxDimensions& getBoxDimensions() const { return m_oDimensions; }
		
	
	private:
		BoxDimensions m_oDimensions;
		std::vector<OpenViBE::CString>* m_vActions;
		std::map<VisualState, OpenViBE::float32>* m_vEventMapScaleSize;
		std::map<VisualState, GColor>* m_vEventMapForegroundColor;
		std::map<VisualState, GColor>* m_vEventMapBackgroundColor;
		std::map<VisualState, OpenViBE::CString>* m_vEventMapSource;
		std::map<VisualState, std::string>* m_vEventMapLabel;
		std::map<VisualState, OpenViBE::boolean>* m_vEventMapIsTextSymbol;
	};
};
#endif
#endif
