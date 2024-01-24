# csdr-eti

This repository contains an implementation of a DAB ETI decoder that can be used
as the first stage of a DAB receiver pipeline. It cn demodulate a DAB Mode I Mux
from IQ data and output it as the corresponding ETI stream if successful.

The code in this repository is a port of the dab2eti tool from the
[dabtools project](https://github.com/Opendigitalradio/dabtools). See below for
an acknowledgement of the individual copyright holders and license.

The code has been adopted to work as a generic
[csdr](https://github.com/jketterl/csdr) module that can be used as part of a
more complex pipeline. The hardware-specific bindings to the rtl-sdr library
have been removed, the module can be fed with IQ data from any available source
and is now basically hardware agnostic.

The output stream is the same ETI data as provided by the original project, so
it can be used with the same applications. One example is piping the ETI stream
into [dablin](https://github.com/Opendigitalradio/dablin) running inside a csdr
`ExecModule` to demodulate the actual audio data.

The main use case for this code is the DAB demodulator pipeline of
[OpenWebRX](https://www.openwebrx.de/).

## Copyrights

csdr-eti Copyright (c) 2024 Jakob Ketterl <jakob.ketterl@gmx.de> published under the GPL license.

Based on the [dabtools](https://github.com/Opendigitalradio/dabtools) project / dab2eti Copyright by Dave Chapman <dave@dchapman.com> published under the GPL license.

Based on OpenDAB by David Crawley published under the GPL license.

Based on [rtl-dab](https://github.com/maydavid/rtl-dab) by David May published under the GPL license.

[Viterbi FEC](https://github.com/Opendigitalradio/ka9q-fec) by Phil Karn, KA9Q published under the GPL license.

Please report any errors or shortcomings in these acknowledgements. I do not wish to discredit any of the original authors in any way.

## I/O parameters

### Input
- IQ data as `Csdr::complex<float>`. This is binary compatible with the C++ native `std::complex<float>`, FFTW3's `fftwf_complex` or a basic `float[2]` containing the respective value for I and Q.
- sample rate: fixed at 2048000 S/s.
- Use an adequately sized buffer to feed the data. Recommended minimum: 524288 samples.

### Output
- ETI binary stream represented as `uint8_t`. Can be converted to other 8-bit data types as required.
- Data rate is inconsistend, depending on signal quality. Maximum data rate tbd.