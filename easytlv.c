/**
 *  _____              _____ _     _   _ 
 * |  ___|            |_   _| |   | | | |
 * | |__  __ _ ___ _   _| | | |   | | | |
 * |  __|/ _` / __| | | | | | |   | | | |
 * | |__| (_| \__ \ |_| | | | |___\ \_/ /
 * \____/\__,_|___/\__, \_/ \_____/\___/ 
 *                  __/ |                
 *                 |___/                 
 *
 * This library implements the ASN1 Tag-Length-Value encoding standard. For
 * more information about this standard, see the ISO8825-1 Basic Encoding
 * Rules: https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 * 
 * Note: This library does not implement indefinite length TLV objects, as
 * described in section 8.1.3
 */

/**
 * MIT License
 *
 * Copyright (c) 2020 John Boyd <johnboydiv@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "easytlv.h"
#include <stdint.h>


// Debugging
#ifndef ETLV_DEBUG
    #define ETLV_PRINTF(...)
    #define ETLV_LOG_HEX(...)
#else
    #include "easytlv_debug.h"
#endif
#define ETLV_LOG(...) ETLV_PRINTF("<etlv debug> " __VA_ARGS__)
#define ETLV_LOG_LINE() ETLV_PRINTF("\n\r")

#define GET_MSBYTE(i) ((i >> 24) & 0xFF)


// Compute the minimum number of bytes required to represent a number
static inline uint8_t min_size(uint32_t d)
{
    uint8_t n = 0;
    while (d) {
        d >>= 8;
        n++;
    }
    return n;
}

// Left-shift a number, until it has no leading zeros
static inline uint32_t trim_leading_zeros(uint32_t i)
{
    while (i <= 0x00FFFFFF)
        i <<= 8;
    return i;
}

// Read the tag field from the source buffer and decode it
// Modifies src to point to next byte after tag bytes
// Returns tag, or negative error
static inline int decode_tag(uint32_t* tag, const uint8_t** src, int srcLen)
{
    if (!tag || !src || !*src)
        return ETLV_ERR_BADARG;
    if (srcLen <= 0)
        return ETLV_ERR_NODATA;

    const uint8_t* s = *src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;
    *src = 0; // Reset src to null

    if ((*s & 0x1F) > 30) { // Is the tag extended?
        // First subsequent octet must not be 0
        if (srcLen < 2 || *(s+1) == 0)
            return ETLV_ERR_INVAL;

        *tag = *s++;
        do {
            // Detect overflow
            if ((*tag << 8) < *tag)
                return ETLV_ERR_OVERFLOW;

            // Join each octet of the tag
            *tag = (*tag << 8) + (*s & 0xFF);
        } while (s < END && *s++ & 0x80); // Last octet will clear MSB

        if (s > END)
            return ETLV_ERR_MSGSIZE;

        *src = s;
    } else { // Tag is just one byte
        *src = s+1;
        *tag = *s;
    }

    return s-BEGIN;
}

// Encode the tag and Write it to the destination buffer
// Modifies dest to point to next byte after tag bytes
// Returns number of bytes written, or negative error
static inline int encode_tag(uint8_t** dest, int destLen, uint32_t tag)
{
    if (!dest || !*dest)
        return ETLV_ERR_BADARG;
    if (destLen <= 0)
        return ETLV_ERR_NOMEM;

    uint8_t* d = *dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + destLen;
    *dest = 0; // Reset dest to null

    if (tag < 0 || tag > 0xFF) { // Extended tag
        // Scan to first tag byte
        tag = trim_leading_zeros(tag);
        
        // Check first byte is valid extended byte
        if ((GET_MSBYTE(tag) & 0x1F) < 31)
            return ETLV_ERR_INVAL;

        // Write tag bytes
        while (tag > 0x00FFFFFF) {
            // Check for enough memory
            if (d >= END)
                return ETLV_ERR_NOMEM;

            // Copy next tag byte
            *d++ = GET_MSBYTE(tag);
            tag <<= 8;
        }
    } else { // Short tag
        if ((tag & 0x1F) > 30)
            return ETLV_ERR_INVAL; // Invalid short tag

        // Check for end of buffer
        if (d >= END)
            return ETLV_ERR_NOMEM;

        // Write tag byte
        *d++ = tag & 0xFF;
    }

    *dest = d;
    return d-BEGIN; 
}

// Read the length field from the source buffer and decode it
// Modifies src to point to next byte after length bytes
// Returns number of bytes read or negative error
static inline int decode_length(uint32_t* length, const uint8_t** src, int srcLen)
{
    if (!length || !src || !*src)
        return ETLV_ERR_BADARG;
    if (srcLen <= 0)
        return ETLV_ERR_NODATA;

    const uint8_t* s = *src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;
    *src = (void*) 0; // Reset src to null

    if (*s & 0x80) { // Is the length in long form?
        // The first byte must not be 0xFF
        if (*s == 0xFF)
            return ETLV_ERR_INVAL;

        const int N = *s++ & 0x7F; // Number of length bytes to use
        if (N > 4 || (N > 3 && s[3] & 0x80))
            return ETLV_ERR_OVERFLOW;

        int len = 0;
        for(int i = 0; i < N; i++)
            len = (len << 8) + s[i];
        *src = s+N;
        *length = len;
    } else { // Tag is just one byte
        *src = s+1;
        *length = *s;
    }

    if (s >= END)
        return ETLV_ERR_MSGSIZE;

    return s-BEGIN;
}

// Encode the length and Write it to the destination buffer
// Modifies dest to point to next byte after length bytes
// Returns number of bytes written, or negative error
static inline int encode_length(uint8_t** dest, int destLen, uint32_t length)
{
    if (!dest || !*dest || length <= 0)
        return ETLV_ERR_BADARG;
    if (destLen <= 0)
        return ETLV_ERR_NOMEM;

    uint8_t* d = *dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + destLen;
    *dest = 0; // Reset dest to null

    if (length > 0x7F) { // Long form
        // Determine how many length bytes
        int n = min_size(length);
        if (n > 0x7E) // This long cannot fit in TLV encoding
            return ETLV_ERR_OVERFLOW;
        if (d >= END)
            return ETLV_ERR_NOMEM;
        *d++ = (n | 0x80) & 0xFF; 

        // Write length bytes
        length = trim_leading_zeros(length);
        while (n-- > 0) {
            if (d >= END)
                return ETLV_ERR_NOMEM;

            *d++ = GET_MSBYTE(length);
            length <<= 8;
        }
    } else { // Short form
        if (d >= END)
            return ETLV_ERR_NOMEM;
        *d++ = length & 0xFF;
    }

    *dest = d;
    return d-BEGIN;
}

int etlv_parse(ETLVToken* t, int* nTok, const void* src, int srcLen)
{
    if (!t || !nTok || *nTok < 0 || !src || srcLen < 0)
        return ETLV_ERR_BADARG;

    const uint8_t* s = src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;

    ETLV_LOG("Parse input: ");
    ETLV_LOG_HEX(src, srcLen);
    ETLV_LOG_LINE();

    int n = 0;
    int err = 0;
    while (s < END) {
        // Check for memory
        if (n >= *nTok) {
            n = ETLV_ERR_NOMEM;
            break;
        }

        // Decode the tag field
        err = decode_tag(&t[n].tag, &s, END-s);
        if (err < 0) {
            n = err;
            break;
        }
        ETLV_LOG("tag: %08X\n\r", t[n].tag);

        // Decode the length field
        err = decode_length(&t[n].len, &s, END-s);
        if (err < 0) {
            n = err;
            break;
        }
        ETLV_LOG("len: %u\n\r", t[n].len);

        // Save pointer to value field
        t[n].val = s;
        if (t[n].val == 0) {
            n = ETLV_ERR_UNKNOWN;
            break;
        }
        ETLV_LOG("val: ");
        ETLV_LOG_HEX(t[n].val, t[n].len);
        ETLV_LOG_LINE();

        // Point to next object
        s = t[n].val + t[n].len; 
        n++;
    }

    // Output the number of tokens found
    if (n >= 0)
        *nTok = n;

    // Handle errors
    if (n < 0) // Error during parse loop
        return n;
    if (s > END) // TLV data exceeds byte array provided
        return ETLV_ERR_MSGSIZE;

    // Return the total length of the TLV data
    return s-BEGIN;
}

int etlv_serialize(void* dest, int* len, const ETLVToken* t, int nTok)
{
    if (!dest || !len || *len < 0 || !t || nTok < 0)
        return ETLV_ERR_BADARG;

    uint8_t* d = dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + *len;

    ETLV_LOG("Serializing %d tokens into %i byte buffer\n\r", nTok, *len);

    *len = 0;
    int err = 0;
    for (int i = 0; i < nTok; i++) {
        // Write the tag field
        ETLV_LOG("tag: %08X\n\r", t[i].tag);
        err = encode_tag(&d, END-d, t[i].tag);
        if (err < 0)
            return err;
        if (*len + err < *len)
            return ETLV_ERR_OVERFLOW;

        // Write the length field
        ETLV_LOG("len: %u\n\r", t[i].len);
        err = encode_length(&d, END-d, t[i].len);
        if (err < 0)
            return err;
        if (*len + err < *len)
            return ETLV_ERR_OVERFLOW;

        // Copy the value
        ETLV_LOG("val: ");
        ETLV_LOG_HEX(t[i].val, t[i].len);
        ETLV_LOG_LINE();
        const uint8_t* v = t[i].val;
        const uint8_t* const VAL_END = v + t[i].len;
        while (v < VAL_END) {
            // Make sure not to overrun the buffer
            if (d >= END)
                return ETLV_ERR_NOMEM;

            *d++ = *v++;
        }
    }

    // Return total serialized length
    return *len = d-BEGIN;
}

int etlv_find(ETLVToken* t, uint32_t tag, const void* src, int srcLen)
{
    if (!t || !src || srcLen < 0)
        return ETLV_ERR_BADARG;

    const uint8_t* s = src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;

    int err;
    uint32_t found;
    uint32_t len;
    int offset = 0;
    while (s < END) {
        offset = s-BEGIN;
        err = decode_tag(&found, &s, END-s);
        if (err < 0)
            break;
        ETLV_LOG("Found tag: 0x%08X\n\r", found);
        err = decode_length(&len, &s, END-s);
        if (err < 0)
            break;
        if (found == tag)
            break;
        s += len;
    }

    if (err < 0)
        return err;

    if (s >= END)
        return ETLV_ERR_NOENT;

    // Output a token for the found tag
    t->tag = found;
    t->len = len;
    t->val = s;

    // Return the byte offset of the found token
    return offset;
}

