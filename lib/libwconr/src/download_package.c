#include "libwconr.h"
#include "libwconr_private.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __WIN32
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
#endif

static int
write_to_file(const unsigned char *buffer, size_t size, const unsigned char *location)
{
#ifdef __WIN32
    HANDLE file = CreateFileA((LPCSTR)location, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;

    if (file == INVALID_HANDLE_VALUE)
        return -1;

    if (!WriteFile(file, buffer, (DWORD)size, &written, NULL)) {
        CloseHandle(file);
        return -1;
    }

    CloseHandle(file);
    return 0;
#else
    int fd = open((const char *)location, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ssize_t written = 0;

    if (fd < 0)
        return -1;

    written = write(fd, buffer, size);
    close(fd);

    return (written < 0) ? -1 : 0;
#endif
}

int
download_package(unsigned char *http_address, unsigned char *location)
{
    printf("%s, %s\n", http_address, location);
    struct _http_connection_s *connection = NULL;
    struct _http_request_parser_s *request = NULL;
    struct _http_response_parser_s *response = NULL;
    unsigned char *buffer = NULL;
    size_t buffer_size = 0;
    ssize_t bytes_read = 0;
    int result = -1;
    FILE *temp = NULL;
    size_t total_size = 0;
    unsigned char *content = NULL;
    const unsigned char *content_length = NULL;
    size_t initial_buffer_size = 4096;
    size_t content_length_value = 0;

    if (!http_address || !location)
        return -1;

    printf("step2\n");

    connection = new_http_connection((const char *)http_address, 80);
    if (!connection)
        return -1;
    printf("step3\n");

    request = new_http_request_parser(connection);
    if (!request) {
        end_http_connection(connection);
        return -1;
    }
    printf("step4\n");

    request->method = (unsigned char *)strdup("GET");
    const char *last_slash = strrchr((const char *)http_address, '/');
    request->route = (unsigned char *)strdup(last_slash ? last_slash : "/");
    request->protocol = (unsigned char *)strdup("HTTP");
    request->version = 0x0101;

    set_header(&request->headers, (const unsigned char *)"Host", http_address);
    set_header(&request->headers, (const unsigned char *)"Connection", (const unsigned char *)"close");

    request_ressource(request);

    response = new_http_response_parser(connection);
    if (!response) {
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step5\n");

    fetch_response(response);

    if (response->status_code != 200) {
        end_http_response_parser(response);
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step6\n");

    temp = tmpfile();
    if (!temp) {
        end_http_response_parser(response);
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step7\n");

    content_length = get_header((const struct _http_header_s * const *)response->headers, (const unsigned char *)"Content-Length");
    if (content_length) {
        content_length_value = (size_t)atoll((const char *)content_length);
        buffer_size = (content_length_value < initial_buffer_size) ? content_length_value : initial_buffer_size;
    } else {
        buffer_size = initial_buffer_size;
    }

    buffer = (unsigned char *)malloc(buffer_size);
    if (!buffer) {
        fclose(temp);
        end_http_response_parser(response);
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step8\n");

    while ((bytes_read = get_chunk(connection, buffer_size, buffer)) > 0) {
        if (content_length && (total_size + (size_t)bytes_read) > content_length_value) {
            bytes_read = (ssize_t)(content_length_value - total_size);
        }
        if (bytes_read < 0 || fwrite(buffer, 1, (size_t)bytes_read, temp) != (size_t)bytes_read) {
            fclose(temp);
            end_http_response_parser(response);
            end_http_request_parser(request);
            end_http_connection(connection);
            return -1;
        }
        total_size += (size_t)bytes_read;
    }
    printf("step9\n");

    content = (unsigned char *)malloc(total_size);
    if (!content) {
        fclose(temp);
        end_http_response_parser(response);
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step10\n");

    rewind(temp);
    if (fread(content, 1, total_size, temp) != total_size) {
        free(content);
        fclose(temp);
        end_http_response_parser(response);
        end_http_request_parser(request);
        end_http_connection(connection);
        return -1;
    }
    printf("step11\n");

    result = write_to_file(content, total_size, location);

    free(content);
    fclose(temp);
    end_http_response_parser(response);
    end_http_request_parser(request);
    end_http_connection(connection);

    return result;
}
