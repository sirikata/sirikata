namespace CBR {
namespace decodabet {
typedef unsigned char uint8;
static const uint8 _URL_SAFE_ALPHABET []= {
        (uint8)'A', (uint8)'B', (uint8)'C', (uint8)'D', (uint8)'E', (uint8)'F', (uint8)'G',
        (uint8)'H', (uint8)'I', (uint8)'J', (uint8)'K', (uint8)'L', (uint8)'M', (uint8)'N',
        (uint8)'O', (uint8)'P', (uint8)'Q', (uint8)'R', (uint8)'S', (uint8)'T', (uint8)'U', 
        (uint8)'V', (uint8)'W', (uint8)'X', (uint8)'Y', (uint8)'Z',
        (uint8)'a', (uint8)'b', (uint8)'c', (uint8)'d', (uint8)'e', (uint8)'f', (uint8)'g',
        (uint8)'h', (uint8)'i', (uint8)'j', (uint8)'k', (uint8)'l', (uint8)'m', (uint8)'n',
        (uint8)'o', (uint8)'p', (uint8)'q', (uint8)'r', (uint8)'s', (uint8)'t', (uint8)'u', 
        (uint8)'v', (uint8)'w', (uint8)'x', (uint8)'y', (uint8)'z',
        (uint8)'0', (uint8)'1', (uint8)'2', (uint8)'3', (uint8)'4', (uint8)'5', 
        (uint8)'6', (uint8)'7', (uint8)'8', (uint8)'9', (uint8)'-', (uint8)'_'
    };
	
static signed char WHITE_SPACE_ENC = -5; // Indicates white space in encoding
static signed char EQUALS_SIGN_ENC = -1; // Indicates equals sign in encoding

static signed char URLSAFEDECODABET [] = {
        -9,-9,-9,-9,-9,-9,-9,-9,-9,                 // Decimal  0 -  8
        -5,-5,                                      // Whitespace: Tab and Linefeed
        -9,-9,                                      // Decimal 11 - 12
        -5,                                         // Whitespace: Carriage Return
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 14 - 26
        -9,-9,-9,-9,-9,                             // Decimal 27 - 31
        -5,                                         // Whitespace: Space
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,              // Decimal 33 - 42
        62,                                         // Plus sign at decimal 43
        -9,62,-9,                                   // Decimal 44 - 46
        63,                                         // Slash at decimal 47
        52,53,54,55,56,57,58,59,60,61,              // Numbers zero through nine
        -9,-9,-9,                                   // Decimal 58 - 60
        -1,                                         // Equals sign at decimal 61
        -9,-9,-9,                                      // Decimal 62 - 64
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,            // Letters 'A' through 'N'
        14,15,16,17,18,19,20,21,22,23,24,25,        // Letters 'O' through 'Z'
        -9,-9,-9,-9,63,-9,                          // Decimal 91 - 94 underscore 95
        26,27,28,29,30,31,32,33,34,35,36,37,38,     // Letters 'a' through 'm'
        39,40,41,42,43,44,45,46,47,48,49,50,51,     // Letters 'n' through 'z'
        -9,-9,-9,-9,-9                                 // Decimal 123 - 127
};
}
static int decode4to3(const unsigned char xsource[4],unsigned char destination[4], int destOffset) {
    signed char source[4];
    source[0]=decodabet::URLSAFEDECODABET[xsource[0]];
    source[1]=decodabet::URLSAFEDECODABET[xsource[1]];
    source[2]=decodabet::URLSAFEDECODABET[xsource[2]];
    source[3]=decodabet::URLSAFEDECODABET[xsource[3]];
    unsigned int outBuf=((source[0]<<18)|(source[1]<<12));
    destination[destOffset]=(unsigned char)(outBuf/65536);
    if (source[2]==decodabet::EQUALS_SIGN_ENC) {
        return 1;
    }
    outBuf|=(source[2]<<6);
    destination[destOffset+1]=(unsigned char)((outBuf/256)&255);                
    if (source[3]==decodabet::EQUALS_SIGN_ENC) {
            return 2;
    }
    outBuf|=source[3];
    destination[destOffset+2]=(unsigned char)(outBuf&255);                        
    return 3;
}

void decode12Base64(unsigned char output[9], unsigned char input[12]) {
    decode4to3(input,output,0);    
    decode4to3(input+4,output+3,0);    
    decode4to3(input+8,output+6,0);
}
int translateBase64(unsigned char*destination, const unsigned char* source, int numSigBytes) {
    unsigned char temp[4];
    unsigned int source0=source[0];
    unsigned int source1=source[1];
    unsigned int source2=source[2];
    unsigned int inBuff =   ( numSigBytes > 0 ? ((source0 << 24) / 256) : 0 )
                     | ( numSigBytes > 1 ? ((source1 << 24) / 65536) : 0 )
                     | ( numSigBytes > 2 ? ((source2 << 24) / 65536/ 256) : 0 );

    destination[ 0 ] = decodabet::_URL_SAFE_ALPHABET[ (inBuff >> 18)        ];
    destination[ 1 ] = decodabet::_URL_SAFE_ALPHABET[ (inBuff >> 12) & 0x3f ];

    switch( numSigBytes )
    {
      case 3:
        destination[ 2 ] = decodabet::_URL_SAFE_ALPHABET[ (inBuff >>  6) & 0x3f ];
        destination[ 3 ] = decodabet::_URL_SAFE_ALPHABET[ (inBuff      ) & 0x3f ];
        return 4;
      case 2:
        destination[ 2 ] = decodabet::_URL_SAFE_ALPHABET[ (inBuff >>  6) & 0x3f ];
        destination[ 3 ] = '=';
        return 4;
        
      case 1:
        destination[ 2 ] = '=';
        destination[ 3 ] = '=';
        return 4;

      default:
        return 0;
    }   // end switch
    
}
void encode9Base64(unsigned char input[9], unsigned char result[12]) {
    
    int offset1=translateBase64(result,input,3);
    translateBase64(result+offset1+translateBase64(result+offset1,input+3,3),input+6,3);
}



}
