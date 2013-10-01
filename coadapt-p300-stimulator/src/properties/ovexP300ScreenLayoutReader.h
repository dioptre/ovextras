#ifndef __ovExternalP300LetterGroupReader__
#define __ovExternalP300LetterGroupReader__

#include "ovexP300PropertyReader.h"
#include "ovexP300KeyDescriptor.h"

#include <exception>
#include <list>
#include <vector>
#include <cstring>
#include <map>

namespace OpenViBEApplications
{
	
	class _AutoCast_
	{
	public:
		_AutoCast_(const char * settingValue) : m_sSettingValue(settingValue) {  }
		operator GColor (void)
		{
			GColor l_oColor;
			int r=0, g=0, b=0;
			sscanf(m_sSettingValue, "%i,%i,%i", &r, &g, &b);
			l_oColor.red=(GLfloat)r/100.0f;
			l_oColor.green=(GLfloat)g/100.0f;
			l_oColor.blue=(GLfloat)b/100.0f;
			return l_oColor;
		}

		const char * m_sSettingValue;
	};	
	
	class P300ScreenLayoutReader : public ExternalP300PropertyReader
	{
		
	public:
		
		P300ScreenLayoutReader(OpenViBE::Kernel::IKernelContext* kernelContext) : ExternalP300PropertyReader(kernelContext) 
		{
			m_lKeyList = new std::vector<P300KeyDescriptor*>();
			m_lSymbolList = new std::list<std::string>();
			m_mDefaultEventMapScaleSize = new std::map<OpenViBE::uint32, OpenViBE::float32>();
			m_mDefaultEventMapForegroundColor = new std::map<OpenViBE::uint32, GColor>();
			m_mDefaultEventMapBackgroundColor = new std::map<OpenViBE::uint32, GColor>();
			m_mDefaultEventMapSource = new std::map<OpenViBE::uint32, OpenViBE::CString>();
			m_mDefaultEventMapLabel = new std::map<OpenViBE::uint32, std::string>();
			m_mDefaultIsTextSymbol = new std::map<OpenViBE::uint32, OpenViBE::boolean>(); 
			
			m_bDefaultKeyProperties = false;
			m_bEventElement = false;
			m_bScaleSize = false;
			m_bForegroundColor = false;
			m_bBackgroundColor = false;
			m_bLabel = false;
			m_bSource = false;
			m_bKeyboardIsGrid = false;
			
			EventStringMap[ KeyEventStrings[0] ] = FLASH;
			EventStringMap[ KeyEventStrings[1] ] = NOFLASH;
			EventStringMap[ KeyEventStrings[2] ] = CENTRAL_FEEDBACK_WRONG;
			EventStringMap[ KeyEventStrings[3] ] = CENTRAL_FEEDBACK_CORRECT;
			EventStringMap[ KeyEventStrings[4] ] = TARGET;
		}
		
		~P300ScreenLayoutReader()
		{
			std::vector<P300KeyDescriptor*>::iterator l_ListIterator = m_lKeyList->begin();
			for (; l_ListIterator!=m_lKeyList->end(); l_ListIterator++)
				delete *l_ListIterator;
			delete m_lKeyList;
			delete m_lSymbolList;
			delete m_mDefaultEventMapScaleSize;
			delete m_mDefaultEventMapForegroundColor;
			delete m_mDefaultEventMapBackgroundColor;
			delete m_mDefaultEventMapSource;
			delete m_mDefaultEventMapLabel;
			delete m_mDefaultIsTextSymbol;
		}
		
		virtual void readPropertiesFromFile(OpenViBE::CString propertyFile);
		
		BoxDimensions getResultAreaDimensions() { return m_dResultAreaDimensions; }
		BoxDimensions getPredictionAreaDimensions() { return m_dPredictionAreaDimensions; }
		BoxDimensions getP300KeyboardDimensions() { return m_dKeyboardDimensions; }
		BoxDimensions getTargetAreaDimensions() { return m_dTargetAreaDimensions; }
		std::vector<P300KeyDescriptor*>* getP300KeyboardLayout() { return m_lKeyList; } 
		std::list<std::string> * getSymbolList() { return m_lSymbolList; }
		OpenViBE::uint32 getNumberOfStandardKeys() { return m_ui32NumberOfStandardKeys; }	
		OpenViBE::uint32 getNumberOfPredictiveKeys() { return m_ui32PredictionAreaRows*m_ui32PredictionAreaColumns; }
		OpenViBE::uint32 getNumberOfKeys() { return m_ui32NumberOfStandardKeys+getNumberOfPredictiveKeys(); }
		OpenViBE::uint32 getPredictionAreaRows() { return m_ui32PredictionAreaRows;}
		OpenViBE::uint32 getPredictionAreaColumns() { return m_ui32PredictionAreaColumns;}
		OpenViBE::boolean isKeyboardTable() { return m_bKeyboardIsGrid;}
		
		const OpenViBE::float32 getDefaultScaleSize(const VisualState event) const { return m_mDefaultEventMapScaleSize->find(event)->second; }
		const GColor& getDefaultForegroundColor(const VisualState event) const { return m_mDefaultEventMapForegroundColor->find(event)->second; }
		const GColor& getDefaultBackgroundColor(const VisualState event) const { return m_mDefaultEventMapBackgroundColor->find(event)->second; }
		//const OpenViBE::CString& getDefaultSource(const VisualState event) const;// { return eventMapSource->find(event)->second; }
		//const std::string& getDefaultLabel(const VisualState event) const;// { return eventMapLabel->find(event)->second; }
		//const OpenViBE::boolean isDefaultTextSymbol(const VisualState event) const;// { return eventMapIsTextSymbol->find(event)->second; }		

	protected:
		void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
		void processChildData(const char* sData); // XML IReaderCallback
		void closeChild(void); // XML ReaderCallback
		
	private:
		void parseDimensions(BoxDimensions& dimensions, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount);
		void parseKeyLabels(const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount);

	protected:
		OpenViBE::uint32 m_ui32NumberOfStandardKeys;
		//OpenViBE::uint32 m_ui32NumberOfPredictiveKeys;
		BoxDimensions m_dKeyboardDimensions;
		BoxDimensions m_dPredictionAreaDimensions;
		BoxDimensions m_dResultAreaDimensions;
		BoxDimensions m_dTargetAreaDimensions;
		std::vector<P300KeyDescriptor*> * m_lKeyList;
		std::list<std::string> * m_lSymbolList;
		
		std::map<OpenViBE::uint32, OpenViBE::float32>* m_mDefaultEventMapScaleSize;
		std::map<OpenViBE::uint32, GColor>* m_mDefaultEventMapForegroundColor;
		std::map<OpenViBE::uint32, GColor>* m_mDefaultEventMapBackgroundColor;
		std::map<OpenViBE::uint32, OpenViBE::CString>* m_mDefaultEventMapSource;
		std::map<OpenViBE::uint32, std::string>* m_mDefaultEventMapLabel;	
		std::map<OpenViBE::uint32, OpenViBE::boolean>* m_mDefaultIsTextSymbol;
		
		OpenViBE::boolean m_bKeyboardIsGrid;
		OpenViBE::uint32 m_ui32PredictionAreaRows;
		OpenViBE::uint32 m_ui32PredictionAreaColumns;
		
	private:
		P300KeyDescriptor* m_pKey;
		OpenViBE::boolean m_bDefaultKeyProperties;
		OpenViBE::boolean m_bEventElement;
		OpenViBE::boolean m_bScaleSize, m_bForegroundColor, m_bBackgroundColor, m_bLabel, m_bSource;
		VisualState m_iState;
		
		static OpenViBE::CString KeyEventStrings[5];
		std::map<OpenViBE::CString,VisualState> EventStringMap;				
	};
};

#endif
