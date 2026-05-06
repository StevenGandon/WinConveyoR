#include "wcr_crypt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

unsigned char *wcr_encrypt(EVP_PKEY *pubkey,
                           const unsigned char *plaintext, size_t plain_len,
                           size_t *out_len)
{
    (void)pubkey; (void)plaintext; (void)plain_len;
    *out_len = 0;
    return NULL;
}

unsigned char *wcr_decrypt(EVP_PKEY *privkey,
                           const unsigned char *ciphertext, size_t cipher_len,
                           size_t *out_len)
{
    (void)privkey; (void)ciphertext; (void)cipher_len;
    *out_len = 0;
    return NULL;
}

EVP_PKEY *parse_pubkey_pem(const char *pem) { (void)pem; return NULL; }
EVP_PKEY *generate_rsa_keypair(void) { return NULL; }
char *serialize_pubkey_pem(EVP_PKEY *key) { (void)key; return NULL; }

#else

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>

static unsigned char *rsa_encrypt_key(EVP_PKEY *pubkey,
                                      const unsigned char *aes_key,
                                      size_t *out_len)
{
    unsigned char *encrypted_key;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pubkey, NULL);

    *out_len = 0;
    if (!ctx)
        return NULL;
    if (EVP_PKEY_encrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_encrypt(ctx, NULL, out_len, aes_key, WCR_AES_KEY_SIZE) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    encrypted_key = malloc(*out_len);
    if (!encrypted_key) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_encrypt(ctx, encrypted_key, out_len, aes_key, WCR_AES_KEY_SIZE) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        free(encrypted_key);
        return NULL;
    }
    EVP_PKEY_CTX_free(ctx);
    return encrypted_key;
}

static unsigned char *aes_encrypt_data(const unsigned char *aes_key,
                                       const unsigned char *iv,
                                       const unsigned char *plaintext, size_t plain_len,
                                       size_t *out_len)
{
    unsigned char *ciphertext;
    int len = 0;
    int final_len = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    *out_len = 0;
    if (!ctx)
        return NULL;
    ciphertext = malloc(plain_len + EVP_MAX_BLOCK_LENGTH);
    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) != 1 ||
        EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, (int)plain_len) != 1 ||
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        return NULL;
    }
    EVP_CIPHER_CTX_free(ctx);
    *out_len = (size_t)(len + final_len);
    return ciphertext;
}

unsigned char *wcr_encrypt(EVP_PKEY *pubkey,
                           const unsigned char *plaintext, size_t plain_len,
                           size_t *out_len)
{
    unsigned char aes_key[WCR_AES_KEY_SIZE];
    unsigned char iv[WCR_AES_IV_SIZE];
    unsigned char *encrypted_key;
    size_t encrypted_key_len;
    unsigned char *ciphertext;
    size_t cipher_len;
    unsigned char *result;

    *out_len = 0;

    if (RAND_bytes(aes_key, WCR_AES_KEY_SIZE) != 1 ||
        RAND_bytes(iv, WCR_AES_IV_SIZE) != 1)
        return NULL;

    encrypted_key = rsa_encrypt_key(pubkey, aes_key, &encrypted_key_len);
    if (!encrypted_key)
        return NULL;

    ciphertext = aes_encrypt_data(aes_key, iv, plaintext, plain_len, &cipher_len);
    if (!ciphertext) {
        free(encrypted_key);
        return NULL;
    }

    *out_len = encrypted_key_len + WCR_AES_IV_SIZE + cipher_len;
    result = malloc(*out_len);
    if (!result) {
        free(encrypted_key);
        free(ciphertext);
        return NULL;
    }
    memcpy(result, encrypted_key, encrypted_key_len);
    memcpy(result + encrypted_key_len, iv, WCR_AES_IV_SIZE);
    memcpy(result + encrypted_key_len + WCR_AES_IV_SIZE, ciphertext, cipher_len);

    free(encrypted_key);
    free(ciphertext);
    return result;
}

static int rsa_decrypt_key(EVP_PKEY *privkey,
                           const unsigned char *encrypted_key,
                           unsigned char *aes_key_out)
{
    size_t aes_key_len = WCR_AES_KEY_SIZE;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(privkey, NULL);

    if (!ctx)
        return -1;
    if (EVP_PKEY_decrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_decrypt(ctx, aes_key_out, &aes_key_len, encrypted_key, WCR_RSA_KEY_BYTES) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }
    EVP_PKEY_CTX_free(ctx);
    return 0;
}

static unsigned char *aes_decrypt_data(const unsigned char *aes_key,
                                       const unsigned char *iv,
                                       const unsigned char *ciphertext, size_t cipher_len,
                                       size_t *out_len)
{
    unsigned char *plaintext;
    int len = 0;
    int final_len = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    *out_len = 0;
    if (!ctx)
        return NULL;
    plaintext = malloc(cipher_len + 1);
    if (!plaintext) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) != 1 ||
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, (int)cipher_len) != 1 ||
        EVP_DecryptFinal_ex(ctx, plaintext + len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    EVP_CIPHER_CTX_free(ctx);
    *out_len = (size_t)(len + final_len);
    plaintext[*out_len] = '\0';
    return plaintext;
}

unsigned char *wcr_decrypt(EVP_PKEY *privkey,
                           const unsigned char *ciphertext, size_t cipher_len,
                           size_t *out_len)
{
    unsigned char aes_key[WCR_AES_KEY_SIZE];
    const unsigned char *iv;
    const unsigned char *aes_data;
    size_t aes_data_len;

    *out_len = 0;

    if (cipher_len < WCR_RSA_KEY_BYTES + WCR_AES_IV_SIZE + 1) {
        fprintf(stderr, "[ERROR] wcr_decrypt: ciphertext too short\n");
        return NULL;
    }

    if (rsa_decrypt_key(privkey, ciphertext, aes_key) != 0)
        return NULL;

    iv = ciphertext + WCR_RSA_KEY_BYTES;
    aes_data = ciphertext + WCR_RSA_KEY_BYTES + WCR_AES_IV_SIZE;
    aes_data_len = cipher_len - WCR_RSA_KEY_BYTES - WCR_AES_IV_SIZE;

    return aes_decrypt_data(aes_key, iv, aes_data, aes_data_len, out_len);
}

EVP_PKEY *parse_pubkey_pem(const char *pem)
{
    EVP_PKEY *key;
    BIO *bio = BIO_new_mem_buf(pem, -1);

    if (!bio)
        return NULL;
    key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return key;
}

EVP_PKEY *generate_rsa_keypair(void)
{
    EVP_PKEY *key = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    if (!ctx ||
        EVP_PKEY_keygen_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, WCR_RSA_KEY_SIZE) <= 0 ||
        EVP_PKEY_keygen(ctx, &key) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    EVP_PKEY_CTX_free(ctx);
    return key;
}

char *serialize_pubkey_pem(EVP_PKEY *key)
{
    char *pem_data;
    char *escaped;
    size_t ei = 0;
    long pem_len;
    long i;
    BIO *bio = BIO_new(BIO_s_mem());

    if (!bio || PEM_write_bio_PUBKEY(bio, key) != 1) {
        BIO_free(bio);
        return NULL;
    }
    pem_len = BIO_get_mem_data(bio, &pem_data);

    escaped = malloc((size_t)pem_len * 2 + 1);
    if (!escaped) {
        BIO_free(bio);
        return NULL;
    }
    for (i = 0; i < pem_len; i++) {
        if (pem_data[i] == '\n') {
            escaped[ei++] = '\\';
            escaped[ei++] = 'n';
        } else {
            escaped[ei++] = pem_data[i];
        }
    }
    escaped[ei] = '\0';
    BIO_free(bio);
    return escaped;
}

#endif
