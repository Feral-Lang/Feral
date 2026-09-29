/*
 License for base64 encode/decode logic from
 https://github.com/IMProject/IMUtility/blob/main/Src/base64.c:

 BSD 3-Clause License

 Copyright (c) 2022 - 2024, IMProject Development Team
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "VM/VM.hpp"

namespace fer
{

constexpr char base64EncodeTable[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char base64URLEncodeTable[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

constexpr unsigned char base64DecodeTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
    0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Functions /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(feralBase64Encode, 1, false,
           "  fn(bytes) -> String\n"
           "Encodes the bytebuffer `bytes` into a base64 string.\n"
           "Returns the encoded string or `nil` if it couldn't encode.\n"
           "Takes the following optional keyword args:\n"
           "* `url = true/false` - if `true`, uses Base64URL standard"
           " which is safe for use in URLs. (default: false)")
{
    EXPECT(VarBytebuffer, args[1], "bytes to encode");
    VarBytebuffer *buf        = as<VarBytebuffer>(args[1]);
    const unsigned char *data = buf->getVal();
    size_t dataLen            = buf->size();

    size_t len = 4 * ((dataLen + 2) / 3);
    if(dataLen == 0 || len < dataLen) return vm.getNil();

    bool url = false;
    if(Var *urlVar = assnArgs->getAttr("url")) {
        EXPECT(VarBool, urlVar, "url mode");
        url = as<VarBool>(urlVar)->getVal();
    }

    const char *table = url ? base64URLEncodeTable : base64EncodeTable;

    String res(len, '\0');
    char *resIt = res.data();

    size_t currLen = 0;
    size_t inPos   = 0;
    bool done      = false;

    while((dataLen - inPos) >= 3) {
        currLen += 4;
        if(currLen > res.size()) {
            done = true;
            break;
        }
        *resIt = table[data[0] >> 2];
        ++resIt;
        *resIt = table[((data[0] & 0x03) << 4) | (data[1] >> 4)];
        ++resIt;
        *resIt = table[((data[1] & 0x0F) << 2) | (data[2] >> 6)];
        ++resIt;
        *resIt = table[data[2] & 0x3F];
        ++resIt;
        data += 3;
        inPos += 3;
    }

    if(!done && dataLen != inPos) {
        currLen += 4;
        if(currLen > res.size()) done = true;

        if(!done) {
            *resIt = table[data[0] >> 2];
            ++resIt;
            if(dataLen - 1 == inPos) {
                *resIt = table[(data[0] & 0x03) << 4];
                ++resIt;
                *resIt = '=';
                ++resIt;
            } else {
                *resIt = table[((data[0] & 0x03) << 4) | (data[1] >> 4)];
                ++resIt;
                *resIt = table[(data[1] & 0x0F) << 2];
                ++resIt;
            }
            *resIt = '=';
            ++resIt;
        }
    }

    *resIt = '\0';
    if(url) {
        while(!res.empty() && res.back() == '=') res.pop_back();
    }
    return vm.makeVar<VarStr>(loc, std::move(res));
}

FERAL_FUNC(feralBase64Decode, 1, false,
           "  fn(data) -> Bytebuffer | Nil\n"
           "Decodes the base64 string `data`. Handles URL style base64 data as well.\n"
           "Returns the decoded data as a Bytebuffer, or `nil` if it couldn't decode.")
{
    EXPECT(VarStr, args[1], "base64 encoded data");
    VarStr *dataStr  = as<VarStr>(args[1]);
    const char *data = dataStr->getVal().c_str();
    size_t dataLen   = dataStr->getVal().size();

    if(dataLen == 0) return vm.getNil();

    bool isPadded = (dataLen > 0) && ((dataLen % 4) != 0 || data[dataLen - 1] == '=');
    int padding   = isPadded ? 1 : 0;
    size_t len    = (((dataLen + 3) / 4) - padding) * 4;
    size_t outLen = (((len / 4) * 3) + padding) + (dataLen > len + 2 && data[len + 2] != '=');

    if(len == 0) return vm.getNil();

    VarBytebuffer *res = vm.makeVar<VarBytebuffer>(loc, outLen);
    unsigned char *out = res->getVal();

    size_t j = 0;
    for(size_t i = 0; i < len; i += 4) {
        uint32_t n = (base64DecodeTable[data[i]] << 18) | (base64DecodeTable[data[i + 1]] << 12) |
                     (base64DecodeTable[data[i + 2]] << 6) | (base64DecodeTable[data[i + 3]]);
        out[j++]   = (n >> 16);
        out[j++]   = (n >> 8) & 0xFF;
        out[j++]   = n & 0xFF;
    }
    if(isPadded) {
        uint32_t n =
            (base64DecodeTable[data[len]] << 18) | (base64DecodeTable[data[len + 1]] << 12);
        if(dataLen > len + 2 && data[len + 2] != '=') {
            n |= base64DecodeTable[data[len + 2]] << 6;
            out[outLen - 2] = n >> 16;
            out[outLen - 1] = (n >> 8) & 0xFF;
        } else {
            out[outLen - 1] = n >> 16;
        }
    }

    res->setLen(outLen);

    return res;
}

INIT_DLL(Base64)
{
    vm.addLocal(loc, "encode", feralBase64Encode);
    vm.addLocal(loc, "decode", feralBase64Decode);
    return true;
}

} // namespace fer