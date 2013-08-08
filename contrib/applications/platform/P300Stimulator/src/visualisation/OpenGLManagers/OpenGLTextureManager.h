#ifndef __OpenGLTextureManager_OV_H__
#define __OpenGLTextureManager_OV_H__

#include "OpenGLResourceManager.h"
//#include <set>

namespace OpenViBEApplications
{
	class OpenGLTextureManager : public OpenGLResourceManager {
	public:
		OpenGLTextureManager(OpenViBE::CString sourceFile);
		OpenGLTextureManager(OpenGLTextureManager* gl_manager) : OpenGLResourceManager(gl_manager)
		{ 
			//m_bSourceExists = gl_manager.m_bSourceExists; 
		}
		virtual ~OpenGLTextureManager();
		virtual OpenGLTextureManager* clone() const { return new OpenGLTextureManager((OpenGLTextureManager*)this); }
		
		//virtual void createResources();
		virtual void deleteResources();
		OpenViBE::boolean sourceExists() { return m_bSourceExists; }
		
	protected:
		virtual std::map<GLuint, OpenViBE::uint32>& getResourceMap();
		virtual void _createResources();
		virtual void _deleteResource(GLuint* resource_id);		
		//static void forwarder(OpenGLResourceManager* context);
		
	protected:
		static std::map<GLuint, OpenViBE::uint32> m_mIdCountMap;
		static std::map<OpenViBE::CString, GLuint> m_mSourceFile2IdMap;
		static std::map<GLuint, OpenViBE::CString> m_mId2SourceFileMap;
		OpenViBE::CString m_sSourceFile;
		OpenViBE::boolean m_bSourceExists;
	};

};

#endif