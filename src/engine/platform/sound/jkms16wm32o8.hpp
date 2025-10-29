/*
============================================================================

JKMS16WM32O8 emulation core
by cam900

This file is licensed under zlib license.

============================================================================

zlib License

(C) 2025-present cam900 and contributors

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

============================================================================
*/
#ifndef _JKMS16WM32O8_HPP
#define _JKMS16WM32O8_HPP

#include <algorithm>
#include <array>

namespace jkms16wm32o8
{
	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;
	using s8 = signed char;
	using s16 = signed short;
	using s32 = signed int;
	using s64 = signed long long;

	template<typename T>
	static const inline T bitfield(const T in, const u8 pos)
	{
		return (in >> pos) & 1;
	} // bitfield

	template<typename T>
	static const inline T bitfield(const T in, const u8 pos, const u8 len)
	{
		return (in >> pos) & ((1 << len) - 1);
	} // bitfield

	template<typename T>
	static const inline T clamp(const T in, const T min, const T max)
	{
		return (in < min) ? min : ((in > max) ? max : in);
	} // clamp

	static inline s32 get_target(u16 in)
	{
		const bool sign = bitfield(in, 15);
		const u16 e = bitfield(in, 11, 4);
		const u16 m = bitfield(in, 0, 11);
		s32 out = 0;
		if (e == 0)
		{
			out = m;
		}
		else
		{
			out = (0x800 | m) << (e - 1);
		}
		return sign ? -out : out;
	}

	static inline s32 get_rate(u16 in)
	{
		const u16 e = bitfield(in, 12, 4);
		const u16 m = bitfield(in, 0, 12);
		if (e == 0)
		{
			return m;
		}
		else
		{
			return (0x1000 | m) << (e - 1);
		}
	}

	static inline u32 get_pitch(u16 in)
	{
		const u16 e = bitfield(in, 12, 4);
		const u16 m = bitfield(in, 0, 12);
		return in ? ((0x1000 | m) << e) : 0;
	}

	static constexpr u8 JKMS16WM32_CHANNEL_BITS = 5;
	static constexpr u8 JKMS16WM32_MAX_CHANNELS = 1 << JKMS16WM32_CHANNEL_BITS;
	static constexpr u8 JKMS16WM32_OPERATOR_BITS = 3;
	static constexpr u8 JKMS16WM32_MAX_OPERATORS = 1 << JKMS16WM32_OPERATOR_BITS;
	static constexpr u8 JKMS16WM32_FILTER_BITS = 3;
	static constexpr u8 JKMS16WM32_MAX_FILTERS = 1 << JKMS16WM32_FILTER_BITS;

	// external waveform RAM interface
	class jkms16wm32o8_intf_t
	{
		public:
			jkms16wm32o8_intf_t()
			{
			}

			virtual u16 read_wave(u16 addr) { return 0; }
			virtual void write_wave(u16 addr, u16 wave) {}
	};

	// core class
	class jkms16wm32o8_t
	{
		private:
			class channel_t
			{
				public:
					class operator_t
					{
						private:
							class envelope_t
							{
								private:
									enum envelope_states_t
									{
										ENV_STATE_IDLE = 0,
										ENV_STATE_DELAY,
										ENV_STATE_ATTACK,
										ENV_STATE_DECAY,
										ENV_STATE_SUSTAIN,
										ENV_STATE_RELEASE
									};
								public:
									envelope_t()
										: m_state(ENV_STATE_IDLE)
										, m_env_level(0)
										, m_delay_counter(0)
										, m_enable(false)
										, m_loop(false)
										, m_initial_level(0)
										, m_delay_rate(0)
										, m_attack_target(0)
										, m_attack_rate(0)
										, m_decay_target(0)
										, m_decay_rate(0)
										, m_sustain_target(0)
										, m_sustain_rate(0)
										, m_release_rate(0)
										, m_multiplier(0)
									{
									}

									void reset()
									{
										m_state = ENV_STATE_IDLE;
										m_env_level = 0;
										m_delay_counter = 0;
										m_enable = false;
										m_loop = false;
										m_initial_level = 0;
										m_delay_rate = 0;
										m_attack_target = 0;
										m_attack_rate = 0;
										m_decay_target = 0;
										m_decay_rate = 0;
										m_sustain_target = 0;
										m_sustain_rate = 0;
										m_release_rate = 0;
										m_multiplier = 0;
									}
									void tick();

									void keyon()
									{
										if (m_enable)
										{
											m_state = (m_delay_rate == 0) ? ENV_STATE_ATTACK : ENV_STATE_DELAY;
											m_env_level = get_target(m_initial_level);
											m_delay_counter = get_rate(m_delay_rate);
										}
									}

									void keyoff()
									{
										if (m_enable)
										{
											if (m_release_rate == 0xffff)
											{
												m_env_level = 0;
												m_state = ENV_STATE_IDLE;
											}
											else
											{
												m_state = ENV_STATE_RELEASE;
											}
										}
									}

									s64 result() const { return (s64(m_env_level) * s64(m_multiplier)) >> 15; }

									// getters
									bool enable() const { return m_enable; }
									bool loop() const { return m_loop; }
									u16 initial_level() const { return m_initial_level; }
									u16 delay_rate() const { return m_delay_rate; }
									u16 attack_target() const { return m_attack_target; }
									u16 attack_rate() const { return m_attack_rate; }
									u16 decay_target() const { return m_decay_target; }
									u16 decay_rate() const { return m_decay_rate; }
									u16 sustain_target() const { return m_sustain_target; }
									u16 sustain_rate() const { return m_sustain_rate; }
									u16 release_rate() const { return m_release_rate; }
									s16 multiplier() const { return m_multiplier; }
									// setters
									void set_enable(const bool enable) { m_enable = enable; }
									void set_loop(const bool loop) { m_loop = loop; }
									void set_initial_level(const u16 level) { m_initial_level = level; }
									void set_delay_rate(const u16 rate) { m_delay_rate = rate; }
									void set_attack_target(const u16 target) { m_attack_target = target; }
									void set_attack_rate(const u16 rate) { m_attack_rate = rate; }
									void set_decay_target(const u16 target) { m_decay_target = target; }
									void set_decay_rate(const u16 rate) { m_decay_rate = rate; }
									void set_sustain_target(const u16 target) { m_sustain_target = target; }
									void set_sustain_rate(const u16 rate) { m_sustain_rate = rate; }
									void set_release_rate(const u16 rate) { m_release_rate = rate; }
									void set_multiplier(const s16 multiplier) { m_multiplier = multiplier; }

								private:
									// states
									envelope_states_t m_state = ENV_STATE_IDLE;
									s32 m_env_level = 0;
									s32 m_delay_counter = 0;
									// registers
									bool m_enable = false;
									bool m_loop = false;
									u16 m_initial_level = 0;
									u16 m_delay_rate = 0;
									u16 m_attack_target = 0;
									u16 m_attack_rate = 0;
									u16 m_decay_target = 0;
									u16 m_decay_rate = 0;
									u16 m_sustain_target = 0;
									u16 m_sustain_rate = 0;
									u16 m_release_rate = 0;
									s16 m_multiplier = 0;
							};

							class lfo_t
							{
								private:
									enum lfo_states_t
									{
										LFO_STATE_IDLE = 0,
										LFO_STATE_DELAY,
										LFO_STATE_RUN
									};
								public:
									lfo_t()
										: m_state(LFO_STATE_IDLE)
										, m_lfo_level(0)
										, m_delay_counter(0)
										, m_lfo_sign(1)
										, m_lfo_out(0)
										, m_noise_counter(0)
										, m_lfsr(1)
										, m_enable(0)
										, m_wave(0)
										, m_delay_rate(0)
										, m_target(0)
										, m_rate(0)
										, m_multiplier(0)
										, m_noise_pitch(0)
										, m_initial_lfsr(0)
										, m_lfsr_mask(0)
									{
									}

									void reset()
									{
										m_state = LFO_STATE_IDLE;
										m_lfo_level = 0;
										m_delay_counter = 0;
										m_lfo_sign = 1;
										m_lfo_out = 0;
										m_noise_counter = 0;
										m_lfsr = 1;
										m_enable = false;
										m_wave = 0;
										m_delay_rate = 0;
										m_target = 0;
										m_rate = 0;
										m_multiplier = 0;
										m_noise_pitch = 0;
										m_initial_lfsr = 0;
										m_lfsr_mask = 0;
									}
									void tick();

									void keyon()
									{
										if (m_enable)
										{
											m_state = (m_delay_rate == 0) ? LFO_STATE_RUN : LFO_STATE_DELAY;
											m_lfo_level = 0;
											m_lfo_sign = 1;
											m_lfo_out = 0;
											m_delay_counter = get_target(m_delay_rate);
											m_lfsr = m_initial_lfsr;
										}
									}

									s64 result() const { return (s64(m_lfo_out) * s64(m_multiplier)) >> 15; }

									// getters
									bool enable() const { return m_enable; }
									u8 wave() const { return m_wave; }
									u16 delay_rate() const { return m_delay_rate; }
									u16 target() const { return m_target; }
									u16 rate() const { return m_rate; }
									s16 multiplier() const { return m_multiplier; }
									u16 noise_pitch() const { return m_noise_pitch; }
									u16 initial_lfsr() const { return m_initial_lfsr; }
									u16 lfsr_mask() const { return m_lfsr_mask; }
									// setters
									void set_enable(const bool enable) { m_enable = enable; }
									void set_wave(const u8 wave) { m_wave = wave; }
									void set_delay_rate(const u16 rate) { m_delay_rate = rate; }
									void set_target(const u16 target) { m_target = target; }
									void set_rate(const u16 rate) { m_rate = rate; }
									void set_multiplier(const s16 multiplier) { m_multiplier = multiplier; }
									void set_noise_pitch(const u16 pitch) { m_noise_pitch = pitch; }
									void set_initial_lfsr(const u16 lfsr) { m_initial_lfsr = lfsr; }
									void set_lfsr_mask(const u16 mask) { m_lfsr_mask = mask; }

								private:
									// internal states
									lfo_states_t m_state = LFO_STATE_IDLE;
									s32 m_lfo_level = 0;
									s32 m_delay_counter = 0;
									s32 m_lfo_sign = 1;
									s32 m_lfo_out = 0;
									u32 m_noise_counter = 0;
									u32 m_lfsr = 0;
									// registers
									bool m_enable = false;
									u8 m_wave = 0;
									u16 m_delay_rate = 0;
									u16 m_target = 0;
									u16 m_rate = 0;
									s16 m_multiplier = 0;
									u16 m_noise_pitch = 0;
									u16 m_initial_lfsr = 0;
									u16 m_lfsr_mask = 0;
							};

							class filter_t
							{
								public:
									filter_t()
										: m_l(0)
										, m_h(0)
										, m_b(0)
										, m_d{0}
										, m_f(0)
										, m_q(0)
										, m_enable(false)
										, m_lp_enable(false)
										, m_hp_enable(false)
										, m_bp_enable(false)
									{
									}

									void reset()
									{
										m_l = 0;
										m_h = 0;
										m_b = 0;
										m_d[0] = m_d[1] = 0;
										m_f = 0;
										m_q = 0;
										m_enable = false;
										m_lp_enable = false;
										m_hp_enable = false;
										m_bp_enable = false;
									}

									void tick(const s32 input);

									void keyon()
									{
										m_l = 0;
										m_h = 0;
										m_b = 0;
										m_d[0] = m_d[1] = 0;
									}

									s32 lowpass_output() const { return m_l; }
									s32 highpass_output() const { return m_h; }
									s32 bandpass_output() const { return m_b; }

									// getters
									s32 f() const { return m_f; }
									s32 q() const { return m_q; }
									bool enable() const { return m_enable; }
									bool lp_enable() const { return m_lp_enable; }
									bool hp_enable() const { return m_hp_enable; }
									bool bp_enable() const { return m_bp_enable; }

									// setters
									void set_f(const s32 f) { m_f = f; }
									void set_q(const s32 q) { m_q = q; }
									void set_enable(const bool enable) { m_enable = enable; }
									void set_lp_enable(const bool enable) { m_lp_enable = enable; }
									void set_hp_enable(const bool enable) { m_hp_enable = enable; }
									void set_bp_enable(const bool enable) { m_bp_enable = enable; }

								private:
									// internal states
									s32 m_l = 0;
									s32 m_h = 0;
									s32 m_b = 0;
									std::array<s32, 2> m_d = {0};

									// registers
									s32 m_f = 0;
									s32 m_q = 0;
									bool m_enable = false;
									bool m_lp_enable = false;
									bool m_hp_enable = false;
									bool m_bp_enable = false;
							};

							class mod_in_t
							{
								public:
									mod_in_t()
										: m_input(0)
										, m_enable(false)
										, m_multiplier(0)
									{
									}

									void reset()
									{
										m_input = 0;
										m_enable = false;
										m_multiplier = 0;
									}

									void add_input(const s32 out) { m_input += out; }

									void reset_input() { m_input = 0; }

									s32 get_input() const { return (s32(m_input) * s32(m_multiplier)) >> 15; }

									// getters
									bool enable() const { return m_enable; }
									s16 multiplier() const { return m_multiplier; }

									// setters
									void set_enable(const bool enable) { m_enable = enable; }
									void set_multiplier(const s16 multiplier) { m_multiplier = multiplier; }

								private:
									// internal state
									s16 m_input = 0;
									// register
									bool m_enable = false;
									s16 m_multiplier = 0;
							};

							class mod_out_t
							{
								public:
									mod_out_t()
										: m_enable(false)
										, m_matrix(0)
										, m_multiplier(0)
										, m_feedback(0)
									{
									}

									void reset()
									{
										m_enable = false;
										m_matrix = 0;
										m_multiplier = 0;
										m_feedback = 0;
									}

									s32 get_feedback(s32 op_out) const { return (op_out * m_feedback) >> 15; }
									s32 get_output(s32 op_out) const { return (op_out * m_multiplier) >> 15; }

									// getters
									bool enable() const { return m_enable; }
									u8 matrix() const { return m_matrix; }
									s16 multiplier() const { return m_multiplier; }
									s16 feedback() const { return m_feedback; }

									// setters
									void set_enable(const bool enable) { m_enable = enable; }
									void set_matrix(const u8 matrix) { m_matrix = matrix; }
									void set_multiplier(const s16 multiplier) { m_multiplier = multiplier; }
									void set_feedback(const s16 feedback) { m_feedback = feedback; }

								private:
									// register
									bool m_enable = false;
									u8 m_matrix = 0;
									s16 m_multiplier = 0;
									s16 m_feedback = 0;
							};

							class exwave_mod_t
							{
								public:
									exwave_mod_t()
										: m_enable(false)
										, m_bitpos(false)
									{
									}

									void reset()
									{
										m_enable = false;
										m_bitpos = 0;
									}

									// getters
									bool enable() const { return m_enable; }
									u16 bitpos() const { return m_bitpos; }
									// setters
									void set_enable(const bool enable) { m_enable = enable; }
									void set_bitpos(const u16 bitpos) { m_bitpos = bitpos; }

								private:
									bool m_enable = false;
									u16 m_bitpos = 0;
							};

						public:
							operator_t(channel_t &host)
								: m_host(host)
								, m_env(envelope_t())
								, m_flfo(lfo_t())
								, m_alfo(lfo_t())
								, m_filter{
									filter_t(), filter_t(), filter_t(), filter_t(),
									filter_t(), filter_t(), filter_t(), filter_t()
								}
								, m_fm_in(mod_in_t())
								, m_pm_in(mod_in_t())
								, m_am_in(mod_in_t())
								, m_fm_out(mod_out_t())
								, m_pm_out(mod_out_t())
								, m_am_out(mod_out_t())
								, m_mute(exwave_mod_t())
								, m_reverse(exwave_mod_t())
								, m_invert(exwave_mod_t())
								, m_is_keyon(false)
								, m_wave_addr(0)
								, m_frac(0)
								, m_wave_out(0)
								, m_op_out(0)
								, m_lout(0)
								, m_rout(0)
								, m_noise_counter(0)
								, m_lfsr(1)
								, m_busy(false)
								, m_speaker_out_enable(false)
								, m_direct_out(false)
								, m_filter_out(false)
								, m_total_level(0)
								, m_wave_base(0)
								, m_wave_bit(0)
								, m_int_wave_size(0)
								, m_ext_wave_size(0)
								, m_pitch(0)
								, m_duty(0)
								, m_speaker_vol(0)
								, m_speaker_lvol(0)
								, m_speaker_rvol(0)
								, m_noise_pitch(0)
								, m_initial_lfsr(0)
								, m_lfsr_mask(0)
							{
							}

							void reset();
							void tick();

							void keyon()
							{
								if (!m_is_keyon)
								{
									m_wave_addr = 0;
									m_frac = 0;
									m_wave_out = 0;
									m_op_out = 0;
									m_lout = 0;
									m_rout = 0;
									m_noise_counter = 0;
									m_lfsr = m_initial_lfsr;
									m_fm_in.reset_input();
									m_pm_in.reset_input();
									m_am_in.reset_input();
									if (m_env.enable())
									{
										m_env.keyon();
									}
									if (m_flfo.enable())
									{
										m_flfo.keyon();
									}
									if (m_alfo.enable())
									{
										m_alfo.keyon();
									}
									for (filter_t &filter : m_filter)
									{
										if (filter.enable())
										{
											filter.keyon();
										}
									}
									m_busy = m_is_keyon = true;
								}
							}
							void keyoff()
							{
								if (m_is_keyon)
								{
									if (m_env.enable())
									{
										m_env.keyoff();
									}
									else
									{
										m_busy = false;
									}
									m_is_keyon = false;
								}
							}

							envelope_t &env() { return m_env; }
							lfo_t &flfo() { return m_flfo; }
							lfo_t &alfo() { return m_alfo; }
							filter_t &filter(const u8 slot) { return m_filter[slot & 3]; }
							mod_in_t &fm_in() { return m_fm_in; }
							mod_in_t &pm_in() { return m_pm_in; }
							mod_in_t &am_in() { return m_am_in; }
							mod_out_t &fm_out() { return m_fm_out; }
							mod_out_t &pm_out() { return m_pm_out; }
							mod_out_t &am_out() { return m_am_out; }
							exwave_mod_t &mute() { return m_mute; }
							exwave_mod_t &reverse() { return m_reverse; }
							exwave_mod_t &invert() { return m_invert; }

							// getters
							bool is_keyon() const { return m_is_keyon; }
							bool busy() const { return m_busy; }
							bool speaker_out_enable() const { return m_speaker_out_enable; }
							bool direct_out() const { return m_direct_out; }
							bool filter_out() const { return m_filter_out; }
							s16 total_level() const { return m_total_level; }
							u16 wave_base() const { return m_wave_base; }
							u8 wave_bit() const { return m_wave_bit; }
							u8 int_wave_size() const { return m_int_wave_size; }
							u8 ext_wave_size() const { return m_ext_wave_size; }
							u16 pitch() const { return m_pitch; }
							u16 duty() const { return m_duty; }
							s16 speaker_vol() const { return m_speaker_vol; }
							s16 speaker_lvol() const { return m_speaker_lvol; }
							s16 speaker_rvol() const { return m_speaker_rvol; }
							u16 noise_pitch() const { return m_noise_pitch; }
							u16 initial_lfsr() const { return m_initial_lfsr; }
							u16 lfsr_mask() const { return m_lfsr_mask; }
							// setters
							void set_busy(const bool busy) { m_busy = busy; }
							void set_speaker_out_enable(const bool enable) { m_speaker_out_enable = enable; }
							void set_direct_out(const bool enable) { m_direct_out = enable; }
							void set_filter_out(const bool enable) { m_filter_out = enable; }
							void set_total_level(const s16 level) { m_total_level = level; }
							void set_wave_base(const u16 base) { m_wave_base = base; }
							void set_wave_bit(const u8 bit) { m_wave_bit = bit; }
							void set_int_wave_size(const u8 size) { m_int_wave_size = size; }
							void set_ext_wave_size(const u8 size) { m_ext_wave_size = size; }
							void set_pitch(const u16 pitch) { m_pitch = pitch; }
							void set_duty(const u16 duty) { m_duty = duty; }
							void set_speaker_vol(const s16 vol) { m_speaker_vol = vol; }
							void set_speaker_lvol(const s16 lvol) { m_speaker_lvol = lvol; }
							void set_speaker_rvol(const s16 rvol) { m_speaker_rvol = rvol; }
							void set_noise_pitch(const u16 pitch) { m_noise_pitch = pitch; }
							void set_initial_lfsr(const u16 lfsr) { m_initial_lfsr = lfsr; }
							void set_lfsr_mask(const u16 mask) { m_lfsr_mask = mask; }

							// for debug
							s32 lout() const { return m_lout; }
							s32 rout() const { return m_rout; }
						private:
							// classes / structs
							channel_t &m_host;
							envelope_t m_env;
							lfo_t m_flfo;
							lfo_t m_alfo;
							std::array<filter_t, JKMS16WM32_MAX_FILTERS> m_filter;
							mod_in_t m_fm_in;
							mod_in_t m_pm_in;
							mod_in_t m_am_in;
							mod_out_t m_fm_out;
							mod_out_t m_pm_out;
							mod_out_t m_am_out;
							exwave_mod_t m_mute;
							exwave_mod_t m_reverse;
							exwave_mod_t m_invert;
							// internal states
							bool m_is_keyon = false;
							u32 m_wave_addr = 0;
							u32 m_frac = 0;
							s32 m_wave_out = 0;
							s32 m_op_out = 0;
							s32 m_lout = 0;
							s32 m_rout = 0;
							s32 m_noise_counter = 0;
							u32 m_lfsr = 0;
							// registers
							bool m_busy = false;
							bool m_speaker_out_enable = false;
							bool m_direct_out = false;
							bool m_filter_out = false;
							s16 m_total_level = 0;
							u16 m_wave_base = 0;
							u8 m_wave_bit = 0;
							u8 m_int_wave_size = 0;
							u8 m_ext_wave_size = 0;
							u16 m_pitch = 0;
							u16 m_duty = 0;
							s16 m_speaker_vol = 0;
							s16 m_speaker_lvol = 0;
							s16 m_speaker_rvol = 0;
							u16 m_noise_pitch = 0;
							u16 m_initial_lfsr = 0;
							u16 m_lfsr_mask = 0;
					};
				public:
					channel_t(jkms16wm32o8_t &host)
						: m_host(host)
						, m_op{
								operator_t(*this), operator_t(*this),
								operator_t(*this), operator_t(*this),
								operator_t(*this), operator_t(*this),
								operator_t(*this), operator_t(*this)
							}
						, m_lout(0)
						, m_rout(0)
						, m_lvol(0)
						, m_rvol(0)
					{
					}

					void reset()
					{
						for (operator_t &op : m_op)
						{
							op.reset();
						}
						m_lout = 0;
						m_rout = 0;
						m_lvol = 0;
						m_rvol = 0;
					}

					void tick();

					operator_t &op(const u8 op) { return m_op[op]; }

					// getters
					s32 lvol() const { return m_lvol; }
					s32 rvol() const { return m_rvol; }
					// setters
					void set_lvol(const s32 lvol) { m_lvol = lvol; }
					void set_rvol(const s32 rvol) { m_rvol = rvol; }

					s32 lout() const { return m_lout; }
					s32 rout() const { return m_rout; }
				private:
					// classes / structs
					jkms16wm32o8_t &m_host;
					std::array<operator_t, JKMS16WM32_MAX_OPERATORS> m_op;
					// internal states
					s32 m_lout = 0;
					s32 m_rout = 0;
					// registers
					s16 m_lvol = 0;
					s16 m_rvol = 0;
			};
		public:
			jkms16wm32o8_t(jkms16wm32o8_intf_t &intf)
				: m_intf(intf)
				, m_channel{
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
					channel_t(*this), channel_t(*this), channel_t(*this), channel_t(*this),
				}
				, m_register_select(0)
				, m_sound_enable(false)
				, m_op_select(0)
				, m_channel_select(0)
			{
			}

			void reset();
			void tick();

			u16 host_r(const u8 addr);
			void host_w(const u8 addr, const u16 data);

			// for debug
			channel_t &channel(const u8 ch) { return m_channel[ch & 0x1f]; }
			s32 lout() const { return m_lout; }
			s32 rout() const { return m_rout; }

		private:
			u16 regs_r();
			void regs_w(const u16 data);

			// classes / structs
			jkms16wm32o8_intf_t &m_intf;
			std::array<channel_t, JKMS16WM32_MAX_CHANNELS> m_channel;
			// internal states
			s32 m_lout = 0;
			s32 m_rout = 0;
			// registers
			u16 m_waveram_addr = 0;
			u16 m_waveram_modulo = 0;
			u8 m_register_select = 0;
			bool m_sound_enable = false;
			u8 m_op_select = 0;
			u8 m_channel_select = 0;
	};
}; // namespace jkms16wm32o8

#endif // _JKMS16WM32O8_HPP