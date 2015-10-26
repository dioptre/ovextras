
// @short Essentially has a buffer per calling thread and then on EOL, does lock();flush buffer();unlock()
// 
// @long Operating philosophy: Maintains a log buffer per thread and flushes it to all the 
// listeners after the thread writes an end of line. As the different listeners
// want to color things in different ways, we buffer the log write operations using
// a vector of boost::any, instead of buffering, for example, a raw log line itself.
// An additional complication of this is that we have to cast the data back runtime 
// from boost::any to the types supported by the listeners.
//
// Known issues: 
// - If the scheduler uses constantly new thread ids, this manager will slowly
// grow in memory size as the buffers are indexed by thread ids and the array
// is never truncated (the API doesn't currently allow logmanager to know about threads).
// - This log manager might not be the most efficient thing, as on each get
// of the buffer it needs to take a lock to the buffer array. However, writing a log line
// should rather be an exception than a rule in normal openvibe number crunching.
//
// @author J.T. Lindgren
// 
#include "ovkCLogManagerMulticore.h"

// #include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

// Explicit type definitions for enums to carry in boost::any
typedef ELogLevel t_ELogLevel;
typedef ELogColor t_ELogColor;

CLogManagerMulticore::CLogManagerMulticore(const IKernelContext& rKernelContext)
	:TKernelObject<ILogManager>(rKernelContext)
{
}

// @note this call is not thread safe
OpenViBE::boolean CLogManagerMulticore::isActive(ELogLevel eLogLevel)
{
	map<ELogLevel, OpenViBE::boolean>::iterator itLogLevel=m_vActiveLevel.find(eLogLevel);
	if(itLogLevel==m_vActiveLevel.end())
	{
		return true;
	}
	return itLogLevel->second;
}

// @note this call is not thread safe
OpenViBE::boolean CLogManagerMulticore::activate(ELogLevel eLogLevel, OpenViBE::boolean bActive)
{
	m_vActiveLevel[eLogLevel]=bActive;
	return true;
}

// @note this call is not thread safe
OpenViBE::boolean CLogManagerMulticore::activate(ELogLevel eStartLogLevel, ELogLevel eEndLogLevel, OpenViBE::boolean bActive)
{
	for(int i=eStartLogLevel; i<=eEndLogLevel; i++)
	{
		m_vActiveLevel[ELogLevel(i)]=bActive;
	}
	return true;
}

// @note this call is not thread safe
OpenViBE::boolean CLogManagerMulticore::activate(OpenViBE::boolean bActive)
{
	return activate(LogLevel_First, LogLevel_Last, bActive);
}

void CLogManagerMulticore::log(const time64 time64Value)
{
	pushToBuffer(time64Value);
}

void CLogManagerMulticore::log(const uint64 ui64Value)
{
	pushToBuffer(ui64Value);
}

void CLogManagerMulticore::log(const uint32 ui32Value)
{
	pushToBuffer(ui32Value);
}

void CLogManagerMulticore::log(const uint16 ui16Value)
{
	pushToBuffer(ui16Value);
}

void CLogManagerMulticore::log(const uint8 ui8Value)
{
	pushToBuffer(ui8Value);
}

void CLogManagerMulticore::log(const int64 i64Value)
{
	pushToBuffer(i64Value);
}

void CLogManagerMulticore::log(const int32 i32Value)
{
	pushToBuffer(i32Value);
}

void CLogManagerMulticore::log(const int16 i16Value)
{
	pushToBuffer(i16Value);
}

void CLogManagerMulticore::log(const int8 i8Value)
{
	pushToBuffer(i8Value);
}

void CLogManagerMulticore::log(const float64 f64Value)
{
	pushToBuffer(f64Value);
}

void CLogManagerMulticore::log(const float32 f32Value)
{
	pushToBuffer(f32Value);
}

void CLogManagerMulticore::log(const OpenViBE::boolean bValue)
{
	pushToBuffer(bValue);
}

void CLogManagerMulticore::log(const CIdentifier& rValue)
{
	pushToBuffer(rValue);
}

void CLogManagerMulticore::log(const CString& rValue)
{
	pushToBuffer(rValue);

	std::string tmp(rValue.toASCIIString());
	if(tmp.find("\n")!=string::npos) 
	{
		flushBuffer();
	}
}

void CLogManagerMulticore::log(const char* rValue)
{
	std::string l_sCopy(rValue);

	pushToBuffer(l_sCopy);
	
	if(l_sCopy.find("\n")!=string::npos) 
	{
		flushBuffer();
	}
}

void CLogManagerMulticore::log(const ELogLevel eLogLevel)
{
	LogBuffer& l_oBuffer = getBuffer();
	l_oBuffer.m_eCurrentLogLevel = eLogLevel;

	pushToBuffer(static_cast<t_ELogLevel>(eLogLevel));
}

void CLogManagerMulticore::log(const ELogColor eLogColor)
{
	pushToBuffer(static_cast<t_ELogColor>(eLogColor));
}

OpenViBE::boolean CLogManagerMulticore::addListener(ILogListener* pListener)
{
	if(pListener==NULL)
	{
		return false;
	}

	vector<ILogListener*>::iterator itLogListener=m_vListener.begin();
	while(itLogListener!=m_vListener.end())
	{
		if((*itLogListener)==pListener)
		{
			return false;
		}
		itLogListener++;
	}

	m_vListener.push_back(pListener);
	return true;
}

OpenViBE::boolean CLogManagerMulticore::removeListener(ILogListener* pListener)
{
	vector<ILogListener*>::iterator itLogListener=m_vListener.begin();
	while(itLogListener!=m_vListener.end())
	{
		if((*itLogListener)==pListener)
		{
			m_vListener.erase(itLogListener);
			return true;	// due to constraint in addListener(), pListener can be in the array only once, so we can return
		}
		itLogListener++;
	}
	return false;
}

CLogManagerMulticore::LogBuffer& CLogManagerMulticore::getBuffer() 
{
	const boost::thread::id l_oThreadId = boost::this_thread::get_id();

	{
		boost::mutex::scoped_lock lock(m_oBufferLock);
	
		// get buffer structure for this thread, if not found, make a new one
		if(m_vLogBuffer.find(l_oThreadId)==m_vLogBuffer.end()) 
		{	
			m_vLogBuffer[l_oThreadId].m_vBuffer = std::vector<boost::any>();
			m_vLogBuffer[l_oThreadId].m_eCurrentLogLevel = LogLevel_Info;
		}

		return m_vLogBuffer[l_oThreadId];
	}
}


OpenViBE::boolean CLogManagerMulticore::flushBuffer()
{
	LogBuffer& l_oBuffer = getBuffer();

	{
		// We need to hold the lock so we can spool the whole line to the listener without interference
		boost::mutex::scoped_lock lock(m_oOutputLock);

		std::vector<OpenViBE::Kernel::ILogListener*>::iterator i;
		for(i=m_vListener.begin(); i!=m_vListener.end(); i++)
		{
			if((*i)->isActive(l_oBuffer.m_eCurrentLogLevel))
			{	
				for(uint32 j=0;j<l_oBuffer.m_vBuffer.size();j++)
				{
					boost::any& l_oValue = l_oBuffer.m_vBuffer[j];
					if(l_oValue.type()==typeid(t_ELogLevel))
					{
						(*i)->log(boost::any_cast<ELogLevel>(l_oValue));
					}
					else if(l_oValue.type()==typeid(t_ELogColor))
					{
						(*i)->log(boost::any_cast<ELogColor>(l_oValue));
					}
					else if(l_oValue.type()==typeid(time64))
					{
						(*i)->log(boost::any_cast<time64>(l_oValue));
					}
					else if(l_oValue.type()==typeid(CString))
					{
						(*i)->log(boost::any_cast<CString>(l_oValue));
					}
					else if(l_oValue.type()==typeid(std::string))
					{
						// handle separately from cstring since some listeners want to put a different color
						const std::string tmp = boost::any_cast<std::string>(l_oValue);
						const char *ptr = tmp.c_str();
						(*i)->log(ptr);
					}
					else if(l_oValue.type()==typeid(CIdentifier))
					{
						(*i)->log(boost::any_cast<CIdentifier>(l_oValue));
					}
					else if(l_oValue.type()==typeid(float64))
					{
						(*i)->log(boost::any_cast<float64>(l_oValue));
					}
					else if(l_oValue.type()==typeid(float32))
					{
						(*i)->log(boost::any_cast<float32>(l_oValue));
					}
					else if(l_oValue.type()==typeid(uint64))
					{
						(*i)->log(boost::any_cast<uint64>(l_oValue));
					}
					else if(l_oValue.type()==typeid(uint32))
					{
						(*i)->log(boost::any_cast<uint32>(l_oValue));
					}
					else if(l_oValue.type()==typeid(uint16))
					{
						(*i)->log(boost::any_cast<uint16>(l_oValue));
					}
					else if(l_oValue.type()==typeid(uint8))
					{
						(*i)->log(boost::any_cast<uint8>(l_oValue));
					}
					else if(l_oValue.type()==typeid(int64))
					{
						(*i)->log(boost::any_cast<int64>(l_oValue));
					}
					else if(l_oValue.type()==typeid(int32))
					{
						(*i)->log(boost::any_cast<int32>(l_oValue));
					}
					else if(l_oValue.type()==typeid(int16))
					{
						(*i)->log(boost::any_cast<int16>(l_oValue));
					}
					else if(l_oValue.type()==typeid(int8))
					{
						(*i)->log(boost::any_cast<int8>(l_oValue));
					}
					else if(l_oValue.type()==typeid(OpenViBE::boolean))
					{
						(*i)->log(boost::any_cast<OpenViBE::boolean>(l_oValue));
					}
					else 
					{
						(*i)->log(CString("Error: Unknown type ") + CString(l_oValue.type().name()) + CString("\n"));
					}
				}
			}
		}
	}

	l_oBuffer.m_vBuffer.clear();

	return true;
}
