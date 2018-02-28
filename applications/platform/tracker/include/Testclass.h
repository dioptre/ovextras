
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <openvibe/ov_all.h>

class TestClass {
public:

	TestClass(OpenViBE::Kernel::IKernelContext& ctx);

	OpenViBE::Kernel::IKernelContext& m_ctx;

};

