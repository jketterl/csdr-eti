#pragma once

#include <cstdint>
#include "dab.hpp"

void fic_depuncture(uint8_t *obuf, uint8_t *inbuf);
void uep_depuncture(uint8_t *obuf, uint8_t *inbuf, struct subchannel_info_t *s, int* len);
void eep_depuncture(uint8_t *obuf, uint8_t *inbuf, struct subchannel_info_t *s, int* len);
