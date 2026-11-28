/* Minimal <intsafe.h> for the bundled win32 header set.
   The bundled headers do not ship intsafe.h. mbedTLS's
   entropy_poll.c includes it on Windows but uses none of its
   checked-arithmetic functions, so an empty header with the
   standard include guard is sufficient for the bundled
   libraries. */
#ifndef INTSAFE_H_BUNDLED_MINIMAL
#define INTSAFE_H_BUNDLED_MINIMAL
#endif /* INTSAFE_H_BUNDLED_MINIMAL */
