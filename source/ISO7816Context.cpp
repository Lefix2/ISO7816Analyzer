#include "ISO7816Context.h"

#include <iostream>

ISO7816Context::ISO7816Context()
{
    init();
}

ISO7816Context::~ISO7816Context()
{

}

void ISO7816Context::init(void)
{
    iso_params_init(&mISOParams);
    mState      = S_ATR;
    mCurrSender = sender_card;
}

sender_t ISO7816Context::toggleSender(void)
{
    if(mCurrSender == sender_card)
        mCurrSender = sender_reader;
    else
        mCurrSender = sender_card;

    return mCurrSender;
}

sender_t ISO7816Context::GetSender(void)
{
    return mCurrSender;
}