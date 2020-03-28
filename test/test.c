#include "../easytlv.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define etlv_printf printf
#define etlv_print_hex print_hex


// Example TLV data, encoding two 32 bit numbers:
const uint8_t testDataShort[] = {
    0x02, 0x04, 0x00, 0x00, 0x00, 0x2A, // Number 42
    0x02, 0x04, 0x00, 0x00, 0x01, 0x01, // Number 257
};

const uint8_t testDataLong[] = {
    0x1F, 0x88, 0x01,	// Extended tag
    0x82, 0x01, 0x01, 	// Extended length: 257 bytes
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0x01,
    0x02,		// Short tag
    0x04,		// Short length: 4 bytes
    0x00, 0x00, 0x01, 0x01,
};

void print_hex(const void* src, int len)
{
    if(!src || len < 0)
        return;

    const uint8_t* s = src;
    printf("(%i) ", len);
    while(len-- > 0)
        printf("%02x", *s++);
}

/**
 * Determine endianness of the host machine
 *
 * The original implementation of this function likely worked just fine if dealing
 * only with big and little endian machines. Trouble could arise for machines
 * using PDP-11 or Honeywell 316.
 *
 * PDP-11 can actually be written by certain pre-v8 ARM architectures when trying
 * to store a word on a 2-byte address boundary.
 *
 * This implementation will allow for us to check for those possibilities as well,
 * if ever necessary.
 *
 * Storing a 32 bit value in the different endianness options is here for reference:
 * Value to store: 0x0A0B0C0D
 *
 * Address: | 0x0 | 0x1 | 0x2 | 0x3 |
 *    Byte: | 0Ah | 0Bh | 0Ch | 0Dh | <- Big Endian
 *    Byte: | 0Dh | 0Ch | 0Bh | 0Ah | <- Little Endian
 *    Byte: | 0Bh | 0Ah | 0Dh | 0Ch | <- PDP-11
 *    Byte: | 0Ch | 0Dh | 0Ah | 0Bh | <- Honeywell 316
 *
 * [output] int    Integer that is non-zero if machine is big_endian
 *
 * Returns non-zero value if host machine is big-endian
 */
static inline int is_big_endian()
{
    volatile uint32_t i = 0x0A0B0C0D;
    int big_endian = 0;
    uint8_t* p = (uint8_t *) &i;
    switch(*p)
    {
        case 0x0A:  //Big Endian
            big_endian = 1;
            break;
        case 0x0B:  //PDP-11
        case 0x0C:  //Honeywell 316
        case 0x0D:  //Little Endian
        default:    //memory corruption perhaps?
            break;
    }
    return big_endian;
}

static inline uint32_t ntohl(uint32_t num)
{
    if (is_big_endian()) {
        return num;
    } else {    
        return  ((num>>24)&0xff) |      // move byte 3 to byte 0
                ((num<<8)&0xff0000) |   // move byte 1 to byte 2
                ((num>>8)&0xff00) |     // move byte 2 to byte 1
                ((num<<24)&0xff000000); // byte 0 to byte 3
    }
}
#define htonl ntohl

int main()
{
    uint8_t tlvRaw[sizeof(testDataLong)];
    uint32_t numChk;

    printf("\n\n\r-------------------- EasyTLV Test --------------------\n");

    // ---- TEST ON SHORT DATA ----
    printf("Parse test (SHORT DATA)\n\r");

    // Parse the two tokens
    ETLVToken t[2];
    int nTok = sizeof(t)/sizeof(t[0]);
    int err = etlv_parse(t, &nTok, testDataShort, sizeof(testDataShort));
    printf(" - result: %i\n\r", err);
    assert(err >= 0);

    // Cast the numbers to uint32_t's and compare
    numChk = ntohl(*(uint32_t *)t[0].val);
    assert(numChk == 42);
    numChk = ntohl(*(uint32_t *)t[1].val);
    assert(numChk == 257);
    printf(" - TEST PASS\n\r");

    printf("Serialization test (SHORT DATA)\n\r");
    printf(" - re-serializing same data as before\n\r");

    // Re-serialize the parsed data and compare to original
    int tlvBufSz = sizeof(tlvRaw);
    err = etlv_serialize(tlvRaw, &tlvBufSz, t, nTok);
    printf(" - result: %i\n\r", err);
    assert(err >= 0);
    assert(0 == memcmp(tlvRaw, testDataShort, tlvBufSz));
    printf(" - TEST PASS\n\r");

    // ---- TEST ON LONG DATA ----
    printf("Parse test (LONG DATA)\n\r");

    // Parse the two tokens
    nTok = sizeof(t)/sizeof(t[0]);
    err = etlv_parse(t, &nTok, testDataLong, sizeof(testDataLong));
    printf(" - result: %i\n\r", err);
    assert(err >= 0);

    // Check the parsed tokens
    assert(t[0].tag == 0x001F8801);
    assert(t[0].len == 257);
    assert(t[0].val == testDataLong + 3 + 3);
    numChk = ntohl(*(uint32_t *)t[1].val);
    assert(numChk == 257);
    printf(" - TEST PASS\n\r");

    printf("Serialization test (LONG DATA)\n\r");
    printf(" - re-serializing same data as before\n\r");

    // Re-serialize the parsed data and compare to original
    tlvBufSz = sizeof(tlvRaw);
    err = etlv_serialize(tlvRaw, &tlvBufSz, t, nTok);
    printf(" - result: %i\n\r", err);
    assert(err >= 0);
    assert(0 == memcmp(tlvRaw, testDataLong, tlvBufSz));
    printf(" - TEST PASS\n\r");

    // Search for both tags
    printf("Tag search test (LONG DATA -- first tag)\n\r");
    ETLVToken needle;
    err = etlv_find(&needle, 0x001F8801, testDataLong, sizeof(testDataLong));
    printf(" - result: %i\n\r", err);
    assert(err == 0);
    assert(0 == memcmp(&t[0], &needle, sizeof(needle)));
    printf(" - TEST PASS\n\r");
    printf("Tag search test (LONG DATA -- second tag)\n\r");
    err = etlv_find(&needle, 0x02, testDataLong, sizeof(testDataLong));
    printf(" - result: %i\n\r", err);
    assert(err == 263);
    assert(0 == memcmp(&t[1], &needle, sizeof(needle)));
    printf(" - TEST PASS\n\r");
}

