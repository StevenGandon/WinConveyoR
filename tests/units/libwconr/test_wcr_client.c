#include <criterion/criterion.h>
#include <stdint.h>
#include <string.h>

#include "wcr_client.h"

#define TEST_SEND_CHUNK_SIZE 3
#define TEST_PAYLOAD "partial write"

static unsigned char sent_bytes[WCR_HEADER_SIZE + sizeof(TEST_PAYLOAD) - 1];
static size_t sent_size;
static size_t send_calls;

#define send mock_send
#define wcr_send wcr_send_with_mock

#include "../../../lib/libwconr/src/wcr_client.c"

#undef wcr_send
#undef send

ssize_t mock_send(int sockfd, const void *buffer, size_t length, int flags)
{
    size_t chunk_size = length < TEST_SEND_CHUNK_SIZE ? length : TEST_SEND_CHUNK_SIZE;

    (void)sockfd;
    (void)flags;

    memcpy(sent_bytes + sent_size, buffer, chunk_size);

    sent_size += chunk_size;

    send_calls++;

    return (ssize_t)chunk_size;
}

Test(wcr_send, retries_until_header_and_payload_are_fully_sent)

{
    const unsigned char expected_header[WCR_HEADER_SIZE] = {
        0xff, 0xc4, 0x07, 0xec,
        0x12, 0x34,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d
    };
    wcr_conn connection = {.sockfd = 42};
    wcr_msg *message = wcr_msg_new(TEST_PAYLOAD, 0x1234);

    sent_size = 0;
    send_calls = 0;

    cr_assert_not_null(message);
    cr_assert_eq(wcr_send_with_mock(&connection, message), 0);
    cr_assert_gt(send_calls, 2);
    cr_assert_eq(sent_size, sizeof(sent_bytes));
    cr_assert_arr_eq(sent_bytes, expected_header, WCR_HEADER_SIZE);
    cr_assert_arr_eq(
        sent_bytes + WCR_HEADER_SIZE,
        TEST_PAYLOAD,
        sizeof(TEST_PAYLOAD) - 1
    );

    wcr_msg_free(message);
}