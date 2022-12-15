#define BTSTACK_FILE__ "plc_test.c"

/*
 * Test LC3 PLC
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wav_util.h"
#include "lc3.h"

// max config
#define MAX_SAMPLES_PER_FRAME 480

// lc3 codec config
static uint32_t sampling_frequency_hz = 48000;
static uint32_t frame_duration_us = 10000;
static uint16_t number_samples_per_frame;
static uint16_t octets_per_frame = 155;

// encoder
static lc3_decoder_mem_48k_t decoder_mem;
static lc3_decoder_t         decoder;    // pointer

lc3_encoder_mem_48k_t        encoder_mem;
lc3_encoder_t                encoder;    // pointer

//
static int16_t pcm[MAX_SAMPLES_PER_FRAME];

// PLC - set frame interval to 0 for no packet drops
static uint16_t plc_dopped_frame_interval = 20;
static uint16_t plc_frame_counter;

// sine generator
static uint8_t  sine_step;
static uint16_t sine_phases[1];

// input signal: pre-computed int16 sine wave, 96000 Hz at 300 Hz
static const int16_t sine_int16[] = {
        0,    643,   1286,   1929,   2571,   3212,   3851,   4489,   5126,   5760,
        6393,   7022,   7649,   8273,   8894,   9512,  10126,  10735,  11341,  11943,
        12539,  13131,  13718,  14300,  14876,  15446,  16011,  16569,  17121,  17666,
        18204,  18736,  19260,  19777,  20286,  20787,  21280,  21766,  22242,  22710,
        23170,  23620,  24062,  24494,  24916,  25329,  25732,  26126,  26509,  26882,
        27245,  27597,  27938,  28269,  28589,  28898,  29196,  29482,  29757,  30021,
        30273,  30513,  30742,  30958,  31163,  31356,  31537,  31705,  31862,  32006,
        32137,  32257,  32364,  32458,  32540,  32609,  32666,  32710,  32742,  32761,
        32767,  32761,  32742,  32710,  32666,  32609,  32540,  32458,  32364,  32257,
        32137,  32006,  31862,  31705,  31537,  31356,  31163,  30958,  30742,  30513,
        30273,  30021,  29757,  29482,  29196,  28898,  28589,  28269,  27938,  27597,
        27245,  26882,  26509,  26126,  25732,  25329,  24916,  24494,  24062,  23620,
        23170,  22710,  22242,  21766,  21280,  20787,  20286,  19777,  19260,  18736,
        18204,  17666,  17121,  16569,  16011,  15446,  14876,  14300,  13718,  13131,
        12539,  11943,  11341,  10735,  10126,   9512,   8894,   8273,   7649,   7022,
        6393,   5760,   5126,   4489,   3851,   3212,   2571,   1929,   1286,    643,
        0,   -643,  -1286,  -1929,  -2571,  -3212,  -3851,  -4489,  -5126,  -5760,
        -6393,  -7022,  -7649,  -8273,  -8894,  -9512, -10126, -10735, -11341, -11943,
        -12539, -13131, -13718, -14300, -14876, -15446, -16011, -16569, -17121, -17666,
        -18204, -18736, -19260, -19777, -20286, -20787, -21280, -21766, -22242, -22710,
        -23170, -23620, -24062, -24494, -24916, -25329, -25732, -26126, -26509, -26882,
        -27245, -27597, -27938, -28269, -28589, -28898, -29196, -29482, -29757, -30021,
        -30273, -30513, -30742, -30958, -31163, -31356, -31537, -31705, -31862, -32006,
        -32137, -32257, -32364, -32458, -32540, -32609, -32666, -32710, -32742, -32761,
        -32767, -32761, -32742, -32710, -32666, -32609, -32540, -32458, -32364, -32257,
        -32137, -32006, -31862, -31705, -31537, -31356, -31163, -30958, -30742, -30513,
        -30273, -30021, -29757, -29482, -29196, -28898, -28589, -28269, -27938, -27597,
        -27245, -26882, -26509, -26126, -25732, -25329, -24916, -24494, -24062, -23620,
        -23170, -22710, -22242, -21766, -21280, -20787, -20286, -19777, -19260, -18736,
        -18204, -17666, -17121, -16569, -16011, -15446, -14876, -14300, -13718, -13131,
        -12539, -11943, -11341, -10735, -10126,  -9512,  -8894,  -8273,  -7649,  -7022,
        -6393,  -5760,  -5126,  -4489,  -3851,  -3212,  -2571,  -1929,  -1286,   -643,
};

static void setup_lc3_encoder(void){
    uint8_t channel;
    encoder = lc3_setup_encoder(frame_duration_us, sampling_frequency_hz, 0, &encoder_mem);
    number_samples_per_frame =  lc3_frame_samples(frame_duration_us, sampling_frequency_hz);
    printf("LC3 Encoder config: sampling reate %u, frame duration %u us, num samples %u, num octets %u\n",
           sampling_frequency_hz, frame_duration_us, number_samples_per_frame, octets_per_frame);
}

static void setup_lc3_decoder(void){
    decoder = lc3_setup_decoder(frame_duration_us, sampling_frequency_hz, 0, &decoder_mem);
}

static void generate_audio(void){
    uint16_t sample;
    // generate sine wave for all channels
    for (sample = 0 ; sample < number_samples_per_frame ; sample++){
        int16_t value = sine_int16[sine_phases[0]] / 4;
        pcm[sample] = value;
        sine_phases[0] += sine_step;
        if (sine_phases[0] >= (sizeof(sine_int16) / sizeof(int16_t))) {
            sine_phases[0] = 0;
        }
    }
}

static void test_encoder(){
    wav_writer_open("plc_test.wav", 1, sampling_frequency_hz);

    // encode 10 seconds of music
    uint32_t audio_duration_in_seconds = 10;
    uint32_t total_samples = sampling_frequency_hz * audio_duration_in_seconds;
    uint32_t generated_samples = 0;
    plc_frame_counter = 0;

    printf("Encoding and decoding %u seconds of audio...\n", audio_duration_in_seconds);
    while (generated_samples < total_samples){

        // generate audio
        generate_audio();

        // encode frame
        uint8_t buffer[200];
        int result = lc3_encode(encoder, LC3_PCM_FORMAT_S16, (const void *) pcm, 1, octets_per_frame, (void*) buffer);
        generated_samples += number_samples_per_frame;
        plc_frame_counter++;

        uint8_t BFI = 0;
        // simulate dropped packets
        if ((plc_dopped_frame_interval != 0) && (plc_frame_counter == plc_dopped_frame_interval)){
            plc_frame_counter = 0;
            BFI = 1;
        }

        // decode codec frame
        uint8_t tmp_BEC_detect;
        if (BFI){
            lc3_decode(decoder, NULL, octets_per_frame, LC3_PCM_FORMAT_S16, (void *) pcm, 1);
        } else {
            lc3_decode(decoder, buffer, octets_per_frame, LC3_PCM_FORMAT_S16, (void *) pcm, 1);
        }

        wav_writer_write_int16(number_samples_per_frame, pcm);
    }

    wav_writer_close();
}

int main(int argc, const char * argv[]){
    (void) argv;
    (void) argc;

    // get num samples per frame
    setup_lc3_encoder();
    setup_lc3_decoder();

    // setup sine generator
    if (sampling_frequency_hz == 44100){
        sine_step = 2;
    } else {
        sine_step = 96000 / sampling_frequency_hz;
    }

    test_encoder();
    return 0;
}
