#ifndef __P300KeyboardHandler_H__
#define __P300KeyboardHandler_H__

#include <map>
#include <cstring>
#include <list>

#include "../visualisation/glGTable.h"
//#include "../visualisation/glP300MainContainer.h"
#include "../visualisation/glGButton.h"
#include "../visualisation/glGLabel.h"
#include "../visualisation/glGPictureSymbol.h"
#include "../properties/ovexP300InterfacePropertyReader.h"
#include "../properties/ovexP300ScreenLayoutReader.h"

namespace OpenViBEApplications
{	
	//template<class TContainer>
	class P300KeyboardHandler : public GObserver, public GObservable
	{	

	public:

		P300KeyboardHandler(GContainer* container, GContainer* container2, P300InterfacePropertyReader* propertyObject, P300ScreenLayoutReader* layoutPropObject);
		
		virtual ~P300KeyboardHandler()
		{
			std::cout << "~P300KeyboardHandler() called\n";
			for (OpenViBE::uint32 i=0; i<8; i++)
			{
				for (OpenViBE::uint32 j=0; j<m_pLayoutObject->getNumberOfKeys(); j++)
					delete m_vButtons->at(i)->at(j);
				delete m_vButtons->at(i);
			}			
			delete m_vButtons;
			std::cout << "~P300KeyboardHandler() finished\n";
		}
		
		//inherited from GObserver
		virtual void update(GObservable* observable, const void * pUserData);
		
		//inherited from GObservable
		virtual void addObserver(GObserver * observer);
		
		virtual void addActionObserver(OpenViBE::CString action, GObserver * observer);
		
		virtual void updateChildProperties();
		virtual void updateChildStates(OpenViBE::uint32* states);
		virtual void resetChildStates();
		
		virtual void updateChildProbabilities(OpenViBE::float64* symbolProbabilities); //to manage 'continuous' color feedback
		virtual void resetMostActiveChildStates(); //to enable overlapping stimuli
		
	private:
		GColor getAssignedProbabilityColor(OpenViBE::uint32 symbolIndex);
		void initializeColorMap(GColor startColor, GColor endColor, OpenViBE::uint32 nSteps, OpenViBE::float32 sigLevel);
		void initializeKeyboard();
		void setGButtonFromSLabel(GButton* gbutton, OpenViBE::uint32 keyIndex, VisualState keyState);
		GLabel* constructGLabelFromDescriptor(OpenViBE::uint32 keyIndex, VisualState state);
		
	protected:
		
		GContainer* m_pSymbolContainer;
		GContainer* m_pPredictionContainer;
		P300InterfacePropertyReader* m_pPropertyObject;
		P300ScreenLayoutReader* m_pLayoutObject;
		
		std::map< OpenViBE::float64, GColor > m_mColorMap;
		std::vector<OpenViBE::float64> m_vSymbolProbabilties;
		std::vector<OpenViBE::boolean> m_vProbableSymbols;
		std::vector<OpenViBE::uint32> m_vActiveCycles;
		
		std::vector<OpenViBE::uint32> m_vSymbolStates;	
		std::vector<OpenViBE::uint32> m_vPreviousSymbolStates;	
		
            struct FontID
            {
                  OpenViBE::CString source;
                  OpenViBE::float32 size;
                  OpenViBE::uint32 keyIndex;
                  bool operator<(const FontID& rhs) const
                  {
                        if (keyIndex < rhs.keyIndex)
                              return true;
                        else if (keyIndex == rhs.keyIndex)
                        {
                              if (size < rhs.size)
                                    return true;
                              else if (size == rhs.size)
                                    return keyIndex < rhs.keyIndex;
                        }
                        return false;
                  }
            };
	private:
		//std::vector<GButtonDescriptor> m_vButtonDescriptors;
		std::vector< std::vector<GButton*>* >* m_vButtons;
		
		OpenViBE::uint32	m_ui32MaximumSymbolActivity;
		
		std::map<VisualState,VisualState> VisualStateMap;
		std::map<FontID,boost::shared_ptr<FTFont> > m_mFontSourceMap;
	};
};
#endif
