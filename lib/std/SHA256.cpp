#include "SHA256.hpp"

#include "VM/VM.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// SHA256 Core ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// Round constants: first 32 bits of the fractional parts of the cube roots of the first 64 primes.
static constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static constexpr uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void compress(uint32_t H[8], const uint8_t blk[64])
{
    uint32_t w[64];
    for(int i = 0; i < 16; ++i) {
        int j = i * 4;
        w[i]  = ((uint32_t)blk[j] << 24) | ((uint32_t)blk[j + 1] << 16) |
               ((uint32_t)blk[j + 2] << 8) | ((uint32_t)blk[j + 3]);
    }
    for(int i = 16; i < 64; ++i) {
        uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i]        = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
    uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

    for(int i = 0; i < 64; ++i) {
        uint32_t S1  = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch  = (e & f) ^ (~e & g);
        uint32_t t1  = h + S1 + ch + K[i] + w[i];
        uint32_t S0  = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2  = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarSHA256Ctx //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarSHA256Ctx::VarSHA256Ctx(ModuleLoc loc) : Var(loc) { reset(); }

void VarSHA256Ctx::reset()
{
    H[0] = 0x6a09e667;
    H[1] = 0xbb67ae85;
    H[2] = 0x3c6ef372;
    H[3] = 0xa54ff53a;
    H[4] = 0x510e527f;
    H[5] = 0x9b05688c;
    H[6] = 0x1f83d9ab;
    H[7] = 0x5be0cd19;

    pendingLen = 0;
    msgLen     = 0;
}

void VarSHA256Ctx::update(const uint8_t *data, size_t len)
{
    msgLen += len;
    for(size_t i = 0; i < len; ++i) {
        pending[pendingLen++] = data[i];
        if(pendingLen == 64) {
            compress(H, pending);
            pendingLen = 0;
        }
    }
}

String VarSHA256Ctx::finalize() const
{
    // Work on local copies so the original context is left unmodified.
    uint32_t tmpH[8];
    uint8_t tmpPending[128]; // at most two 64-byte blocks after padding
    size_t tmpLen = pendingLen;

    memcpy(tmpH, H, sizeof(H));
    memcpy(tmpPending, pending, pendingLen);

    // Padding: 0x80, zeros until length ≡ 56 (mod 64), then 64-bit big-endian bit count.
    tmpPending[tmpLen++] = 0x80;
    while(tmpLen % 64 != 56) tmpPending[tmpLen++] = 0x00;
    uint64_t bitLen      = msgLen * 8;
    tmpPending[tmpLen++] = (bitLen >> 56) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 48) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 40) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 32) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 24) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 16) & 0xFF;
    tmpPending[tmpLen++] = (bitLen >> 8) & 0xFF;
    tmpPending[tmpLen++] = (bitLen) & 0xFF;

    for(size_t i = 0; i < tmpLen; i += 64) compress(tmpH, tmpPending + i);

    static const char hex[] = "0123456789abcdef";
    String result;
    result.reserve(64);
    for(int i = 0; i < 8; ++i) {
        result += hex[(tmpH[i] >> 28) & 0xF];
        result += hex[(tmpH[i] >> 24) & 0xF];
        result += hex[(tmpH[i] >> 20) & 0xF];
        result += hex[(tmpH[i] >> 16) & 0xF];
        result += hex[(tmpH[i] >> 12) & 0xF];
        result += hex[(tmpH[i] >> 8) & 0xF];
        result += hex[(tmpH[i] >> 4) & 0xF];
        result += hex[(tmpH[i]) & 0xF];
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(newSHA256Ctx, 0, false,
           "  fn() -> SHA256Ctx\n"
           "Creates and returns a fresh SHA-256 streaming context.\n"
           "Use with update() and final() for incremental hashing.")
{
    return vm.makeVar<VarSHA256Ctx>(loc);
}

FERAL_FUNC(sha256Update, 1, false,
           "  var.fn(data) -> Nil\n"
           "Feeds the bytes of string `data` into the SHA-256 context `var`.\n"
           "May be called any number of times before calling final().")
{
    EXPECT2(VarStr, VarBytebuffer, args[1], "data string");
    VarSHA256Ctx *ctx = as<VarSHA256Ctx>(args[0]);
    if(args[1]->is<VarStr>()) {
        const String &s = as<VarStr>(args[1])->getVal();
        ctx->update(reinterpret_cast<const uint8_t *>(s.data()), s.size());
    } else if(args[1]->is<VarBytebuffer>()) {
        VarBytebuffer *buf = as<VarBytebuffer>(args[1]);
        ctx->update(buf->getVal(), buf->size());
    }
    return vm.getNil();
}

FERAL_FUNC(sha256Final, 0, false,
           "  var.fn() -> Str\n"
           "Finalises the SHA-256 hash in the context `var` and returns the 64-character lowercase "
           "hex digest.\n"
           "Does not mutate `var`, so it may be called more than once on the same context.")
{
    VarSHA256Ctx *ctx = as<VarSHA256Ctx>(args[0]);
    return vm.makeVar<VarStr>(loc, ctx->finalize());
}

INIT_DLL(SHA256)
{
    vm.addLocalType<VarSHA256Ctx>(loc, "SHA256Ctx", "A SHA-256 streaming hash context.");

    vm.addLocal(loc, "newCtx", newSHA256Ctx);

    vm.addTypeFn<VarSHA256Ctx>(loc, "update", sha256Update);
    vm.addTypeFn<VarSHA256Ctx>(loc, "final", sha256Final);
    return true;
}

} // namespace fer
