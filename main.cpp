#include <iostream>

#include <debug-levels.h>
#include <dot2.h>
#include <conf.h>
#include <dot3-wsmp.h>
#include <dot3-wme-mib.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <ctime>
#include <chrono>

#include "utils.h"
#include "aerolink_dot2.h"

#define BSM_OPTS_WSM_PSID_DEFAULT   (0x10)
#define P1609_CONFIG_DEF_PRIORITY  2
#define P1609_RX_BUF_SIZE 4096
/// MAC address length in uint8_t
#define WSM_HDR_MAC_LEN 6

/// MAC address defined to 0xFFFFFFFFFFFF in Tx
#define WSM_HDR_MAC_TX_FILL  0xFF

/// WSM header expiry time=0, never expire
#define WSM_HDR_DEFAULT_EXPIRY_TIME 0

extern int D_LEVEL;

static uint8_t p2pcd_callback(void *userData, uint32_t pduLength, uint8_t *pdu) {

    //////  Output Timestamp /////////
    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
    std::cout << "Timestamp Before Callback: " << ms.count() << std::endl;
    //////////////////////////////////
    
    printf("p2pcd_callback, len: %u\n", pduLength);
    print_hex(pdu, pduLength);

    auto buf = (uint8_t *)malloc((pduLength + WSMP_HDR_SIZE) * sizeof(uint8_t));
    memcpy(buf+WSMP_HDR_SIZE, pdu, pduLength);
    size_t buf_len = 0;

    struct Dot3WSMPHdr *pHdr = (struct Dot3WSMPHdr *)buf;
    memset(pHdr->Tx.DA, WSM_HDR_MAC_TX_FILL, WSM_HDR_MAC_LEN);
    pHdr->Tx.Priority   = P1609_CONFIG_DEF_PRIORITY;
    pHdr->Tx.ExpiryTime = WSM_HDR_DEFAULT_EXPIRY_TIME;
    pHdr->Version       = DOT3_WSMP_VERSION_3;
    pHdr->ChannelNumber = 178;
    pHdr->DataRate      = DOT3_WSMP_WSM_DATARATE_6MBPS;
    pHdr->TxPower       = 32;
    pHdr->PSID          = htonl(0x20);
    pHdr->Length        = htons(pduLength);
    pHdr->HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
    buf_len = WSMP_HDR_SIZE + pduLength;

    int *fd = (int *)userData;
    int res = sendto(*fd, buf, buf_len, 0, nullptr, 0);
    if (res < 0) {
        std::cerr << "Error sending: " << errno << std::endl;
        res = -errno;
        return res;
    }
    return 0;
}

int init_socket() {
    int fd;
    int res;
    struct Dot3WSMPSockAddr sockAddr{};

    // open socket
    fd = socket(AF_IEEE1609_WSMP, SOCK_DGRAM, PROTO_IEEE1609DOT3_WSMP);
    if (fd < 0) {
        perror("socket failed: ");
        return fd;
    }

    // set address
    sockAddr.Family = AF_IEEE1609_WSMP;
    memset(sockAddr.Hdr.Tx.DA, 0, sizeof(sockAddr.Hdr.Tx.DA));
    sockAddr.Hdr.Version       = DOT3_WSMP_VERSION_3;
    sockAddr.Hdr.Tx.Priority   = 2;
    sockAddr.Hdr.Tx.ExpiryTime = 0;
    sockAddr.Hdr.ChannelNumber = 178;
    sockAddr.Hdr.DataRate      = DOT3_WSMP_WSM_DATARATE_6MBPS;
    sockAddr.Hdr.TxPower       = 32;
    sockAddr.Hdr.PSID          = htonl(DOT3_WSMP_PSID_ALL); // Promiscuous receive
    sockAddr.Hdr.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr

    res = bind(fd, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
    if (res < 0) {
        perror("bind failed: ");
        close(fd);
        return res;
    }

    // success
    return fd;
}

void* sender(void* fd){

    int debug = 1;
    int res = -ENOSYS;
    auto test_data = (uint8_t *)malloc(128 * sizeof(uint8_t));
    memset(test_data, 'a', 128);

    while(true){

        auto buf = (uint8_t *)malloc(2048 * sizeof(uint8_t));
        size_t buf_len = 0;
        auto signed_data = buf + WSMP_HDR_SIZE;
        size_t signed_len = 2048 - WSMP_HDR_SIZE;
        //uint8_t *to_be_signed = (uint8_t *)malloc(128 * sizeof(uint8_t));
        //size_t to_be_signed_len = 128;
        // uint8_t *to_be_signed = nullptr;
        // size_t to_be_signed_len = 128;
        struct Dot3WSMPHdr *pHdr = nullptr;

        if (dot2_sign(test_data, 128, signed_data, &signed_len) != 0) {
            std::cerr << "Error signing" << std::endl;
            free(test_data);
            free(buf); 
            pthread_exit((void *)-1);
        }

        std::cout << "Signed Data Length: " << signed_len << std::endl;
    //    if (dot2_verify(signed_data, signed_len, (const uint8_t **)&to_be_signed, &to_be_signed_len) != 0) {
    //        std::cerr << "Error verifying" << std::endl;
    //    } else {
    //        std::cout << "To Be Signed Data Length: " << to_be_signed_len << std::endl;
    //    }
        pHdr = (struct Dot3WSMPHdr *)buf;
        memset(pHdr->Tx.DA, WSM_HDR_MAC_TX_FILL, WSM_HDR_MAC_LEN);
        pHdr->Tx.Priority   = P1609_CONFIG_DEF_PRIORITY;
        pHdr->Tx.ExpiryTime = WSM_HDR_DEFAULT_EXPIRY_TIME;
        pHdr->Version       = DOT3_WSMP_VERSION_3;
        pHdr->ChannelNumber = 178;
        pHdr->DataRate      = DOT3_WSMP_WSM_DATARATE_6MBPS;
        pHdr->TxPower       = 32;
        pHdr->PSID          = htonl(0x20);
        pHdr->Length        = htons(signed_len);
        pHdr->HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
        buf_len = WSMP_HDR_SIZE + signed_len;

        // send data
        
        res = sendto(*(int*)fd, buf, buf_len, 0, nullptr, 0);
        if (res < 0) {
            std::cerr << "Error sending" << std::endl;
            res = -errno;
            free(test_data);
            free(buf);
            pthread_exit((void *)-1);
        }
        usleep(100000);
        std::cout << "Keep Sending ..." << std::endl;
        free(buf);
    }


    std::cout << "Free test_data" << std::endl;
    free(test_data);

    return 0;
}

void* receiver(void* fd){

    int debug = 1;
    int res = -ENOSYS;
    uint8_t *pBuf = nullptr;
    uint8_t *pData = nullptr;
    size_t bufLen = P1609_RX_BUF_SIZE;
    struct Dot3WSMPSockAddr sockAddr{};
    socklen_t addrLen = sizeof(sockAddr);
    uint8_t *toBeSigned = nullptr;
    size_t toBeSignedLen = 2048;

    pBuf = static_cast<uint8_t *>(calloc(bufLen, sizeof(uint8_t)));
    if (pBuf == nullptr) {
        perror("calloc failed: ");
        pthread_exit((void *)-1);
    }

    while (true) {
        // receive
        std::cout << "receiving ... " << std::endl;
        res = recvfrom(*(int*)fd, pBuf, bufLen, 0, (struct sockaddr *)&sockAddr, &addrLen);
        if (res < 0) {
            perror("recvfrom failed: ");
            res = -errno;
            break;
        }

        // nothing
        if (res == 0)
            continue;

        pData = pBuf + WSMP_HDR_SIZE;

        // print
        if (debug) {
            pBuf[res] = '\0';
            printf("Received %d bytes (, including %d bytes of header):\n", res, WSMP_HDR_SIZE);
            print_hex(pBuf, res);
        }

        //////  Output Timestamp /////////
        std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        );
        std::cout << "Timestamp Before Verifying: " << ms.count() << std::endl;
        //////////////////////////////////

        int rv = dot2_verify(pData, res - WSMP_HDR_SIZE - 4, (const uint8_t **)&toBeSigned, &toBeSignedLen);
        if (rv != 0) {
            if (rv == -2){
                int rvv = p2p_process(res - WSMP_HDR_SIZE - 4, pData);
                if (rvv != 0){
                    std::cout << "p2p_process error, return is: " << rvv << std::endl;
                }
            }
            perror("Error dot2_verify: ");
            //////  Output Timestamp /////////
            ms = std::chrono::duration_cast< std::chrono::milliseconds >(
                std::chrono::system_clock::now().time_since_epoch()
            );
            std::cout << "Timestamp After Verifying: " << ms.count() << std::endl;
            //////////////////////////////////
        }

        printf("==========\n");
    }

    free(pBuf);
    return 0;

}

int main() {

    int fd;
    int res = -ENOSYS;
    pthread_t tids[2];

    uint8_t dataForTest[379] = {
        0x03, 0x81, 0x00, 0x40, 0x03, 0x80, 0x81, 0x80, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x48, 0x01, 0x20, 0x00, 0x01, 0xb7, 0x7f, 0xf6, 0x70, 0x63, 0xe8, 0x15, 0x8a, 0xf7, 0x81, 0x01, 0x01, 0x80, 0x03, 0x00, 0x80, 0xc0, 0x21, 0xdc, 0x62, 0x6d, 0x15, 0x8a, 0xf7, 0x30, 0x80, 0x80, 0x00, 0x08, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x11, 0x22, 0x33, 0x44, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x12, 0xab, 0x34, 0x00, 0x00, 0x1c, 0xcd, 0x95, 0x80, 0x84, 0x00, 0x30, 0x00, 0x01, 0x01, 0x80, 0x01, 0x20, 0x80, 0x00, 0x80, 0x80, 0x83, 0xaf, 0x6c, 0x79, 0x17, 0xe2, 0xa3, 0xf8, 0x74, 0x0b, 0x34, 0xd5, 0x31, 0x46, 0x06, 0x90, 0xb9, 0x00, 0xb8, 0xac, 0x70, 0xa4, 0x10, 0xf9, 0x2d, 0x14, 0xed, 0xc3, 0x7a, 0xe2, 0xb8, 0x52, 0xf7, 0x80, 0x83, 0xd2, 0x65, 0x5d, 0xb9, 0x7f, 0xb9, 0x03, 0x24, 0x16, 0x72, 0xd9, 0x0f, 0x8b, 0xe3, 0x4c, 0x5f, 0x90, 0xa2, 0x6b, 0x88, 0x81, 0x83, 0xbb, 0x93, 0x06, 0x01, 0x34, 0xcd, 0x06, 0x76, 0xd8, 0x2b, 0x71, 0x5a, 0xc4, 0xda, 0xa6, 0xbf, 0x9c, 0x7c, 0x71, 0x01, 0x64, 0x08, 0x1a, 0x0e, 0x77, 0x75, 0x8f, 0xea, 0xe8, 0x2c, 0x99, 0xa2, 0x20, 0x05, 0x2c, 0xad, 0x32, 0x59, 0x89, 0x48, 0x5a, 0xcd, 0x80, 0x82, 0x34, 0xb2, 0x20, 0x64, 0x50, 0x60, 0x97, 0x3a, 0x54, 0x1c, 0x78, 0x83, 0x90, 0x63, 0x57, 0xc5, 0x19, 0x93, 0xa4, 0x49, 0xfc, 0x16, 0x07, 0xda, 0x4f, 0x4c, 0x56, 0x15, 0xa2, 0x2b, 0x99, 0x7e, 0x34, 0xdb, 0x35, 0x4b, 0x91, 0x0e, 0x9f, 0x73, 0x89, 0xd7, 0x8b, 0x98, 0x26, 0x0f, 0x99, 0x2a, 0x9a, 0xdb, 0xcc, 0x5f, 0x82, 0x9c, 0x7d, 0x41, 0x08, 0x7b, 0x0e, 0x3e, 0x6d, 0xfa, 0x08, 0x08
    };
    

    D_LEVEL_INIT();

    if (dot2_init() != 0) {
        return EXIT_FAILURE;
    }

    p2p_setCallback(&fd, p2pcd_callback);

    fd = init_socket();
    if (fd < 0) {
        res = -errno;
        return res;
    }

    int rets = pthread_create(&tids[0], NULL, sender, (void*)&fd);

    if (rets != 0)
    {
       std::cout << "pthread_create error: error_code=" << rets << std::endl;
    }

    int retr = pthread_create(&tids[1], NULL, receiver, (void*)&fd);

    if (retr != 0)
    {
       std::cout << "pthread_create error: error_code=" << retr << std::endl;
    }

    pthread_exit(NULL);

    close(fd);

    return EXIT_SUCCESS;
}



