/* Test CA and localhost server certificate for the default
   libraries battery (tests/default_libs_net.c). Self-signed
   test material only, generated for this purpose; the CA
   signs the server certificate (SAN: IP 127.0.0.1, DNS
   localhost). */
#ifndef DEFAULT_LIBS_CERT_H
#define DEFAULT_LIBS_CERT_H

static const char TEST_CA_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDETCCAfmgAwIBAgIUCfzCW2fW80iz1gI+223lwWg8uyEwDQYJKoZIhvcNAQEL\n"
    "BQAwGDEWMBQGA1UEAwwNdGlueWMtdGVzdC1jYTAeFw0yNjExMjgwNDA2MzlaFw0z\n"
    "NjExMjUwNDA2MzlaMBgxFjAUBgNVBAMMDXRpbnljLXRlc3QtY2EwggEiMA0GCSqG\n"
    "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCl7DS4n21/dSjiQaSh9QoCP17ziuHwqUUL\n"
    "RMy6uPCLod1IHCcXsvRSKJ5ASw/EWqk7N+iPN3SrbqfhSbNyC3FvyDT5PH/z0WDG\n"
    "v7Dg6zsXgn/fROkldZwfFolFldWceS7rPNR/k1vpYsiP/mTKhNWGkFQKW6YYZyYe\n"
    "yoXbYY4Vyyesv7iN+b6Q5NNKFiYndsJ1dV/vMoMcdPTfQpOLinLBRY8FmdawaU/s\n"
    "yKy5Z99R7OPx5tYQWMW+/9R8I/7IlYJaophvauWG1bs2Qm5hgg8UVnMHj+NoWASQ\n"
    "ndO+WWMfZc9/axeDROQF/9C0bERnE6mgMcOS+owGLg0/RX4Muc+RAgMBAAGjUzBR\n"
    "MB0GA1UdDgQWBBR1109DoW4HeN98/sRXA0V2KrO2tjAfBgNVHSMEGDAWgBR1109D\n"
    "oW4HeN98/sRXA0V2KrO2tjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQBZy+I231POsYjASMihBuY7kcDjg858cUyb+a+Gf8rVNaPjvD88tFmVqxzQ\n"
    "2gMsripakHeyfTGdiW1pLe26i/sdH+drnnmd5ktAO6rfgdhuft59g7uLh//49BZO\n"
    "beGFyffO0JlGTIymne8TbmgFSTrd13PzSF6ayDo2xdoApEgbssc32oBUzK7CA7L8\n"
    "S0HX/2dn/uT4Vm7+9zm/JWcxn3dLKm9hQtE1L0NVBf/fiOqK0ve2F2mlxPo6QlMv\n"
    "PId7PMMyM1xyx7077zlJe9DLo1n2Qnzd8X+fcIo8ll1UiFTBCTy/U/NgUbS0ZeF0\n"
    "sd88ZF8IJQLAg10yCRnmhPceHauP\n"
    "-----END CERTIFICATE-----\n";

static const char TEST_SRV_CERT_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDGDCCAgCgAwIBAgIUKd1/FiRygy+Gqj025J7cdeHG/ucwDQYJKoZIhvcNAQEL\n"
    "BQAwGDEWMBQGA1UEAwwNdGlueWMtdGVzdC1jYTAeFw0yNjExMjgwNDA2NDBaFw0z\n"
    "NjExMjUwNDA2NDBaMBQxEjAQBgNVBAMMCWxvY2FsaG9zdDCCASIwDQYJKoZIhvcN\n"
    "AQEBBQADggEPADCCAQoCggEBAL/k2MgTrjBRjVuQA7lD4xm5yVgTEdIFMIekDD50\n"
    "FFOKosZUjgSY1ev51yhpqtEynV5vM3r4HmGjJrD3gVOJwRS4NRHpPXb3hsFxBEpU\n"
    "GA6tg9byAOyAXhncfPW2QaE+HYiVTc/er8S7PkvnNXYbdB2TMZM6uLhnk8+dzcFs\n"
    "Hy59Dw2aeeTlrwC0VAAp+nCDyp8m0LsDCdIWJA+kzMb4yh1SSVXNr1SWvV0rswmy\n"
    "WgjJ8hRntrpoNTgLYE1VIdDquziT/G3unPvKQYcWcDERiczTsaNQFycWzCiTrYyO\n"
    "UrNoK0T3/Vm2n3XRCpcnTWBglPeL9K7ux3sQnOLUrP14ENECAwEAAaNeMFwwGgYD\n"
    "VR0RBBMwEYcEfwAAAYIJbG9jYWxob3N0MB0GA1UdDgQWBBTQ5vbvB5WiYJJr0JBE\n"
    "zC02CENkOTAfBgNVHSMEGDAWgBR1109DoW4HeN98/sRXA0V2KrO2tjANBgkqhkiG\n"
    "9w0BAQsFAAOCAQEAXrt2uk7pe5UibRWxGfEEVBGbx6D359ChgirLDuoUMsZxYCke\n"
    "yRB8kyDWhn5ecG3WocF/bMuf8zyw3DeqgPbCtHwoR8lepq2FZqVUTRKumeIZBogU\n"
    "ebgmILz8LM8I6j99UcpdmBVOpdm4DM5nkUuWKgHdDmCOCOAVuYmYpiQ3uas7e4+7\n"
    "6zI0zochdInBsked2NjysP0hZKEVbAZLc1nYXxuPScRnCcAqx/bSqR1beasOzu8e\n"
    "gCC0ld+v6Quq8b06BH0o/NKbtA9av6WUkb/b/7BBYmei5oKMh28exKdD2hvuxZc6\n"
    "XpANM1nsCGjluSuW233rlAorwZDqT5Dd7FLphA==\n"
    "-----END CERTIFICATE-----\n";

static const char TEST_SRV_KEY_PEM[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEuwIBADANBgkqhkiG9w0BAQEFAASCBKUwggShAgEAAoIBAQC/5NjIE64wUY1b\n"
    "kAO5Q+MZuclYExHSBTCHpAw+dBRTiqLGVI4EmNXr+dcoaarRMp1ebzN6+B5hoyaw\n"
    "94FTicEUuDUR6T1294bBcQRKVBgOrYPW8gDsgF4Z3Hz1tkGhPh2IlU3P3q/Euz5L\n"
    "5zV2G3QdkzGTOri4Z5PPnc3BbB8ufQ8Nmnnk5a8AtFQAKfpwg8qfJtC7AwnSFiQP\n"
    "pMzG+ModUklVza9Ulr1dK7MJsloIyfIUZ7a6aDU4C2BNVSHQ6rs4k/xt7pz7ykGH\n"
    "FnAxEYnM07GjUBcnFswok62MjlKzaCtE9/1Ztp910QqXJ01gYJT3i/Su7sd7EJzi\n"
    "1Kz9eBDRAgMBAAECgf9JHQxxlXRx4BFdPSrmXIGWZUzRzDTP6ipGotlPG1vq85rJ\n"
    "wyYHXOyLaBGNTzBazHhmuwxN3XkSFqHp3+jmlF2ltWsm3agDXzffPvgWthKI9cbk\n"
    "lKJfE5QjMeRWNUkoaQJSkj6dxu5GAcoHaT7HS6RJTolm8KG4q1fedjyYHsgpEYJD\n"
    "PEW0wg1McboUOKak+70XM8lnBY9IquEr6U0l3JQL1G5YKLocw/F5/4tLmGMK3KKw\n"
    "ysU9hRcRUyp1Wu6yYN1GB/z5WmhoX/+XhV0JQpzOt/TB0D3IHy0lsPKaiL6QP0Xb\n"
    "2nVX4yJ0ljeG/FoQCnaNul3sRXGEB6jsAqERfRkCgYEA5N2OpZHntbVhAYLSFu+O\n"
    "opBGj9WpD/+FXJV4jugvg1yx5MVtrAD1bLrDIdHDpqcsoW52r7IQsgJdmq9O3v42\n"
    "ayu30FlfGm3cdOOTOdkMUp53Q47NA71q4QoLueMVdzAJT1d3HRsDgUPlj7FRLOHh\n"
    "CjGPJPSXJDpZntS2n9jYuV0CgYEA1qUknlvw6Jxhy6GKdtTn+RgcrYpcpbeDfWoi\n"
    "ZFkwJJzOYeb1h+O9GCYNfjvRgoSabvSbJiCePlI/S3Yg2H8pk0Kfb2myrJj/Q02w\n"
    "5FwoXKPBQaGi1lNaxZ3oVtTgqgUms/da/BL1rrJPVrHCE9/IpbdCx+41ZY6hHXdj\n"
    "amx9GgUCgYBmgj4kNsfcAZOjg57qGv5+HbUdM4v32HE5WNGY4LJ3Wgtr9mn7Aqrq\n"
    "unQwL648dOBupfWPGGiV8q1OXXssMhzZv8kjDwSj59U4nb4RyP/V+ZhdsBZvuGWo\n"
    "ykEaT3YL3n4tBXYamUDmoyytQbSkL8AGOtBsswkHdwMxOwFZc10PGQKBgAWpq7Kl\n"
    "2FA6t/XIWD/UVB5lPXRynwkcM1LetvOQys1cebFph3bDUlzNvDNlXpyj5PMibult\n"
    "mN8qClNyVWGA0Vb6PfyRfE5DZxCefxk3OTVgeJvDqOkgn9dw2A1aZmb15dR/pR3b\n"
    "XjHysFcoL2eY0tFUaWWe6je4sJV3bd8D57UFAoGBALSL+E42wAbY5kYFM/glSeAu\n"
    "w5e87MvwQoOQJXNYSPLpvgpA48m/4jx1uhv/JB7NekVyebES49rAb+v6iEZsLaJo\n"
    "i5cb62MSSZwDktcSG/l+GMZlTkJdEPlmIdAcr8okOUgWbjBlSQ29VAXEgdkBDdT8\n"
    "su/rXu96Sh03SI9+Ud08\n"
    "-----END PRIVATE KEY-----\n";

#endif
