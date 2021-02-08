#include "ISO7816Defs.h"

#include <cstring>

/* nb_Tx[Yi] table, number of interface bytes for a given Y */
static const U8 nb_Tx_table[16] =
{
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/* fmax table, indicated fmax for given TA1[8-5] */
static const U32 fmax_table[16] =
{
	4000000, 5000000, 6000, 8000000, 12000000, 16000000, 20000000, 0, 0, 5000000, 7500000, 10000000, 15000000, 20000000, 0, 0
};

/* Fi table, indicated F for given TA1[8-5] */
static const U16 f_table[16] =
{
	372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0
};

/* Di table, indicated D for given TA1[4-1] */
static const U16 d_table[16] =
{
	0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0
};

static const U16 i_table[4] =
{
	25, 50, 100, 0
};

void atr_init(atr_t* atr)
{
	memset(atr, 0, sizeof(atr_t));
}

void pps_init(pps_t* pps)
{
	memset(pps, 0, sizeof(pps_t));
}

void iso_params_init (iso_params_t *params)
{
	params->state			= sc_state_power_off;

	params->frequency		= 4000000;
	params->convention		= convention_direct;
	params->supported_prot	= 0;
	params->default_protocol= 0;

	atr_init(&(params->ATR));

	params->F				= ATR_DEFAULT_F;
	params->D				= ATR_DEFAULT_D;
	params->N				= ATR_DEFAULT_N;
	params->WI				= ATR_DEFAULT_WI;

	params->Nd				= 0;
	params->Nc				= 0;
	params->DAD				= ATR_DEFAULT_DAD;
	params->SAD				= ATR_DEFAULT_SAD;
	params->WTX				= 0;
	params->IFSC			= ATR_DEFAULT_IFS;
	params->IFSD			= ATR_DEFAULT_IFS;
	params->BWI				= ATR_DEFAULT_BWI;
	params->CWI				= ATR_DEFAULT_CWI;
	params->EDC				= ATR_DEFAULT_EDC;
	params->SPU.present		= false;

}

U16 GetFn(U8 F)
{
	return f_table[F];
}

U16 GetDn(U8 D)
{
	return d_table[D];
}