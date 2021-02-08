#ifndef ISO7816_PROTOCOLLAYER_H
#define ISO7816_PROTOCOLLAYER_H

#include "ISO7816Analyzer.h"
#include "ISO7816Context.h"
#include "ISO7816Node.h"

class ISO7816ProtocolLayer
{
public:
	ISO7816ProtocolLayer(ISO7816Analyzer* analyzer);
	virtual ~ISO7816ProtocolLayer();

    virtual void initTransaction (void);
    virtual bool isTransactionComplete(void);
    virtual void newData(ISO7816Node* node) = 0;

protected: // functions

protected: //vars
    ISO7816Analyzer* mAnalyzer;
};

#endif // ISO7816_PROTOCOLLAYER_H