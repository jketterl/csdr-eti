#pragma once

#include <cstdint>
#include <functional>

/* A demapped transmission frame represents a transmission frame in
   the final state before the FIC-specific and MSC-specific decoding
   stages.

   This format is used as the output of the hardware-specific drivers
   and the input to the processing stages which are common across all
   supported devices.
 */

/* The FIBs for one transmission frame, and the results of CRC checking */
struct tf_fibs_t {
  uint8_t ok_count;
  uint8_t FIB[12][32];    /* The actual FIB data, including CRCs */
  uint8_t FIB_CRC_OK[12]; /* 1 = CRC OK, 0 = CRC Error */ 
};

// treshold values for the FIB CRC check to detect signal lock
#define FIB_CRC_LOCK_VALUE_TRESHOLD 9
#define FIB_CRC_LOCK_COUNT_TRESHOLD 10

struct demapped_transmission_frame_t {
  uint8_t fic_symbols_demapped[3][3072];
  struct tf_fibs_t fibs;  /* The decoded and CRC-checked FIBs */
  uint8_t msc_symbols_demapped[72][3072];
};

struct subchannel_info_t {
  int id;  /* Or -1 if not active */
  int eepprot;
  int slForm;
  int uep_index;
  int start_cu;
  int size;
  int bitrate;
  int protlev;
  int ASCTy;
};

/* The information from the FIBs required to construct the ETI stream */
struct tf_info_t {
  uint16_t EId;           /* Ensemble ID */
  uint8_t CIFCount_hi;    /* 5 bits - 0 to 31 */
  uint8_t CIFCount_lo;    /* 8 bits - 0 to 249 */

  /* Information on all subchannels defined by the FIBs in this
     tranmission frame.  Note that this may not be all the active
     sub-channels in the ensemble, so we need to merge the data from
     multiple transmission frames.
  */
  struct subchannel_info_t subchans[64];
};

struct ens_info_t {
  uint16_t EId;           /* Ensemble ID */
  uint8_t CIFCount_hi;    /* Our own CIF Count */
  uint8_t CIFCount_lo;
  struct subchannel_info_t subchans[64];
};

struct dab_state_t
{
  struct demapped_transmission_frame_t tfs[5]; /* We need buffers for 5 tranmission frames - the four previous, plus the new */
  struct tf_info_t tf_info;
  struct ens_info_t ens_info;

  unsigned char* cifs_msc[16];  /* Each CIF consists of 3072*18 bits */
  unsigned char* cifs_fibs[16];  /* Each CIF consists of 3072*18 bits */
  int ncifs;  /* Number of CIFs in buffer - we need 16 to start outputting them */
  int tfidx;  /* Next tf buffer to read to. */
  int locked;
  int ens_info_shown;
  int okcount;

  /* Callback function to process a decoded ETI frame */
  std::function<void(uint8_t* eti)> eti_callback;
};

struct dab_state_t* init_dab_state(std::function<void(uint8_t* eti)> eti_callback);
void dab_process_frame(struct dab_state_t *dab);
