#pragma once

#ifndef NESAPU_USE_BLIPBUF
#define NESAPU_USE_BLIPBUF 1
#endif

#include <stdint.h>
#include <stdbool.h>
#if NESAPU_USE_BLIPBUF
# include "blip_buf.h"
#endif
#include "fixedpoint.h"
#include "file_reader.h"
#include "vgm_conf.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NESAPU_SAMPLE_RATE
# define NESAPU_SAMPLE_RATE      44100
#endif

#ifndef NESAPU_MAX_SAMPLES
# define NESAPU_MAX_SAMPLES      1500
#endif

#ifndef NESAPU_RAM_CACHE_SIZE
# define NESAPU_RAM_CACHE_SIZE   4096
#endif


typedef struct nesapu_ram_s nesapu_ram_t;
struct nesapu_ram_s
{
    size_t offset;          // data start position in VGM file
    uint16_t addr;          // ram start address
    uint16_t len;           // length of ram block
    uint16_t cache_addr;    // cache start address
    uint16_t cache_len;     // cache length
    uint8_t  *cache;        // cache data
    nesapu_ram_t *next;     // next ram data block
};


typedef struct nesapu_s
{
    file_reader_t *reader;      // reader interface
    bool format;                // true: PAL, false: NTSC
    unsigned int clock_rate;    // NES clock rate (typ. 1789772)
#if NESAPU_USE_BLIPBUF    
    // Blip
    blip_buffer_t *blip;
    int16_t blip_last_sample;
#else    
    // Sampling counter
    q16_t    sample_period_fp;
    q16_t    sample_accu_fp;
#endif    
    // frame counter
    uint8_t  sequencer_step;    // sequencer step, 1-2-3-4 or 1-2-3-4-5
    bool     sequence_mode;     // false: 4-step sequence. true: 5-step sequence. Set by $4017 bit 7
    bool     quarter_frame;     // true @step 1,2,3,4 for 4-step sequence or @step 1,2,3,5 for 5-step sequence
    bool     half_frame;        // true @step 2,4 for 4-step sequence or @step 2,5 for 5-step sequence
    bool     frame_force_clock; // When writing 0x80 to $4017, immediate clock all units at first step
    q16_t    frame_accu_fp;     // frame counter accumulator (fixed point)
    q16_t    frame_period_fp;   // frame clock (240Hz) period (fixed point)
    // Pulse1 Channel
    struct pulse_t
    {
        bool          enabled;
        unsigned int  duty;             // duty cycle, 0,1,2,3 -> 12.5%,25%,50%,-25%, see pulse_waveform_table
        unsigned int  length_value;     // Length counter value
        bool          lenhalt_envloop;  // Length counter halt or Envelope loop
        bool          constant_volume;  // Constant volume (true) or Envelope enable (false)
        bool          envelope_start;   // Envelope start flag
        unsigned int  envelope_decay;   // Envelope decay value (15, 14, ...)
        unsigned int  envelope_value;   // Envelope clock divider (counter value)
        unsigned int  volume_envperiod; // Volume or Envelope period
        bool          sweep_enabled;    // Sweep enabled
        unsigned int  sweep_period;     // Sweep period
        unsigned int  sweep_value;      // Sweep divider value
        bool          sweep_negate;     // Sweep negate flag
        unsigned int  sweep_shift;      // Sweep shift
        unsigned int  sweep_target;     // Sweep target
        bool          sweep_reload;     // To reload sweep_value
        unsigned int  timer_period;     // Pulse1 channel main timer period
        unsigned int  timer_value;      // Pulse1 channel main timer divider (counter value), counting from 2x timer_period
        unsigned int  sequencer_value;  // Current suquence (8 steps of waveform)
        bool          sweep_timer_mute; // If sweep or timer will mute the channel
    } pulse[2];
    // Triangle Channel
    bool          triangle_enabled;
    unsigned int  triangle_length_value;        // Length counter value
    bool          triangle_lnrctl_lenhalt;      // Linear counter control / length counter halt
    unsigned int  triangle_linear_period;       // Linear counter period
    unsigned int  triangle_linear_value;        // Linear counter value
    bool          triangle_linear_reload;       // Linear counter reload flag
    unsigned int  triangle_timer_period;        // Channel timer period
    bool          triangle_timer_period_bad;    // If timer period is out of range
    unsigned int  triangle_timer_value;         // Channel timer value
    unsigned int  triangle_sequencer_value;     // Current suquence (32 steps of waveform)
    // Noise Channel
    bool          noise_enabled;
    bool          noise_lenhalt_envloop;        // Length counter halt
    bool          noise_constant_volume;        // Constant volume
    unsigned int  noise_volume_envperiod;       // Volume / Envelope period
    bool          noise_mode;                   // Noise mode / flag
    bool          noise_envelope_start;         // Envelope start flag
    unsigned int  noise_envelope_decay;         // Envelope decay value (15, 14, ...)
    unsigned int  noise_envelope_value;         // Envelope clock divider (counter value)
    unsigned int  noise_length_value;           // Length counter value
    unsigned int  noise_timer_period;           // Channel timer period
    unsigned int  noise_timer_value;            // Channel timer value
    uint16_t      noise_shift_reg;              // Noise shift register
    // DMC Channel
    bool          dmc_enabled;
    bool          dmc_loop;
    unsigned int  dmc_output;
    uint16_t      dmc_sample_addr;
    uint16_t      dmc_sample_len;
    unsigned int  dmc_timer_period;
    unsigned int  dmc_timer_value;
    uint16_t      dmc_read_addr;
    uint16_t      dmc_read_remaining;
    uint8_t       dmc_read_buffer;
    bool          dmc_read_buffer_empty;
    uint8_t       dmc_output_shift_reg;
    bool          dmc_output_silence;
    unsigned int  dmc_output_bits_remaining;
    nesapu_ram_t  *ram_list;                    // APU accessible RAM list
    uint8_t       *ram_cache;                   // Read cache for RAM, shared by all RAM blocks
    nesapu_ram_t  *ram_active;                  // Active ram block (using cache)
    // Fade control
    bool          fadeout_enabled;
    q16_t         fadeout_period_fp;
    q16_t         fadeout_accu_fp;
    unsigned int  fadeout_sequencer_value;
} nesapu_t;


nesapu_t * nesapu_create(file_reader_t *reader, bool format, unsigned int clock, unsigned int sample_rate);
void    nesapu_destroy(nesapu_t *apu);
void    nesapu_reset(nesapu_t *apu);
void    nesapu_write_reg(nesapu_t *apu, uint16_t reg, uint8_t val);
void    nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples);
void    nesapu_add_ram(nesapu_t *apu, size_t offset, uint16_t addr, uint16_t len);
uint8_t nesapu_read_ram(nesapu_t *apu, uint16_t addr);
void    nesapu_fade_enable(nesapu_t *apu, unsigned int samples);

#ifdef __cplusplus
}
#endif
