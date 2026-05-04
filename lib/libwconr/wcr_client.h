#ifndef WCR_CLIENT_H_
    #define WCR_CLIENT_H_

    #include <openssl/evp.h>

    #define WCR_MAGIC 0xffc407ec
    #define WCR_DEFAULT_PORT 1674
    #define WCR_HEADER_SIZE 14
    #define WCR_PAYLOAD_SMALL 256
    #define WCR_PAYLOAD_LARGE 512
    #define WCR_RSA_KEY_SIZE 2048
    #define WCR_RSA_KEY_BYTES (WCR_RSA_KEY_SIZE / 8)
    #define WCR_AES_KEY_SIZE 32
    #define WCR_AES_IV_SIZE 16
    #define WCR_JSON_OVERHEAD 128

    typedef struct wcr_conn_s {
        int sockfd;
        char session_id[64];
        EVP_PKEY *client_privkey;
        EVP_PKEY *server_pubkey;
        int encrypted;
    } wcr_conn;

    wcr_conn *wcr_open(const char *host, int port);
    void wcr_close(wcr_conn *conn);
    int wcr_handshake(wcr_conn *conn, const char *server_pubkey_pem);
    int wcr_auth(wcr_conn *conn, const char *access_key);
    char *wcr_send_recv(wcr_conn *conn, const char *json_payload);

    char *wcr_get_listing(wcr_conn *conn);
    char *wcr_get_hash(wcr_conn *conn);
    char *wcr_get_package_listing(wcr_conn *conn, const char *package_name);
    char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash);

#endif /* !WCR_CLIENT_H_ */
