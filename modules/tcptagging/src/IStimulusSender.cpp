
#include "CStimulusSender.h"

OV_API TCPTagging::IStimulusSender* TCPTagging::createStimulusSender(void)
{
	return new TCPTagging::CStimulusSender();
}

OV_API TCPTagging::IStimulusMultiSender* TCPTagging::createStimulusMultiSender(void)
{
	return new TCPTagging::CStimulusMultiSender();
}
