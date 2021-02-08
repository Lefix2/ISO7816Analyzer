#ifndef ISO7816_CONTEXT_H
#define ISO7816_CONTEXT_H

#include <LogicPublicTypes.h>

#include "ISO7816Exception.h"

typedef enum {
    CONV_DIR,
    CONV_INV,
    CONV_NUMBER_OR_INVALID
}convention_t;

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

public: //vars
    convention_t    mConvention;
    state_t         mState;

    U64             mF;
    U64             mD;
};

#endif //ISO7816_CONTEXT_H
