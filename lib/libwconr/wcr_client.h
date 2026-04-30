#ifndef WCR_CLIENT_H_
    #define WCR_CLIENT_H_

    #define WCR_MAGIC 0xffc407ec
    #define WCR_DEFAULT_PORT 1674

    typedef struct wcr_conn_s {
        int sockfd;
        char session_id[64];
    } wcr_conn;

    wcr_conn *wcr_open(const char *host, int port);
    void wcr_close(wcr_conn *conn);
    int wcr_auth(wcr_conn *conn, const char *access_key);
    char *wcr_send_recv(wcr_conn *conn, const char *json_payload);

    char *wcr_get_listing(wcr_conn *conn);
    char *wcr_get_hash(wcr_conn *conn);
    char *wcr_get_package_listing(wcr_conn *conn, const char *package_name);
    char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash);

#endif /* !WCR_CLIENT_H_ */
