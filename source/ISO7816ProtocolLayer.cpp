#include "ISO7816ProtocolLayer.h"

ISO7816ProtocolLayer::ISO7816ProtocolLayer(ISO7816Analyzer* analyzer)
:   mAnalyzer(analyzer)
{

}
ISO7816ProtocolLayer::~ISO7816ProtocolLayer()
{
    
}
    
void ISO7816ProtocolLayer::initTransaction (void)
{

}

bool ISO7816ProtocolLayer::isTransactionComplete(void)
{
    return true;
}