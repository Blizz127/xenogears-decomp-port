/* glibc 2.43 gave acosf, asinf and sqrtf new default symbol versions, so a
 * port linked on a glibc 2.43 host would refuse to start on the older glibc
 * of the owner's handhelds ("version `GLIBC_2.43' not found").  PsyCross
 * calls these three.  build_port.sh links with --wrap for each name, which
 * routes every reference to the __wrap_ functions here; they call the
 * GLIBC_2.2.5 versions that every x86_64 glibc exports.  The executable
 * itself never defines acosf/asinf/sqrtf (a definition would capture the
 * versioned reference and recurse).  Same libm code behind the older tag. */
#if defined(__linux__)
#include <features.h> /* defines __GLIBC__ on glibc */
#endif
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBC__)

__asm__(".symver xg_compat_acosf, acosf@GLIBC_2.2.5");
__asm__(".symver xg_compat_asinf, asinf@GLIBC_2.2.5");
__asm__(".symver xg_compat_sqrtf, sqrtf@GLIBC_2.2.5");

extern float xg_compat_acosf(float x);
extern float xg_compat_asinf(float x);
extern float xg_compat_sqrtf(float x);

float __wrap_acosf(float x)
{
    return xg_compat_acosf(x);
}

float __wrap_asinf(float x)
{
    return xg_compat_asinf(x);
}

float __wrap_sqrtf(float x)
{
    return xg_compat_sqrtf(x);
}

#endif
