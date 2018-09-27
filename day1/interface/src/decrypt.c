
#include <stdio.h>
#include <string.h>
#include <syslog.h>
/*===================================================================*/

#define Algorithm          AES_128
#ifndef AES_128
#define AES_128   2
#endif

#ifndef AES_256
#define AES_256   3
#endif

#if (Algorithm == AES_128)
//定义解密密钥: 128位
unsigned char DecryptKey[16] ={
    0xCC, 0x95, 0x3F, 0x8C, 0x39, 0x76, 0x16, 0x58,
    0x25, 0x43, 0xE3, 0x28, 0x45, 0xD8, 0x48, 0xAA
};
#elif (Algorithm == AES_256)
//定义解密密钥: 256位
unsigned char DecryptKey[32] ={
    0xD0, 0x94, 0x3F, 0x8C, 0x29, 0x76, 0x15, 0xD8,
    0xEA, 0x8B, 0x2A, 0x73, 0x16, 0xE9, 0xB0, 0x49,
    0x20, 0x40, 0xE3, 0x27, 0x45, 0xD8, 0x48, 0xAD,
    0x45, 0xB3, 0x39, 0x28, 0x0A, 0xC3, 0x28, 0x36
};
#endif

//使用 OPT_MODE 进行代码大小优化或者运行速度优化
//OPT_SPEED 可以提高解密速度 20% 以上
#define OPT_SIZE  1
#define OPT_SPEED 2
#define OPT_MODE  OPT_SIZE
#define BPOLY 0x1b          //!< Lower 8 bits of (x^8+x^4+x^3+x+1), ie. (x^4+x^3+x+1).
#define BLOCKSIZE 16        //!< 数据块大小(字节).

#ifndef Algorithm
#define Algorithm AES_128
#endif

#if (Algorithm == AES_128)
  #define ROUNDS 10         //循环次数
  #define KEYLENGTH 16      //密钥长度
#elif (Algorithm == AES_256)
  #define ROUNDS 14
  #define KEYLENGTH 32
#else
#error "Unknow encrypt algorithm!"
#endif

unsigned char block1[256];  //工作区1
unsigned char block2[256];  //工作区2
unsigned char tempbuf[256]; //临时缓冲区
unsigned char chain[16];    //密钥块链

unsigned char *powTbl;      //!< Final location of exponentiation lookup table.
unsigned char *logTbl;      //!< Final location of logarithm lookup table.
unsigned char *sBox;        //!< Final location of s-box.
unsigned char *sBoxInv;     //!< Final location of inverse s-box.
unsigned char *expandedKey; //!< Final location of expanded key.


void CalcPowLog(unsigned char *powTbl, unsigned char *logTbl)
{
    unsigned char i = 0;
    unsigned char t = 1;

    do {    // Use 0x03 as root for exponentiation and logarithms.
        powTbl[i] = t;
        logTbl[t] = i;
        i++;
        t ^= (t << 1) ^ (t & 0x80 ? BPOLY : 0);   // Muliply t by 3 in GF(2^8).
    }while( t != 1 ); // Cyclic properties ensure that i < 255.

    powTbl[255] = powTbl[0]; // 255 = '-0', 254 = -1, etc.
}

void CalcSBox( unsigned char * sBox )
{
    unsigned char i, rot;
    unsigned char temp;
    unsigned char result;

    i = 0;   // Fill all entries of sBox[].
    do {     //Inverse in GF(2^8).
        if( i > 0 ) {
            temp = powTbl[ 255 - logTbl[i] ];
        } else {
            temp = 0;
        }

        // Affine transformation in GF(2).
        result = temp ^ 0x63; // Start with adding a vector in GF(2).
        for( rot = 4; rot > 0; rot-- ) {
            temp = (temp<<1) | (temp>>7);   // Rotate left.
            result ^= temp;   // Add rotated byte in GF(2).
        }
        // Put result in table.
        sBox[i] = result;
    } while( ++i != 0 );
}

void CalcSBoxInv( unsigned char * sBox, unsigned char * sBoxInv )
{
    unsigned char i = 0;
    unsigned char j = 0;

  // Iterate through all elements in sBoxInv using  i.
    do {
        // Search through sBox using j.
        do {
           // Check if current j is the inverse of current i.
            if( sBox[j] == i ) {
                // If so, set sBoxInc and indicate search finished.
                sBoxInv[i] = j;
                j = 255;
            }
        } while( ++j != 0 );
    } while( ++i != 0 );
}

#if OPT_MODE == OPT_SIZE
void CalcCols(unsigned char *col)
{
    unsigned char i;

    for(i = 4; i > 0; i--) {
        *col = (*col << 1) ^ (*col & 0x80 ? BPOLY : 0);
        col++;
    }
}
#endif

void InvMixColumn( unsigned char * column )
{
    unsigned char r[4];

    r[0] = column[1] ^ column[2] ^ column[3];
    r[1] = column[0] ^ column[2] ^ column[3];
    r[2] = column[0] ^ column[1] ^ column[3];
    r[3] = column[0] ^ column[1] ^ column[2];

#if OPT_MODE == OPT_SIZE
    CalcCols(column);
#else
    column[0] = (column[0] << 1) ^ (column[0] & 0x80 ? BPOLY : 0);
    column[1] = (column[1] << 1) ^ (column[1] & 0x80 ? BPOLY : 0);
    column[2] = (column[2] << 1) ^ (column[2] & 0x80 ? BPOLY : 0);
    column[3] = (column[3] << 1) ^ (column[3] & 0x80 ? BPOLY : 0);
#endif

    r[0] ^= column[0] ^ column[1];
    r[1] ^= column[1] ^ column[2];
    r[2] ^= column[2] ^ column[3];
    r[3] ^= column[0] ^ column[3];

#if OPT_MODE == OPT_SIZE
    CalcCols(column);
#else
    column[0] = (column[0] << 1) ^ (column[0] & 0x80 ? BPOLY : 0);
    column[1] = (column[1] << 1) ^ (column[1] & 0x80 ? BPOLY : 0);
    column[2] = (column[2] << 1) ^ (column[2] & 0x80 ? BPOLY : 0);
    column[3] = (column[3] << 1) ^ (column[3] & 0x80 ? BPOLY : 0);
#endif

    r[0] ^= column[0] ^ column[2];
    r[1] ^= column[1] ^ column[3];
    r[2] ^= column[0] ^ column[2];
    r[3] ^= column[1] ^ column[3];

#if OPT_MODE == OPT_SIZE
    CalcCols(column);
#else
    column[0] = (column[0] << 1) ^ (column[0] & 0x80 ? BPOLY : 0);
    column[1] = (column[1] << 1) ^ (column[1] & 0x80 ? BPOLY : 0);
    column[2] = (column[2] << 1) ^ (column[2] & 0x80 ? BPOLY : 0);
    column[3] = (column[3] << 1) ^ (column[3] & 0x80 ? BPOLY : 0);
#endif

    column[0] ^= column[1] ^ column[2] ^ column[3];
    r[0] ^= column[0];
    r[1] ^= column[0];
    r[2] ^= column[0];
    r[3] ^= column[0];

    column[0] = r[0];
    column[1] = r[1];
    column[2] = r[2];
    column[3] = r[3];
}

void InvMixColumns( unsigned char * state )
{
    InvMixColumn( state + 0*4 );
    InvMixColumn( state + 1*4 );
    InvMixColumn( state + 2*4 );
    InvMixColumn( state + 3*4 );
}

void SubBytes( unsigned char * bytes, unsigned char count )
{
    do {
        *bytes = sBox[ *bytes ]; // Substitute every byte in state.
        bytes++;
    } while( --count );
}

void InvSubBytesAndXOR( unsigned char * bytes, unsigned char * key, unsigned char count )
{
    do {
        // *bytes = sBoxInv[ *bytes ] ^ *key; // Inverse substitute every byte in state and add key.
        *bytes = block2[ *bytes ] ^ *key; // Use block2 directly. Increases speed.
        bytes++;
        key++;
    } while( --count );
}

void InvShiftRows( unsigned char * state )
{
    unsigned char temp;
    // Note: State is arranged column by column.

#if OPT_MODE == OPT_SIZE
    unsigned char i, j;

    for(i = 3; i > 0; i--) {
        for(j = i; j > 0; j--) {
            temp = state[i + 3 * 4];
            state[i + 3 * 4] = state[i + 2 * 4];
            state[i + 2 * 4] = state[i + 1 * 4];
            state[i + 1 * 4] = state[i + 0 * 4];
            state[i + 0 * 4] = temp;
        }
    }
#else
    // Cycle second row right one time.
    temp = state[ 1 + 3*4 ];
    state[ 1 + 3*4 ] = state[ 1 + 2*4 ];
    state[ 1 + 2*4 ] = state[ 1 + 1*4 ];
    state[ 1 + 1*4 ] = state[ 1 + 0*4 ];
    state[ 1 + 0*4 ] = temp;

    // Cycle third row right two times.
    temp = state[ 2 + 0*4 ];
    state[ 2 + 0*4 ] = state[ 2 + 2*4 ];
    state[ 2 + 2*4 ] = temp;
    temp = state[ 2 + 1*4 ];
    state[ 2 + 1*4 ] = state[ 2 + 3*4 ];
    state[ 2 + 3*4 ] = temp;

    // Cycle fourth row right three times, ie. left once.
    temp = state[ 3 + 0*4 ];
    state[ 3 + 0*4 ] = state[ 3 + 1*4 ];
    state[ 3 + 1*4 ] = state[ 3 + 2*4 ];
    state[ 3 + 2*4 ] = state[ 3 + 3*4 ];
    state[ 3 + 3*4 ] = temp;
#endif
}

void XORBytes( unsigned char * bytes1, unsigned char * bytes2, unsigned char count )
{
    do {
        *bytes1 ^= *bytes2; // Add in GF(2), ie. XOR.
        bytes1++;
        bytes2++;
    } while( --count );
}

void CopyBytes( unsigned char * to, unsigned char * from, unsigned char count )
{
    do {
        *to = *from;
        to++;
        from++;
    } while( --count );
}

void KeyExpansion( unsigned char * expandedKey )
{
    unsigned char temp[4], t;
    unsigned char i;
    unsigned char Rcon[4] = { 0x01, 0x00, 0x00, 0x00 }; // Round constant.

    const unsigned char * key = DecryptKey;

    // Copy key to start of expanded key.
    CopyBytes(expandedKey, (unsigned char *)key, KEYLENGTH);
    expandedKey += KEYLENGTH;

    // Prepare last 4 bytes of key in temp.
    CopyBytes(temp, (unsigned char *)key + KEYLENGTH - 4, 4);

    // Expand key.
    i = KEYLENGTH;
    while( i < BLOCKSIZE*(ROUNDS+1) ) {
        // Are we at the start of a multiple of the key size?
        if( (i % KEYLENGTH) == 0 ) {
            // Cycle left once.
            t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;
            SubBytes( temp, 4 ); // Substitute each byte.
            XORBytes( temp, Rcon, 4 ); // Add constant in GF(2).
            *Rcon = (*Rcon << 1) ^ (*Rcon & 0x80 ? BPOLY : 0);
        }

        // Keysize larger than 24 bytes, ie. larger that 192 bits?
#if KEYLENGTH > 24
    // Are we right past a block size?
        else if( (i % KEYLENGTH) == BLOCKSIZE ) {
            SubBytes( temp, 4 ); // Substitute each byte.
        }
 #endif
        // Add bytes in GF(2) one KEYLENGTH away.
        XORBytes( temp, expandedKey - KEYLENGTH, 4 );

        // Copy result to current 4 bytes.
        *(expandedKey++) = temp[0];
        *(expandedKey++) = temp[1];
        *(expandedKey++) = temp[2];
        *(expandedKey++) = temp[3];
        i += 4; // Next 4 bytes.
    }
}

void InvCipher( unsigned char * block, unsigned char * expandedKey )
{
    unsigned char round = ROUNDS-1;
    expandedKey += BLOCKSIZE * ROUNDS;

    XORBytes( block, expandedKey, 16 );
    expandedKey -= BLOCKSIZE;

    do {
        InvShiftRows( block );
        InvSubBytesAndXOR( block, expandedKey, 16 );
        expandedKey -= BLOCKSIZE;
        //for(i = 4, j = 0; i > 0; i--, j+=4)
        //  InvMixColumn( block + j );
        InvMixColumns(block);
    } while( --round );

    InvShiftRows( block );
    InvSubBytesAndXOR( block, expandedKey, 16 );
}

unsigned char Multiply( unsigned char num, unsigned char factor )
{
    unsigned char mask = 1;
    unsigned char result = 0;

    while( mask != 0 ) {
        // Check bit of factor given by mask.
        if( mask & factor ) {
            // Add current multiple of num in GF(2).
            result ^= num;
        }
        // Shift mask to indicate next bit.
        mask <<= 1;

        // Double num.
        num = (num << 1) ^ (num & 0x80 ? BPOLY : 0);
     }
     return result;
}

unsigned char DotProduct( unsigned char * vector1, unsigned char * vector2 )
{
    unsigned char result = 0;

    result ^= Multiply( *vector1++, *vector2++ );
    result ^= Multiply( *vector1++, *vector2++ );
    result ^= Multiply( *vector1++, *vector2++ );
    result ^= Multiply( *vector1  , *vector2   );

    return result;
}

void ShiftRows( unsigned char * state )
{
    unsigned char temp;
    // Note: State is arranged column by column.

    // Cycle second row left one time.
    temp = state[ 1 + 0*4 ];
    state[ 1 + 0*4 ] = state[ 1 + 1*4 ];
    state[ 1 + 1*4 ] = state[ 1 + 2*4 ];
    state[ 1 + 2*4 ] = state[ 1 + 3*4 ];
    state[ 1 + 3*4 ] = temp;

    // Cycle third row left two times.
    temp = state[ 2 + 0*4 ];
    state[ 2 + 0*4 ] = state[ 2 + 2*4 ];
    state[ 2 + 2*4 ] = temp;
    temp = state[ 2 + 1*4 ];
    state[ 2 + 1*4 ] = state[ 2 + 3*4 ];
    state[ 2 + 3*4 ] = temp;

    // Cycle fourth row left three times, ie. right once.
    temp = state[ 3 + 3*4 ];
    state[ 3 + 3*4 ] = state[ 3 + 2*4 ];
    state[ 3 + 2*4 ] = state[ 3 + 1*4 ];
    state[ 3 + 1*4 ] = state[ 3 + 0*4 ];
    state[ 3 + 0*4 ] = temp;
}

void MixColumn( unsigned char * column )
{
    unsigned char result[4];
    unsigned char row[8] = {
        0x02, 0x03, 0x01, 0x01,
        0x02, 0x03, 0x01, 0x01
    }; // Prepare first row of matrix twice, to eliminate need for cycling.

    // Take dot products of each matrix row and the column vector.
    result[0] = DotProduct( row+0, column );
    result[1] = DotProduct( row+3, column );
    result[2] = DotProduct( row+2, column );
    result[3] = DotProduct( row+1, column );

    // Copy temporary result to original column.
    column[0] = result[0];
    column[1] = result[1];
    column[2] = result[2];
    column[3] = result[3];
}

void MixColumns( unsigned char * state )
{
    MixColumn( state + 0*4 );
    MixColumn( state + 1*4 );
    MixColumn( state + 2*4 );
    MixColumn( state + 3*4 );
}

void Cipher( unsigned char * block, unsigned char * expandedKey )    //完成一个块(16字节，128bit)的加密
{
    unsigned char round = ROUNDS-1;

    XORBytes(block, expandedKey, 16);
    expandedKey += BLOCKSIZE;

    do {
        SubBytes(block, 16);
        ShiftRows(block);
        MixColumns(block);
        XORBytes(block, expandedKey, 16);
        expandedKey += BLOCKSIZE;
    } while(--round);

    SubBytes(block, 16);
    ShiftRows(block);
    XORBytes(block, expandedKey, 16);
}

void aesDecInit(void)
{
    unsigned char i;

    for(i = 0; i < 16; i++)
        chain[i] = 0;

    powTbl = block1;
    logTbl = block2;
    CalcPowLog(powTbl, logTbl);

    sBox = tempbuf;
    CalcSBox( sBox );

    expandedKey = block1;
    KeyExpansion( expandedKey );

    sBoxInv = block2; // Must be block2.
    CalcSBoxInv( sBox, sBoxInv );
}

//销毁密钥
void DestroyKey(void)
{
    unsigned char i;

#if (Algorithm == AES_128)
    for(i = 16; i > 0; i--)
#else
    for(i = 32; i > 0; i--)
#endif
        DecryptKey[i - 1] = 0;
}

//解密数据块
void DecryptBlock(unsigned char *buf, unsigned int nSize)
{
    unsigned int n;
    aesDecInit();
    //memset(tempbuf,0,sizeof(tempbuf));
    for(n = nSize; n > 0; n -= 16) {
        CopyBytes( tempbuf, buf, BLOCKSIZE );
        InvCipher( buf, expandedKey );
        XORBytes( buf, chain, BLOCKSIZE );
        CopyBytes( chain, tempbuf, BLOCKSIZE );
        buf += 16;
    }
}

void aesEncInit(void)
{
    unsigned char i;

    for(i = 0; i < 16; i++)
        chain[i] = 0;

    powTbl = block1;
    logTbl = tempbuf;
    CalcPowLog( powTbl, logTbl );

    sBox = block2;
    CalcSBox( sBox );

    expandedKey = block1;
    KeyExpansion( expandedKey );

    //sBoxInv = block2; // Must be block2.
    //CalcSBoxInv( sBox, sBoxInv );
}

void EncryptBlock(unsigned char *buf, unsigned int nSize)
{
    unsigned int n;
    aesEncInit();
    //memset(tempbuf,0,sizeof(tempbuf));
    for(n = nSize; n > 0; n -= 16) {
        XORBytes( buf, chain, BLOCKSIZE );
        Cipher( buf, expandedKey );
        CopyBytes( chain, buf, BLOCKSIZE );
        buf += 16;
    }
}

//把用ascii表示的数据转化成对应的数值，如把a5转成数值0xa5
unsigned short GetValueFromASCII(unsigned char uFirst, unsigned char uNext)
{
    unsigned short i = 0, j = 0;
    if( (uFirst >= 48) && (uFirst <= 57) ) //'0'-'9'
        i = uFirst - 48;
    else if( (uFirst >= 65) && (uFirst <= 70) )//'A'-'F'
        i = uFirst - 55;
    else if ( (uFirst>= 97) && (uFirst <= 102) )//'a'-'f'
        i = uFirst - 87;
    //else是无效数据

    if( (uNext >= 48) && (uNext <= 57) )
        j = uNext - 48;
    else if ( (uNext >= 65) && (uNext <= 70) )
        j = uNext - 55;
    else if ( (uNext >= 97) && (uNext <= 102) )
    	j = uNext - 87;
	//else是无效数据
    return (i<<4)+j;
}

//把theValue转化成16进制代码表示
void ConvertToHexCode(unsigned char theValue,char * uFirst,char * uNext)
{
    char temp[3]="\0";
    sprintf(temp,"%02X",theValue);
    *uFirst=(char) temp[0];
    *uNext=(char) temp[1];
}

int atohex(char * str,int len)
{
    int i,sum = 0;
    for(i = 0; i<len; i++)
    {
        if(str[i]>='0' && str[i]<='9')
            sum = sum*16 + str[i]-'0';
        else if(str[i]<='f' && str[i]>='a')
            sum = sum*16 + str[i]-'a'+10;
        else if(str[i]<='F' && str[i]>='A')
            sum = sum*16 + str[i]-'A'+10;
    }
    return sum;
}

const unsigned char crc8table[256] = { //reversed, 8-bit, poly=0x07
  0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
  0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
  0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
  0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
  0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
  0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
  0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
  0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
  0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
  0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
  0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
  0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
  0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
  0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
  0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
  0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
  0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
  0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
  0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
  0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
  0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
  0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
  0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
  0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
  0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
  0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
  0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
  0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
  0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
  0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
  0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
  0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF };

unsigned char crc8(unsigned char  *input, unsigned int count) {
    unsigned char fcs = 0xFF;
    unsigned int i;
    for (i = 0; i < count; i++) {
        fcs = crc8table[fcs^input[i]];
    }
    return fcs;
}

int ValidateSN(unsigned char *pData,char *pDeviceID,unsigned int *pExpireDateTime,unsigned char *option)
{
    int i, len;
    unsigned short sum,n=0;
    char DeviceId[16];
    unsigned char data[16];
    unsigned int ExpireDate;
    unsigned short num;
    memset(DeviceId,0,sizeof(DeviceId));
    memset(data,0,sizeof(data));
    *option=0;

    len = strlen(pDeviceID);
   // printk(" len = %d\n", len);
    pDeviceID[strlen(pDeviceID)-1]='\0';

    if(strlen((char *)pData)!=32) {
        syslog(LOG_LOCAL7|LOG_INFO,"Serial num ERROR\r\n");
        return 2;
    }
    for(i=0;i<16;i++) {
        data[i]=GetValueFromASCII(pData[2*i], pData[2*i+1]);
    }

    //AES解码
    DecryptBlock(data,16);

    //计算校验
    sum= data[15];
    n=crc8(data,15);
    if(sum!=n) {
        //printf("Serial sum ERROR %x %x\r\n",sum,n);
        syslog(LOG_LOCAL7|LOG_INFO,"Serial sum ERROR %x %x\r\n",sum,n);
        return 3;
    }

    strcpy(&DeviceId[0],"NSC20");
    //B2-B1
    strncpy(&DeviceId[len-9], (char *)data, 6);

    //编号
    num=(data[6]);
    sprintf(&DeviceId[len-3],"%02d",num);

    //时间
    ExpireDate= (data[10]<<24)|(data[11]<<16)|(data[12]<<8)|data[13];

    if((ExpireDate/10000)<2010 || (ExpireDate/10000)>2099) {
        //printf("Serial num ERROR: wrong date\r\n");
        syslog(LOG_LOCAL7|LOG_INFO,"Serial num ERROR: wrong date\n");
        return 4;
    }

    if(strcmp(&DeviceId[len-9],&pDeviceID[len-9])!=0) {
        //printf("Serial num ERROR: wrong device id\r\n");
        syslog(LOG_LOCAL7|LOG_INFO,"Serial num ERROR: wrong date\n");
        return 5;
    }
    *option=data[14];
    *pExpireDateTime=ExpireDate;

    return 0;
}
