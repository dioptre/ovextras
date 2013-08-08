#ifndef __ovP300KeyDescriptor__
#define __ovP300KeyDescriptor__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <xml/IReader.h>

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
			actions = new std::vector<OpenViBE::CString>();
			eventMapScaleSize = new std::map<VisualState, OpenViBE::float32>();
			eventMapForegroundColor = new std::map<VisualState, GColor>();
			eventMapBackgroundColor = new std::map<VisualState, GColor>();
			eventMapSource = new std::map<VisualState, OpenViBE::CString>();
			eventMapLabel = new std::map<VisualState, std::string>();
			eventMapIsTextSymbol = new std::map<VisualState, OpenViBE::boolean>();
		}
		
		~P300KeyDescriptor()
		{
			eventMapScaleSize->clear();
			eventMapForegroundColor->clear();
			eventMapBackgroundColor->clear();
			eventMapSource->clear();
			eventMapLabel->clear();
			actions->clear();
			eventMapIsTextSymbol->clear();
			delete eventMapScaleSize;
			delete eventMapForegroundColor;
			delete eventMapBackgroundColor;
			delete eventMapSource;
			delete eventMapLabel;	
			delete actions;
			delete eventMapIsTextSymbol;
		}
		
		void toString()
		{
			std::map<VisualState, OpenViBE::float32>::iterator iteratorScaleSize = eventMapScaleSize->begin();
			//std::vector<OpenViBE::CString>* actions;
			std::map<VisualState, GColor>::iterator iteratorForeground = eventMapForegroundColor->begin();
			std::map<VisualState, GColor>::iterator iteratorBackground = eventMapBackgroundColor->begin();
			std::map<VisualState, OpenViBE::CString>::iterator iteratorSource = eventMapSource->begin();
			std::map<VisualState, std::string>::iterator iteratorLabel = eventMapLabel->begin();
			std::map<VisualState, OpenViBE::boolean>::iterator iteratorIsText = eventMapIsTextSymbol->begin();
			
			std::cout << "Key descriptor\n";
			for (;iteratorIsText!=eventMapIsTextSymbol->end(); iteratorIsText++)
				std::cout << "Is text label in state " << iteratorIsText->first << "? " << iteratorIsText->second << "\n";			
			for (;iteratorLabel!=eventMapLabel->end(); iteratorLabel++)
				std::cout << "Label in state " << iteratorLabel->first << " is " << iteratorLabel->second.c_str() << "\n";
			for (;iteratorSource!=eventMapSource->end(); iteratorSource++)
				std::cout << "Source file in state " << iteratorSource->first << " is " << iteratorSource->second.toASCIIString() << "\n";			
			for (;iteratorScaleSize!=eventMapScaleSize->end(); iteratorScaleSize++)
				std::cout << "Scale size in state " << iteratorScaleSize->first << " is " << iteratorScaleSize->second << "\n";
			for (;iteratorBackground!=eventMapBackgroundColor->end(); iteratorBackground++)
				std::cout << "Background color in state " << iteratorBackground->first << " is " << iteratorBackground->second.red <<
				", " << iteratorBackground->second.green << ", " << iteratorBackground->second.blue << "\n";			
			for (;iteratorForeground!=eventMapForegroundColor->end(); iteratorForeground++)
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
		
		void addAction(const OpenViBE::CString& action) { actions->push_back(action); }
		void setDimensions(BoxDimensions& dim) { dimensions = dim; }
		
		//getters
		const OpenViBE::float32 getScaleSize(const VisualState event) const;
		const GColor& getForegroundColor(const VisualState event) const;
		const GColor& getBackgroundColor(const VisualState event) const;
		const OpenViBE::CString& getSource(const VisualState event) const;
		const std::string& getLabel(const VisualState event) const;
		const OpenViBE::boolean isTextSymbol(const VisualState event) const;
		const OpenViBE::boolean isActionEnabled(OpenViBE::CString action);
		const std::vector<OpenViBE::CString>* getActions() const { return actions; }
		const OpenViBE::CString getAction() const { return actions->at(0); }
		const BoxDimensions& getBoxDimensions() const { return dimensions; }
		
	
	private:
		BoxDimensions dimensions;
		std::vector<OpenViBE::CString>* actions;
		std::map<VisualState, OpenViBE::float32>* eventMapScaleSize;
		std::map<VisualState, GColor>* eventMapForegroundColor;
		std::map<VisualState, GColor>* eventMapBackgroundColor;
		std::map<VisualState, OpenViBE::CString>* eventMapSource;
		std::map<VisualState, std::string>* eventMapLabel;
		std::map<VisualState, OpenViBE::boolean>* eventMapIsTextSymbol;
	};
};

#endif