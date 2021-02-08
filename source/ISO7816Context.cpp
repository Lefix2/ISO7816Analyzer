#include "ISO7816Context.h"

ISO7816Context::ISO7816Context()
{
    init();
}

ISO7816Context::~ISO7816Context()
{

}

void ISO7816Context::init(void)
{
    mConvention = CONV_DIR;
    mState = S_ATR;

    mF = 372;
    mD = 1;
}