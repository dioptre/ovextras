#include "ovIObserver.h"
#include "ovCObservable.h"


#include "ov_base.h"
#include <iostream>

using namespace OpenViBE;

CObservable::CObservable(void): m_bHasChanged(false)
{

}

void CObservable::addObserver(IObserver *o)
{
	m_vObservers.push_back(o);
}

void CObservable::deleteObserver(IObserver *o)
{
	for(std::vector<IObserver *>::iterator it = m_vObservers.begin(); it != m_vObservers.end() ; ++it)
	{
		if((*it) == o){
			std::cout << "here" << std::endl;
			m_vObservers.erase(it);
			//We only suppress the first occurence, no need to continue
			return;
		}
	}
}

void CObservable::setChanged()
{
	m_bHasChanged = true;
}

void CObservable::clearChanged()
{
	m_bHasChanged = false;
}

boolean CObservable::hasChanged()
{
	return m_bHasChanged;
}

void CObservable::notifyObservers(void* data)
{
	if(m_bHasChanged)
	{
		for(std::vector<IObserver *>::iterator it = m_vObservers.begin(); it != m_vObservers.end() ; ++it)
		{
			((IObserver * )*it)->update(*this, data);
		}
		m_bHasChanged = false;
	}
}
