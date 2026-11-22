#ifndef WCR_CRYPT_H_
    #define WCR_CRYPT_H_

    #include "wcr_client.h"
    #include <stddef.h>

    unsigned char *wcr_encrypt(EVP_PKEY *pubkey,
                               const unsigned char *plaintext, size_t plain_len,
                               size_t *out_len);

    unsigned char *wcr_decrypt(EVP_PKEY *privkey,
                               const unsigned char *ciphertext, size_t cipher_len,
                               size_t *out_len);

    EVP_PKEY *parse_pubkey_pem(const char *pem);
    EVP_PKEY *generate_rsa_keypair(void);
    char *serialize_pubkey_pem(EVP_PKEY *key);

#endif /* !WCR_CRYPT_H_ */
