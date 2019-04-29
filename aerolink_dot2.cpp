//
// Created by h1994st on 6/21/18.
//
#include "aerolink_dot2.h"

#include <iostream>

#define SECURITY_CONTEXT_FILE "exampleobe.wsc"

SecurityContextC p_sc;

int dot2_init() {
    AEROLINK_RESULT ret;

    ret = securityServices_initialize();
    if (ret != WS_SUCCESS) {
        std::cerr << "SecurityServices_initialize() FAIL result " << ws_errid(ret) << std::endl;
        return -1;
    }

    ret = sc_open(SECURITY_CONTEXT_FILE, &p_sc);
    if (ret != WS_SUCCESS) {
        std::cerr << "sc_open(" << SECURITY_CONTEXT_FILE <<  ") failed: " << ws_errid(ret) << std::endl;
        return -1;
    }

    std::cout << "Aerolink version " << securityServices_getVersion() << std::endl;

    return 0;
}

int dot2_deinit() {
    if (p_sc != nullptr) {
        sc_close(p_sc);
    }

    securityServices_shutdown();
}

int dot2_sign(uint8_t *p_to_be_signed, size_t to_be_signed_len,
              uint8_t *p_signed, size_t *p_signed_len) {
    AEROLINK_RESULT ret;
    SecuredMessageGeneratorC p_smg;
    SIGNER_TYPE_OVERRIDE sto = STO_AUTO;

    ret = smg_new(p_sc, &p_smg);
    if (ret != WS_SUCCESS) {
        std::cerr << "Error creating new SMG: " << ws_errid(ret) << std::endl;
        return -1;
    }

    ret = smg_sign(p_smg, 0x20, nullptr, 0, sto, p_to_be_signed, to_be_signed_len, nullptr, 0, p_signed, p_signed_len);
    if (ret != WS_SUCCESS) {
        std::cerr << "Failed to sign message with PSID 0x20: " << ws_errid(ret) << ", " << ws_strerror(ret) << std::endl;
        smg_delete(p_smg);
        return -1;
    }

    return 0;
}

int dot2_verify(uint8_t *p_signed, size_t signed_len,
                const uint8_t **pp_to_be_signed, size_t *p_to_be_signed_len) {
    AEROLINK_RESULT ret;
    MessageSecurityType cert_type;
    SecuredMessageParserC p_smp;

    ret = smp_new(p_sc, &p_smp);
    if (ret != WS_SUCCESS) {
        std::cerr << "Error creating new SMP: " << ws_errid(ret) << std::endl;
        return -1;
    }

    ret = smp_extract(p_smp, p_signed, signed_len, &cert_type, pp_to_be_signed, p_to_be_signed_len);
    if (ret != WS_SUCCESS) {
        std::cerr << "Error extracting message: " << ws_errid(ret) << std::endl;
        if (p_smp != nullptr) smp_delete(p_smp);
        return -2;
    }
    std::cout << "Message security type: " << cert_type << std::endl;

    // ret = smp_checkRelevance(p_smp);
    // if (ret != WS_SUCCESS) {
    //     std::cerr << "Error checking message relevance: " << ws_errid(ret) << std::endl;
    //     if (p_smp != nullptr) smp_delete(p_smp);
    //     return -1;
    // }

//    ret = smp_checkConsistency(p_smp);
//    if (ret != WS_SUCCESS) {
//        std::cerr << "Error checking message consistency: " << ws_errid(ret) << std::endl;
//        if (p_smp != nullptr) smp_delete(p_smp);
//        return -1;
//    }

    ret = smp_verifySignatures(p_smp, nullptr, 0);
    if (ret != WS_SUCCESS) {
        std::cerr << "Failed to verify signature: " << ws_errid(ret) << ", " << ws_strerror(ret) << std::endl;
        if (p_smp != nullptr) smp_delete(p_smp);
        return -1;
    } else {
        std::cout << "Validated!" << std::endl;
    }

    if (p_smp != nullptr) smp_delete(p_smp);

    return 0;
}
