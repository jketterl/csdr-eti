#pragma once

#include <csdr/module.hpp>
#include <csdr/complex.hpp>

extern "C" {
#include "dab.h"
#include <fftw3.h>
}

namespace Csdr::Eti {

    class EtiDecoder: public Csdr::Module<Csdr::complex<float>, unsigned char> {
        public:
            EtiDecoder();
            ~EtiDecoder() override;
            bool canProcess() override;
            void process() override;
        private:
            uint32_t coarse_timeshift = 0;
            int32_t fine_timeshift = 0;
            int32_t coarse_freq_shift = 0;
            double fine_freq_shift = 0;
            bool force_timesync = false;
            bool sdr_demod(Csdr::complex<float>* input, struct demapped_transmission_frame_t* tf);
            uint32_t get_coarse_time_sync(Csdr::complex<float>* input);
            int32_t get_fine_time_sync(Csdr::complex<float>* input);
            int32_t get_coarse_freq_shift(Csdr::complex<float>* input);
            double get_fine_freq_corr(Csdr::complex<float>* input);
            /* raw symbols */
            fftwf_complex symbols[76][2048] = {0, 0};
            /* symbols d-qpsk-ed */
            fftwf_complex * symbols_d;
            struct dab_state_t* dab = nullptr;
    };

}