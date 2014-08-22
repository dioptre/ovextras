#ifndef OVIOBSERVER_H
#define OVIOBSERVER_H

#include "ovCObservable.h"
#include "ov_base.h"

namespace OpenViBE
{
	class CObservable;

	class IObserver
	{
	public:
		virtual void update(CObservable &o, void* data) = 0;

	};
}

#endif // OVIOBSERVER_H
