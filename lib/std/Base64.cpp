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

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Functions /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(feralBase64Encode, 1, false,
           "  fn(bytes) -> String\n"
           "Encodes the bytebuffer `bytes` into a bas64 string.\n"
           "Returns the encoded string.\n"
           "Takes the following optional keyword args:\n"
           "* `url = true/false` - if `true`, uses Base64URL standard"
           " which is safe for use in URLs. (default: false)")
{
    EXPECT(VarBytebuffer, args[1], "bytes to encode");
    VarBytebuffer *buf        = as<VarBytebuffer>(args[1]);
    const unsigned char *data = buf->getVal();
    size_t dataLen            = buf->size();

    size_t len = 4U * ((dataLen + 2U) / 3U);
    if(len < dataLen) return vm.makeVar<VarStr>(loc, "");

    bool url = false;
    if(Var *urlVar = assnArgs->getAttr("url")) url = as<VarBool>(urlVar)->getVal();

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
        *resIt = table[((data[0] & 0x03U) << 4) | (data[1] >> 4)];
        ++resIt;
        *resIt = table[((data[1] & 0x0FU) << 2) | (data[2] >> 6)];
        ++resIt;
        *resIt = table[data[2] & 0x3FU];
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
                *resIt = table[(data[0] & 0x03U) << 4];
                ++resIt;
                *resIt = '=';
                ++resIt;
            } else {
                *resIt = table[((data[0] & 0x03U) << 4) | (data[1] >> 4)];
                ++resIt;
                *resIt = table[(data[1] & 0x0FU) << 2];
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

INIT_DLL(Base64)
{
    vm.addLocal(loc, "encode", feralBase64Encode);
    return true;
}

} // namespace fer