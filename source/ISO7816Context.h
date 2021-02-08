#ifndef ISO7816_CONTEXT_H
#define ISO7816_CONTEXT_H

#include <LogicPublicTypes.h>

#include "ISO7816Exception.h"
#include "ISO7816Defs.h"
#include "ISO7816Node.h"

typedef enum {
    S_ATR,
    S_PPS,
    S_T0,
    S_T1,
    S_NUMBER_OR_INVALID
}state_t;

class ISO7816Context
{
public:
	ISO7816Context();
	virtual ~ISO7816Context();

    void init(void);
    sender_t toggleSender(void);

    sender_t GetSender(void);

public: //vars
    iso_params_t    mISOParams;
    state_t         mState;

private: //vars
    sender_t        mCurrSender;
};

#endif //ISO7816_CONTEXT_H
