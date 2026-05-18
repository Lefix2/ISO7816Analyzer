#include "ISO7816SimulationDataGenerator.h"
#include "ISO7816AnalyzerSettings.h"

#include <AnalyzerHelpers.h>

static const U8 msb2lsb[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static const bool parity[256] =
{
#   define P2(n) n, n^1, n^1, n
#   define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#   define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
    P6(0), P6(1), P6(1), P6(0)
};

ISO7816SimulationDataGenerator::ISO7816SimulationDataGenerator()
    : mSettings(nullptr), mSimulationSampleRateHz(0),
      mEtu(372.0), mInverseConvention(false),
      mSimulationDataVCC(nullptr), mSimulationDataRST(nullptr),
      mSimulationDataCLK(nullptr), mSimulationDataIO(nullptr)
{
}

ISO7816SimulationDataGenerator::~ISO7816SimulationDataGenerator()
{
}

void ISO7816SimulationDataGenerator::Initialize( U32 simulation_sample_rate, ISO7816AnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mSimulationDataVCC = mSimulationChannelsISO7816.Add( mSettings->mChannelVCC,
	                                                     simulation_sample_rate, BIT_LOW );
	mSimulationDataRST = mSimulationChannelsISO7816.Add( mSettings->mChannelRST,
	                                                     simulation_sample_rate, BIT_LOW );
	mSimulationDataCLK = mSimulationChannelsISO7816.Add( mSettings->mChannelCLK,
	                                                     simulation_sample_rate, BIT_LOW );
	mSimulationDataIO  = mSimulationChannelsISO7816.Add( mSettings->mChannelIO,
	                                                     simulation_sample_rate, BIT_LOW );
}

U32 ISO7816SimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );
	(void)adjusted;

	mEtu = 372.0;
	mInverseConvention = false;
	mCLKGenerator.Init( 4000000, sample_rate );

	// All lines start low before first power-on
	mSimulationDataVCC->TransitionIfNeeded( BIT_LOW );
	mSimulationDataRST->TransitionIfNeeded( BIT_LOW );
	mSimulationDataCLK->TransitionIfNeeded( BIT_LOW );
	mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	mSimulationChannelsISO7816.AdvanceAll( 20000 );

	GenerateSessionT0DirectPPS();
	GenerateSessionT0MinimalNoPPS();
	GenerateSessionT1DirectPPS();
	GenerateSessionT0Inverse();
	GenerateSessionErrors();

	*simulation_channel = mSimulationChannelsISO7816.GetArray();
	return mSimulationChannelsISO7816.GetCount();
}

// ----------------------------------------------------------------------------
// Direct convention: LSB first, 0->LOW, 1->HIGH, even parity
// ----------------------------------------------------------------------------
void ISO7816SimulationDataGenerator::GenerateCharDirect( U8 value, bool force_parity_err )
{
	mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	GenerateEtu(1);

	for ( U8 i = 0; i < 8; i++ )
	{
		mSimulationDataIO->TransitionIfNeeded( (value & (0x01 << i)) ? BIT_HIGH : BIT_LOW );
		GenerateEtu(1);
	}

	bool par = parity[value];
	if ( force_parity_err ) par = !par;
	mSimulationDataIO->TransitionIfNeeded( par ? BIT_HIGH : BIT_LOW );
	GenerateEtu(1);

	mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu(2);
}

// ----------------------------------------------------------------------------
// Inverse convention: MSB first, 0->HIGH, 1->LOW, odd parity (same formula)
// The parity formula is identical because odd parity on inverted bits == even parity
// on original bits, so parity[value]?HIGH:LOW is correct for both conventions.
// ----------------------------------------------------------------------------
void ISO7816SimulationDataGenerator::GenerateCharInverse( U8 value, bool force_parity_err )
{
	mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	GenerateEtu(1);

	for ( int i = 7; i >= 0; i-- )
	{
		mSimulationDataIO->TransitionIfNeeded( (value & (1 << i)) ? BIT_LOW : BIT_HIGH );
		GenerateEtu(1);
	}

	bool par = parity[value];
	if ( force_parity_err ) par = !par;
	mSimulationDataIO->TransitionIfNeeded( par ? BIT_HIGH : BIT_LOW );
	GenerateEtu(1);

	mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu(2);
}

void ISO7816SimulationDataGenerator::GenerateCharAuto( U8 value, bool force_parity_err )
{
	if ( mInverseConvention )
		GenerateCharInverse( value, force_parity_err );
	else
		GenerateCharDirect( value, force_parity_err );
}

void ISO7816SimulationDataGenerator::GenerateEtu( U8 value )
{
	for ( U64 i = 0; i < (U64)(mEtu * value); i++ )
	{
		mSimulationDataCLK->TransitionIfNeeded( BIT_LOW );
		mSimulationChannelsISO7816.AdvanceAll( mCLKGenerator.AdvanceByHalfPeriod(0.5) );
		mSimulationDataCLK->TransitionIfNeeded( BIT_HIGH );
		mSimulationChannelsISO7816.AdvanceAll( mCLKGenerator.AdvanceByHalfPeriod(0.5) );
	}
}

// ----------------------------------------------------------------------------
// Power cycle helpers
// ----------------------------------------------------------------------------
void ISO7816SimulationDataGenerator::PowerOn()
{
	mSimulationDataVCC->TransitionIfNeeded( BIT_HIGH );
	mSimulationChannelsISO7816.AdvanceAll( 400 );
	mSimulationDataCLK->TransitionIfNeeded( BIT_HIGH );
	mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	mSimulationChannelsISO7816.AdvanceAll( 200 );
	GenerateEtu( 100 );  // >= 400 CLK before RST rise
	mSimulationDataRST->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu( 20 );   // guard before ATR
}

void ISO7816SimulationDataGenerator::PowerOff()
{
	GenerateEtu( 200 );
	mSimulationDataRST->TransitionIfNeeded( BIT_LOW );
	GenerateEtu( 10 );
	mSimulationDataCLK->TransitionIfNeeded( BIT_LOW );
	mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	mSimulationChannelsISO7816.AdvanceAll( 500 );
	mSimulationDataVCC->TransitionIfNeeded( BIT_LOW );
	mSimulationChannelsISO7816.AdvanceAll( 10000 );
}

// ============================================================================
// Session 1: T=0 direct convention, PPS Fi=512/Di=8 (ETU=64), 6 APDUs
//
// ATR: 3B 90 94 00
//   TS=3B (direct), T0=90 (TA1+TD1 present, K=0),
//   TA1=94 (Fi=512 idx9, Di=8 idx4), TD1=00 (T=0, no more iface bytes)
//   No historical bytes, no TCK (T=0 only)
//
// PPS: FF 10 94 7B  (T=0, PPS1=94 -> F=9/D=4, PCK=FF^10^94=7B)
// ============================================================================
void ISO7816SimulationDataGenerator::GenerateSessionT0DirectPPS()
{
	mEtu = 372.0;
	mInverseConvention = false;

	PowerOn();

	// ATR
	static const U8 atr[] = { 0x3B, 0x90, 0x94, 0x00 };
	for ( U8 i = 0; i < sizeof(atr); i++ ) GenerateCharDirect( atr[i] );
	GenerateEtu( 150 );

	// PPS request (reader -> card): FF 10 94 7B
	static const U8 pps[] = { 0xFF, 0x10, 0x94, 0x7B };
	for ( U8 i = 0; i < sizeof(pps); i++ ) GenerateCharDirect( pps[i] );
	GenerateEtu( 80 );

	// PPS response (card -> reader): echo same
	for ( U8 i = 0; i < sizeof(pps); i++ ) GenerateCharDirect( pps[i] );
	GenerateEtu( 200 );

	// ETU changes to F/D = 512/8 = 64 after PPS
	mEtu = 64.0;

	// --- APDU 1: SELECT AID (case 4 short: reader Lc=5, card SW=61 17) ---
	// Reader: 00 A4 04 00 05 | Card: ACK=A4 | Reader: A0 00 00 00 03 | Card: 61 17
	{
		static const U8 hdr[]  = { 0x00, 0xA4, 0x04, 0x00, 0x05 };
		static const U8 data[] = { 0xA0, 0x00, 0x00, 0x00, 0x03 };
		for ( U8 i = 0; i < sizeof(hdr);  i++ ) GenerateCharDirect( hdr[i]  );
		GenerateEtu( 5 );
		GenerateCharDirect( 0xA4 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharDirect( data[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x61 );  // SW1: 23 bytes available
		GenerateCharDirect( 0x17 );  // SW2
	}
	GenerateEtu( 10 );

	// --- APDU 2: GET RESPONSE Le=23 (case 2: card sends FCI) ---
	// Reader: 00 C0 00 00 17 | Card: ACK=C0, 23 bytes FCI, 90 00
	{
		static const U8 hdr[] = { 0x00, 0xC0, 0x00, 0x00, 0x17 };
		// 23-byte FCI (realistic TLV structure)
		static const U8 fci[] = {
			0x6F, 0x15, 0x84, 0x08, 0xA0, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
			0xA5, 0x09, 0x88, 0x01, 0x05, 0x5F, 0x2D, 0x02, 0x65, 0x6E, 0x00
		};
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0xC0 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(fci); i++ ) GenerateCharDirect( fci[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- APDU 3: VERIFY PIN (case 3: reader sends 8 PIN bytes) ---
	// Reader: 00 20 00 80 08 | Card: ACK=20 | Reader: 8 PIN bytes | Card: 90 00
	{
		static const U8 hdr[] = { 0x00, 0x20, 0x00, 0x80, 0x08 };
		static const U8 pin[] = { 0x24, 0x12, 0x34, 0x56, 0xFF, 0xFF, 0xFF, 0xFF };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x20 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(pin); i++ ) GenerateCharDirect( pin[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- APDU 4: READ BINARY with 2 NULL bytes (case 2: Le=4) ---
	// Reader: 00 B0 00 00 04 | Card: NULL NULL ACK=B0, 4 data bytes, 90 00
	{
		static const U8 hdr[]  = { 0x00, 0xB0, 0x00, 0x00, 0x04 };
		static const U8 data[] = { 0x11, 0x22, 0x33, 0x44 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0xB0 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharDirect( data[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- APDU 5: SELECT file not found (immediate error SW) ---
	// Reader: 00 A4 04 00 06 | Card: 6A 82 (no ACK, direct SW)
	{
		static const U8 hdr[] = { 0x00, 0xA4, 0x04, 0x00, 0x06 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x6A );  // SW1: wrong params
		GenerateCharDirect( 0x82 );  // SW2: file not found
	}
	GenerateEtu( 10 );

	// --- APDU 6: UPDATE BINARY (case 3: reader sends 4 bytes) ---
	// Reader: 00 D6 00 04 04 | Card: ACK=D6 | Reader: 4 data bytes | Card: 90 00
	{
		static const U8 hdr[]  = { 0x00, 0xD6, 0x00, 0x04, 0x04 };
		static const U8 data[] = { 0xAA, 0xBB, 0xCC, 0xDD };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0xD6 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharDirect( data[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}

	PowerOff();
}

// ============================================================================
// Session 2: T=0 direct convention, minimal ATR (3B 00), no PPS, default ETU=372
//
// ATR: 3B 00  (TS=3B direct, T0=00: no iface bytes, no historical, T=0, no TCK)
// No PPS exchange
// ============================================================================
void ISO7816SimulationDataGenerator::GenerateSessionT0MinimalNoPPS()
{
	mEtu = 372.0;
	mInverseConvention = false;

	PowerOn();

	// ATR: 2 bytes
	GenerateCharDirect( 0x3B );
	GenerateCharDirect( 0x00 );
	GenerateEtu( 200 );

	// No PPS: first non-0xFF byte goes straight to T=0

	// --- APDU 1: INTERNAL AUTHENTICATE challenge (case 3) -> SW=61 08 ---
	// Reader: 00 88 00 00 08 | Card: ACK=88 | Reader: 8-byte challenge | Card: 61 08
	{
		static const U8 hdr[]       = { 0x00, 0x88, 0x00, 0x00, 0x08 };
		static const U8 challenge[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
		for ( U8 i = 0; i < sizeof(hdr);       i++ ) GenerateCharDirect( hdr[i]       );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x88 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(challenge); i++ ) GenerateCharDirect( challenge[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x61 );  // SW1: 8 bytes available
		GenerateCharDirect( 0x08 );  // SW2
	}
	GenerateEtu( 10 );

	// --- APDU 2: GET RESPONSE Le=8 (case 2) ---
	// Reader: 00 C0 00 00 08 | Card: ACK=C0, 8 response bytes, 90 00
	{
		static const U8 hdr[]  = { 0x00, 0xC0, 0x00, 0x00, 0x08 };
		static const U8 resp[] = { 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18 };
		for ( U8 i = 0; i < sizeof(hdr);  i++ ) GenerateCharDirect( hdr[i]  );
		GenerateEtu( 5 );
		GenerateCharDirect( 0xC0 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(resp); i++ ) GenerateCharDirect( resp[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- APDU 3: SELECT wrong params (error 6A 80) ---
	// Reader: 00 A4 04 00 02 | Card: 6A 80 (no ACK)
	{
		static const U8 hdr[] = { 0x00, 0xA4, 0x04, 0x00, 0x02 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x6A );  // SW1: wrong params
		GenerateCharDirect( 0x80 );  // SW2
	}

	PowerOff();
}

// ============================================================================
// Session 3: T=1 direct convention, PPS to T=1
//
// ATR: 3B 80 81 31 FE 45 8B
//   TS=3B, T0=80 (TD1 present, K=0), TD1=81 (TD2 present, T=1),
//   TD2=31 (TA3+TB3 present, T=1), TA3=FE (IFSC=254), TB3=45 (BWI=4,CWI=5),
//   TCK=80^81^31^FE^45=8B
//
// PPS: FF 01 FE  (T=1, no F/D change, PCK=FF^01=FE)
//
// T=1 block suite (LRC = XOR of NAD..last_data):
//   S(IFS) req  card: 00 C1 01 FE 3E
//   S(IFS) resp reader:00 E1 01 FE 1E
//   I(0,0) reader SELECT: 00 00 0A [00 A4 04 00 05 A0 00 00 00 03] 0C
//   R(1) card ACK: 00 90 00 90
//   I(0,0) card resp: 00 00 02 90 00 92
//   R(1) reader ACK: 00 90 00 90
//   S(WTX) req card: 00 C3 01 01 C3
//   S(WTX) resp reader: 00 E3 01 01 E3
//   I(1,M=1) reader chain blk1: 00 60 08 [00 A4 04 00 08 01 02 03] C0
//   R(0) card ACK: 00 80 00 80
//   I(0,0) reader chain blk2: 00 00 05 [04 05 06 07 08] 0D
//   R(1) card ACK: 00 90 00 90
//   I(1,0) card final resp: 00 40 02 90 00 D2
//   R(0) reader ACK: 00 80 00 80
//   S(RESYNC) req reader: 00 C0 00 C0
//   S(RESYNC) resp card: 00 E0 00 E0
// ============================================================================
void ISO7816SimulationDataGenerator::GenerateSessionT1DirectPPS()
{
	mEtu = 372.0;
	mInverseConvention = false;

	PowerOn();

	// ATR: 3B 80 81 31 FE 45 8B (7 bytes)
	static const U8 atr[] = { 0x3B, 0x80, 0x81, 0x31, 0xFE, 0x45, 0x8B };
	for ( U8 i = 0; i < sizeof(atr); i++ ) GenerateCharDirect( atr[i] );
	GenerateEtu( 150 );

	// PPS request (reader): FF 01 FE  (T=1, no PPS1, PCK=FF^01=FE)
	static const U8 pps[] = { 0xFF, 0x01, 0xFE };
	for ( U8 i = 0; i < sizeof(pps); i++ ) GenerateCharDirect( pps[i] );
	GenerateEtu( 80 );

	// PPS response (card): echo
	for ( U8 i = 0; i < sizeof(pps); i++ ) GenerateCharDirect( pps[i] );
	GenerateEtu( 200 );

	// ETU stays 372 (no F/D negotiation in PPS)

	// --- T=1 block helper: just emit bytes with inter-block gap ---
	// S(IFS) request from card: NAD=00 PCB=C1 LEN=01 IFS=FE LRC=3E
	{
		static const U8 blk[] = { 0x00, 0xC1, 0x01, 0xFE, 0x3E };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// S(IFS) response from reader: 00 E1 01 FE 1E
	{
		static const U8 blk[] = { 0x00, 0xE1, 0x01, 0xFE, 0x1E };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// I(N(S)=0, M=0) from reader: SELECT AID
	// NAD=00 PCB=00 LEN=0A [00 A4 04 00 05 A0 00 00 00 03] LRC=0C
	{
		static const U8 blk[] = {
			0x00, 0x00, 0x0A,
			0x00, 0xA4, 0x04, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x03,
			0x0C
		};
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// R(N(R)=1) from card ACK: 00 90 00 90
	{
		static const U8 blk[] = { 0x00, 0x90, 0x00, 0x90 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// I(N(S)=0, M=0) from card: response 90 00
	// NAD=00 PCB=00 LEN=02 [90 00] LRC=92
	{
		static const U8 blk[] = { 0x00, 0x00, 0x02, 0x90, 0x00, 0x92 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// R(N(R)=1) from reader ACK: 00 90 00 90
	{
		static const U8 blk[] = { 0x00, 0x90, 0x00, 0x90 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// S(WTX) request from card: NAD=00 PCB=C3 LEN=01 WTX=01 LRC=C3
	{
		static const U8 blk[] = { 0x00, 0xC3, 0x01, 0x01, 0xC3 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// S(WTX) response from reader: 00 E3 01 01 E3
	{
		static const U8 blk[] = { 0x00, 0xE3, 0x01, 0x01, 0xE3 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// Chained I-blocks from reader (2-block chain carrying 13-byte APDU)
	// Block 1: I(N(S)=1, M=1) PCB=0x60, LEN=08, data=[00 A4 04 00 08 01 02 03], LRC=C0
	{
		static const U8 blk[] = {
			0x00, 0x60, 0x08,
			0x00, 0xA4, 0x04, 0x00, 0x08, 0x01, 0x02, 0x03,
			0xC0
		};
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// R(N(R)=0) from card (received N(S)=1, expecting N(S)=0): 00 80 00 80
	{
		static const U8 blk[] = { 0x00, 0x80, 0x00, 0x80 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// Block 2: I(N(S)=0, M=0) PCB=0x00, LEN=05, data=[04 05 06 07 08], LRC=0D
	{
		static const U8 blk[] = {
			0x00, 0x00, 0x05,
			0x04, 0x05, 0x06, 0x07, 0x08,
			0x0D
		};
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// R(N(R)=1) from card: 00 90 00 90
	{
		static const U8 blk[] = { 0x00, 0x90, 0x00, 0x90 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// I(N(S)=1, M=0) from card final response: PCB=0x40, LEN=02, [90 00], LRC=D2
	{
		static const U8 blk[] = { 0x00, 0x40, 0x02, 0x90, 0x00, 0xD2 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// R(N(R)=0) from reader: 00 80 00 80
	{
		static const U8 blk[] = { 0x00, 0x80, 0x00, 0x80 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// S(RESYNC) request from reader (error recovery): 00 C0 00 C0
	{
		static const U8 blk[] = { 0x00, 0xC0, 0x00, 0xC0 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}
	GenerateEtu( 20 );

	// S(RESYNC) response from card: 00 E0 00 E0
	{
		static const U8 blk[] = { 0x00, 0xE0, 0x00, 0xE0 };
		for ( U8 i = 0; i < sizeof(blk); i++ ) GenerateCharDirect( blk[i] );
	}

	PowerOff();
}

// ============================================================================
// Session 4: T=0 inverse convention
//
// ATR: 3F 00  (logical values, encoded in inverse convention)
//   TS=3F (inverse): analyzer decodes as 0x03 -> sets inverse, stores 0x3F
//   T0=00: no iface bytes, no historical, T=0 default, no TCK
//
// All subsequent bytes also encoded in inverse convention.
// No PPS.
// ============================================================================
void ISO7816SimulationDataGenerator::GenerateSessionT0Inverse()
{
	mEtu = 372.0;
	mInverseConvention = true;  // ALL bytes in this session use inverse encoding

	PowerOn();

	// ATR (inverse encoding)
	// TS=0x3F: generated inverse -> analyzer reads as 0x03 -> sets inverse convention
	// T0=0x00: generated inverse -> analyzer reads as 0x00
	GenerateCharInverse( 0x3F );  // TS
	GenerateCharInverse( 0x00 );  // T0
	GenerateEtu( 200 );

	// No PPS: 0x00 != 0xFF so parser goes to T=0

	// --- APDU 1: READ BINARY Le=4 (case 2) ---
	// Reader: 00 B0 00 00 04 | Card: ACK=B0, 4 bytes, 90 00
	{
		static const U8 hdr[]  = { 0x00, 0xB0, 0x00, 0x00, 0x04 };
		static const U8 data[] = { 0x01, 0x02, 0x03, 0x04 };
		for ( U8 i = 0; i < sizeof(hdr);  i++ ) GenerateCharInverse( hdr[i]  );
		GenerateEtu( 5 );
		GenerateCharInverse( 0xB0 );  // ACK
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharInverse( data[i] );
		GenerateEtu( 5 );
		GenerateCharInverse( 0x90 );
		GenerateCharInverse( 0x00 );
	}
	GenerateEtu( 10 );

	// --- APDU 2: UPDATE BINARY Lc=2 (case 3) ---
	// Reader: 00 D6 00 00 02 | Card: ACK=D6 | Reader: 12 34 | Card: 90 00
	{
		static const U8 hdr[]  = { 0x00, 0xD6, 0x00, 0x00, 0x02 };
		static const U8 data[] = { 0x12, 0x34 };
		for ( U8 i = 0; i < sizeof(hdr);  i++ ) GenerateCharInverse( hdr[i]  );
		GenerateEtu( 5 );
		GenerateCharInverse( 0xD6 );  // ACK
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharInverse( data[i] );
		GenerateEtu( 5 );
		GenerateCharInverse( 0x90 );
		GenerateCharInverse( 0x00 );
	}

	mInverseConvention = false;  // reset before PowerOff
	PowerOff();
}

// ============================================================================
// Session 5: Error cases (direct convention, minimal ATR 3B 00, no PPS)
//
// Error 1: Parity error on middle data byte of UPDATE BINARY
// Error 2: 5 NULL bytes before ACK on READ BINARY
// Error 3: Bad procedure byte (0x33) after SELECT header -> analyzer prints
//          exception, parser stays in procedure state, then 6A 86 recovery
// Error 4: Abort via RST during partial header (2 bytes)
//          Followed by warm reset and a clean recovery session
// ============================================================================
void ISO7816SimulationDataGenerator::GenerateSessionErrors()
{
	mEtu = 372.0;
	mInverseConvention = false;

	PowerOn();

	// ATR
	GenerateCharDirect( 0x3B );
	GenerateCharDirect( 0x00 );
	GenerateEtu( 200 );

	// --- Error 1: Parity error on byte 2 of UPDATE BINARY data ---
	// Analyzer marks the bad byte with ErrorX but continues decoding
	{
		static const U8 hdr[]  = { 0x00, 0xD6, 0x00, 0x00, 0x03 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0xD6 );              // ACK
		GenerateCharDirect( 0xAA );              // data byte 1: correct parity
		GenerateCharDirect( 0xBB, true );        // data byte 2: WRONG PARITY -> ErrorX marker
		GenerateCharDirect( 0xCC );              // data byte 3: correct parity
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- Error 2: 5 NULL bytes before READ BINARY ACK ---
	{
		static const U8 hdr[]  = { 0x00, 0xB0, 0x00, 0x00, 0x03 };
		static const U8 data[] = { 0xA1, 0xB2, 0xC3 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0x60 );  // NULL
		GenerateCharDirect( 0xB0 );  // ACK
		for ( U8 i = 0; i < sizeof(data); i++ ) GenerateCharDirect( data[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}
	GenerateEtu( 10 );

	// --- Error 3: Bad procedure byte after SELECT header ---
	// Parser throws ISO7816ExceptionProtocol("Bad procedure byte") for 0x33,
	// stays in stateTPDUT0_procedure. Next byte 6A is recognized as SW1.
	{
		static const U8 hdr[] = { 0x00, 0xA4, 0x04, 0x00, 0x05 };
		for ( U8 i = 0; i < sizeof(hdr); i++ ) GenerateCharDirect( hdr[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x33 );  // BAD procedure byte -> exception, parser stays in proc state
		GenerateEtu( 5 );
		GenerateCharDirect( 0x6A );  // SW1: recovery (0x6A & 0xF0 == 0x60 -> recognized as SW1)
		GenerateCharDirect( 0x86 );  // SW2: command not allowed
	}
	GenerateEtu( 10 );

	// --- Error 4: Abort via RST mid-header, then warm reset + recovery ---
	// Partial header: only CLA=00 and INS=A4 sent before RST goes low.
	// WorkerThread detects RST change before next IO edge -> breaks inner loop.
	// RST pulse -> new session: fresh ATR + one clean command.
	GenerateCharDirect( 0x00 );  // CLA only
	GenerateCharDirect( 0xA4 );  // INS only (P1 never sent)
	GenerateEtu( 5 );            // guard: IO stays HIGH, RST will transition here

	// RST goes LOW (abort) while IO is idle HIGH
	mSimulationDataRST->TransitionIfNeeded( BIT_LOW );
	GenerateEtu( 50 );           // RST low duration, CLK keeps running

	// Warm reset: RST goes HIGH, new session starts
	mSimulationDataRST->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu( 20 );           // guard before new ATR

	// --- Recovery sub-session after warm reset ---
	// New ATR (direct, minimal)
	GenerateCharDirect( 0x3B );
	GenerateCharDirect( 0x00 );
	GenerateEtu( 150 );

	// One clean GET CHALLENGE command (case 2, Le=8)
	// Reader: 00 84 00 00 08 | Card: ACK=84, 8 random bytes, 90 00
	{
		static const U8 hdr[]  = { 0x00, 0x84, 0x00, 0x00, 0x08 };
		static const U8 resp[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
		for ( U8 i = 0; i < sizeof(hdr);  i++ ) GenerateCharDirect( hdr[i]  );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x84 );  // ACK = INS
		for ( U8 i = 0; i < sizeof(resp); i++ ) GenerateCharDirect( resp[i] );
		GenerateEtu( 5 );
		GenerateCharDirect( 0x90 );
		GenerateCharDirect( 0x00 );
	}

	PowerOff();
}
