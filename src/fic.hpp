#pragma once

int crc16(unsigned char *buf, int len, int width);
struct tf_info_t fib_decode(struct tf_fibs_t *fibs, int nfibs);
void fic_decode(struct demapped_transmission_frame_t *tf);
void dump_tf_info(struct tf_info_t* info);
