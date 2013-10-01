#ifndef __GP300MainContainer_OV_H__
#define __GP300MainContainer_OV_H__

#include <list>
#include <vector>
#include <queue>

#include "../properties/ovexP300InterfacePropertyReader.h"
#include "../properties/ovexP300ScreenLayoutReader.h"
#include "../ovexITagger.h"
#include "../handlers/ovexP300KeyboardHandler.h"
#include "../handlers/ovexP300ResultAreaHandler.h"
#include "../handlers/ovexUndoHandler.h"
#include "../handlers/ovexBackspaceHandler.h"
#include "../handlers/ovexP300PredictionboardHandler.h"
#include "../handlers/ovexP300TargetAreaHandler.h"

#include "glGTable.h"

#define WORDPREDICTION

namespace OpenViBEApplications
{
	//class P300KeyboardHandler;
	
	class P300MainContainer : public GContainer
	{
	//typedef void (P300MainContainer::*_change_symbol_property_)(GSymbol* l_Symbol, void* pUserData);	

		
	public:
		P300MainContainer(P300InterfacePropertyReader* propertyObject, P300ScreenLayoutReader* layoutPropObject);	
		P300MainContainer(const P300MainContainer& gcontainer);
		virtual ~P300MainContainer();		
		
		/*P300MainContainer& operator= (P300MainContainer const& mainContainer)
		{
			if(this!=&mainContainer)
			{
				GTable* l_gGridArea = mainContainer.m_gGridArea->clone();
				GTable* l_gResultArea = mainContainer.m_gResultArea->clone();
				GTable* l_gPredictionArea = mainContainer.m_gPredictionArea->clone();

				delete m_gGridArea;
				delete m_gResultArea;
				delete m_gPredictionArea;
				
				this->m_gResultArea = l_gResultArea; this->getChild(0) = this->m_gResultArea;
				this->m_gGridArea = l_gGridArea; this->getChild(1) = this->m_gGridArea;
				this->m_gPredictionArea = l_gPredictionArea; this->getChild(2) = this->m_gPredictionArea;
				//this->m_bSymbolPositionsChanged = mainContainer.m_bSymbolPositionsChanged;
				//this->m_pPropertyObject = mainContainer.m_pPropertyObject;
				//this->m_oSDLSurface = mainContainer.m_oSDLSurface;
			}
			return *this;
		}*/
		
		P300MainContainer* clone() const
		{
			return new P300MainContainer(*this);
		}		
		
		void changeBackgroundColorDiodeArea(GColor bColor);
		
		P300KeyboardHandler* getKeyboardHandler();	
		
		//inherited functions		
		std::string toString() const;
		
		//encapsulation for draw functions
		void drawAndSync(ITagger * tagger, std::queue<OpenViBE::uint32>& l_qEventQueue);
		
		static void initializeGL(OpenViBE::boolean fullScreen, OpenViBE::float32 width, OpenViBE::float32 height);
	private:
		//void draw(ITagger * tagger, std::queue<OpenViBE::uint32> &l_qEventQueue);
		void draw();
		
		void initialize(OpenViBE::uint32 nGridCells);
		void initializeGridArea(OpenViBE::uint32 nCells);
		void initializeResultArea();
		void initializePredictionArea();
		void initializeTargetArea();
		void initializeDiodeArea();
	
		static void do_ortho(OpenViBE::float32 width, OpenViBE::float32 height);	
		
	private:
		static SDL_Surface* m_oSDLSurface;
		
		#ifdef WORDPREDICTION
		P300PredictionboardHandler* m_pP300PredictionHandler;
		#endif
		P300UndoHandler* m_pP300UndoHandler;
		P300ResultAreaHandler* m_pP300ResultAreaHandler;
		P300TargetAreaHandler* m_pP300TargetAreaHandler;
		P300KeyboardHandler* m_pP300KeyboardHandler;
		P300BackspaceHandler* m_pP300BackspaceHandler;
		
		//WordPredictionInterface* m_pWordPredictionEngine;
		
		GContainer* m_gGridArea; //cleaned up by GContainer where they are added as childs
		GTable* m_gTargetArea;
		GTable* m_gResultArea;
		GTable* m_gPredictionArea;
		GContainer* m_gDiodeArea;
		
		P300InterfacePropertyReader* m_pInterfacePropertyObject; //responsibility of the caller to clean this up
		P300ScreenLayoutReader* m_pScreenLayoutObject;

		#ifdef OUTPUT_TIMING
            FILE * timingFile;
		#endif
	};
	
};

#endif
