#include "ovexP300KeyboardHandler.h"
#include "ovexP300PredictionboardHandler.h"

#include <utility> 

#include "../visualisation/glGSymbol.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

P300KeyboardHandler::P300KeyboardHandler(GContainer* container, GContainer* container2, P300InterfacePropertyReader* propertyObject, P300ScreenLayoutReader* layoutPropObject) :
m_pSymbolContainer(container), m_pPredictionContainer(container2), m_pPropertyObject(propertyObject), m_pLayoutObject(layoutPropObject)
{			
	this->initializeColorMap(propertyObject->getFeedbackStartColor(), propertyObject->getFeedbackEndColor(), 
						propertyObject->getColorFeedbackSteps(), propertyObject->getFeedbackStartValue());
	
	m_vButtons = new std::vector< std::vector<GButton*>* >(8);
	for (uint32 i=0; i<8; i++)
		m_vButtons->at(i) = new std::vector<GButton*>();

	VisualStateMap[ FLASH ] = FLASH;
	VisualStateMap[ NOFLASH ] = NOFLASH;
	VisualStateMap[ CENTRAL_FEEDBACK_WRONG ] = CENTRAL_FEEDBACK_WRONG;
	VisualStateMap[ CENTRAL_FEEDBACK_CORRECT ] = CENTRAL_FEEDBACK_CORRECT;
	VisualStateMap[ NONCENTRAL_FEEDBACK_WRONG ] = CENTRAL_FEEDBACK_WRONG;
	VisualStateMap[ NONCENTRAL_FEEDBACK_CORRECT ] = CENTRAL_FEEDBACK_CORRECT;	
	VisualStateMap[ NONCENTRAL_FEEDBACK_WRONG_SELECTED ] = NOFLASH;
	VisualStateMap[ TARGET ] = TARGET;
	
	m_ui32MaximumSymbolActivity = 0;
	
	initializeKeyboard();
}

void P300KeyboardHandler::addObserver(GObserver * observer)
{
	GObservable::addObserver(observer);
	for (uint32 j=0; j<m_pSymbolContainer->getNumberOfChildren(); j++)
		m_pSymbolContainer->getChild(j)->addObserver(observer);
	for (uint32 j=0; j<m_pLayoutObject->getNumberOfKeys(); j++)
		for (uint32 i=0; i<8; i++)
			m_vButtons->at(i)->at(j)->addObserver(observer);	
	for (uint32 j=0; j<m_pPredictionContainer->getNumberOfChildren(); j++)
		m_pPredictionContainer->getChild(j)->addObserver(observer);
}

void P300KeyboardHandler::addActionObserver(CString action, GObserver * observer)
{
	//we don't add any observers to buttons in FLASH or NOFLASH state as these should never trigger an action (only a visual state change in the keyboard)
	std::vector<P300KeyDescriptor*>* l_lKeyDefinitions = m_pLayoutObject->getP300KeyboardLayout();
	/*for (uint32 j=0; j<m_pSymbolContainer->getNumberOfChildren(); j++)
	{
		if (l_lKeyDefinitions->at(j)->isActionEnabled(action))
			m_pSymbolContainer->getChild(j)->addObserver(observer);
	}*/
	for (uint32 j=0; j<m_pLayoutObject->getNumberOfKeys(); j++)
	{
		if (l_lKeyDefinitions->at(j)->isActionEnabled(action))
		{
			for (uint32 i=2; i<8; i++)
				m_vButtons->at(i)->at(j)->addObserver(observer);
		}
	}
	/*uint32 k=m_pSymbolContainer->getNumberOfChildren();
	for (uint32 j=0; j<m_pPredictionContainer->getNumberOfChildren(); j++, k++)
	{
		if (l_lKeyDefinitions->at(k)->isActionEnabled(action))
			m_pPredictionContainer->getChild(j)->addObserver(observer);
	}*/
}

void P300KeyboardHandler::update(GObservable* observable, const void * pUserData)
{
	GButton* l_Button = dynamic_cast<GButton*>(observable);
	P300PredictionboardHandler* l_pPredictionHandler = dynamic_cast<P300PredictionboardHandler*>(observable);
	if (l_Button!=NULL)
	{
		//std::cout << "P300KeyboardHandler notification of " << l_Button->toString() << "\n";
		const GButton* l_pNewButton = static_cast<const GButton*>(pUserData);
		//std::cout << "P300KeyboardHandler notification for new " << l_pNewButton->toString() << "\n";
		if (l_pNewButton!=NULL)
			*l_Button = *l_pNewButton;
		/*std::cout << l_Button->toString() << " on position " << l_Button->getX() 
		<< "," << l_Button->getY() << "," << l_Button->getWidth() << "," << l_Button->getHeight() << "," << l_Button->getDepth() 
		<< " and foreground color " << l_Button->getLabel()->getForegroundColor().red << "," << l_Button->getLabel()->getForegroundColor().green << "," << l_Button->getLabel()->getForegroundColor().blue << "\n"; 
		std::cout << "replace by " << l_pNewButton->toString() << " on position " << l_pNewButton->getX() 
		<< "," << l_pNewButton->getY() << "," << l_pNewButton->getWidth() << "," << l_pNewButton->getHeight() << "," << l_pNewButton->getDepth() 
		<< " and foreground color " << l_pNewButton->getLabel()->getForegroundColor().red << "," << l_pNewButton->getLabel()->getForegroundColor().green << "," << l_pNewButton->getLabel()->getForegroundColor().blue << "\n"; 
		*/
		//std::cout << "Updating " << l_Button->toString() << " with state " << l_Button->getState() <<
		//" replacing it with " << l_pNewButton->toString() << " with state " << l_pNewButton->getState() << "\n";	
	}
	else if(l_pPredictionHandler!=NULL)
	{
		const std::vector<std::string>* l_lWords = static_cast< const std::vector<std::string>* >(pUserData);
		//std::cout << "P300KeyboardHandler notification of prediction handler, new words have to be added, such as " << l_lWords->at(0) << "\n";
		std::vector<std::string>::const_iterator l_lIterator = l_lWords->begin();
		
		uint32 k = m_pSymbolContainer->getNumberOfChildren();
		for (uint32 j=0; j<m_pPredictionContainer->getNumberOfChildren(); j++, k++)
		{
			GButton* l_pButton = dynamic_cast<GButton*>(m_pPredictionContainer->getChild(j));
			GSymbol* l_pSymbol = dynamic_cast<GSymbol*>(l_pButton->getLabel());
			//std::cout << "KeyboardHandler " << (*l_lIterator).c_str() << "\n";
			//l_pSymbol->setTextLabel((*l_lIterator).c_str());
			for (uint32 i=0; i<8; i++)
			{
				l_pSymbol = dynamic_cast<GSymbol*>(m_vButtons->at(i)->at(k)->getLabel());
				if (l_pSymbol!=NULL)
					l_pSymbol->setTextLabel((*l_lIterator).c_str());
			}
			l_lIterator++;
		}
	}
}

void P300KeyboardHandler::updateChildProperties()
{
	for (uint32 i=0; i<m_pSymbolContainer->getNumberOfChildren(); i++)
		if (m_vPreviousSymbolStates[i] != m_vSymbolStates[i])
			m_pSymbolContainer->getChild(i)->notifyObservers(m_vButtons->at(m_vSymbolStates[i])->at(i));
		
	uint32 j=m_pSymbolContainer->getNumberOfChildren();
	for (uint32 i=0; i<m_pPredictionContainer->getNumberOfChildren(); i++, j++)
		if (m_vPreviousSymbolStates[j] != m_vSymbolStates[j])
			m_pPredictionContainer->getChild(i)->notifyObservers(m_vButtons->at(m_vSymbolStates[j])->at(j));
}

void P300KeyboardHandler::updateChildStates(uint32* states)
{
	if (m_vSymbolStates.size()!=0)
	{
		for (uint32 i=0; i<m_pLayoutObject->getNumberOfKeys(); i++)
		{
			m_vPreviousSymbolStates[i] = m_vSymbolStates[i];
			if (states[i]!=NONE)
				m_vSymbolStates[i] = states[i];
			if (m_vSymbolStates[i]==FLASH)
				m_vActiveCycles[i]++;
			if (m_vActiveCycles[i]>m_ui32MaximumSymbolActivity)
				m_ui32MaximumSymbolActivity = m_vActiveCycles[i];
		}
	}
	else
		std::cout << "P300KeyboardHandler::updateChildStates: should not reach this state\n";	
}

void P300KeyboardHandler::resetMostActiveChildStates()
{
	uint32 l_ui32Maximum = 0;
	for (uint32 i=0; i<m_vActiveCycles.size(); i++)
	{
		if (m_vActiveCycles[i]==m_ui32MaximumSymbolActivity)
		{
			m_vSymbolStates[i] = NOFLASH;	
			m_vPreviousSymbolStates[i] = NONE;
			m_vActiveCycles[i] = 0;
		}
		if (m_vActiveCycles[i]>l_ui32Maximum)
			l_ui32Maximum = m_vActiveCycles[i];
	}
	m_ui32MaximumSymbolActivity = l_ui32Maximum;	
}

void P300KeyboardHandler::resetChildStates()
{
	for (uint32 i=0; i<m_vActiveCycles.size(); i++)
	{
		m_vActiveCycles[i] = 0;
		m_vSymbolStates[i] = NOFLASH;	
		m_vPreviousSymbolStates[i] = NONE;
	}
	m_ui32MaximumSymbolActivity = 0;	
}

void P300KeyboardHandler::updateChildProbabilities(float64* symbolProbabilities)
{
	//TODO add predictions words to the probability list
	/*m_vSymbolProbabilties.clear();
	m_vProbableSymbols.assign(m_pSymbolContainer->getNumberOfChildren(),false);
	std::multimap<float64, uint32> l_mSortedProbabilities;

	for (uint32 i=0; i<m_vProbableSymbols.size(); i++)
	{
		m_vSymbolProbabilties.push_back(symbolProbabilities[i]);
		l_mSortedProbabilities.insert(std::pair<float64, uint32>(symbolProbabilities[i],i));
	}

	std::multimap<float64, uint32>::reverse_iterator l_Iterator = l_mSortedProbabilities.rbegin();
	for (uint32 i=0; i<m_pPropertyObject->getMaxFeedbackSymbols(); i++, l_Iterator++)
	{
		if ((*l_Iterator).first>m_pPropertyObject->getFeedbackStartValue())
			m_vProbableSymbols[(*l_Iterator).second] = true;
	}*/	
}

void P300KeyboardHandler::initializeKeyboard()
{
	OpenViBE::uint32 l_ui32NumberOfSymbols = m_pLayoutObject->getNumberOfKeys();
	m_vSymbolStates = std::vector<OpenViBE::uint32>(l_ui32NumberOfSymbols,NOFLASH); 
	m_vPreviousSymbolStates = std::vector<OpenViBE::uint32>(l_ui32NumberOfSymbols,NOFLASH); 
	m_vProbableSymbols = std::vector<OpenViBE::boolean>(l_ui32NumberOfSymbols,false); 
	m_vActiveCycles = std::vector<OpenViBE::uint32>(l_ui32NumberOfSymbols,0); 
	m_vSymbolProbabilties = std::vector<OpenViBE::float64>(l_ui32NumberOfSymbols,0.0); 		
	
	GLabel * l_Symbol;
	GButton * l_Button;

	std::vector<P300KeyDescriptor*>* l_lKeyDefinitions = m_pLayoutObject->getP300KeyboardLayout();
	
	for (uint32 it=0; it<l_ui32NumberOfSymbols; it++)
	{
		//l_lKeyDefinitions->at(it)->toString();
		l_Symbol = constructGLabelFromDescriptor(it,NOFLASH);
		l_Symbol->setForegroundColor(l_lKeyDefinitions->at(it)->getForegroundColor(NOFLASH));
		l_Symbol->setLabelScaleSize(l_lKeyDefinitions->at(it)->getScaleSize(NOFLASH));	
		l_Symbol->setBackgroundColor(l_lKeyDefinitions->at(it)->getBackgroundColor(NOFLASH));
		l_Button = new GButton();
		l_Button->setLabel(l_Symbol);
		l_Button->setState(GButton_Inactive);
		l_Button->setAction(l_lKeyDefinitions->at(it)->getAction());
		GTable* l_pKeyboardTable = dynamic_cast<GTable*>(m_pSymbolContainer);
		GTable* l_pPredictionTable = dynamic_cast<GTable*>(m_pPredictionContainer);
		if (!l_lKeyDefinitions->at(it)->isActionEnabled(CString("wordprediction")))
		{
			if (l_pKeyboardTable!=NULL)
				l_pKeyboardTable->addChild(l_Button,0.1f);
			else
			{
				GContainer* l_pKeyboardTable = dynamic_cast<GContainer*>(m_pSymbolContainer);
				if (l_pKeyboardTable!=NULL)
				{
					BoxDimensions dim = l_lKeyDefinitions->at(it)->getBoxDimensions();
					l_pKeyboardTable->addChild(l_Button,dim.x, dim.y, dim.width, dim.height,0.1f);
				}
				else
					std::cout << "Not a container, not a table, what is it then?\n";
			}		
		}
		else if (l_pPredictionTable!=NULL)
			l_pPredictionTable->addChild(l_Button,0.1f);
		else
			std::cout << "One should specify coordinates for " << l_Button->toString() << "\n";
		//l_Button->generateGButtonGLDisplayLists();
            m_vButtons->at(0)->push_back(l_Button->clone());
            //m_vButtons->at(0)->back()->generateGLDisplayLists();
	}
	
	for (uint32 i=1; i<8; i++)
	{
		for (uint32 it=0; it<l_ui32NumberOfSymbols; it++)
		{	
			m_vButtons->at(i)->push_back(m_vButtons->at(0)->at(it)->clone());
			setGButtonFromSLabel(m_vButtons->at(i)->back(), it, static_cast<VisualState>(i));
			//std::cout << m_vButtons->at(i)->back()->toString() << " set state to " << m_vButtons->at(i)->back()->getState() << "\n";
		}
	}

      //std::cout << "FontMap size is " << m_mFontSourceMap.size() << "\n";
}

void P300KeyboardHandler::setGButtonFromSLabel(GButton* gbutton, uint32 keyIndex, VisualState keyState)
{
	std::vector<P300KeyDescriptor*>* l_lKeyDefinitions = m_pLayoutObject->getP300KeyboardLayout();
	BoxDimensions l_Dimensions;
	l_Dimensions.x = m_pSymbolContainer->getX()+0.25f*m_pSymbolContainer->getWidth(); l_Dimensions.y = m_pSymbolContainer->getY()+0.25f*m_pSymbolContainer->getHeight();
	l_Dimensions.width = 0.5f*m_pSymbolContainer->getWidth(); l_Dimensions.height = 0.5f*m_pSymbolContainer->getHeight();		
	l_Dimensions.depth = gbutton->getLabel()->getDepth()+0.6f;		
	
	GLabel* l_pGLabel = constructGLabelFromDescriptor(keyIndex,VisualStateMap[keyState]);
	gbutton->setLabel(l_pGLabel);
	gbutton->getLabel()->setForegroundColor(l_lKeyDefinitions->at(keyIndex)->getForegroundColor(VisualStateMap[keyState]));
	gbutton->getLabel()->setBackgroundColor(l_lKeyDefinitions->at(keyIndex)->getBackgroundColor(VisualStateMap[keyState]));
	gbutton->getLabel()->setLabelScaleSize(l_lKeyDefinitions->at(keyIndex)->getScaleSize(VisualStateMap[keyState]));
	
	
 	if (keyState==FLASH)		
		gbutton->setState(GButton_Active);	
 	else if (keyState==NOFLASH)		
		gbutton->setState(GButton_Inactive);		
 	else if (keyState==CENTRAL_FEEDBACK_CORRECT)
	{
		gbutton->setState(GButton_Clicked);	
		gbutton->setDimParameters(l_Dimensions);			
	      //gbutton->generateGButtonGLDisplayLists();
      }
	else if (keyState==CENTRAL_FEEDBACK_WRONG)
	{
		gbutton->setState(GButton_Clicked);	
		gbutton->setDimParameters(l_Dimensions);
            //gbutton->generateGButtonGLDisplayLists();
	}
	else if (keyState==NONCENTRAL_FEEDBACK_CORRECT)
		gbutton->setState(GButton_Clicked);			
	else if (keyState==NONCENTRAL_FEEDBACK_WRONG)	
		gbutton->setState(GButton_Inactive);
	else if (keyState==TARGET)
		gbutton->setState(GButton_Focus);	
	else if (keyState==NONCENTRAL_FEEDBACK_WRONG_SELECTED)	
		gbutton->setState(GButton_WrongClick);	
}

GLabel* P300KeyboardHandler::constructGLabelFromDescriptor(uint32 keyIndex, VisualState state)
{
	P300KeyDescriptor* descriptor = m_pLayoutObject->getP300KeyboardLayout()->at(keyIndex);
	GLabel* l_pGLabel;
	//std::cout << "constructGLabelFromDescriptor " << descriptor->getLabel(state).c_str() << "\n";
	if (descriptor->isTextSymbol(state))
	{
		boost::shared_ptr<FTFont> l_ftglFont;
            FontID l_pFontId = {descriptor->getSource(state),descriptor->getScaleSize(state),keyIndex};
            std::map< FontID, boost::shared_ptr<FTFont> >::iterator l_pIterator;
            l_pIterator = m_mFontSourceMap.find(l_pFontId);
		if (l_pIterator!=m_mFontSourceMap.end())
		{
			//std::cout << "Font already exists, use count " << l_pIterator->second.use_count() << "\n";
		      l_pGLabel = new GSymbol(descriptor->getLabel(state).c_str(), l_pIterator->second, descriptor->getScaleSize(state));
		}
            else
            {
                  //std::cout << "New font created with source " << l_pFontId.source.toASCIIString() << " and scale size " << l_pFontId.size << " and key " << l_pFontId.keyIndex << "\n";
			l_ftglFont = boost::shared_ptr<FTFont>(new FTGLPolygonFont(descriptor->getSource(state).toASCIIString()));
                  l_ftglFont->UseDisplayList(false);
		      m_mFontSourceMap.insert(std::make_pair(l_pFontId,l_ftglFont));
                  //l_pIterator = m_mFontSourceMap.find(l_pFontId);
		      l_pGLabel = new GSymbol(descriptor->getLabel(state).c_str(), l_ftglFont, descriptor->getScaleSize(state));
		}
	}
	else
		l_pGLabel = new GPictureSymbol(descriptor->getSource(state), descriptor->getScaleSize(state));
	return l_pGLabel;
}

void P300KeyboardHandler::initializeColorMap(GColor startColor, GColor endColor, uint32 nSteps, float32 sigLevel)
{
	float32 l_fRedBegin = startColor.red; float32 l_fGreenBegin = startColor.green; float32 l_fBlueBegin = startColor.blue;
	float32 l_fRedEnd = endColor.red; float32 l_fGreenEnd = endColor.green; float32 l_fBlueEnd = endColor.blue;

	GColor l_oColorTriple;
	l_oColorTriple.red = l_fRedBegin;
	l_oColorTriple.green = l_fGreenBegin;
	l_oColorTriple.blue = l_fBlueBegin;	
	m_mColorMap.insert(std::pair<float64, GColor>(sigLevel, l_oColorTriple));	
	for (uint32 i=1; i<nSteps; i++)
	{
	    	l_oColorTriple.red = (float32)(l_fRedBegin - ((l_fRedBegin-l_fRedEnd)*i)/(float32)nSteps);
	    	l_oColorTriple.green = (float32)(l_fGreenBegin - ((l_fGreenBegin-l_fGreenEnd)*i)/(float32)nSteps);
		//std::cout << (l_fGreenBegin-l_fGreenEnd)*i << " " << ((l_fGreenBegin-l_fGreenEnd)*i)/(int)nSteps << " " << (l_fGreenBegin - ((l_fGreenBegin-l_fGreenEnd)*i)/(int)nSteps) << "\n";
	    	l_oColorTriple.blue = (float32)(l_fBlueBegin - ((l_fBlueBegin-l_fBlueEnd)*i)/(float32)nSteps);	
		//std::cout << "Color key = " << pow(logRate,-(startKey+i-1)) << ", color R: " << l_oColorTriple.color[0] << ", G: " << l_oColorTriple.color[1] << ", B:" << l_oColorTriple.color[2] <<"\n";
		m_mColorMap.insert(std::pair<float64, GColor>(sigLevel+i*(1.0-sigLevel)/nSteps, l_oColorTriple));
	}
}

GColor P300KeyboardHandler::getAssignedProbabilityColor(uint32 symbolIndex)
{
	GColor l_cProbabilityColor;
	float64 l_f64Probability = m_vSymbolProbabilties[symbolIndex];
	std::map<float64, GColor>::iterator l_oColorIterator = m_mColorMap.begin();
	while(l_f64Probability > (*l_oColorIterator).first)
		l_oColorIterator++;
	l_cProbabilityColor = (*l_oColorIterator).second;
	
	return l_cProbabilityColor;
}
		
