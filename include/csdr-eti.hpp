#pragma once

#include <csdr/module.hpp>
#include <csdr/complex.hpp>
#include "meta.hpp"
#include "dab.h"

extern "C" {
#include <fftw3.h>
}

namespace Csdr::Eti {

    class EtiDecoder: public Csdr::Module<Csdr::complex<float>, unsigned char> {
        public:
            EtiDecoder();
            ~EtiDecoder() override;
            bool canProcess() override;
            void process() override;
            void setMetaWriter(MetaWriter* writer);
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
            struct dab_state_t* dab = nullptr;
            MetaWriter* metawriter = nullptr;

            fftwf_plan forward_plan;
            fftwf_plan backward_plan;
            fftwf_plan coarse_plan;

            void sendMetaData(std::map<std::string, std::string> data);
    };

}