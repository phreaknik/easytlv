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

#ifndef __EASYTLV_H__
#define __EASYTLV_H__

#include <stdint.h>

#define EASYTLV_VER_MAJOR 1
#define EASYTLV_VER_MINOR 0
#define EASYTLV_VER_PATCH 0


typedef enum {
    ETLV_ERR_UNKNOWN    = -128, // Unknown failure
    ETLV_ERR_BADARG,            // Bad argument
    ETLV_ERR_OVERFLOW,          // Overflow detected
    ETLV_ERR_NOMEM,             // Not enough memory
    ETLV_ERR_INVAL,             // Invalid TLV data
    ETLV_ERR_MSGSIZE,           // TLV data exceeds provided size
    ETLV_ERR_NODATA,            // Not enough data was provided
    ETLV_ERR_NOENT,             // No entry was found
    ETLV_ERR_OK         = 0,    // No error
} ETLVError;

// A token describes a TLV object. Tag field does not need to occupy the
// entire 32b width. For example, a tag of 0x14 will properly be encoded as a
// single byte field.
typedef struct {
    uint32_t    tag;
    uint32_t    len;
    const void* val;
} ETLVToken;


/**
 * Parse TLV encoded data for TLV objects
 *
 * This parser will parse only one level of TLV objects. An object may include
 * TLV encoded data itself (ie nested TLV). To parse nested TLV objects, this
 * parser should be invoked again on the object's value data.
 *
 * In the event of an ETLV_ERR_NOMEM error, the output of `nTok` will still
 * represent the total number of tokens found in the byte array.
 *
 * [output] t       Array of tokens to be populated with parsed data
 * [in/out] nTok    Input size of the array / Output number of tokens parsed
 * [input]  src     Source pointer to TLV data
 * [input]  srcLen  Length source data to parse
 *
 * Returns number of tokens parsed, or negative error
 */
int etlv_parse(ETLVToken* t, int* nTok, const void* src, int srcLen);

/**
 * Serialize an array of TLV objects
 *
 * [output] dest    Destination to receive serialized data
 * [in/out] len     Input size of the destination / Output serialized length
 * [input]  t       Array of tokens to be serialized
 * [input]  nTok    Number of tokens to be serialized
 *
 * Returns the length of the serialized data, or negative error
 */
int etlv_serialize(void* dest, int* len, const ETLVToken* t, int nTok);

/**
 * Find the first occurance of a tag in a TLV encoded payload
 *
 * [output] t       Token information for the found tag (if found)
 * [input]  src     Source pointer to TLV data
 * [input]  srcLen  Length source data to search
 *
 * Returns the byte offset of the found token, or negative error
 */
int etlv_find(ETLVToken* t, uint32_t tag, const void* src, int srcLen);

#endif /* __EASYTLV_H__ */
