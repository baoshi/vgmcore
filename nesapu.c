#include <stdlib.h>
#include <memory.h>
#include "nesapu.h"


//
// Length Counter (Pulse1, Pulse2, Triangle, Noise)
// https://www.nesdev.org/wiki/APU_Length_Counter
//
static const unsigned int length_counter_table[32] = 
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


// Pulse channel
// Duty lookup table, see wiki "Implementation details"
static const bool pulse_waveform_table[][8] = 
{
    { 0, 0, 0, 0, 0, 0, 0, 1 }, // 12.5%
    { 0, 0, 0, 0, 0, 0, 1, 1 }, // 25%
    { 0, 0, 0, 0, 1, 1, 1, 1 }, // 50%
    { 1, 1, 1, 1, 1, 1, 0, 0 }  // 25% negated
};


//
// Triangle wave table
// https://www.nesdev.org/wiki/APU_Triangle
//
static const unsigned int triangle_waveform_table[32] = 
{
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};


//
// DMC channel 
// https://www.nesdev.org/wiki/APU_DMC
static const uint16_t dmc_timer_period_ntsc[16] =
{
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84, 72, 54
};

static const uint16_t dmc_timer_period_pal[16] =
{
    398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50
};


//
// Mixer
// https://www.nesdev.org/wiki/APU_Mixer
//
// Generate table using make_const_tables.c


static const q29_t mixer_pulse_table[31] =
{
             0,    6232609,   12315540,   18254120,   24053428,   29718306,   35253376,   40663044,
      45951532,   51122860,   56180880,   61129276,   65971580,   70711160,   75351248,   79894960,
      84345240,   88704968,   92976872,   97163576,  101267592,  105291368,  109237216,  113107392,
     116904048,  120629256,  124285008,  127873240,  131395816,  134854496,  138251008
};


static const q29_t mixer_tnd_table[203] =
{
             0,    3596940,    7164553,   10703196,   14213217,   17694966,   21148782,   24574998,
      27973946,   31345950,   34691328,   38010392,   41303460,   44570820,   47812788,   51029652,
      54221704,   57389228,   60532508,   63651820,   66747436,   69819624,   72868656,   75894784,
      78898280,   81879376,   84838328,   87775384,   90690792,   93584784,   96457600,   99309472,
     102140624,  104951272,  107741656,  110511992,  113262480,  115993344,  118704800,  121397032,
     124070264,  126724680,  129360504,  131977904,  134577088,  137158224,  139721536,  142267184,
     144795360,  147306224,  149799968,  152276768,  154736784,  157180208,  159607168,  162017872,
     164412480,  166791120,  169153984,  171501200,  173832960,  176149376,  178450624,  180736848,
     183008176,  185264784,  187506800,  189734352,  191947600,  194146672,  196331728,  198502848,
     200660208,  202803920,  204934128,  207050944,  209154512,  211244944,  213322352,  215386864,
     217438624,  219477696,  221504256,  223518400,  225520224,  227509856,  229487408,  231452976,
     233406688,  235348624,  237278928,  239197680,  241104976,  243000944,  244885648,  246759232,
     248621760,  250473328,  252314064,  254144032,  255963376,  257772096,  259570384,  261358256,
     263135856,  264903216,  266660496,  268407712,  270144992,  271872416,  273590048,  275297984,
     276996320,  278685088,  280364448,  282034432,  283695072,  285346528,  286988832,  288622080,
     290246336,  291861696,  293468160,  295065888,  296654912,  298235264,  299807136,  301370464,
     302925376,  304471968,  306010240,  307540288,  309062208,  310576032,  312081824,  313579648,
     315069568,  316551680,  318025984,  319492608,  320951584,  322402944,  323846752,  325283104,
     326712064,  328133664,  329547904,  330954912,  332354784,  333747488,  335133088,  336511680,
     337883296,  339248000,  340605792,  341956800,  343301056,  344638560,  345969408,  347293664,
     348611328,  349922464,  351227136,  352525408,  353817280,  355102848,  356382112,  357655168,
     358922016,  360182720,  361437312,  362685856,  363928384,  365164960,  366395584,  367620320,
     368839232,  370052352,  371259680,  372461344,  373657280,  374847584,  376032320,  377211456,
     378385120,  379553280,  380716000,  381873312,  383025248,  384171872,  385313216,  386449280,
     387580128,  388705792,  389826272,  390941696,  392052032,  393157312,  394257568,  395352864,
     396443264,  397528704,  398609248
};



#define NESAPU_FADE_STEPS       256
static const q29_t fadeout_table[NESAPU_FADE_STEPS] =
{
             0,    2105376,    4210752,    6316128,    8421505,   10526881,   12632257,   14737633, 
      16843010,   18948386,   21053762,   23159138,   25264514,   27369890,   29475266,   31580642, 
      33686020,   35791396,   37896772,   40002148,   42107524,   44212900,   46318276,   48423652, 
      50529028,   52634404,   54739780,   56845156,   58950532,   61055908,   63161284,   65266660, 
      67372040,   69477416,   71582792,   73688168,   75793544,   77898920,   80004296,   82109672, 
      84215048,   86320424,   88425800,   90531176,   92636552,   94741928,   96847304,   98952680, 
     101058056,  103163432,  105268808,  107374184,  109479560,  111584936,  113690312,  115795688, 
     117901064,  120006440,  122111816,  124217192,  126322568,  128427944,  130533320,  132638696, 
     134744080,  136849456,  138954832,  141060208,  143165584,  145270960,  147376336,  149481712, 
     151587088,  153692464,  155797840,  157903216,  160008592,  162113968,  164219344,  166324720, 
     168430096,  170535472,  172640848,  174746224,  176851600,  178956976,  181062352,  183167728, 
     185273104,  187378480,  189483856,  191589232,  193694608,  195799984,  197905360,  200010736, 
     202116112,  204221488,  206326864,  208432240,  210537616,  212642992,  214748368,  216853744, 
     218959120,  221064496,  223169872,  225275248,  227380624,  229486000,  231591376,  233696752, 
     235802128,  237907504,  240012880,  242118256,  244223632,  246329008,  248434384,  250539760, 
     252645136,  254750512,  256855888,  258961264,  261066640,  263172016,  265277392,  267382768, 
     269488160,  271593536,  273698912,  275804288,  277909664,  280015040,  282120416,  284225792, 
     286331168,  288436544,  290541920,  292647296,  294752672,  296858048,  298963424,  301068800, 
     303174176,  305279552,  307384928,  309490304,  311595680,  313701056,  315806432,  317911808, 
     320017184,  322122560,  324227936,  326333312,  328438688,  330544064,  332649440,  334754816, 
     336860192,  338965568,  341070944,  343176320,  345281696,  347387072,  349492448,  351597824, 
     353703200,  355808576,  357913952,  360019328,  362124704,  364230080,  366335456,  368440832, 
     370546208,  372651584,  374756960,  376862336,  378967712,  381073088,  383178464,  385283840, 
     387389216,  389494592,  391599968,  393705344,  395810720,  397916096,  400021472,  402126848, 
     404232224,  406337600,  408442976,  410548352,  412653728,  414759104,  416864480,  418969856, 
     421075232,  423180608,  425285984,  427391360,  429496736,  431602112,  433707488,  435812864, 
     437918240,  440023616,  442128992,  444234368,  446339744,  448445120,  450550496,  452655872, 
     454761248,  456866624,  458972000,  461077376,  463182752,  465288128,  467393504,  469498880, 
     471604256,  473709632,  475815008,  477920384,  480025760,  482131136,  484236512,  486341888, 
     488447264,  490552640,  492658016,  494763392,  496868768,  498974144,  501079520,  503184896, 
     505290272,  507395648,  509501024,  511606400,  513711776,  515817152,  517922528,  520027904, 
     522133280,  524238656,  526344032,  528449408,  530554784,  532660160,  534765536,  536870912 
};

//
// Noise channel
// https://www.nesdev.org/wiki/APU_Noise
static const uint16_t noise_timer_period_ntsc[16] =
{
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t noise_timer_period_pal[16] =
{
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};



/**
 * @brief Count down timer
 * 
 * @param counter       Input initial counter value. When counting finish, return finish counter value.
 * @param period        Counting period.
 * @return unsigned int Number of times counter has reloaded from 0 to next period
 * 
 * @note Most cases \param cycles < \param period, it is faster we use while loop than divide
 *
 */
static inline unsigned int timer_count_down(unsigned int *counter, unsigned int period, unsigned int cycles)
{
    unsigned int clocks = 0;
    while (cycles >= period)
    {
        cycles -= period;
        ++clocks;
    }
    unsigned int extra = cycles;
    if (extra > *counter)
    {
        *counter = *counter + period - extra;
        ++clocks;
    }
    else
    {
        *counter = *counter - extra;
    }
    return clocks;
}


/**
 * @brief Count up timer
 * 
 * @param counter       Input initial counter value. When counting finish, return finish counter value
 * @param period        Countering period
 * @return unsigned int Number of times counter has reloaded from period to 0
 *
 */
static inline unsigned int timer_count_up(unsigned int *counter, unsigned int period, unsigned int cycles)
{
    unsigned int clocks = cycles / period;
    unsigned int extra = cycles % period;
    if (*counter + extra >= period)
    {
        *counter = *counter + extra - period;
        ++clocks;
    }
    else
    {
        *counter = *counter + extra;
    }
    return clocks;
}


static inline void update_frame_counter(nesapu_t *apu, unsigned int cycles)
{
    //
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    //
    // Frame counter clocks 4 or 5 sequencer at 240Hz or 7457.38 CPU clocks.
    // Fixed point is used here to increase accuracy.
    if (apu->frame_force_clock)
    {
        apu->quarter_frame = true;
        apu->half_frame = true;
        apu->frame_force_clock = false;
    }
    else
    {
        apu->quarter_frame = false;
        apu->half_frame = false;
    }
    q16_t cycles_fp = int_to_q16(cycles);
    apu->frame_accu_fp += cycles_fp;
    if (apu->frame_accu_fp >= apu->frame_period_fp)
    {
        apu->frame_accu_fp -= apu->frame_period_fp;
        ++apu->sequencer_step;
        if (apu->sequence_mode)
        {
            // 5 step mode
            // step      quarter_frame         half_frame
            //  1          clock                  -
            //  2          clock                clock
            //  3          clock                  -
            //  4            -                    -
            //  5          clock                clock
            switch (apu->sequencer_step)
            {
            case 1:
                apu->quarter_frame = true;
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
                break;
            case 4:
                apu->quarter_frame = false;
                break;
            case 5:
                apu->quarter_frame = true;
                apu->half_frame = true;
                apu->sequencer_step = 0;
                break;
            }
        }
        else
        {
            // 4 step mode
            // step      quarter_frame         half_frame
            //  1          clock                  -
            //  2          clock                clock
            //  3          clock                  -
            //  4          clock                clock
            switch (apu->sequencer_step)
            {
            case 1:
                apu->quarter_frame = true;
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
                break;
            case 4:
                apu->quarter_frame = true;
                apu->half_frame = true;
                apu->sequencer_step = 0;
                break;
            }
        }
    }
}


/*
 * https://www.nesdev.org/wiki/APU_Pulse
 *
 *                  Sweep -----> Timer
 *                    |            |
 *                    |            |
 *                    |            v
 *                    |        Sequencer   Length Counter
 *                    |            |             |
 *                    |            |             |
 *                    v            v             v
 * Envelope -------> Gate -----> Gate -------> Gate ---> (to mixer)
 */
static inline unsigned int update_pulse(nesapu_t *apu, int ch, unsigned int cycles)
{
    //
    // Clock envelope @ quater frame
    // https://www.nesdev.org/wiki/APU_Envelope
    if (apu->quarter_frame)
    {
        if (apu->pulse[ch].envelope_start)
        {
            apu->pulse[ch].envelope_decay = 15;
            apu->pulse[ch].envelope_value = apu->pulse[ch].volume_envperiod;
            apu->pulse[ch].envelope_start = false;
        }
        else
        {
            if (apu->pulse[ch].envelope_value)
            {
                --(apu->pulse[ch].envelope_value);
            }
            else
            {
                apu->pulse[ch].envelope_value = apu->pulse[ch].volume_envperiod;
                if (apu->pulse[ch].envelope_decay)
                {
                    --(apu->pulse[ch].envelope_decay);
                }
                else if (apu->pulse[ch].lenhalt_envloop)
                {
                    apu->pulse[ch].envelope_decay = 15;
                }
            }
        }
    }
    //
    // Clock sweep unit @ half frame 
    // https://www.nesdev.org/wiki/APU_Sweep
    //
    if (apu->half_frame)
    {
        // 1. If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is set to the target period.
        // 2. If the divider's counter is zero or the reload flag is true: The divider counter is set to P and the reload flag is cleared.
        //    Otherwise, the divider counter is decremented.
        if (apu->pulse[ch].sweep_reload)
        {
            apu->pulse[ch].sweep_value = apu->pulse[ch].sweep_period;
            apu->pulse[ch].sweep_reload = false;
        }
        else 
        {
            if (apu->pulse[ch].sweep_value)
            {
                --(apu->pulse[ch].sweep_value);
            } 
            else if (apu->pulse[ch].sweep_enabled && apu->pulse[ch].sweep_shift)  // TODO: Not sure if pulse_sweep_enabled must be true
            {
                // Calculate target period.
                unsigned int c = apu->pulse[ch].timer_period >> apu->pulse[ch].sweep_shift;
                if (apu->pulse[ch].sweep_negate)
                {
                    if (ch == 0) apu->pulse[ch].sweep_target -= c + 1;
                    else if (ch == 1) apu->pulse[ch].sweep_target -= c;
                }
                else
                {
                    apu->pulse[ch].sweep_target += c;
                }
                if (!apu->pulse[ch].sweep_timer_mute)
                {
                    // When the sweep unit is muting a pulse channel, the channel's current period remains unchanged.
                    apu->pulse[ch].timer_period = apu->pulse[ch].sweep_target;
                }
                // Two conditions cause the sweep unit to mute the channel:
                // 1. If the current period is less than 8, the sweep unit mutes the channel.
                // 2. If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
                apu->pulse[ch].sweep_timer_mute = ((apu->pulse[ch].timer_period < 8) || (apu->pulse[ch].sweep_target > 0x7ff));
                // Reload divider
                apu->pulse[ch].sweep_value = apu->pulse[ch].sweep_period;
            }
        }
    }
    //
    // Clock pulse channel timer and update sequencer
    // https://www.nesdev.org/wiki/APU_Pulse
    // Timer counting downwards from 0 at every other CPU cycle. So we set timer limit to  2x (timer_period + 1).
    if (!apu->pulse[ch].sweep_timer_mute)
    {
        unsigned int seq_clk = timer_count_down(&(apu->pulse[ch].timer_value), (apu->pulse[ch].timer_period + 1) << 1, cycles);
        if (seq_clk) timer_count_down(&(apu->pulse[ch].sequencer_value), 8, seq_clk);
    }
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->pulse[ch].lenhalt_envloop && apu->pulse[ch].length_value && apu->half_frame)
    {
        --(apu->pulse[ch].length_value);
    }
    // Determine Pulse channel output
    if (!apu->pulse[ch].enabled) return 0;
    if (!apu->pulse[ch].length_value) return 0;
    if (apu->pulse[ch].sweep_timer_mute) return 0;
    if (!pulse_waveform_table[apu->pulse[ch].duty][apu->pulse[ch].sequencer_value]) return 0;
    return (apu->pulse[ch].constant_volume ? apu->pulse[ch].volume_envperiod : apu->pulse[ch].envelope_decay);
}


/*
 * https://www.nesdev.org/wiki/APU_Triangle
 *
 *       Linear Counter    Length Counter
 *              |                |
 *              v                v
 *  Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
 */
static inline unsigned int update_triangle(nesapu_t *apu, unsigned int cycles)
{
    //
    // Clock linear counter @ quater frame
    // https://www.nesdev.org/wiki/APU_Triangle
    if (apu->quarter_frame)
    {
        // In order:
        // 1. If the linear counter reload flag is set, the linear counter is reloaded with the counter reload value,
        //    otherwise if the linear counter is non-zero, it is decremented.
        // 2. If the control flag is clear, the linear counter reload flag is cleared.
        if (apu->triangle_linear_reload)
        {
            apu->triangle_linear_value = apu->triangle_linear_period;
        }
        else if (apu->triangle_linear_value)
        {
            --(apu->triangle_linear_value);
        }
        if (!apu->triangle_lnrctl_lenhalt)
        {
            apu->triangle_linear_reload = false;
        }
    }
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->triangle_lnrctl_lenhalt && apu->triangle_length_value && apu->half_frame)
    {
        --(apu->triangle_length_value);
    }
    // Clock triangle channel timer and return
    // Trick: Just keep sequencer unchanged if the channel should be silenced, it minizes pop
    if (apu->triangle_enabled 
        &&
        !apu->triangle_timer_period_bad
        &&
        apu->triangle_length_value
        &&
        apu->triangle_linear_value)
    {
        unsigned int seq_clk = timer_count_down(&(apu->triangle_timer_value), apu->triangle_timer_period + 1, cycles);
        if (seq_clk) timer_count_up(&(apu->triangle_sequencer_value), 32, seq_clk);
    }
    return triangle_waveform_table[apu->triangle_sequencer_value];
}


/*
 * https://www.nesdev.org/wiki/APU_Noise
 * 
 *     Timer --> Shift Register   Length Counter
 *                    |                |
 *                    v                v
 * Envelope -------> Gate ----------> Gate --> (to mixer)
 */
static inline unsigned int update_noise(nesapu_t* apu, unsigned int cycles)
{
    //
    // Clock envelope @ quater frame
    // https://www.nesdev.org/wiki/APU_Envelope
    if (apu->quarter_frame)
    {
        if (apu->noise_envelope_start)
        {
            apu->noise_envelope_decay = 15;
            apu->noise_envelope_value = apu->noise_volume_envperiod;
            apu->noise_envelope_start = false;
        }
        else
        {
            if (apu->noise_envelope_value)
            {
                --(apu->noise_envelope_value);
            }
            else
            {
                apu->noise_envelope_value = apu->noise_volume_envperiod;
                if (apu->noise_envelope_decay)
                {
                    --(apu->noise_envelope_decay);
                }
                else if (apu->noise_lenhalt_envloop)
                {
                    apu->noise_envelope_decay = 15;
                }
            }
        }
    }
    // Clock noise channel timer
    if (apu->noise_timer_period > 0)
    {
        unsigned int clocks = timer_count_down(&(apu->noise_timer_value), apu->noise_timer_period + 1, cycles);
        while (clocks)
        {
            // When the timer clocks the shift register, the following occur in order:
            // 1) Feedback is calculated as the exclusive-OR of bit 0 and one other bit: bit 6 if Mode flag is set, otherwise bit 1.
            uint16_t feedback;
            if (apu->noise_mode)
            {
                feedback = ((apu->noise_shift_reg) ^ (apu->noise_shift_reg >> 6)) & 0x0001;
            }
            else
            {
                feedback = ((apu->noise_shift_reg) ^ (apu->noise_shift_reg >> 1)) & 0x0001;
            }
            // 2) The shift register is shifted right by one bit.
            apu->noise_shift_reg = apu->noise_shift_reg >> 1;
            // 3) Bit 14, the leftmost bit, is set to the feedback calculated earlier.
            if (feedback)
            {
                apu->noise_shift_reg |= 0x4000;  // 100 0000 0000 0000
            }
            else
            {
                apu->noise_shift_reg &= 0x3fff; // 011 1111 1111 1111
            }
            --clocks;
        }
    }
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->noise_lenhalt_envloop && apu->noise_length_value && apu->half_frame)
    {
        --(apu->noise_length_value);
    }
    // return value
    if (!apu->noise_enabled) return 0;
    // The mixer receives the current envelope volume except when bit 0 of the shift register is set, or the length counter is 0
    if (apu->noise_shift_reg & 0x01) return 0;
    if (!apu->noise_length_value) return 0;
    return (apu->noise_constant_volume ? apu->noise_volume_envperiod : apu->noise_envelope_decay);
}


static void dmc_transfer(nesapu_t *apu)
{
    if (apu->dmc_read_buffer_empty && apu->dmc_read_remaining)
    {
        // 1. The CPU is stalled for up to 4 CPU cycles to allow the longest
        //    possible write (the return address and write after an IRQ) to
        //    finish. (not important as we are playing recording)
        // 2. The sample buffer is filled with the next sample byte read from the current address
        apu->dmc_read_buffer = nesapu_read_ram(apu, apu->dmc_read_addr);
        apu->dmc_read_buffer_empty = false;
        // 3. The address is incremented. If it exceeds $FFFF, it wraps to $8000
        ++(apu->dmc_read_addr);
        if (apu->dmc_read_addr == 0x0000)
            apu->dmc_read_addr = 0x8000;
        // 4. The bytes remaining counter is decremented. If it becomes zero and the loop flag is set, the sample is
        //    restarted; otherwise, if the bytes remaining counter becomes zero and the IRQ enabled flag is set,
        //    the interrupt flag is set.
        --(apu->dmc_read_remaining);
        if (apu->dmc_read_remaining == 0)
        {
            if (apu->dmc_loop)
            {
                apu->dmc_read_addr = apu->dmc_sample_addr;
                apu->dmc_read_remaining = apu->dmc_sample_len;
            }
        }
    }
}


/*
 * https://www.nesdev.org/wiki/APU_DMC
 *
 *                          Timer
 *                            |
 *                            v
 * Reader ---> Buffer ---> Shifter ---> Output level ---> (to the mixer)
 */
static inline unsigned int update_dmc(nesapu_t* apu, unsigned int cycles)
{
    if (!apu->dmc_enabled) return 0;
    // clock channel clock
    unsigned int clocks = timer_count_down(&(apu->dmc_timer_value), apu->dmc_timer_period + 1, cycles);
    while (clocks)
    {
        // When the timer outputs a clock, the following actions occur in order: 
        // 1. If the silence flag is clear, the output level changes based on bit 0 of the shift register.
        //    If the bit is 1, add 2; otherwise, subtract 2.
        //    But if adding or subtracting 2 would cause the output level to leave the 0-127 range,
        //    leave the output level unchanged. This means subtract 2 only if the current level is at least 2,
        //    or add 2 only if the current level is at most 125.
        if (!apu->dmc_output_silence)
        {
            if (apu->dmc_output_shift_reg & 0x01)
                apu->dmc_output += (apu->dmc_output <= 125) ? 2 : 0;
            else
                apu->dmc_output -= (apu->dmc_output >= 2) ? 2 : 0;
        }
        // 2. The right shift register is clocked
        apu->dmc_output_shift_reg >>= 1;
        // 3. As stated above, the bits - remaining counter is decremented. If it becomes zero, a new output cycle is started.
        if (apu->dmc_output_bits_remaining > 0)
        {
            --(apu->dmc_output_bits_remaining);
        }
        if (apu->dmc_output_bits_remaining == 0)
        {
            // A new cycle is started as follows :
            // 1. The bits-remaining counter is loaded with 8.
            apu->dmc_output_bits_remaining = 8;
            // 2. If the sample buffer is empty, then the silence flag is set; 
            //    otherwise, the silence flag is cleared and the sample buffer is emptied into the shift register.
            if (apu->dmc_read_buffer_empty)
            {
                apu->dmc_output_silence = true;
            }
            else
            {
                apu->dmc_output_silence = false;
                apu->dmc_output_shift_reg = apu->dmc_read_buffer;
                apu->dmc_read_buffer_empty = true;
                // Perform memory read
                dmc_transfer(apu);
            }
        }
        --clocks;
    }
    return (apu->dmc_output);
}


nesapu_t * nesapu_create(file_reader_t *reader, bool format, unsigned int clock, unsigned int sample_rate)
{
    nesapu_t *apu = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == apu)
        return NULL;
    memset(apu, 0, sizeof(nesapu_t));
    apu->reader = reader;
    apu->format = format;
    apu->clock_rate = clock;
#if NESAPU_USE_BLIPBUF
    // blip
    apu->blip = blip_new(NESAPU_MAX_SAMPLES);
    blip_set_rates(apu->blip, apu->clock_rate, sample_rate);
    apu->frame_period_fp = float_to_q16((float)apu->clock_rate / 240.0f);  // 240Hz frame counter period
#else
    // Sampling
    apu->sample_period_fp = float_to_q16((float)apu->clock_rate / sample_rate);
#endif
    // ram
    apu->ram_list = NULL;
    apu->ram_active = NULL;
    apu->ram_cache = NULL;
    nesapu_reset(apu);
    return apu;
}


void nesapu_destroy(nesapu_t *apu)
{
    if (apu != NULL)
    {
        // Free ram list
        nesapu_ram_t *ram = apu->ram_list, *tram;
        while (ram)
        {
            tram = ram;
            ram = ram->next;
            VGM_FREE(tram);
        }
        if (apu->ram_cache)
        {
            VGM_FREE(apu->ram_cache);
        }
#if NESAPU_USE_BLIPBUF        
        // Free blip
        if (apu->blip)
        {
            blip_delete(apu->blip);
            apu->blip = 0;
        }
#endif
        VGM_FREE(apu);
    }
}


void nesapu_reset(nesapu_t* apu)
{
#if !NESAPU_USE_BLIPBUF
    // samplling
    apu->sample_accu_fp = 0;
#endif
    // channel mask
    apu->mask_pulse1 = false;
    apu->mask_pulse2 = false;
    apu->mask_triangle = false;
    apu->mask_noise = false;
    apu->mask_dmc = false;
    // fade control
    apu->fadeout_enabled = false;
    apu->fadeout_period_fp = 0;
    apu->fadeout_accu_fp = 0;
    apu->fadeout_sequencer_value = 0;
    // frame counter
    apu->sequencer_step = 0;
    apu->sequence_mode = false;
    apu->quarter_frame = false;
    apu->half_frame = false;
    apu->frame_accu_fp = 0;
    //
    // Enable Channel NT21 on startup
    // From: https://www.nesdev.org/wiki/Talk:NSF
    // Regarding 4015h, well... it's empirical. My experience says that setting 4015h to 0Fh
    // is required in order to get *a lot of* tunes starting playing. I don't remember of *any* broken
    // tune by setting such value. So, it's recommended *to follow* such thing. --Zepper 14:25, 29 March 2012 (PDT)
    // 
    // Pulse1
    apu->pulse[0].enabled = true;
    apu->pulse[0].duty = 0;
    apu->pulse[0].lenhalt_envloop = false;
    apu->pulse[0].constant_volume = false;
    apu->pulse[0].envelope_start = false;
    apu->pulse[0].envelope_value = 0;
    apu->pulse[0].volume_envperiod = 0;
    apu->pulse[0].sweep_enabled = false;
    apu->pulse[0].sweep_period = 0;
    apu->pulse[0].sweep_value = 0;
    apu->pulse[0].sweep_negate = false;
    apu->pulse[0].sweep_shift = 0;
    apu->pulse[0].sweep_target = 0;
    apu->pulse[0].sweep_reload = false;
    apu->pulse[0].timer_period = 0;
    apu->pulse[0].timer_value = 0;
    apu->pulse[0].sequencer_value = 0;
    apu->pulse[0].sweep_timer_mute = true;
    // Pulse2
    apu->pulse[1].enabled = true;
    apu->pulse[1].duty = 0;
    apu->pulse[1].lenhalt_envloop = false;
    apu->pulse[1].constant_volume = false;
    apu->pulse[1].envelope_start = false;
    apu->pulse[1].envelope_value = 0;
    apu->pulse[1].volume_envperiod = 0;
    apu->pulse[1].sweep_enabled = false;
    apu->pulse[1].sweep_period = 0;
    apu->pulse[1].sweep_value = 0;
    apu->pulse[1].sweep_negate = false;
    apu->pulse[1].sweep_shift = 0;
    apu->pulse[1].sweep_target = 0;
    apu->pulse[1].sweep_reload = false;
    apu->pulse[1].timer_period = 0;
    apu->pulse[1].timer_value = 0;
    apu->pulse[1].sequencer_value = 0;
    apu->pulse[1].sweep_timer_mute = true;
    // Triangle
    apu->triangle_enabled = true;
    apu->triangle_length_value = 0;
    apu->triangle_lnrctl_lenhalt = false;
    apu->triangle_linear_period = 0;
    apu->triangle_linear_value = 0;
    apu->triangle_linear_reload = false;
    apu->triangle_timer_period = 0;
    apu->triangle_timer_period_bad = true;
    apu->triangle_timer_value = 0;
    apu->triangle_sequencer_value = 0;
    // Noise
    apu->noise_enabled = true;
    apu->noise_lenhalt_envloop = false;
    apu->noise_constant_volume = false;
    apu->noise_constant_volume = 0;
    apu->noise_volume_envperiod = 0;
    apu->noise_mode = false;
    apu->noise_envelope_start = false;
    apu->noise_envelope_decay = 0;
    apu->noise_envelope_value = 0;
    apu->noise_length_value = 0;
    apu->noise_timer_period = 0;
    apu->noise_timer_value = 0;
    apu->noise_shift_reg = 1;
    // DMC
    apu->dmc_enabled = false;
    apu->dmc_loop = false;
    apu->dmc_output = 0;
    apu->dmc_sample_addr = 0;
    apu->dmc_sample_len = 0;
    apu->dmc_timer_period = 0;
    apu->dmc_timer_value = 0;
    apu->dmc_read_buffer = 0;
    apu->dmc_read_buffer_empty = true;
    apu->dmc_read_addr = 0;
    apu->dmc_read_remaining = 0;
    apu->dmc_output_shift_reg = 0;
    apu->dmc_output_bits_remaining = 0;
    apu->dmc_output_silence = true;
}


static inline int16_t nesapu_run_and_sample(nesapu_t *apu, unsigned int cycles)
{
    update_frame_counter(apu, cycles);
    unsigned int p1 = update_pulse(apu, 0, cycles);
    unsigned int p2 = update_pulse(apu, 1, cycles);
    unsigned int tr = update_triangle(apu, cycles);
    unsigned int ns = update_noise(apu, cycles);
    unsigned int dm = update_dmc(apu, cycles);
    if (apu->mask_pulse1) p1 = 0;
    if (apu->mask_pulse2) p2 = 0;
    if (apu->mask_triangle) tr = 0;
    if (apu->mask_noise) ns = 0;
    if (apu->mask_dmc) dm = 0;
    q29_t f = mixer_pulse_table[p1 + p2] + mixer_tnd_table[3 * tr + 2 * ns + dm];
    if (apu->fadeout_enabled)
    {
        if (apu->fadeout_sequencer_value > 0)
        {
             apu->fadeout_accu_fp += int_to_q16(1);
             if (apu->fadeout_accu_fp >= apu->fadeout_period_fp)
             {
                apu->fadeout_accu_fp -= apu->fadeout_period_fp;
                --(apu->fadeout_sequencer_value);
             }
        }
        f = q29_mul(f, fadeout_table[apu->fadeout_sequencer_value]);
    }
    return q29_to_sample(f);
}


#if NESAPU_USE_BLIPBUF

void nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples)
{
    unsigned int cycles = (unsigned int)blip_clocks_needed(apu->blip, (int)samples);
    unsigned int period = cycles / samples; // rough sampling period. blip helps resampling
    unsigned int time = 0;
    int16_t s;
    int delta;
    while (cycles > period)
    {
        // run period clocks
        s = nesapu_run_and_sample(apu, period);
        delta = s - apu->blip_last_sample;
        apu->blip_last_sample = s;
        time += period;
        blip_add_delta(apu->blip, time, delta);
        cycles -= period;
    }
    // run remaining clocks
    s = nesapu_run_and_sample(apu, cycles);
    delta = s - apu->blip_last_sample;
    apu->blip_last_sample = s;
    time += cycles;
    blip_add_delta(apu->blip, time, delta);
    blip_end_frame(apu->blip, time);
    blip_read_samples(apu->blip, (short *)buf, (int)samples, 0);
}

#else

void nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples)
{
    static int32_t prev = 0;
    int32_t t, s;
    for (unsigned int i = 0; i < samples; ++i)
    {
        apu->sample_accu_fp += apu->sample_period_fp;
        unsigned int cycles = (unsigned int)(q16_to_int(apu->sample_accu_fp));
        s = nesapu_run_and_sample(apu, cycles);
        // simple weighted filter
        t = s;
        s = (s + s + s + prev) >> 2;
        prev = t;
        buf[i] = (int16_t)s;
        apu->sample_accu_fp -= int_to_q16(cycles);
    }
}

#endif


void nesapu_write_reg(nesapu_t *apu, uint16_t reg, uint8_t val)
{
    switch (reg)
    {
    // Pulse1 regs
    case 0x00:  // $4000: DDLC VVVV, Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
        apu->pulse[0].duty = (val & 0xc0) >> 6;         // Duty (DD), index to tbl_pulse_waveform
        apu->pulse[0].lenhalt_envloop = (val & 0x20);     // Length counter halt or Envelope loop (L)
        apu->pulse[0].constant_volume = (val & 0x10);   // Constant volume (true) or Envelope enable (false) (C)
        apu->pulse[0].volume_envperiod = (val & 0xf);    // Volume or period of envelope (VVVV)
        break;
    case 0x01:  // $4001: EPPP NSSS, Sweep unit: enabled (E), period (P), negate (N), shift (S)
        apu->pulse[0].sweep_enabled = (val & 0x80);     // Sweep enable (E)
        apu->pulse[0].sweep_period = (val & 0x70) >> 4; // Sweep period (PPP)
        apu->pulse[0].sweep_negate = (val & 0x08);      // Sweep negate (N)
        apu->pulse[0].sweep_shift = val & 0x07;         // Sweep shift (SSS)
        apu->pulse[0].sweep_reload = true;              // Side effects : Sets reload flag 
        break;
    case 0x02:  // $4002: LLLL LLLL, Timer low (T)
        apu->pulse[0].timer_period &= 0xff00;
        apu->pulse[0].timer_period |= val;                          // Pulse1 timer period low (TTTT TTTT)
        apu->pulse[0].sweep_target = apu->pulse[0].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[0].sweep_timer_mute = ((apu->pulse[0].timer_period < 8) || (apu->pulse[0].sweep_target > 0x7ff));
        break;
    case 0x03:  // $4003: llll lHHH, Length counter load (L), timer high (T)
        apu->pulse[0].length_value = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll) 
        apu->pulse[0].timer_period &= 0x00ff;
        apu->pulse[0].timer_period |= ((unsigned int)(val & 0x07)) << 8;        // Pulse1 timer period high 3 bits (TTT)
        apu->pulse[0].sweep_target = apu->pulse[0].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[0].sweep_timer_mute = ((apu->pulse[0].timer_period < 8) || (apu->pulse[0].sweep_target > 0x7ff));
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse[0].envelope_start = true;
        apu->pulse[0].sequencer_value = 0;
        break;
    // Pulse2
    case 0x04:  // $4004: DDLC VVVV, Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
        apu->pulse[1].duty = (val & 0xc0) >> 6;               // Duty (DD), index to tbl_pulse_waveform
        apu->pulse[1].lenhalt_envloop = (val & 0x20);         // Length counter halt or Envelope loop (L)
        apu->pulse[1].constant_volume = (val & 0x10);         // Constant volume (true) or Envelope enable (false) (C)
        apu->pulse[1].volume_envperiod = (val & 0xf);         // Volume or period of envelope (VVVV)
        break;
    case 0x05:  // $4005: EPPP NSSS, Sweep unit: enabled (E), period (P), negate (N), shift (S)
        apu->pulse[1].sweep_enabled = (val & 0x80);           // Sweep enable (E)
        apu->pulse[1].sweep_period = (val & 0x70) >> 4;       // Sweep period (PPP)
        apu->pulse[1].sweep_negate = (val & 0x08);            // Sweep negate (N)
        apu->pulse[1].sweep_shift = val & 0x07;               // Sweep shift (SSS)
        apu->pulse[1].sweep_reload = true;                    // Side effects : Sets reload flag 
        break;
    case 0x06:  // $4006: LLLL LLLL, Timer low (T)
        apu->pulse[1].timer_period &= 0xff00;
        apu->pulse[1].timer_period |= val;                          // Pulse2 timer period low (TTTT TTTT)
        apu->pulse[1].sweep_target = apu->pulse[1].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[1].sweep_timer_mute = ((apu->pulse[1].timer_period < 8) || (apu->pulse[1].sweep_target > 0x7ff));
        break;
    case 0x07:  // $4007: llll lHHH, Length counter load (L), timer high (T)
        apu->pulse[1].length_value = length_counter_table[(val & 0xf8) >> 3];   // Length counter load (lllll) 
        apu->pulse[1].timer_period &= 0x00ff;
        apu->pulse[1].timer_period |= ((unsigned int)(val & 0x07)) << 8;          // Pulse2 timer period high 3 bits (TTT)
        apu->pulse[1].sweep_target = apu->pulse[1].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[1].sweep_timer_mute = ((apu->pulse[1].timer_period < 8) || (apu->pulse[1].sweep_target > 0x7ff));
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse[1].envelope_start = true;
        apu->pulse[1].sequencer_value = 0;
        break;
    // Triangle
    case 0x08:  // $4008: CRRR RRRR, Linear counter setup
        apu->triangle_lnrctl_lenhalt = (val & 0x80);    // Control flag / length counter halt
        apu->triangle_linear_period = (val & 0x7f);     // Linear counter reload value 
        break;
    case 0x09:  // Not used
        break;
    case 0x0a:  // $400A: LLLL LLLL, Timer low (write) 
        apu->triangle_timer_period &= 0xff00;
        apu->triangle_timer_period |= val;  // Timer low (LLLL LLLL)
        apu->triangle_timer_period_bad = ((apu->triangle_timer_period >= 0x7fe) || (apu->triangle_timer_period <= 1));
        break;
    case 0x0b:  // $400B: llll lHHH, Length counter load and timer high
        apu->triangle_timer_period &= 0x00ff;
        apu->triangle_timer_period |= ((unsigned int)(val & 0x07)) << 8;        // Timer period high (HHH)
        apu->triangle_timer_period_bad = ((apu->triangle_timer_period >= 0x7fe) || (apu->triangle_timer_period <= 1));
        apu->triangle_length_value = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll)
        apu->triangle_linear_reload = true;  // site effect: sets the linear counter reload flag 
        //apu->triangle_timer_value = apu->triangle_timer_period;
        break;
    // Noise
    case 0x0c:  // $400C: --LC VVVV, Length counter halt, constant volume/envelope flag, and volume/envelope divider period
        apu->noise_lenhalt_envloop = (val & 0x20);      // Envelope loop/length counter halt (L)
        apu->noise_constant_volume = (val & 0x10);      // Constant volume (C)
        apu->noise_volume_envperiod = (val & 0x0f);     // Volume / Envelope period (VVVV)
        break;
    case 0x0d:  // Not used
        break;
    case 0x0e:  // $400E: M--- PPPP, Mode and period
        apu->noise_mode = (val & 0x80); // Mode Flag (M)
        if (apu->format)    // PAL
        {
            apu->noise_timer_period = noise_timer_period_pal[val & 0x0f];   // Noise period (PPPP) 
        }
        else
        {
            apu->noise_timer_period = noise_timer_period_ntsc[val & 0x0f];  // Noise period (PPPP)
        }
        break;
    case 0x0f:  // $400F: llll l---, Length counter load and envelope restart 
        apu->noise_length_value = length_counter_table[(val & 0xf8) >> 3];  // Length counter value (lllll)
        apu->noise_envelope_start = true;   //Side effects: Sets start flag
        break;
    // DMC
    case 0x10:  // $4010: IL-- RRRR, Flags and Rate
        apu->dmc_loop = (val & 0x40);   // Loop flag (L)
        if (apu->format)
        {
            apu->dmc_timer_period = dmc_timer_period_ntsc[val & 0x0f];  // Rate index (RRRR)
        }
        else
        {
            apu->dmc_timer_period = dmc_timer_period_pal[val & 0x0f];  // Rate index (RRRR)
        }
        break;
    case 0x11:  // $4011: -DDD DDDD, Direct load
        apu->dmc_output = val & 0x7f;   // Direct load (DDD DDDD)
        break;
    case 0x12:  // $4012: AAAA AAAA, Sample address
        apu->dmc_sample_addr = (uint16_t)(0xc000 + ((uint16_t)val << 6)); // Sample address = %11AAAAAA.AA000000 = $C000 + (A * 64)
        break;
    case 0x13:  // $4013: LLLL LLLL, Sample length
        apu->dmc_sample_len = (uint16_t)(((uint16_t)val << 4) + 1); // Sample length = %LLLL.LLLL0001 = (L * 16) + 1 bytes
        break;
    // Status
    case 0x15:  // $4015: ---D NT21, Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1)
        if (val & 0x01)
        {
            apu->pulse[0].enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->pulse[0].enabled = false;
            apu->pulse[0].length_value = 0;
        }
        if (val & 0x02)
        {
            apu->pulse[1].enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->pulse[1].enabled = false;
            apu->pulse[1].length_value = 0;
        }
        if (val & 0x04)
        {
            apu->triangle_enabled = true;
        }
        else
        {
            apu->triangle_enabled = false;
            apu->triangle_length_value = 0;
        }
        if (val & 0x08)
        {
            apu->noise_enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->noise_enabled = false;
            apu->noise_length_value = 0;
        }
        if (val & 0x10)
        {
            apu->dmc_enabled = true;
            // If the DMC bit is set, the DMC sample will be restarted only if its bytes remaining is 0.
            if (0 == apu->dmc_read_remaining)
            {
                apu->dmc_read_addr = apu->dmc_sample_addr;
                apu->dmc_read_remaining = apu->dmc_sample_len;
                if (!apu->dmc_output_shift_reg) 
                {
                    dmc_transfer(apu);
                }
            }
            else
            {
                // If there are bits remaining in the 1-byte sample buffer, these will finish playing before the next sample is fetched.
            }
        }
        else
        {
            apu->dmc_enabled = false;
            // If the DMC bit is clear, the DMC bytes remaining will be set to 0 and the DMC will silence when it empties
            apu->dmc_read_remaining = 0;
        }
        break;
    // Frame counter
    case 0x17:  // $4017:
        apu->sequence_mode = (val & 0x80);
        apu->sequencer_step = 0;
        apu->frame_accu_fp = 0;
        // TODO: Needs test
        // Writing to $4017 with bit 7 set ($80) will immediately clock all of its controlled units at the beginning of the 5-step sequence;
        // with bit 7 clear, only the sequence is reset without clocking any of its units. 
        if (apu->sequence_mode)
        {
            apu->frame_force_clock = true;
        }
        else
        {
            apu->frame_force_clock = false;
        }
        break;
    }
}


void nesapu_add_ram(nesapu_t *apu, size_t offset, uint16_t addr, uint16_t len)
{
    if (len == 0)
        return;
    // We only have one ram cache. If it is used by other ram block, remove it.
    if (apu->ram_active)
    {
        apu->ram_active->cache = NULL;
        apu->ram_active->cache_addr = 0;
        apu->ram_active->cache_len = 0;
    }
    // Allocate cache if not already allocated
    if (!apu->ram_cache)
    {
        apu->ram_cache = (uint8_t *)VGM_MALLOC(NESAPU_RAM_CACHE_SIZE);
    }
    if (apu->ram_cache)
    {
        // Allocate new ram structure and read first cache
        nesapu_ram_t *ram = (nesapu_ram_t *)VGM_MALLOC(sizeof(nesapu_ram_t));
        if (ram)
        {
            ram->offset = offset;
            ram->addr = addr;
            ram->len = len;
            uint16_t toread = len > NESAPU_RAM_CACHE_SIZE ? NESAPU_RAM_CACHE_SIZE : len;
            if (apu->reader->read(apu->reader, apu->ram_cache, offset, toread) == toread)
            {
                ram->cache = apu->ram_cache;
                ram->cache_addr = addr;
                ram->cache_len = toread;
            }
            else
            {
                VGM_FREE(ram);
                ram = NULL;
            }
        }
        if (ram)
        {
            // Insert into ram list
            ram->next = apu->ram_list;
            apu->ram_list = ram;
            apu->ram_active = ram;
        }
    }
    // If anything error (out of memory, cannt read file, etc), just treat the ram does not exist. Music still plays
    // only DMC channel cannot output sample.
}


uint8_t nesapu_read_ram(nesapu_t *apu, uint16_t addr)
{
    // 1. Find which ram block is responsible for the address
    nesapu_ram_t *ram = NULL;
    do
    {
        ram = apu->ram_active;
        if (ram)
        {
            if ((addr >= ram->addr) && (addr < ram->addr + ram->len))
                break;
        }
        ram = apu->ram_list;
        while (ram)
        {
            if ((addr >= ram->addr) && (addr < ram->addr + ram->len))
                break;
            ram = ram->next;
        }
    } while (0);
    if (!ram)
    {
        VGM_PRINTDBG("APU: RAM read at 0x%04d not found\n", addr);
        return 0;
    }
    // 2. Test if we need to refetch ram
    bool refetch = true;
    if (ram == apu->ram_active)
    {
        // If read from active ram, check cache range
        if ((addr >= ram->cache_addr) && (addr < ram->cache_addr + ram->cache_len))
            refetch = false;
    }
    else
    {
        // If not read from active ram, switch active ram
        apu->ram_active->cache = NULL;
        apu->ram_active->cache_addr = 0;
        apu->ram_active->cache_len = 0;
        apu->ram_active = ram;
    }
    // 3. Refetch
    if (refetch)
    {
        size_t offset = ram->offset + addr - ram->addr;
        uint16_t avail = (uint16_t)(ram->addr + ram->len - addr);
        uint16_t toread = avail > NESAPU_RAM_CACHE_SIZE ? NESAPU_RAM_CACHE_SIZE : avail;
        if (apu->reader->read(apu->reader, apu->ram_cache, offset, toread) == toread)
        {
            ram->cache = apu->ram_cache;
            ram->cache_addr = addr;
            ram->cache_len = toread;
        }
        else
        {
            ram->cache = NULL;
            ram->cache_addr = 0;
            ram->cache_len = 0;
        }
    }
    // 4. Read
    uint8_t read = ram->cache ? ram->cache[addr - ram->cache_addr] : 0;
    return read;
}


void nesapu_enable_fade(nesapu_t *apu, unsigned int samples)
{
    if (apu->fadeout_enabled) return;
    apu->fadeout_sequencer_value = NESAPU_FADE_STEPS - 1;
    // We want to decrease fadeout_sequencer_value to 0 in the next samples
    apu->fadeout_period_fp = float_to_q16((float)samples / NESAPU_FADE_STEPS);
    apu->fadeout_accu_fp = 0;
    apu->fadeout_enabled = true;
}


void nesapu_enable_channel(nesapu_t *apu, uint8_t mask, bool enable)
{
    if (mask & NESAPU_CHANNEL_PULSE1) apu->mask_pulse1 = !enable;
    if (mask & NESAPU_CHANNEL_PULSE2) apu->mask_pulse2 = !enable;
    if (mask & NESAPU_CHANNEL_TRIANGLE) apu->mask_triangle = !enable;
    if (mask & NESAPU_CHANNEL_NOISE) apu->mask_noise = !enable;
    if (mask & NESAPU_CHANNEL_DMC) apu->mask_dmc = !enable;
}
