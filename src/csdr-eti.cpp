/*
david may 2012
david.may.muc@googlemail.com
*/

#include "csdr-eti.hpp"

extern "C" {
#include "sdr_prstab.h"
#include "dab_tables.h"
};

#include <iostream>
#include <cstring>
#include <functional>

using namespace Csdr::Eti;

EtiDecoder::EtiDecoder() {
    init_dab_state(&dab, [](uint8_t* eti, void* ctx) {
        EtiDecoder* me = (EtiDecoder*) ctx;
        std::memcpy(me->writer->getWritePointer(), eti, 6144);
        me->writer->advance(6144);
    }, this);
}

EtiDecoder::~EtiDecoder() {
    delete metawriter;
}

void EtiDecoder::setMetaWriter(MetaWriter *writer) {
    auto old = metawriter;
    metawriter = writer;
    delete old;
}

void EtiDecoder::sendMetaData(std::map<std::string, std::string> data) {
    if (metawriter == nullptr) return;
    metawriter->sendMetaData(data);
}

bool EtiDecoder::canProcess() {
    return this->reader->available() >= 196608 * 2;
}

void EtiDecoder::process() {
    Csdr::complex<float>* input = this->reader->getReadPointer();

    if (sdr_demod(input, &dab->tfs[dab->tfidx])) {
        dab_process_frame(dab);
    }

    this->reader->advance(196608 + coarse_timeshift + fine_timeshift);
}

bool EtiDecoder::sdr_demod(Csdr::complex<float>* input, struct demapped_transmission_frame_t* tf) {
    coarse_timeshift = get_coarse_time_sync(input);
    force_timesync = false;
    if (coarse_timeshift) {
        std::cerr << "coarse time shift: " << coarse_timeshift << std::endl;
        return false;
    }

    if (coarse_freq_shift) {
        fine_timeshift = 0;
    } else {
        fine_timeshift = get_fine_time_sync(input);
    }

    coarse_freq_shift = get_coarse_freq_shift(input);
    if (abs(coarse_freq_shift) > 1) {
        sendMetaData(std::map<std::string, std::string>{ {"coarse_frequency_shift", std::to_string(coarse_freq_shift)} });
        //std::cerr << "coarse frequency shift: " << coarse_freq_shift << std::endl;
        force_timesync = true;
        return false;
    }

    fine_freq_shift = get_fine_freq_corr(input);
    if (fine_freq_shift != 0) {
        sendMetaData(std::map<std::string, std::string>{ {"fine_frequency_shift", std::to_string(fine_freq_shift)} });
        //std::cerr << "fine frequency shift: " << fine_freq_shift << std::endl;
    }


    /* raw symbols */
    fftwf_complex symbols[76][2048] = {0, 0};

    /* d-qpsk */
    for (int i=0;i<76;i++) {
        fftwf_plan p = fftwf_plan_dft_1d(2048, (fftwf_complex*) &input[2656+(2552*i)+504], symbols[i], FFTW_FORWARD, FFTW_ESTIMATE);
        fftwf_execute(p);
        fftwf_destroy_plan(p);
        fftwf_complex tmp;
        for (int j = 0; j < 2048/2; j++)
        {
            tmp[0]     = symbols[i][j][0];
            tmp[1]     = symbols[i][j][1];
            symbols[i][j][0]    = symbols[i][j+2048/2][0];
            symbols[i][j][1]    = symbols[i][j+2048/2][1];
            symbols[i][j+2048/2][0] = tmp[0];
            symbols[i][j+2048/2][1] = tmp[1];
        }

    }

    /* symbols d-qpsk-ed */
    fftwf_complex* symbols_d = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * 2048 * 76);

    for (int j=1;j<76;j++) {
        for (int i=0;i<2048;i++)
        {
            symbols_d[j*2048+i][0] =
                    ((symbols[j][i][0]*symbols[j-1][i][0])
                    +(symbols[j][i][1]*symbols[j-1][i][1]))
                    /(symbols[j-1][i][0]*symbols[j-1][i][0]+symbols[j-1][i][1]*symbols[j-1][i][1]);
            symbols_d[j*2048+i][1] =
                    ((symbols[j][i][0]*symbols[j-1][i][1])
                    -(symbols[j][i][1]*symbols[j-1][i][0]))
                    /(symbols[j-1][i][0]*symbols[j-1][i][0]+symbols[j-1][i][1]*symbols[j-1][i][1]);
        }
    }

    uint8_t* dst = tf->fic_symbols_demapped[0];

    int k,kk;
    for (int j=1;j<76;j++) {
        if (j == 4) { dst = tf->msc_symbols_demapped[0]; }
        k = 0;
        for (int i=256;i<1793;i++){
            if (i!=1024) {
                /* Frequency deinterleaving and QPSK demapping combined */
                kk = rev_freq_deint_tab[k++];
                dst[kk] = (symbols_d[j*2048+i][0]>0)?0:1;
                dst[1536+kk] = (symbols_d[j*2048+i][1]>0)?1:0;
            }
        }
        dst += 3072;
    }

    fftwf_free(symbols_d);

    return true;
}

uint32_t EtiDecoder::get_coarse_time_sync(Csdr::complex<float>* input) {
    int32_t tnull = 2656; // was 2662? why?
    int32_t j,k;
    float filt[196608 - tnull];

    // check for energy in fist tnull samples
    float e = 0;
    float threshold = 40;
    for (k = 0; k < tnull; k += 10)
        e += fabs(input[k].i());

    if (e < threshold && !force_timesync)
        return 0;

    //fprintf(stderr,"Resync\n");
    // energy was to high so we assume we are not in sync
    // subsampled filter to detect where the null symbol is
    for (j=0;j<(196608-tnull)/10;j++)
        filt[j] = 0;
    for (j=0;j<196608-tnull;j+=10)
        for (k=0;k<tnull;k+=10)
            filt[j/10] = filt[j/10] + fabs(input[j + k].i());

    // finding the minimum in filtered data gives position of null symbol
    float minVal=9999999;
    uint32_t minPos=0;
    for (j=0;j<(196608-tnull)/10;j++){
        if (filt[j]<minVal) {
            minVal = filt[j];
            minPos = j*10;
        }
    }
    //fprintf(stderr,"calculated position of nullsymbol: %f",minPos*2);
    return minPos;
}

int32_t EtiDecoder::get_fine_time_sync(Csdr::complex<float> *input) {
    /* correlation in frequency domain
       e.g. J.Cho "PC-based receiver for Eureka-147" 2001
       e.g. K.Taura "A DAB receiver" 1996
    */

    /* first we have to transfer the receive prs symbol in frequency domain */
    fftwf_complex prs_received_fft[2048];
    fftwf_plan p;
    p = fftwf_plan_dft_1d(2048, (fftwf_complex*) &input[2656+504], &prs_received_fft[0], FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_destroy_plan(p);

    /* now we build the complex conjugate of the known prs */
    // 1536 as only the carries are used
    fftwf_complex prs_star[1536];
    int i;
    for (i=0;i<1536;i++) {
        prs_star[i][0] = prs_static[i][0];
        prs_star[i][1] = -1 * prs_static[i][1];
    }

    /* fftshift the received prs
       at this point we have to be coarse frequency sync
       however we can simply shift the bins */
    fftwf_complex prs_rec_shift[1536];
    // TODO allow for coarse frequency shift !=0
    int32_t cf_shift = 0;
    // matlab notation (!!!-1)
    // 769:1536+s
    //  2:769+s why 2? I dont remember, but peak is very strong
    for (i=0;i<1536;i++) {
        if (i<768) {
            prs_rec_shift[i][0] = prs_received_fft[i+1280][0];
            prs_rec_shift[i][1] = prs_received_fft[i+1280][1];
        }
        if (i>=768) {
            prs_rec_shift[i][0] = prs_received_fft[i-765][0];
            prs_rec_shift[i][1] = prs_received_fft[i-765][1];
        }
    }

    /* now we convolute both symbols */
    fftwf_complex convoluted_prs[1536];
    int s;
    for (s=0;s<1536;s++) {
        convoluted_prs[s][0] = prs_rec_shift[s][0]*prs_star[s][0]-prs_rec_shift[s][1]*prs_star[s][1];
        convoluted_prs[s][1] = prs_rec_shift[s][0]*prs_star[s][1]+prs_rec_shift[s][1]*prs_star[s][0];
    }

    /* and finally we transfer the convolution back into time domain */
    fftwf_complex convoluted_prs_time[1536];
    fftwf_plan px;
    px = fftwf_plan_dft_1d(1536, &convoluted_prs[0], &convoluted_prs_time[0], FFTW_BACKWARD, FFTW_ESTIMATE);
    fftwf_execute(px);
    fftwf_destroy_plan(px);

    uint32_t maxPos=0;
    float tempVal = 0;
    float maxVal=-99999;
    for (i=0;i<1536;i++) {
        tempVal = sqrt((convoluted_prs_time[i][0]*convoluted_prs_time[i][0])+(convoluted_prs_time[i][1]*convoluted_prs_time[i][1]));
        if (tempVal>maxVal) {
            maxPos = i;
            maxVal = tempVal;
        }
    }

    if (maxPos<1536/2) {
        return maxPos+8;
    } else {
        return (maxPos-(1536));
    }
    //return 0;
}

int32_t EtiDecoder::get_coarse_freq_shift(Csdr::complex<float> *input) {
    fftwf_complex symbols[2048] = {0, 0};
    fftwf_plan p;
    p = fftwf_plan_dft_1d(2048, (fftwf_complex*) &input[2656+505+fine_timeshift], symbols, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_destroy_plan(p);

    fftwf_complex tmp;
    for (int i = 0; i < 2048/2; i++)
    {
        tmp[0]     = symbols[i][0];
        tmp[1]     = symbols[i][1];
        symbols[i][0]    = symbols[i+2048/2][0];
        symbols[i][1]    = symbols[i+2048/2][1];
        symbols[i+2048/2][0] = tmp[0];
        symbols[i+2048/2][1] = tmp[1];
    }

    int len = 128;
    fftwf_complex convoluted_prs[len];
    int s;
    int freq_hub = 14; // + and - center freq
    int k;
    float global_max = -99999;
    int global_max_pos=0;
    for (k=-freq_hub;k<=freq_hub;k++) {

        for (s=0;s<len;s++) {
            convoluted_prs[s][0] = prs_static[freq_hub+s][0] * symbols[freq_hub+k+256+s][0]-
                    (-1)*prs_static[freq_hub+s][1] * symbols[freq_hub+k+256+s][1];
            convoluted_prs[s][1] = prs_static[freq_hub+s][0] * symbols[freq_hub+k+256+s][1]+
                    (-1)*prs_static[freq_hub+s][1] * symbols[freq_hub+k+256+s][0];
        }
        fftwf_complex convoluted_prs_time[len];
        fftwf_plan px;
        px = fftwf_plan_dft_1d(len, &convoluted_prs[0], &convoluted_prs_time[0], FFTW_BACKWARD, FFTW_ESTIMATE);
        fftwf_execute(px);
        fftwf_destroy_plan(px);

        uint32_t maxPos=0;
        float tempVal = 0;
        float maxVal=-99999;
        for (s=0;s<len;s++) {
            tempVal = sqrt((convoluted_prs_time[s][0]*convoluted_prs_time[s][0])+(convoluted_prs_time[s][1]*convoluted_prs_time[s][1]));
            if (tempVal>maxVal) {
                maxPos = s;
                maxVal = tempVal;
            }
        }
        //fprintf(stderr,"%f ",maxVal);


        if (maxVal>global_max) {
            global_max = maxVal;
            global_max_pos = k;
        }
    }
    //fprintf(stderr,"MAXPOS %d\n",global_max_pos);
    return global_max_pos;
}

double EtiDecoder::get_fine_freq_corr(Csdr::complex<float> *input) {
    fftwf_complex *left;
    fftwf_complex *right;
    fftwf_complex *lr;
    double angle[504];
    double mean=0;
    double ffs;
    left = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * 504);
    right = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * 504);
    lr = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * 504);
    uint32_t i;
    // remove this? it's not really doing anything
    int32_t fine_timeshift = 0;
    for (i=0;i<504;i++) {
        left[i][0] = input[2656+2048+i+fine_timeshift].i();
        left[i][1] = input[2656+2048+i+fine_timeshift].q();
        right[i][0] = input[2656+i+fine_timeshift].i();
        right[i][1] = input[2656+i+fine_timeshift].q();
    }
    for (i=0;i<504;i++){
        lr[i][0] = (left[i][0]*right[i][0]-left[i][1]*(-1)*right[i][1]);
        lr[i][1] = (left[i][0]*(-1)*right[i][1]+left[i][1]*right[i][0]);
    }

    for (i=0;i<504;i++){
        angle[i] = atan2(lr[i][1],lr[i][0]);
    }
    for (i=0;i<504;i++){
        mean = mean + angle[i];
    }
    mean = (mean/504);
    //printf("\n%f %f\n",left[0][0],left[0][1]);
    //printf("\n%f %f\n",right[0][0],right[0][1]);
    //printf("\n%f %f\n",lr[0][0],lr[0][1]);
    //printf("\n%f\n",angle[0]);

    ffs = mean / (2 * M_PI) * 1000;
    //printf("\n%f\n",ffs);

    fftwf_free(left);
    fftwf_free(right);
    fftwf_free(lr);

    return ffs;
}