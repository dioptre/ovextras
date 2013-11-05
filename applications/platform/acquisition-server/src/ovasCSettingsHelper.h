#ifndef __OpenViBE_AcquisitionServer_CSettingsHelper_H__
#define __OpenViBE_AcquisitionServer_CSettingsHelper_H__

#include <ovas_base.h>
// #include "ovasIDriver.h"
// #include <openvibe/ovCIdentifier.h>

#include <sstream>
#include <map>
#include <iostream>

namespace OpenViBEAcquisitionServer
{
	/**
	  * \class Property
	  * \author Jussi T. Lindgren (Inria)
	  * \date 2013-11
	  * \brief Base class for properties. A property is essentially a <name,value> pair. 
	  *
	  * \note This class is intended for typed inheritance. It carries no data.
	  */
	class Property
	{
	public:
		Property(const OpenViBE::CString& name) 
			: m_name(name)
		{ }
		virtual ~Property() {}

		const OpenViBE::CString getName(void) const { return m_name; }

		virtual std::ostream& toStream(std::ostream& out) const = 0;
		virtual std::istream& fromStream(std::istream& in) = 0;

	private:
		OpenViBE::CString m_name;

	};

	// Helper operators to map base class << and >> to those of the derived classes. These can not be members.
	inline std::ostream& operator<< (std::ostream& out, const OpenViBEAcquisitionServer::Property& obj) {
		return obj.toStream(out);
	}

	inline std::istream& operator>> (std::istream& in, OpenViBEAcquisitionServer::Property& obj) {
		return obj.fromStream(in);
	}

	/*
	 * \class TypedProperty
	 * \author Jussi T. Lindgren (Inria)
	 * \date 2013-11
	 * \brief A property with a typed data pointer.
	 *
	 * \note The class does NOT own the data pointer, but may allow modification of its contents via replaceData().
	 */
	template<typename T> class TypedProperty : public Property
	{
	public:
		TypedProperty (const OpenViBE::CString& name, T* data)
			: Property(name), m_data(data) { };

		// Access data
		const T* getData(void) const { return m_data; };

		// Overwrites existing data with new contents.
		void replaceData(T& data) { *m_data = data; };
	
		virtual std::ostream& toStream(std::ostream& out) const { out << *m_data; return out; } ;
		virtual std::istream& fromStream(std::istream& in) { in >> *m_data; return in; } ;
	private:
		T* m_data;
	};

	// Operators used to convert between typical variables (as used in properties) and streams

	inline std::ostream& operator<< (std::ostream& out, const OpenViBE::CString& var)
	{
		out << std::string(var.toASCIIString());

		return out;
	}

	inline std::istream& operator>> (std::istream& in, OpenViBE::CString& var)
	{
		std::string tmp;

		std::getline(in, tmp);

		var.set(tmp.c_str());

		// std::cout << "Parsed [" << var.toASCIIString() << "]\n";
		return in;
	}


#if 0
	inline std::ostream& operator<< (std::ostream& out, const OpenViBEAcquisitionServer::CHeader& var)
	{
		out << var.getChannelCount(); out << " ";
		out << var.getSubjectAge(); out << " ";

		return out;
	}

	inline std::istream& operator>> (std::istream& in, OpenViBEAcquisitionServer::CHeader& var)
	{
		OpenViBE::uint32 tmp;

		in >> tmp; var.setChannelCount(tmp);
		in >> tmp; var.setSubjectAge(tmp);

		return in;
	}
#endif

	inline std::ostream& operator<< (std::ostream& out, const std::map<OpenViBE::uint32, OpenViBE::uint32>& var)
	{
		std::map<OpenViBE::uint32, OpenViBE::uint32>::const_iterator it = var.begin();
		for(;it!=var.end();++it) {
			out << it->first;
			out << " ";
			out << it->second;
			out << " ";
		}

		return out;
	}

	inline std::istream& operator>> (std::istream& in, std::map<OpenViBE::uint32, OpenViBE::uint32>& var)
	{
		var.clear();
		OpenViBE::uint32 key;
		OpenViBE::uint32 value;
		while( in >> key ) {
			in >> value;
			var[key] = value; 
		}

		return in;
	}

	/*
	 * \class SettingsHelper
	 * \author Jussi T. Lindgren (Inria)
	 * \date 2013-11
	 * \brief Convenience helper class that eases the saving and loading of variables (properties) to/from the configuration manager
	 *
	 * \note For registering exotic types, the user must provide the << and >> overloads to/from streams
	 */
	class SettingsHelper {
	public:
		SettingsHelper(const char *prefix, OpenViBE::Kernel::IConfigurationManager& rMgr) 
			: m_sPrefix(prefix), m_rMgr(rMgr) { } ; 
		~SettingsHelper() {
			std::map<OpenViBE::CString, Property*>::const_iterator it = m_vProperties.begin();
			for(;it!=m_vProperties.end();++it) {
				delete(it->second);
			}
			m_vProperties.clear();
		}

		// Register or replace a variable
		template<typename T> OpenViBE::boolean add(const OpenViBE::CString& name, T* var) {

			if(!var) 
			{
				// cout << "Tried to add a NULL pointer\n";
				return false;
			}

			// If key is in map, replace
			std::map<OpenViBE::CString, Property*>::const_iterator it = m_vProperties.find(name);
			if(it!=m_vProperties.end()) {
				// m_rContext.getLogManager() << LogLevel_Trace << "Replacing key [" << name << "]\n";
				delete it->second;
			}
			
			TypedProperty<T> *myProperty = new TypedProperty<T>(name, var);
			m_vProperties[myProperty->getName()] = myProperty;

			return true;
		}

		// Save all registered variables to the configuration manager
		void save(void);

		// Load all registered variables from the configuration manager
		void load(void);

		// Get access to the registered variables
		const std::map<OpenViBE::CString, Property*>& getAllProperties(void) const { return m_vProperties; }
	
	private:
		OpenViBE::CString m_sPrefix;
		OpenViBE::Kernel::IConfigurationManager& m_rMgr;

		std::map<OpenViBE::CString, Property*> m_vProperties;
	};

};


#endif

