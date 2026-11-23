#ifndef XENO_KROM_DISC_WIN_HASH_H
#define XENO_KROM_DISC_WIN_HASH_H

/* Use Windows CNG directly so checking owner-provided media does not require
 * shipping a separate crypto DLL. The provider allocates its own hash object. */
#include <windows.h>
#include <bcrypt.h>

typedef struct {
    BCRYPT_ALG_HANDLE algorithm;
    BCRYPT_HASH_HANDLE hash;
} XenoWinHash;

static inline int xeno_win_hash_init(XenoWinHash *ctx, LPCWSTR algorithm)
{
    return BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&ctx->algorithm, algorithm, NULL, 0)) &&
           BCRYPT_SUCCESS(BCryptCreateHash(ctx->algorithm, &ctx->hash, NULL, 0, NULL, 0, 0));
}

static inline void xeno_win_hash_close(XenoWinHash *ctx)
{
    if (ctx->hash)
        BCryptDestroyHash(ctx->hash);
    if (ctx->algorithm)
        BCryptCloseAlgorithmProvider(ctx->algorithm, 0);
}
#endif
