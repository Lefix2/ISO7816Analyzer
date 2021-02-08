#ifndef ISO7816_PROTOCOLATR_H
#define ISO7816_PROTOCOLATR_H

#include "ISO7816Context.h"
#include "ISO7816Node.h"
#include "ISO7816ProtocolLayer.h"

#define SC_Fd				372
#define SC_Dd				1

#define SC_PROTOCOL_T0		0x0
#define SC_PROTOCOL_T1		0x1
#define SC_PROTOCOL_T15		0xF
#define SC_EDC_LRC			0
#define SC_EDC_CRC			1
#define SC_DEFAULT_WT_ETU	9600
#define SC_DEFAULT_WT_MS	893

#define ATR_INTERFACE_A		0
#define ATR_INTERFACE_B		1
#define ATR_INTERFACE_C		2
#define ATR_INTERFACE_D		3

#define ATR_MAX_LENGTH		46
#define ATR_MAX_INTERFACE	4
#define ATR_MAX_PROTOCOL	7
#define ATR_MAX_HISTORICAL	15
#define ATR_DEFAULT_FMAX	5000000
#define ATR_DEFAULT_F		372
#define ATR_DEFAULT_D		1
#define ATR_DEFAULT_I		50
#define ATR_DEFAULT_N		0
#define ATR_DEFAULT_P		5
#define ATR_DEFAULT_WI		10
#define ATR_DEFAULT_DAD		0
#define ATR_DEFAULT_SAD		0
#define ATR_DEFAULT_EDC		SC_EDC_LRC
#define ATR_DEFAULT_IFS		32
#define ATR_DEFAULT_CWI		13
#define ATR_DEFAULT_BWI		4

#define PPS_MAX_LENGTH		6
#define PPSS_IDX			0
#define PPS0_IDX			1

#define TPDU_HEADER_SIZE	5
#define APDU_S_MAX_SIZE		(TPDU_HEADER_SIZE + 0xFF + 1)
#define APDU_E_MAX_SIZE		(TPDU_HEADER_SIZE + 0xFFFF + 2)

#define CLA_IDX				0
#define INS_IDX				1
#define P1_IDX				2
#define P2_IDX				3
#define P3_IDX				4

#define C1_IDX				0
#define C2_IDX				1
#define C3_IDX				2
#define C4_IDX				3
#define C5_IDX				4
#define C6_IDX				5
#define C7_IDX				6

#define LRC_SIZE			1
#define CRC_SIZE			2
#define T1_PROLOGUE_SIZE	3
#define T1_MAX_DATA_SIZE	0xFE
#define T1_MAX_BLOCK_SIZE	(T1_PROLOGUE_SIZE + T1_MAX_DATA_SIZE + CRC_SIZE)
#define NAD_IDX				0
#define PCB_IDX				1
#define LEN_IDX				2

#define PCB_I_BLOCK			0x00
#define PCB_I_MORE			0x40

#define PCB_R_BLOCK			0x80
#define PCB_R_ACK			0x00
#define PCB_R_EDC_ERROR		0x01
#define PCB_R_OTHER_ERROR	0x02

#define PCB_S_BLOCK			0xC0
#define PCB_S_RESYNC		0x00
#define PCB_S_IFS			0x01
#define PCB_S_ABORT			0x02
#define PCB_S_WTX			0x03
#define PCB_S_RESPONSE		0x20

#define INS_GET_RESPONSE	0xC0
#define INS_ENVELOPPE		0xC2

typedef enum {
    stateATR_TS,
    stateATR_T0,
    stateATR_TA,
    stateATR_TB,
    stateATR_TC,
    stateATR_TD,
    stateATR_Historical,
    stateATR_TCK,
    stateATR_finished,
    stateATR_count_or_invalid
}stateATR_t;

/* Interface Byte struct typedef */
typedef struct {
	bool	present;	/* Indicate IB presence */
	uint8_t	value;		/* IB value only if present */
}itfB_t;

/* ATR TypeDef */
typedef struct {

	uint8_t	TS;
	uint8_t	T0;

	uint8_t	nb_T;
	itfB_t	T[ATR_MAX_PROTOCOL][ATR_MAX_INTERFACE];

	uint8_t	nb_HB;
	uint8_t	HB[ATR_MAX_HISTORICAL];

	itfB_t	TCK;
}atr_t;

class ISO7816ProtocolATR : public ISO7816ProtocolLayer
{
public:
    ISO7816ProtocolATR(ISO7816Analyzer* analyzer);
    virtual ~ISO7816ProtocolATR();

    virtual void initTransaction (void);
    virtual bool isTransactionComplete(void);
    virtual void newData(ISO7816Node* node);

protected: // functions
    void nextTDState(void);

protected: //vars
    stateATR_t      mStateATR;
    atr_t           mATR;
    U8              mTDi;
    U8              mNb_T;
    ISO7816Node*    mNode;
};

#endif // ISO7816_PROTOCOLATR_H