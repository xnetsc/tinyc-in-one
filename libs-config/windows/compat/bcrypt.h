/* Minimal <bcrypt.h> for the bundled win32 header set.
   The bundled headers predate the CNG API surface; the only
   consumer in the bundled libraries is mbedTLS's entropy
   source, which calls BCryptGenRandom with the system
   preferred RNG. Only that entry point and its constants are
   declared here. The matching import definition is bcrypt.def,
   staged next to the other win32 .def files. */
#ifndef BCRYPT_H_BUNDLED_MINIMAL
#define BCRYPT_H_BUNDLED_MINIMAL

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef LONG NTSTATUS;
typedef PVOID BCRYPT_ALG_HANDLE;

#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002

NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE hAlgorithm,
                                PUCHAR pbBuffer, ULONG cbBuffer,
                                ULONG dwFlags);

#ifdef __cplusplus
}
#endif

#endif /* BCRYPT_H_BUNDLED_MINIMAL */
