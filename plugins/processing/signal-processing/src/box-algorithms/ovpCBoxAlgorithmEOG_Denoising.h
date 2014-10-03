
#if defined(TARGET_HAS_ThirdPartyEIGEN)

#ifndef __OpenViBEPlugins_BoxAlgorithm_EOG_Denoising_H__
#define __OpenViBEPlugins_BoxAlgorithm_EOG_Denoising_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <fstream>
#include <iostream>

// Verify Eigen Path
#include <Eigen/Dense>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OVP_ClassId_BoxAlgorithm_Test OpenViBE::CIdentifier(0xC223FF12, 0x069A987E)
#define OVP_ClassId_BoxAlgorithm_TestDesc OpenViBE::CIdentifier(0x4F9BE623, 0xF2027046)

namespace OpenViBEPlugins
{
namespace SignalProcessing
{
/**
         * \class CBoxAlgorithmEOG_Denoising
         * \author Joao-Pedro Berti-Ligabo / Inria
         * \date Tue May 20 15:33:22 2014
         * \brief The class CBoxAlgorithmEOG_Denoising describes the box Test.
         *
         */
class CBoxAlgorithmEOG_Denoising : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
{
public:
    virtual void release(void) { delete this; }

    virtual OpenViBE::boolean initialize(void);
    virtual OpenViBE::boolean uninitialize(void);

    //Here is the different process callbacks possible
    // - On clock ticks :
    //virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
    // - On new input received (the most common behaviour for signal processing) :
    virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
    // - On message received :
    //virtual OpenViBE::boolean processMessage(const OpenViBE::Kernel::IMessageWithData& msg, OpenViBE::uint32 inputIndex);

    // If you want to use processClock, you must provide the clock frequency.
    //virtual OpenViBE::uint64 getClockFrequency(void);

    virtual OpenViBE::boolean process(void);

    // As we do with any class in openvibe, we use the macro below
    // to associate this box to an unique identifier.
    // The inheritance information is also made available,
    // as we provide the superclass OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
    _IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_Test);

protected:
    // Codec algorithms specified in the skeleton-generator:
    // Signal stream decoder
    OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmEOG_Denoising > m_oAlgo0_SignalDecoder;
    OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmEOG_Denoising > m_oAlgo1_SignalDecoder;


    //    OpenViBE::Kernel::IAlgorithmProxy* m_pMatrixRegressionAlgorithm;
    //    OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pMatrixRegressionAlgorithm_Matrix0;
    //    OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pMatrixRegressionAlgorithm_Matrix1;

    //    OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pMatrixRegressionAlgorithm_Matrix;
    //    OpenViBE::Kernel::TParameterHandler < OpenViBE::CString > par_Filename;


    // Signal stream encoder
    OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmEOG_Denoising > m_oAlgo2_SignalEncoder;

    OpenViBE::CString m_stringFilename;
    std::ifstream m_fBMatrixFile;
    Eigen::MatrixXd l_oNoiseCoeff;




    OpenViBE::uint32 m_uint32NbChannels0;
    OpenViBE::uint32 m_uint32NbChannels1;

    OpenViBE::uint32 m_uint32NbSamples0;
    OpenViBE::uint32 m_uint32NbSamples1;

};


// If you need to implement a box Listener, here is a sekeleton for you.
// Use only the callbacks you need.
// For example, if your box has a variable number of input, but all of them must be stimulation inputs.
// The following listener callback will ensure that any newly added input is stimulations :
/*
        virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
        {
            rBox.setInputType(ui32Index, OV_TypeId_Stimulations);
        };
        */

/*
        // The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
        // Please uncomment below the callbacks you want to use.
        class CBoxAlgorithmEOG_DenoisingListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
        {
        public:

            //virtual OpenViBE::boolean onInitialized(OpenViBE::Kernel::IBox& rBox) { return true; };
            //virtual OpenViBE::boolean onNameChanged(OpenViBE::Kernel::IBox& rBox) { return true; };
            //virtual OpenViBE::boolean onInputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onInputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onInputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onOutputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingDefaultValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
            //virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };

            _IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
        };
        */

/**
         * \class CBoxAlgorithmEOG_DenoisingDesc
         * \author JP (Inria)
         * \date Tue May 20 15:33:22 2014
         * \brief Descriptor of the box Test.
         *
         */
class CBoxAlgorithmEOG_DenoisingDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
{
public:

    virtual void release(void) { }

    virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("EOG Denoising"); }
    virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Joao-Pedro Berti-Ligabo"); }
    virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
    virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("EOG Denoising using Regression Analysis"); }
    virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Algorithm implementation as suggested in Schlogl's article of 2007"); }
    virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Denoising"); }
    virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("023"); }
    virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gnome-fs-regular.png"); }

    virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_Test; }
    virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessing::CBoxAlgorithmEOG_Denoising; }

    /*
            virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmEOG_DenoisingListener; }
            virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
            */
    virtual OpenViBE::boolean getBoxPrototype(
            OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
    {

        rBoxAlgorithmPrototype.addInput("EEG",OV_TypeId_Signal);
        rBoxAlgorithmPrototype.addInput("EOG",OV_TypeId_Signal);

        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);

        rBoxAlgorithmPrototype.addOutput("EEG_Corrected",OV_TypeId_Signal);
        rBoxAlgorithmPrototype.addSetting("Filename b Matrix", OV_TypeId_Filename, "b-Matrix-EEG.txt");

        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);
        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);

        //No Message input specified.To add Message inputs use :
        //rBoxAlgorithmPrototype.addMessageInput("Input Name");

        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyMessageInput);
        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddMessageInput);

        //No Message output specified.To add Message outputs use :
        //rBoxAlgorithmPrototype.addMessageOutput("Output Name");

        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyMessageOutput);
        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddMessageOutput);

        //No setting specified.To add settings use :
        //rBoxAlgorithmPrototype.addSetting("Setting Name",OV_TypeId_XXXX,"default value");

        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
        //rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);

        rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);

        return true;
    }
    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_TestDesc);
};
};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_Test_H__

#endif