#!/usr/bin/env python3
"""ctypes-path test: load the extracted dylib with ctypes, compile
hello.c through the exported tinyc_main, and confirm the Python
process survives the call (the library form must not exit the host).

Usage: ctypes_test.py <dylib> <tcc-lib-path> <hello.c> <output-exe>
Prints CTYPES-ALIVE and exits 0 on success.
"""
import ctypes
import os
import sys


def main():
    dylib, tcclib, hello, out = sys.argv[1:5]
    lib = ctypes.CDLL(dylib)
    lib.tinyc_main.restype = ctypes.c_int
    lib.tinyc_main.argtypes = [ctypes.c_int,
                               ctypes.POINTER(ctypes.c_char_p)]

    def call_main(args):
        enc = [a.encode() for a in args]
        argv = (ctypes.c_char_p * (len(enc) + 1))(*enc, None)
        return lib.tinyc_main(len(enc), argv)

    # The -dumpversion path used to end in exit(0) before patching;
    # reaching the next line proves the process survived.
    rc = call_main(["tinyc", "-dumpversion"])
    print("ctypes tinyc_main(-dumpversion) rc:", rc)
    if rc != 0:
        print("CTYPES-FAIL: -dumpversion")
        return 1

    rc = call_main(["tinyc", "-B", tcclib, "-o", out, hello])
    print("ctypes tinyc_main(compile hello.c) rc:", rc)
    if rc != 0 or not os.path.exists(out):
        print("CTYPES-FAIL: compile")
        return 1

    print("CTYPES-ALIVE")
    return 0


if __name__ == "__main__":
    sys.exit(main())
