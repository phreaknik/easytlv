# EasyTLV
A lightweight TLV encode/decode library.

### What is TLV?
TLV (stands for Tag-Length-Value) is a commonly used data encoding scheme in communication protocols. It allows for payloads that include arbitrary length data, in arbitrary formats, at arbitrary offsets in the payload. While the protocol is simple, manual TLV parsing is often error prone. You may have seen manual TLV serialization or parsing code such as below:
```
// Read some TLV data
uint8_t rawData[] = ...;
if(rawData[0] == MY_TAG) {           // Match the TAG (Hopefully only one TAG byte is used!)
    int len = rawData[1];            // Read the length (Hopefully only one length byte is used!)
    memcpy(dest, &rawData[2], len);  // Copy the `value` of MY_TAG object
}

// Make some TLV data
uint8_t myTLV[] = ...;
myTLV[0] = MY_TAG;
myTLV[1] = sizeof(myObject); // Fingers crossed my object is <= 127 bytes, or this will cause errors!
memcpy(&myTLV[2], myObject, sizeof(myObject)); // Copy the data
```
The above code works for small simple payloads, but things can quickly get out of hand with more complicated payloads. This style of parsing is brittle, that often leads to hidden buffer errors when packets change or code gets refactored. This code is also very difficult to adapt for larger payloads or dynamic payloads (which is one of the core features TLV offers).

# EasyTLV makes it easy to use TLV encoded data in a safe & scalable way
The purpose of this library is to provide easy tools to operate on TLV data in a safe & scalable way. No need to manually juggle pointers and offsets. No need to write large if/else/switch blocks to handle every permutation of tag/length that a payload might contain.

### Parsing example:
The below example demonstrates how to parse a TLV packet containing two
unsigned 32 bit integers.
```
// Example TLV data, encoding two 32 bit numbers:
const uint8_t tlvRaw[] = {
    0x02, 0x04, 0x00, 0x00, 0x00, 0x2A, // Number 42 (network byte order)
    0x02, 0x04, 0x00, 0x00, 0x01, 0x01, // Number 257 (network byte order)
};

// Parse the two numbers
ETLVToken t[2];
int nTok = sizeof(t)/sizeof(t[0]);
int err = etlv_parse(t, &nTok, tlvRaw, sizeof(tlvRaw));
assert(err >= 0);

// Cast the numbers to uint32_t's and correct endianness
uint32_t num0 = ntohl(*(uint32_t *)t[0].val);
uint32_t num1 = ntohl(*(uint32_t *)t[1].val);
assert(num0 == 42);
assert(num1 == 257);
```

### Serializing example:
The below example demonstrates how to serialize two integers into a TLV packet
unsigned 32 bit integers.
```
// Data to be serialized
uint32_t num0 = htonl(42);
uint32_t num1 = htonl(257);

// Tokenize the data
const ETLVToken t[2] = {
    {.tag = 0x20, .len = sizeof(num0), .val = &num0},
    {.tag = 0x20, .len = sizeof(num1), .val = &num1},
};

// Serialize the data
uint8_t tlvRaw[255];
int bufSz = sizeof(tlvRaw);
int err = etlv_serialize(tlvRaw, &bufSz, t, sizeof(t)/sizeof(t[0]));
assert(err >= 0);
```

