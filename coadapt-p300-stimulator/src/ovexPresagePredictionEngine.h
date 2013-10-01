#ifdef TARGET_HAS_ThirdPartyPresage

#ifndef __PresagePredictionEngine_H__
#define __PresagePredictionEngine_H__

#include "ovexWordPredictionInterface.h"
#include "presage.h"

namespace OpenViBEApplications
{	
	#ifdef TARGET_OS_Windows
	static const char * get_past_stream(void* arg)
	{
		return (char*)arg;
	}
	static const char * get_future_stream(void* arg)
	{
		return (char*)arg;
	}
	#endif

	#ifdef TARGET_OS_Linux
	class MyPresageCallback : public PresageCallback
	{
	public:
		MyPresageCallback(const std::string& _past_context) : past_context(_past_context) { }

		std::string get_past_stream() const { return past_context; }
		std::string get_future_stream() const { return empty; }

	private:
		const std::string& past_context;
		const std::string empty;

	};	
	#endif
	
	class PresagePredictionEngine : public WordPredictionInterface
	{	
	public:
		/*#ifdef TARGET_OS_Windows
		static const char* get_past_stream(void* arg)
		{
			return (char*) arg;
		}

		static const char* get_future_stream(void* arg)
		{
			return (char*) arg;
		}
		#endif	*/	
		
		PresagePredictionEngine(OpenViBE::CString nGramDatabaseName);
		
		~PresagePredictionEngine();
		
		virtual std::vector<std::string>* getMostProbableWords(const std::string& prefix, OpenViBE::uint32 nWords);
		
	protected:
		std::vector<std::string>* m_lPredictedWords;
		
		#ifdef TARGET_OS_Linux
		std::string m_sSpellerContext;
		MyPresageCallback* m_pPresageCallback;
		Presage* m_pPresageEngine;	
		#endif
		#ifdef TARGET_OS_Windows
		presage_t m_pPresageEngine;
		char * m_sSpellerContext;
		#endif
	};
};
#endif
#endif