#pragma once

#include "common.hpp"

#include <cmath>

namespace gameboy {
    namespace spu {
        u8 s = 0;

        int16_t generate_wave_sample(u16& sample, double t, double f, double a) {
            if ((!f) || (!a)) return 0x0;

            double c = ((double)SPU_SAMPLERATE / f),
                   sl = c / 32;

            uint32_t sll = std::round(sl);

            if (!(sll)) return 0x0;

            if (!((uint32_t)std::round(t) % sll)) {
                sample++;
                sample &= 0x1f;
                s = (wave_ram.at(((sample & 0xfffe) >> 1)) >> ((1 - sample & 0x1) * 4)) & 0xf;
            }

            return s * 0x7ff;
        }
        
        struct wave_t {
            double clk = 0.0;

            u8* nr = nullptr;

            struct current_sound_t {
                bool playing = false,
                     infinite = false;

                size_t remaining_samples = 0;
                u16    current_sample_idx = 0;

                double freq = 0.0, amp = 0.0;
            } cs;

            int16_t get_sample() {
                if (cs.playing) {
                    if (cs.infinite ? (cs.remaining_samples--) : (cs.remaining_samples--)) {
                        int16_t sample = generate_wave_sample(cs.current_sample_idx, clk++, cs.freq, cs.amp);

                        return sample;
                    } else {
                        cs.playing = false;
                        return 0;
                    }
                } else {
                    return 0;
                }
            }

            void update_state() {
                if (TEST_REG(SPUNR_CTRL, CTRL_RESTR)) {
                    SDL_ClearQueuedAudio(dev);

                    bool i = !(nr[SPUNR_CTRL] & CTRL_LENCT);

                    double r = nr[SPUNR_FREQ] & 0x7,
                           s = (nr[SPUNR_FREQ] & 0xf0) >> 4;

                    u16 rf = (nr[SPUNR_FREQ] | ((nr[SPUNR_CTRL] & CTRL_FREQH) << 8)) & 0x7ff;

                    int f = 4194304 / (64 * (2048 - rf));
                    // /double f = 65536 / (2048 - rf);
                    //_log(debug, "rf=%04x,f=%i", rf, f);

                    //_log(debug, "rf=%04x (%li), f=%f", rf, rf, f);

                    // (256-t1)*(1/256)
                    size_t l = ((double)(256 - nr[SPUNR_LENC]) / 256) * SPU_SAMPLERATE;

                    cs = {
                        true,   // cs.playing
                        i,      // cs.infinite
                        l,      // cs.remaining_samples
                        0,      // cs.current_sample_idx
                        f,      // cs.freq
                        1       // cs.amp
                    };

                    //_log(debug,
                    //    "full ch4 state:\n\tplaying=%s\n\ti=%s\n\tenvc=%s\n\tlen=%llu\n\tenvc=%llu\n\tenvl=%llu\n\tenvr=%llu\n\tenvs=%f\n\tenvd=%s\n\tf=%f\n\ta=%f",
                    //    cs.playing ? "true" : "false",
                    //    cs.infinite ? "true" : "false",
                    //    cs.env_enabled ? "true" : "false",
                    //    cs.remaining_samples,
                    //    cs.env_step_count,
                    //    cs.env_step_length,
                    //    cs.env_step_remaining,
                    //    cs.env_step,
                    //    cs.env_direction ? "up" : "down",
                    //    cs.current_freq,
                    //    cs.current_amp
                    //);
                }
            }

            void init(u8& nr){
                this->nr = &nr;
            }
        } ch3;
    }
}