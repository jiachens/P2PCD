//
// Created by h1994st on 6/21/18.
//

#ifndef P2PCD_TESTING_AEROLINK_DOT2_H
#define P2PCD_TESTING_AEROLINK_DOT2_H

#include <cstdint>
#include <cstdlib>

#include <SecurityServicesC.h>
#include <SecurityContextC.h>
#include <SecuredMessageGeneratorC.h>
#include <SecuredMessageParserC.h>
#include <PeerToPeerC.h>
#include <ws_errno.h>

int dot2_init();

int dot2_deinit();

int dot2_sign(uint8_t *p_to_be_signed, size_t to_be_signed_len,
              uint8_t *p_signed, size_t *p_signed_len);
int dot2_verify(uint8_t *p_signed, size_t signed_len,
                const uint8_t **pp_to_be_signed, size_t *p_to_be_signed_len);

#endif //P2PCD_TESTING_AEROLINK_DOT2_H
