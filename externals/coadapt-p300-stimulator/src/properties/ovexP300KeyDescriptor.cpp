#include "ovexP300KeyDescriptor.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

using namespace std;

NoSuchEventException noSuchEventException;

void P300KeyDescriptor::addScaleSize(const VisualState event, const OpenViBE::float32& value) 
{ 
	eventMapScaleSize->insert(std::pair<VisualState, OpenViBE::float32>(event, value));
}
void P300KeyDescriptor::addForegroundColor(const VisualState event, const GColor& value) 
{ 
	eventMapForegroundColor->insert(std::pair<VisualState, GColor>(event, value));
}
void P300KeyDescriptor::addBackgroundColor(const VisualState event, const GColor& value) 
{ 
	eventMapBackgroundColor->insert(std::pair<VisualState, GColor>(event, value));
}
void P300KeyDescriptor::addSource(const VisualState event, const OpenViBE::CString& value) 
{ 
	eventMapSource->insert(std::pair<VisualState, OpenViBE::CString>(event, value));
}
void P300KeyDescriptor::addLabel(const VisualState event, const std::string& value) 
{ 
	//overwrites the value for a key that is already there
	if (eventMapLabel->find(event)!=eventMapLabel->end())
		eventMapLabel->erase(event);
	eventMapLabel->insert(std::pair<VisualState, std::string>(event, value));
}
void P300KeyDescriptor::setIfTextSymbol(const VisualState event, const OpenViBE::boolean value) 
{ 
	eventMapIsTextSymbol->insert(std::pair<VisualState, OpenViBE::boolean>(event, value));
}

const boolean P300KeyDescriptor::isActionEnabled(CString action)
{
	boolean l_bInList = false;
	for (uint32 i=0; i<actions->size(); i++)
	{
		if (actions->at(i)==action)
		{
			l_bInList = true; 
			break;
		}
	}
	return l_bInList;
}

const float32 P300KeyDescriptor::getScaleSize(const VisualState event) const 
{ 
	std::map<VisualState, OpenViBE::float32>::iterator iterator = eventMapScaleSize->find(event); 
	if (iterator!=eventMapScaleSize->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}
const GColor& P300KeyDescriptor::getForegroundColor(const VisualState event) const 
{ 
	std::map<VisualState, GColor>::iterator iterator = eventMapForegroundColor->find(event);
	if (iterator!=eventMapForegroundColor->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}

const GColor& P300KeyDescriptor::getBackgroundColor(const VisualState event) const 
{ 
	std::map<VisualState, GColor>::iterator iterator = eventMapBackgroundColor->find(event);
	if (iterator!=eventMapBackgroundColor->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}

const CString& P300KeyDescriptor::getSource(const VisualState event) const 
{ 
	std::map<VisualState, OpenViBE::CString>::iterator iterator = eventMapSource->find(event);
	if (iterator!=eventMapSource->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}

const std::string& P300KeyDescriptor::getLabel(const VisualState event) const 
{ 
	std::map<VisualState, std::string>::iterator iterator = eventMapLabel->find(event);
	if (iterator!=eventMapLabel->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}

const boolean P300KeyDescriptor::isTextSymbol(const VisualState event) const 
{ 
	std::map<VisualState, OpenViBE::boolean>::iterator iterator = eventMapIsTextSymbol->find(event);
	if (iterator!=eventMapIsTextSymbol->end())
		return iterator->second; 
	else
		throw noSuchEventException;
}