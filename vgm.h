#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "audio_file_reader.h"
#include "nesapu.h"


#ifdef __cplusplus
extern "C" {
#endif


#define VGM_GD3_STR_MAX_LEN     64      // Max string length in GD3 tags
#define VGM_SAMPLE_RATE         44100   // Fixed sample rate for all VGM files
#define VGM_FADEOUT_SECONDS     2

PACK(struct vgm_header_s
{
    uint32_t ident;             // 0x00: identification "Vgm " 0x206d6756
    uint32_t eof_offset;        // 0x04: relative offset to end of file (should be file_length - 4)
    uint32_t version;           // 0x08: BCD version. Ver 1.7.1 is 0x00000171
    uint32_t sn76489_clk;
    uint32_t ym2413_clk;
    uint32_t gd3_offset;        // 0x14: relative offset to GD3 tag (0 if no GD3 tag
    uint32_t total_samples;     // 0x18: total samples (sum of wait values) in the file
    uint32_t loop_offset;       // 0x1c: relative offset to loop point, or 0 if no loop
    uint32_t loop_samples;      // 0x20: number of samples in one loop. or 0 if no loop
    uint32_t rate;              // 0x24: rate of recording. typeical 50 for PAL and 60 for NTSC
    uint16_t sn76489_feedback;
    uint16_t sn76489_shiftreg;
    uint32_t ym2612_clk;
    uint32_t ym2151_clk;
    uint32_t data_offset;       // 0x34: relative offset to VGM data stream. for version 1.5 and below, its value is 0x0c and data starts at 0x40
    uint32_t sega_pcm_clk;
    uint32_t sega_pcm_if_reg;
    uint32_t rf5c68_clk;
    uint32_t ym2203_clk;
    uint32_t ym2608_clk;
    uint32_t ym2610b_clk;
    uint32_t ym3812_clk;
    uint32_t ym3526_clk;
    uint32_t y8950_clk;
    uint32_t ymf262_clk;
    uint32_t ymf278b_clk;
    uint32_t ymf271_clk;
    uint32_t ymz280b_clk;
    uint32_t rf5c164_clk;
    uint32_t pwm_clock;
    uint32_t ay8910_clk;
    uint8_t  ay8910_type;
    uint8_t  ay8910_flags;
    uint8_t  ym2203_ay8910_flags;
    uint8_t  ym2608_ay8910_flags;
    uint8_t  volume_modifier;   // 0x7c: range [-63 .. 192], volume = 2 ^ (volume_modifier / 0x20)
    uint8_t  reserved1;
    uint8_t  loop_base; 
    uint8_t  loop_modifier;
    uint32_t gb_dmg_clk;
    uint32_t nes_apu_clk;       // 0x84: NES APU Clock (typical 1789772). Bit 31 indicates FDS addition
    uint32_t multi_pcm_clk;
    uint32_t upd7759_clk;
    uint32_t okim6258_clk;
    uint8_t  okim6258_flags;
    uint8_t  k054539_flags;
    uint8_t  c140_flags;
    uint8_t  reserved2;
    uint32_t oki6295_clk;
    uint32_t k051649_clk;
    uint32_t k054539_clk;
    uint32_t huc6280_clk;
    uint32_t c140_clk;
    uint32_t k053260_clk;
    uint32_t pokey_clk;
    uint32_t qsound_clock;
    uint32_t scsp_clk;
    uint32_t extra_header_offset;   // 0xBC: relative offset to the extra header or 0 if no extra header is present
    uint32_t wswan_clk;
    uint32_t vsu_clk;
    uint32_t saa1090_clk;
    uint32_t es5503_clk;
    uint32_t es5506_clk;
    uint8_t  es5503_chns;
    uint8_t  es5506_chns;
    uint8_t  c352_clk_div;
    uint8_t  reserved3;
    uint32_t x1010_clk;
    uint32_t c352_clk;
    uint32_t ga20_clk;
    uint8_t  reserved4[7 * 4];
});
typedef struct vgm_header_s vgm_header_t;


typedef struct vgm_s
{
    file_reader_t *reader;
    // From VGM file
    uint32_t version;
    uint32_t data_offset;
    unsigned int total_samples;
    int loops;
    uint32_t loop_offset;
    unsigned int loop_samples;
    uint32_t rate;          // (experimental: to find out 50/60Hz)
    uint32_t nes_apu_clk;   // NES APU clock
    char *track_name_en;    // track name in English
	char *game_name_en;     // game name in English
	char *sys_name_en;      // system name in English
	char *author_name_en;   // author name in English
	char *release_date;     // release date
	char *creator;          // VGM creator name
	char *notes;            // notes
    // Playback control
    unsigned int sample_rate;       // sample rate
    size_t data_pos;                // position of current data
    unsigned int samples_waiting;   // # of samples waiting, in 44100Hz unit
    nesapu_t *apu;                  // NES APU
    unsigned long complete_samples; // Total samples including total + loop
    unsigned long played_samples;   // Played samples
    unsigned int fadeout_samples;   // From which sample fadeout shall start
 } vgm_t;


vgm_t* vgm_create(file_reader_t *reader);
void vgm_destroy(vgm_t *vgm);
bool vgm_prepare_playback(vgm_t *vgm, unsigned int srate, bool fadeout);
int vgm_get_samples(vgm_t *vgm, int16_t *buf, unsigned int size);

#ifdef __cplusplus
}
#endif