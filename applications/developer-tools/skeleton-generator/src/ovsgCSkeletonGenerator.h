#ifndef __OpenViBESkeletonGenerator_CSkeletonGenerator_H__
#define __OpenViBESkeletonGenerator_CSkeletonGenerator_H__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <openvibe/kernel/ovIKernelObject.h>

//#include <configuration/ovkCConfigurationManager.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <map>

namespace OpenViBESkeletonGenerator
{
	class CSkeletonGenerator
	{
	public:

		CSkeletonGenerator(OpenViBE::Kernel::IKernelContext & rKernelContext, ::GtkBuilder * pBuilderInterface);
		virtual ~CSkeletonGenerator(void);

	protected:
		OpenViBE::Kernel::IKernelContext &m_rKernelContext;
		
		::GtkBuilder * m_pBuilderInterface;

		OpenViBE::CString             m_sAuthor;
		OpenViBE::CString             m_sCompany;
		OpenViBE::CString             m_sTargetDirectory;

		virtual bool initialize(void)=0;
		
		OpenViBE::CString m_sConfigurationFile; // basic application config file
		bool m_bConfigurationFileLoaded;

		void getCommonParameters(void);
		bool saveCommonParameters(OpenViBE::CString sFileName);
		bool loadCommonParameters(OpenViBE::CString sFileName);

		bool cleanConfigurationFile(OpenViBE::CString sFileName);
		
		// returns a sed-compliant expression to be parsed in a substitution command
		OpenViBE::CString ensureSedCompliancy(OpenViBE::CString sExpression); 
		// executes a regex replace and builds a new file, by replacing the matching expressions by the substitute. If no destination file is provided, the template file is modified.
		// Note that the input must be a valid sed format regex pattern, the function does not check.
		bool regexReplace(const OpenViBE::CString& sTemplateFile, const OpenViBE::CString& sExpression, const OpenViBE::CString& sSubstitute, const OpenViBE::CString& sDestinationFile = OpenViBE::CString(""));

		// get the formatted string date
		OpenViBE::CString getDate();
		
		// generate a new file, giving a template file, a destination file, and a map ofsubstitutions (Tag,Substitute)
		// return false if an error occurred.
		bool generate(OpenViBE::CString sTemplateFile, OpenViBE::CString sDestinationFile, std::map<OpenViBE::CString,OpenViBE::CString> mSubstitutions, OpenViBE::CString& rLog);

		virtual void getCurrentParameters(void) = 0;
		virtual bool save(OpenViBE::CString sFileName) = 0;
		virtual bool load(OpenViBE::CString sFileName) = 0;
		
		virtual OpenViBE::Kernel::ILogManager& getLogManager(void) const { return m_rKernelContext.getLogManager(); }
		virtual OpenViBE::Kernel::IErrorManager& getErrorManager(void) const { return m_rKernelContext.getErrorManager(); }
	};

}

#endif //__OpenViBESkeletonGenerator_CSkeletonGenerator_H__
