#pragma once

#ifndef _AUDIOFFT_H
#define _AUDIOFFT_H



#include <cstddef>
#include <memory>
#include <cassert>
#include <cmath>
#include <cstring>

//#define AUDIOFFT_APPLE_ACCELERATE //AUDIOFFT_INTEL_IPP//AUDIOFFT_FFTW3//AUDIOFFT_APPLE_ACCELERATE

#if defined(AUDIOFFT_INTEL_IPP)
#define AUDIOFFT_INTEL_IPP_USED
  #include <ipp.h>
#elif defined(AUDIOFFT_APPLE_ACCELERATE)
#define AUDIOFFT_APPLE_ACCELERATE_USED
  #include <Accelerate/Accelerate.h>
  #include <vector>
#elif defined (AUDIOFFT_FFTW3)
#define AUDIOFFT_FFTW3_USED
  #include <fftw3.h>
#else
#if !defined(AUDIOFFT_OOURA)
#define AUDIOFFT_OOURA
#endif
#define AUDIOFFT_OOURA_USED
#include <vector>
#endif

namespace audiofft
{

    namespace detail
    {
        class AudioFFTImpl;
    }

    /**
     * @class AudioFFT
     * @brief Performs 1D FFTs
     */
    class AudioFFT
    {
    public:
        /**
         * @brief Constructor
         */
        AudioFFT();

        AudioFFT(const AudioFFT&) = delete;
        AudioFFT& operator=(const AudioFFT&) = delete;

        /**
         * @brief Destructor
         */
        ~AudioFFT();

        /**
         * @brief Initializes the FFT object
         * @param size Size of the real input (must be power 2)
         */
        void init(size_t size);

        /**
         * @brief Performs the forward FFT
         * @param data The real input data (has to be of the length as specified in init())
         * @param re The real part of the complex output (has to be of length as returned by ComplexSize())
         * @param im The imaginary part of the complex output (has to be of length as returned by ComplexSize())
         */
        void fft(const float* data, float* re, float* im);

        /**
         * @brief Performs the inverse FFT
         * @param data The real output data (has to be of the length as specified in init())
         * @param re The real part of the complex input (has to be of length as returned by ComplexSize())
         * @param im The imaginary part of the complex input (has to be of length as returned by ComplexSize())
         */
        void ifft(float* data, const float* re, const float* im);

        /**
         * @brief Calculates the necessary size of the real/imaginary complex arrays
         * @param size The size of the real data
         * @return The size of the real/imaginary complex arrays
         */
        static size_t ComplexSize(size_t size);

    private:
        std::unique_ptr<detail::AudioFFTImpl> _impl;
    };


    /**
     * @deprecated
     * @brief Let's keep an AudioFFTBase type around for now because it has been here already in the 1st version in order to avoid breaking existing code.
     */
    typedef AudioFFT AudioFFTBase;

    namespace detail
    {
        class AudioFFTImpl
        {
        public:
            AudioFFTImpl() = default;
            AudioFFTImpl(const AudioFFTImpl&) = delete;
            AudioFFTImpl& operator=(const AudioFFTImpl&) = delete;
            virtual ~AudioFFTImpl() = default;
            virtual void init(size_t size) = 0;
            virtual void fft(const float* data, float* re, float* im) = 0;
            virtual void ifft(float* data, const float* re, const float* im) = 0;
        };
    }

} // End of namespace


#endif // Header guard
