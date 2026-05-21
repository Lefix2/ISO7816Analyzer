#ifndef ISO7816_NODE_H
#define ISO7816_NODE_H


#include <Analyzer.h>
#include <vector>
#include <string>

typedef enum
{
    nodeLevel_char,
    nodeLevel_tpdu,
    nodeLevel_apdu,
    nodeLevel_pps,
    nodeLevel_atr,
    nodeLevel_count_or_invalid,
} nodeLevel_t;

typedef enum
{
    sender_card,
    sender_reader,
    sender_undefined,
    sender_count_or_invalid
} sender_t;

/*
 * ISO7816 Node abstract class definition
 */
class ISO7816Node
{
  public:
    ISO7816Node( nodeLevel_t nodeLevel, sender_t sender, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816Node();

  public:
    void SetNodeId( U64 nodeId );
    U64 GetNodeId( void );
    void SetStartSample( S64 startSample );
    S64 GetStartSample( void );
    void SetEndSample( S64 endSample );
    S64 GetEndSample( void );
    sender_t GetSender( void );
    void AddDescription( const char* str );

    void AddChildNode( ISO7816Node* child );
    ISO7816Node* GetNodeAt( S64 index );
    ISO7816Node* GetFirstNode( void );
    ISO7816Node* GetLastNode( void );
    size_t GetChildCount( void ) const;
    virtual nodeLevel_t GetLevel( void );
    virtual void GetDataStr( char* resultString, U32 maxStrLen ) = 0;
    virtual void GetShortStr( char* buf, U32 maxLen );
#ifdef LOGIC2
    virtual const char* GetFrameV2Type( void );
#endif

  protected: // vars
    const nodeLevel_t mNodeLevel;
    sender_t mSender;
    S64 mStartSample;
    S64 mEndSample;
    U64 mNodeId;
    std::string mDescription;
    std::vector<ISO7816Node*> mChilds;
};

#ifndef LOGIC2
/*
 * ISO7816 Node for APDU class definition
 */
class ISO7816NodeAPDU : public ISO7816Node
{
  public:
    ISO7816NodeAPDU( S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816NodeAPDU();

  public:
    void GetDataStr( char* resultString, U32 maxStrLen ) override;
    void GetShortStr( char* buf, U32 maxLen ) override;
};


/*
 * ISO7816 Node for TPDU class definition
 */
class ISO7816NodeTPDU : public ISO7816Node
{
  public:
    ISO7816NodeTPDU( S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816NodeTPDU();

  public:
    void GetDataStr( char* resultString, U32 maxStrLen ) override;
    void GetShortStr( char* buf, U32 maxLen ) override;
};
#endif // !LOGIC2


/*
 * ISO7816 Node for PPS class definition
 */
class ISO7816NodePPS : public ISO7816Node
{
  public:
    ISO7816NodePPS( sender_t sender, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816NodePPS();

  public:
    void GetDataStr( char* resultString, U32 maxStrLen ) override;
    void GetShortStr( char* buf, U32 maxLen ) override;
};


/*
 * ISO7816 Node for ATR class definition
 */
class ISO7816NodeATR : public ISO7816Node
{
  public:
    ISO7816NodeATR( S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816NodeATR();

  public:
    void GetDataStr( char* resultString, U32 maxStrLen ) override;
    void GetShortStr( char* buf, U32 maxLen ) override;
};


/*
 * ISO7816 Node for character class definition
 */
class ISO7816NodeChar : public ISO7816Node
{
  public:
    ISO7816NodeChar( sender_t sender, U8 charVal, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0 );
    virtual ~ISO7816NodeChar();

  public:
    void GetDataStr( char* resultString, U32 maxStrLen ) override;
    void GetShortStr( char* buf, U32 maxLen ) override;
    void GetMedStr( char* buf, U32 maxLen, DisplayBase displayBase );   // label + value
    void GetLongStr( char* buf, U32 maxLen, DisplayBase displayBase );  // label + value + params

  public:
    U8 mCharVal;
};

#endif // ISO7816_NODE_H
