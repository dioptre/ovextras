#ifndef __OpenViBE_Kernel_Player_IMyMessage_H__
#define __OpenViBE_Kernel_Player_IMyMessage_H__

#include "../ovIKernelObject.h"

namespace OpenViBE
{
	namespace Kernel
	{
		/**
		 * \class IMyMessage
		 * \author Loic Mahe (INRIA)
		 * \date 2013-10-02
		 * \brief The message class
		 * \ingroup Group_Player
		 * \ingroup Group_Kernel
		 *
		 * Message that are exchanged between boxes. A message can hold four types of parameters, uint64, float64, CString and CMatrix.
		 * Each type gets a map.
		 *
		 */
		class OV_API IMyMessage : public OpenViBE::Kernel::IKernelObject
		{
		public:
			//@}
			/** \name Getters */
			//@{
			// Returned references are invalid after processMessage().
			/**
			 * \brief Gets the integer value stored under this key
			 * \param key : a reference to the name of the key
			 * \param success : a boolean which gives the status of the function, true if the retrieval is successful, false otherwise
			 * \return \e the value in case of success
			 * \return \e 0 in case of error
			 */
				virtual OpenViBE::uint64 getValueUint64(const OpenViBE::CString &key, bool &success) const=0;
			/**
			 * \brief Gets the float value stored under this key
			 * \param key : a reference to the name of the key
			 * \param success : a boolean which gives the status of the function, true if the retrieval is successful, false otherwise
			 * \return \e the value in case of success
			 * \return \e 0 in case of error
			 */
				virtual OpenViBE::float64 getValueFloat64(const CString &key, bool &success) const=0;
			/**
			 * \brief Gets a pointer to the CString value stored under this key
			 * \param key : a reference to the name of the key
			 * \param success : a boolean which gives the status of the function, true if the retrieval is successful, false otherwise
			 * \return \e a pointer to the value in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::CString* getValueCString(const CString &key, bool &success) const=0;
			/**
			 * \brief Gets a pointer to the CMatrix value stored under this key
			 * \param key : a reference to the name of the key
			 * \param success : a boolean which gives the status of the function, true if the retrieval is successful, false otherwise
			 * \return \e a pointer to the value in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::IMatrix* getValueCMatrix(const CString &key, bool &success) const=0;
			//@}
			/** \name Setters */
			//@{
			/**
			 * \brief Sets the message internal UInt64 value stored under this key
			 * \param key : the name of the key
			 * \param valueIn : the integer value to put in the message
			 * \return \e true in case of success
			 * \return \e false in case of error
			 */
				virtual bool setValueUint64(CString key, uint64 valueIn)=0;
			/**
			 * \brief Sets the message internal Float64 value stored under this key
			 * \param key : the name of the key
			 * \param valueIn : the integer value to put in the message
			 * \return \e true in case of success
			 * \return \e false in case of error
			 */
				virtual bool setValueFloat64(CString key, float64 valueIn)=0;
			/**
			 * \brief Sets the message internal CString value stored under this key
			 * \param key : the name of the key
			 * \param valueIn : the integer value to put in the message
			 * \return \e true in case of success
			 * \return \e false in case of error
			 */
				virtual bool setValueCString(CString key, const CString &valueIn)=0;
			/**
			 * \brief Sets the message internal CMatrix value stored under this key
			 * \param key : the name of the key
			 * \param valueIn : the integer value to put in the message
			 * \return \e true in case of success
			 * \return \e false in case of error
			 */
				virtual bool setValueCMatrix(CString key, const CMatrix &valueIn)=0;
			//@}
			/** \name Keys getters */
			//@{
			/**
			 * \brief Get the first key of the CString map
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::CString* getFirstCStringToken() const=0;
			/**
			 * \brief Get the first key of the UInt64 map
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::CString* getFirstUInt64Token() const=0;
			/**
			 * \brief Get the first key of the Float64 map
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::CString* getFirstFloat64Token() const=0;
			/**
			 * \brief Get the first key of the CMatrix map
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error
			 */
				virtual const OpenViBE::CString* getFirstIMatrixToken() const=0;
			/**
			 * \brief Get the next key of the CString map
			 * \param previousToken : a reference to the previous key
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error or if we reached the end of the map
			 */
				virtual const OpenViBE::CString* getNextCStringToken(const OpenViBE::CString &previousToken) const=0;
			/**
			 * \brief Get the next key of the UInt64 map
			 * \param previousToken : a reference to the previous key
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error or if we reached the end of the map
			 */
				virtual const OpenViBE::CString* getNextUInt64Token(const OpenViBE::CString &previousToken) const=0;
			/**
			 * \brief Get the next key of the Float64 map
			 * \param previousToken : a reference to the previous key
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error or if we reached the end of the map
			 */
				virtual const OpenViBE::CString* getNextFloat64Token(const OpenViBE::CString &previousToken) const=0;
			/**
			 * \brief Get the next key of the CMatrix map
			 * \param previousToken : a reference to the previous key
			 * \return \e a pointer to the key in case of success
			 * \return \e NULL in case of error or if we reached the end of the map
			 */
				virtual const OpenViBE::CString* getNextIMatrixToken(const OpenViBE::CString &previousToken) const=0;
			//@}

			_IsDerivedFromClass_(OpenViBE::Kernel::IKernelObject, OV_ClassId_Kernel_Player_MyMessage)

		};
	};
};

#endif // __OpenViBE_Kernel_Player_IMyMessage_H__
