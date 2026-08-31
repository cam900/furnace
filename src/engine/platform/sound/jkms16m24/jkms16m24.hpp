/*

============================================================================

JKMS16M24 emulator
by cam900

This file is licensed under zlib license.

============================================================================

zlib License

(C) 2026-present cam900 and contributors

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

#ifndef _JKMS16M24_H
#define _JKMS16M24_H

#include <algorithm>
#include <array>
#include <cstdint>

namespace jkms16m24
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;
	using s8 = std::int8_t;
	using s16 = std::int16_t;
	using s32 = std::int32_t;
	using s64 = std::int64_t;

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

	template<typename T>
	static const inline T clamp16(const T in)
	{
		return clamp<T>(in, -0x8000, 0x7fff);
	} // clamp

	#define GETTER(T, var) \
		T get_##var() const { return m_##var; }

	#define SETTER(T, var) \
		void set_##var(const T var) { m_##var = var; }

	#define SETGET(T, var) \
		SETTER(T, var) \
		GETTER(T, var)

	// interface class
	class jkms16m24_intf_t
	{
		public:
			jkms16m24_intf_t()
			{
			}

			virtual s16 sample_r(u16 addr) { return 0; }
			virtual void sample_w(u16 addr, s16 data) {}
	};

	// main class
	class jkms16m24_t
	{
		private:
			class group_t
			{
				private:
					class operator_t
					{
						private:
							class wavegen_t
							{
								public:
									wavegen_t(jkms16m24_t &host)
										: m_host(host)
										, m_int_wave_enable(false)
										, m_delta_noise(false)
										, m_white_noise(false)
										, m_periodic_noise(false)
										, m_pulse(false)
										, m_inv_pulse(false)
										, m_sawtooth(false)
										, m_inv_sawtooth(false)
										, m_triangle(false)
										, m_inv_triangle(false)
										, m_pulse_width(0)
										, m_wave_len(0)
										, m_ext_wave_enable(false)
										, m_hinv_enable(false)
										, m_hinv_bitoffs(0)
										, m_vinv_enable(false)
										, m_vinv_bitoffs(0)
										, m_mute_enable(false)
										, m_mute_bitoffs(0)
										, m_base_addr(0)
										, m_ext_wave_len(0)
										, m_lfsr_init(1)
										, m_lfsr_mask(0)
										, m_lfsr_scale(0)
										, m_lfsr_state(1)
										, m_lfsr_wave(0)
									{
									}

									void reset()
									{
										m_int_wave_enable = false;
										m_delta_noise = false;
										m_white_noise = false;
										m_periodic_noise = false;
										m_pulse = false;
										m_inv_pulse = false;
										m_sawtooth = false;
										m_inv_sawtooth = false;
										m_triangle = false;
										m_inv_triangle = false;
										m_pulse_width = 0;
										m_wave_len = 0;

										m_ext_wave_enable = false;
										m_hinv_enable = false;
										m_hinv_bitoffs = 0;
										m_vinv_enable = false;
										m_vinv_bitoffs = 0;
										m_mute_enable = false;
										m_mute_bitoffs = 0;
										m_base_addr = 0;
										m_ext_wave_len = 0;

										m_lfsr_init = 1;
										m_lfsr_mask = 0;
										m_lfsr_scale = 0;

										m_lfsr_state = 1;
										m_lfsr_wave = 0;
									}

									void keyon()
									{
										m_lfsr_state = m_lfsr_init;
										m_lfsr_wave = 0;
									}

									void lfsr_tick()
									{
										u8 update = 0;
										for (u8 i = 0; i < 16; i++)
										{
											if (bitfield(m_lfsr_mask, i))
												update ^= bitfield(m_lfsr_state, i);
										}
										m_lfsr_state = (m_lfsr_state >> 1) | (u32(update) << 15);
										if (m_lfsr_state == 0)
											m_lfsr_state = 1;

										if (bitfield(m_lfsr_state, 0))
											m_lfsr_wave = std::min<s32>(m_lfsr_wave + m_lfsr_scale, 0x7fff);
										else
											m_lfsr_wave = std::max<s32>(m_lfsr_wave - m_lfsr_scale, -0x8000);
									}

									s16 get_int_wave(const u16 addr) const
									{
										if (!m_int_wave_enable)
											return 0;

										const u16 idx = addr >> m_wave_len;
										u16 output = 0;
										if (m_delta_noise)
											output ^= u16(m_lfsr_wave) ^ 0x8000;
										if (m_white_noise)
											output ^= m_lfsr_state & 0xffff;
										if (m_periodic_noise)
											output ^= bitfield(m_lfsr_state, 0) ? 0xffff : 0;
										if (m_triangle)
											output ^= bitfield(idx, 15) ? (0xffff - (idx << 1)) : (idx << 1);
										if (m_inv_triangle)
											output ^= bitfield(~idx, 15) ? (0xffff - (idx << 1)) : (idx << 1);
										if (m_sawtooth)
											output ^= idx;
										if (m_inv_sawtooth)
											output ^= 0xffff - idx;
										if (m_pulse)
											output ^= (idx < m_pulse_width) ? 0 : 0xffff;
										if (m_inv_pulse)
											output ^= (idx > m_pulse_width) ? 0 : 0xffff;
										return s16(output ^ 0x8000);
									}

									s16 get_ext_wave(const u16 addr) const
									{
										if ((!m_ext_wave_enable) || (m_mute_enable && bitfield(addr, 16 - m_mute_bitoffs)))
											return 0;
										else
										{
											s16 output = m_host.m_intf.sample_r(get_ext_addr(addr));
											if (m_vinv_enable && bitfield(addr, 16 - m_vinv_bitoffs))
												output = -output;
											return output;
										}
									}

									SETGET(bool, int_wave_enable)
									SETGET(bool, delta_noise)
									SETGET(bool, white_noise)
									SETGET(bool, periodic_noise)
									SETGET(bool, pulse)
									SETGET(bool, inv_pulse)
									SETGET(bool, sawtooth)
									SETGET(bool, inv_sawtooth)
									SETGET(bool, triangle)
									SETGET(bool, inv_triangle)
									SETGET(u16, pulse_width)
									SETGET(u8, wave_len)
									SETGET(bool, ext_wave_enable)
									SETGET(bool, hinv_enable)
									SETGET(u8, hinv_bitoffs)
									SETGET(bool, vinv_enable)
									SETGET(u8, vinv_bitoffs)
									SETGET(bool, mute_enable)
									SETGET(u8, mute_bitoffs)
									SETGET(u16, base_addr)
									SETGET(u8, ext_wave_len)
									SETGET(u32, lfsr_init)
									SETGET(u32, lfsr_mask)
									SETGET(s16, lfsr_scale)

								private:
									jkms16m24_t &m_host;

									// internal
									bool m_int_wave_enable = false;
									// waveform
									bool m_delta_noise = false;
									bool m_white_noise = false;
									bool m_periodic_noise = false;
									bool m_pulse = false;
									bool m_inv_pulse = false;
									bool m_sawtooth = false;
									bool m_inv_sawtooth = false;
									bool m_triangle = false;
									bool m_inv_triangle = false;
									u16 m_pulse_width = 0;
									u8 m_wave_len = 0;

									// external
									bool m_ext_wave_enable = false;
									// horizontal invert waveform
									bool m_hinv_enable = false;
									u8 m_hinv_bitoffs = 0;
									// vertical invert waveform
									bool m_vinv_enable = false;
									u8 m_vinv_bitoffs = 0;
									// mute waveform
									bool m_mute_enable = false;
									u8 m_mute_bitoffs = 0;
									// base address
									u16 m_base_addr = 0;
									u8 m_ext_wave_len = 0;

									// lfsr
									u32 m_lfsr_init = 1;
									u32 m_lfsr_mask = 0;
									s16 m_lfsr_scale = 0;

									u32 m_lfsr_state = 1;
									s32 m_lfsr_wave = 0;

									// functions
									u16 get_ext_addr(const u16 addr) const
									{
										const u16 mask = 0xffff >> m_ext_wave_len;
										u16 output = addr & mask;
										if (m_hinv_enable && bitfield(addr, 16 - m_hinv_bitoffs));
											output = mask - output;
										return m_base_addr + output;
									}
							};

							class envelope_t
							{
								public:
									envelope_t()
										: m_enable(false)
										, m_loop(false)
										, m_delay_time(0)
										, m_attack_rate(0)
										, m_attack_target(0)
										, m_decay_rate(0)
										, m_decay_target(0)
										, m_sustain_rate(0)
										, m_sustain_target(0)
										, m_release_rate(0)
										, m_busy(false)
										, m_env_state(IDLE)
										, m_delay(0)
										, m_rate(0)
										, m_target(0)
										, m_output(0)
									{
									}

									void reset()
									{
										// control
										m_enable = false;
										m_loop = false;

										// parameters
										m_delay_time = 0;
										m_attack_rate = 0;
										m_attack_target = 0;
										m_decay_rate = 0;
										m_decay_target = 0;
										m_sustain_rate = 0;
										m_sustain_target = 0;
										m_release_rate = 0;

										// status
										m_busy = false;
										m_env_state = IDLE;
										m_delay = 0;
										m_rate = 0;
										m_target = 0;
										m_output = 0;
									}

									void keyon()
									{
										if (m_enable)
										{
											m_delay = get_env_rate(m_delay_time);
											m_env_state = DELAY;
											m_busy = true;
										}
										else
										{
											m_env_state = IDLE;
											m_busy = false;
										}
									}

									void keyoff()
									{
										if (m_env_state != RELEASE)
										{
											m_rate = get_env_rate(m_release_rate);
											m_target = 0;
											m_env_state = RELEASE;
										}
									}

									void tick()
									{
										if (!m_busy)
											return;

										switch (m_env_state)
										{
											case DELAY:
												if (m_delay-- <= 0)
												{
													m_rate = get_env_rate(m_attack_rate);
													m_target = get_env_target(m_attack_target);
													m_env_state = ATTACK;
												}
												break;
											case ATTACK:
												if (m_target > m_output)
													m_output = std::min<s32>(m_output + m_rate, m_target);
												else if (m_target < m_output)
													m_output = std::max<s32>(m_output - m_rate, m_target);
												if (m_output == m_target)
												{
													m_rate = get_env_rate(m_decay_rate);
													m_target = get_env_target(m_decay_target);
													m_env_state = DECAY;
												}
												break;
											case DECAY:
												if (m_target > m_output)
													m_output = std::min<s32>(m_output + m_rate, m_target);
												else if (m_target < m_output)
													m_output = std::max<s32>(m_output - m_rate, m_target);
												if (m_output == m_target)
												{
													m_rate = get_env_rate(m_sustain_rate);
													m_target = get_env_target(m_sustain_target);
													m_env_state = SUSTAIN;
												}
												break;
											case SUSTAIN:
												if (m_target > m_output)
													m_output = std::min<s32>(m_output + m_rate, m_target);
												else if (m_target < m_output)
													m_output = std::max<s32>(m_output - m_rate, m_target);
												if (m_output == m_target)
												{
													if (m_loop)
													{
														m_rate = get_env_rate(m_decay_rate);
														m_target = get_env_target(m_decay_target);
														m_env_state = DECAY;
													}
													else
													{
														m_rate = 0;
														m_target = 0;
														m_env_state = IDLE;
													}
												}
												break;
											case RELEASE:
												if (0 > m_output)
													m_output = std::min<s32>(m_output + m_rate, 0);
												else if (0 < m_output)
													m_output = std::max<s32>(m_output - m_rate, 0);
												if (m_output == 0)
												{
													m_rate = 0;
													m_target = 0;
													m_env_state = IDLE;
												}
												break;
										}
									}

									// getter/setters
									SETGET(bool, enable)
									SETGET(bool, loop)
									SETGET(u16, delay_time)
									SETGET(u16, attack_rate)
									SETGET(u16, attack_target)
									SETGET(u16, decay_rate)
									SETGET(u16, decay_target)
									SETGET(u16, sustain_rate)
									SETGET(u16, sustain_target)
									SETGET(u16, release_rate)

									GETTER(s32, output)

								private:
									// state
									enum state_t : u8
									{
										IDLE,
										DELAY,
										ATTACK,
										DECAY,
										SUSTAIN,
										RELEASE
									};

									// control
									bool m_enable = false;
									bool m_loop = false;

									// parameters
									u16 m_delay_time = 0;
									u16 m_attack_rate = 0;
									u16 m_attack_target = 0;
									u16 m_decay_rate = 0;
									u16 m_decay_target = 0;
									u16 m_sustain_rate = 0;
									u16 m_sustain_target = 0;
									u16 m_release_rate = 0;

									// status
									bool m_busy = false;
									state_t m_env_state = IDLE;
									s32 m_delay = 0;
									s32 m_rate = 0;
									s32 m_target = 0;
									s32 m_output = 0;

									s32 get_env_rate(const u16 reg) const
									{
										const u16 mantissa = bitfield(reg, 0, 12);
										const u8 exponent = bitfield(reg, 12, 4);
										if (exponent == 0)
											return mantissa;
										else
											return (0x1000 | mantissa) << (exponent - 1);
									}

									s32 get_env_target(const u16 reg) const
									{
										const u16 mantissa = bitfield(reg, 0, 11);
										const u8 exponent = bitfield(reg, 11, 4);
										const bool issign = bitfield(reg, 15);
										s32 out = 0;
										if (exponent == 0)
											out = mantissa;
										else
											out = (0x800 | mantissa) << (exponent - 1);
										out = std::min<s32>(out, 0x2000000);
										return issign ? -out : out;
									}
							};

							class lfo_t
							{
								public:
									lfo_t()
										: m_enable(false)
										, m_invert(false)
										, m_waveform(OFF)
										, m_lfo_delay_time(0)
										, m_lfo_rate(0)
										, m_busy(false)
										, m_lfo_state(IDLE)
										, m_delay(0)
										, m_rate(0)
										, m_target(0)
										, m_output(0)
									{
									}

									void reset()
									{
										m_enable = false;
										m_invert = false;
										m_waveform = OFF;

										m_lfo_delay_time = 0;
										m_lfo_rate = 0;

										m_busy = false;
										m_lfo_state = IDLE;
										m_delay = 0;
										m_rate = 0;
										m_target = 0;

										m_output = 0;
									}

									void keyon()
									{
										if (m_enable)
										{
											m_delay = get_lfo_rate(m_lfo_delay_time);
											m_lfo_state = DELAY;
											m_busy = true;
										}
										else
										{
											m_lfo_state = IDLE;
											m_busy = false;
										}
									}

									void tick()
									{
										if (!m_busy)
											return;
										switch (m_lfo_state)
										{
											case DELAY:
												if (m_delay-- <= 0)
												{
													m_rate = get_lfo_rate(m_lfo_rate);
													m_target = m_invert ? -0x2000000 : 0x2000000;
													m_lfo_state = ACTIVE;
												}
												break;
											case ACTIVE:
												if (m_target > m_output)
													m_output = std::min<s32>(m_output + m_rate, m_target);
												else if (m_target < m_output)
													m_output = std::max<s32>(m_output - m_rate, m_target);
												if (m_output == m_target)
												{
													switch (m_waveform)
													{
														default:
														case TRIANGLE:
														case SQUARE:
															m_target = -m_target;
															break;
														case SAWTOOTH:
															m_output = -m_output;
															break;
													}
												}
												break;
										}
									}

									// getter/setters
									SETGET(bool, enable)
									SETGET(bool, invert)

									void set_waveform(const u8 wave)
									{
										m_waveform = waveform_t(wave);
									}
									u8 get_waveform() const
									{
										return u8(m_waveform);
									}

									SETGET(u16, lfo_delay_time)
									SETGET(u16, lfo_rate)

									s32 get_output() const
									{
										switch (m_waveform)
										{
											case OFF:
												return 0;
											case SQUARE:
												return ((m_lfo_state == ACTIVE) && (m_target > 0)) ? 0x2000000 : -0x2000000;
										}
										return m_output;
									}

								private:
									// state
									enum state_t : u8
									{
										IDLE,
										DELAY,
										ACTIVE
									};

									enum waveform_t : u8
									{
										OFF,
										TRIANGLE,
										SAWTOOTH,
										SQUARE
									};

									bool m_enable = false;
									bool m_invert = false;
									waveform_t m_waveform = OFF;

									// parameters
									u16 m_lfo_delay_time = 0;
									u16 m_lfo_rate = 0;

									// status
									bool m_busy = false;
									state_t m_lfo_state = IDLE;
									s32 m_delay = 0;
									s32 m_rate = 0;
									s32 m_target = 0;

									s32 m_output = 0;

									s32 get_lfo_rate(const u16 reg) const
									{
										const u16 mantissa = bitfield(reg, 0, 12);
										const u8 exponent = bitfield(reg, 12, 4);
										if (exponent == 0)
											return mantissa;
										else
											return (0x1000 | mantissa) << (exponent - 1);
									}
							};

							class filter_t
							{
								private:
									class filter_pole_t
									{
										public:
											filter_pole_t()
												: m_enable(false)
												, m_add_lp(false)
												, m_add_hp(false)
												, m_add_bp(false)
												, m_cutoff(0)
												, m_resonance(0)
												, m_scale(0)
												, m_lp(0)
												, m_hp(0)
												, m_bp(0)
												, m_output(0)
											{
											}

											void reset()
											{
												m_enable = false;
												m_add_lp = false;
												m_add_hp = false;
												m_add_bp = false;

												m_cutoff = 0;
												m_resonance = 0;
												m_scale = 0;

												m_lp = 0;
												m_hp = 0;
												m_bp = 0;

												m_output = 0;
											}

											void tick(const s32 in)
											{
												m_output = 0;
												if (!m_enable)
													return;

												m_lp = (m_lp + (m_cutoff * m_bp) >> 15);
												m_hp = in - m_lp - (((0x1000 - m_resonance) * m_bp) >> 12);
												m_bp = ((m_cutoff * m_hp) >> 15) + m_bp;

												if (m_add_lp)
													m_output += m_lp;
												if (m_add_hp)
													m_output += m_hp;
												if (m_add_bp)
													m_output += m_bp;

												m_output = (m_output * m_scale) >> 15;
											}

											SETGET(bool, enable)
											SETGET(bool, add_lp)
											SETGET(bool, add_hp)
											SETGET(bool, add_bp)
											SETGET(s16, cutoff)
											SETGET(u16, resonance)
											SETGET(s16, scale)

											GETTER(s32, output)

										private:
											// control
											bool m_enable = false;
											bool m_add_lp = false;
											bool m_add_hp = false;
											bool m_add_bp = false;

											// parameter
											s16 m_cutoff = 0;
											u16 m_resonance = 0;
											s16 m_scale = 0;

											// status
											s32 m_lp = 0;
											s32 m_hp = 0;
											s32 m_bp = 0;

											s32 m_output = 0;
									};
								public:
									filter_t()
										: m_pole{
											filter_pole_t(), filter_pole_t(),
											filter_pole_t(), filter_pole_t()
										}
										, m_enable(false)
										, m_scale(0)
										, m_output(0)
									{
									}

									void reset()
									{
										for (filter_pole_t &pole : m_pole)
											pole.reset();
										m_enable = false;
										m_scale = 0;
										m_output = 0;
									}

									void tick(const s32 in)
									{
										m_output = in;
										for (filter_pole_t &pole : m_pole)
										{
											if (pole.get_enable())
											{
												pole.tick(m_output);
												m_output = pole.get_output();
											}
										}
										m_output = (m_output * m_scale) >> 15;
									}

									filter_pole_t &pole(u8 index) { return m_pole[index]; }

									SETGET(bool, enable)
									SETGET(s16, scale)

									GETTER(s32, output)

								private:
									std::array<filter_pole_t, 4> m_pole;
									bool m_enable = false;
									s16 m_scale = 0;
									s32 m_output = 0;
							};

						public:
							operator_t(jkms16m24_t &host, jkms16m24_t::group_t &group)
								: m_host(host)
								, m_group(group)
								, m_wavegen(wavegen_t(host))
								, m_envelope(envelope_t())
								, m_vibrato(lfo_t())
								, m_tremolo(lfo_t())
								, m_filter(filter_t())
								, m_pitch(0)
								, m_env_sync(false)
								, m_vibrato_sync(false)
								, m_tremolo_sync(false)
								, m_envelope_scale(0)
								, m_vibrato_scale(0)
								, m_tremolo_scale(0)
								, m_wave_scale(0)
								, m_output_scale(0)
								, m_fm_input_en(false)
								, m_am_input_en(false)
								, m_pm_input_en(false)
								, m_fm_output_en(false)
								, m_am_output_en(false)
								, m_pm_output_en(false)
								, m_fm_output_matrix(0)
								, m_am_output_matrix(0)
								, m_pm_output_matrix(0)
								, m_fm_input_scale(0)
								, m_am_input_scale(0)
								, m_pm_input_scale(0)
								, m_fm_output_scale(0)
								, m_am_output_scale(0)
								, m_pm_output_scale(0)
								, m_speaker_output_en(false)
								, m_loutput_scale(0)
								, m_routput_scale(0)
								, m_busy(false)
								, m_keyon(false)
								, m_addr(0)
								, m_counter(0)
								, m_fm_input_val(0)
								, m_am_input_val(0)
								, m_pm_input_val(0)
								, m_wave_out(0)
								, m_output(0)
								, m_loutput(0)
								, m_routput(0)
							{
							}

							void reset()
							{
								m_wavegen.reset();
								m_envelope.reset();
								m_vibrato.reset();
								m_tremolo.reset();
								m_filter.reset();

								m_pitch = 0;

								m_env_sync = false;
								m_vibrato_sync = false;
								m_tremolo_sync = false;
								m_envelope_scale = 0;
								m_vibrato_scale = 0;
								m_tremolo_scale = 0;

								m_wave_scale = 0;
								m_output_scale = 0;

								m_fm_input_en = false;
								m_am_input_en = false;
								m_pm_input_en = false;

								m_fm_output_en = false;
								m_am_output_en = false;
								m_pm_output_en = false;

								m_fm_output_matrix = 0;
								m_am_output_matrix = 0;
								m_pm_output_matrix = 0;

								m_fm_input_scale = 0;
								m_am_input_scale = 0;
								m_pm_input_scale = 0;

								m_fm_output_scale = 0;
								m_am_output_scale = 0;
								m_pm_output_scale = 0;

								m_speaker_output_en = false;

								m_loutput_scale = 0;
								m_routput_scale = 0;

								m_busy = false;
								m_keyon = false;
								m_addr = 0;
								m_counter = 0;

								m_fm_input_val = 0;
								m_am_input_val = 0;
								m_pm_input_val = 0;

								m_wave_out = 0;
								m_output = 0;
								m_loutput = 0;
								m_routput = 0;
							}

							void tick()
							{
								m_output = 0;
								m_loutput = m_routput = 0;

								s64 fm_input_val = 0;
								if (m_fm_input_en)
									fm_input_val = (clamp16<s64>(m_fm_input_val) * m_fm_input_scale) >> 15;
								m_fm_input_val = 0;

								s64 am_input_val = 0;
								if (m_am_input_en)
									am_input_val = (clamp16<s64>(m_am_input_val) * m_am_input_scale) >> 15;
								m_am_input_val = 0;

								s64 pm_input_val = 0;
								if (m_pm_input_en)
									pm_input_val = (clamp16<s64>(m_pm_input_val) * m_pm_input_scale) >> 15;
								m_pm_input_val = 0;

								if (!m_busy)
									return;
								if (m_counter-- <= 0)
								{
									s64 pitch = m_pitch;
									if (m_vibrato.get_enable())
									{
										const s64 vibrato_out = (s64(m_vibrato.get_output()) * s64(m_vibrato_scale)) >> 25;
										pitch = clamp<s64>(pitch - vibrato_out, 0, 0xffff);
									}
									if (m_fm_input_en)
										pitch = clamp<s64>(pitch - fm_input_val, 0, 0xffff);

									u16 addr = m_addr;
									if (m_pm_input_en)
										addr += pm_input_val;

									m_wave_out = clamp16<s32>(m_wavegen.get_int_wave(addr) + m_wavegen.get_ext_wave(addr));
									m_wavegen.lfsr_tick();
									if (m_env_sync)
										m_envelope.tick();
									if (m_vibrato_sync)
										m_vibrato.tick();
									if (m_tremolo_sync)
										m_tremolo.tick();
									m_addr++;
									m_counter = m_pitch;
								}
								if (!m_env_sync)
									m_envelope.tick();
								if (!m_vibrato_sync)
									m_vibrato.tick();
								if (!m_tremolo_sync)
									m_tremolo.tick();

								if (m_filter.get_enable())
								{
									m_filter.tick(m_wave_out);
									m_output = m_filter.get_output();
								}
								m_output = (m_output * m_wave_scale) >> 15;
								if (m_envelope.get_enable())
								{
									const s64 env_out = s64(s64(m_envelope.get_output()) * s64(m_envelope_scale)) >> 15;
									m_output = clamp16<s64>((m_output * env_out) >> 25);
								}
								if (m_tremolo.get_enable())
								{
									const s64 tremolo_out = s64(s64(m_tremolo.get_output()) * s64(m_tremolo_scale)) >> 25;
									m_output = clamp16<s64>(m_output + tremolo_out);
								}
								if (m_am_input_en)
								{
									m_output = clamp16<s64>(m_output + ((am_input_val * m_am_input_scale) >> 15));
								}

								m_output = clamp16<s64>((m_output * m_output_scale) >> 15);
								// modulator output
								if (m_fm_output_en || m_am_output_en || m_pm_output_en)
								{
									for (u8 op = 0; op < 4; op++)
									{
										if (m_fm_output_en)
										{
											const s64 fm_output = clamp16<s64>((m_output * m_fm_output_scale) >> 15);
											if (bitfield(m_fm_output_matrix, op))
												m_group.m_op[op].add_fm_input_val(fm_output);
										}
										if (m_am_output_en)
										{
											const s64 am_output = clamp16<s64>((m_output * m_am_output_scale) >> 15);
											if (bitfield(m_am_output_matrix, op))
												m_group.m_op[op].add_am_input_val(am_output);
										}
										if (m_pm_output_en)
										{
											const s64 pm_output = clamp16<s64>((m_output * m_pm_output_scale) >> 15);
											if (bitfield(m_pm_output_matrix, op))
												m_group.m_op[op].add_pm_input_val(pm_output);
										}
									}
								}
								if (m_speaker_output_en)
								{
									if (m_loutput_scale != 0)
										m_loutput = clamp16<s64>((m_output * m_loutput_scale) >> 15);
									if (m_routput_scale != 0)
										m_routput = clamp16<s64>((m_output * m_routput_scale) >> 15);
								}
							}

							void keyon()
							{
								m_busy = true;
								m_keyon = true;
								m_addr = 0;
								m_counter = 0;
								m_wavegen.keyon();
								m_envelope.keyon();
								m_vibrato.keyon();
								m_tremolo.keyon();
								m_wave_out = 0;
								m_output = 0;
							}

							void keyoff()
							{
								m_keyon = false;
								if (m_envelope.get_enable())
									m_envelope.keyoff();
								else
									m_busy = false;
							}

							void add_fm_input_val(const s64 in)
							{
								m_fm_input_val += in;
							}
							void add_am_input_val(const s64 in)
							{
								m_am_input_val += in;
							}
							void add_pm_input_val(const s64 in)
							{
								m_pm_input_val += in;
							}

							wavegen_t &wavegen() { return m_wavegen; }
							envelope_t &envelope() { return m_envelope; }
							lfo_t &vibrato() { return m_vibrato; }
							lfo_t &tremolo() { return m_tremolo; }
							filter_t &filter() { return m_filter; }

							SETGET(u16, pitch)
							SETGET(bool, env_sync)
							SETGET(bool, vibrato_sync)
							SETGET(bool, tremolo_sync)
							SETGET(s16, envelope_scale)
							SETGET(s16, vibrato_scale)
							SETGET(s16, tremolo_scale)
							SETGET(s16, wave_scale)
							SETGET(s16, output_scale)
							SETGET(bool, fm_input_en)
							SETGET(bool, am_input_en)
							SETGET(bool, pm_input_en)
							SETGET(bool, fm_output_en)
							SETGET(bool, am_output_en)
							SETGET(bool, pm_output_en)
							SETGET(u8, fm_output_matrix)
							SETGET(u8, am_output_matrix)
							SETGET(u8, pm_output_matrix)
							SETGET(s16, fm_input_scale)
							SETGET(s16, am_input_scale)
							SETGET(s16, pm_input_scale)
							SETGET(s16, fm_output_scale)
							SETGET(s16, am_output_scale)
							SETGET(s16, pm_output_scale)
							SETGET(bool, speaker_output_en)
							SETGET(s16, loutput_scale)
							SETGET(s16, routput_scale)

							GETTER(bool, keyon)

							GETTER(s64, loutput)
							GETTER(s64, routput)

						private:
							jkms16m24_t &m_host;
							jkms16m24_t::group_t &m_group;

							// structs
							wavegen_t m_wavegen;
							envelope_t m_envelope;
							lfo_t m_vibrato;
							lfo_t m_tremolo;
							filter_t m_filter;

							// pitch
							u16 m_pitch = 0;

							// envelope/lfo
							bool m_env_sync = false;
							bool m_vibrato_sync = false;
							bool m_tremolo_sync = false;
							s16 m_envelope_scale = 0;
							s16 m_vibrato_scale = 0;
							s16 m_tremolo_scale = 0;

							// volume
							s16 m_wave_scale = 0;
							s16 m_output_scale = 0;

							// modulator
							bool m_fm_input_en = false;
							bool m_am_input_en = false;
							bool m_pm_input_en = false;

							bool m_fm_output_en = false;
							bool m_am_output_en = false;
							bool m_pm_output_en = false;

							u8 m_fm_output_matrix = 0;
							u8 m_am_output_matrix = 0;
							u8 m_pm_output_matrix = 0;

							s16 m_fm_input_scale = 0;
							s16 m_am_input_scale = 0;
							s16 m_pm_input_scale = 0;

							s16 m_fm_output_scale = 0;
							s16 m_am_output_scale = 0;
							s16 m_pm_output_scale = 0;

							// speaker output
							bool m_speaker_output_en = false;

							s16 m_loutput_scale = 0;
							s16 m_routput_scale = 0;

							// status
							bool m_busy = false;
							bool m_keyon = false;
							u16 m_addr = 0;
							s32 m_counter = 0;

							s64 m_fm_input_val = 0;
							s64 m_am_input_val = 0;
							s64 m_pm_input_val = 0;

							s32 m_wave_out = 0;
							s64 m_output = 0;
							s64 m_loutput = 0;
							s64 m_routput = 0;
					};

				public:
					group_t(jkms16m24_t &host)
						: m_host(host)
						, m_op{
							operator_t(host, *this), operator_t(host, *this),
							operator_t(host, *this), operator_t(host, *this)
						}
						, m_lvol(0)
						, m_rvol(0)
						, m_lout(0)
						, m_rout(0)
					{
					}

					void reset()
					{
						for (operator_t &op : m_op)
							op.reset();

						m_lvol = 0;
						m_rvol = 0;
						m_lout = 0;
						m_rout = 0;
					}

					void tick()
					{
						m_lout = m_rout = 0;
						for (operator_t &op : m_op)
						{
							op.tick();
							m_lout += (op.get_loutput() * m_lvol) >> 15;
							m_rout += (op.get_routput() * m_rvol) >> 15;
						}
						m_lout = clamp16<s64>(m_lout);
						m_rout = clamp16<s64>(m_rout);
					}

					operator_t &op(u8 index) { return m_op[index]; }

					SETGET(s16, lvol)
					SETGET(s16, rvol)

					GETTER(s64, lout)
					GETTER(s64, rout)

				private:
					jkms16m24_t &m_host;
					std::array<operator_t, 4> m_op;
					s16 m_lvol = 0;
					s16 m_rvol = 0;
					s64 m_lout = 0;
					s64 m_rout = 0;
			};

		public:
			jkms16m24_t(jkms16m24_intf_t &intf)
				: m_intf(intf)
				, m_group{
					*this, *this, *this, *this,
					*this, *this, *this, *this,
					*this, *this, *this, *this,
					*this, *this, *this, *this,
					*this, *this, *this, *this,
					*this, *this, *this, *this
				}
				, m_reg_index(0)
				, m_group_index(0)
				, m_operator_index(0)
				, m_filter_pole_index(0)
				, m_wave_addr(0)
				, m_enable(false)
				, m_lvol(0)
				, m_rvol(0)
				, m_lout(0)
				, m_rout(0)
			{
			}

			void reset()
			{
				for (group_t &group : m_group)
					group.reset();

				m_reg_index = 0;
				m_group_index = 0;
				m_operator_index = 0;
				m_filter_pole_index = 0;
				m_wave_addr = 0;

				m_enable = false;
				m_lvol = 0;
				m_rvol = 0;

				m_lout = 0;
				m_rout = 0;
			}

			void tick()
			{
				m_lout = m_rout = 0;
				if (!m_enable)
					return;

				for (group_t &group : m_group)
				{
					group.tick();
					m_lout += (group.get_lout() * m_lvol) >> 15;
					m_rout += (group.get_rout() * m_rvol) >> 15;
				}
				m_lout = clamp16<s64>(m_lout);
				m_rout = clamp16<s64>(m_rout);
			}

			u16 read(const u8 addr)
			{
				u16 ret = 0;
				switch (addr & 3)
				{
					case 0:
						ret = (m_reg_index & 0xff);
						break;
					case 1:
						ret = reg_r();
						break;
					case 2:
						ret = m_wave_addr;
						break;
					case 3:
						ret = m_intf.sample_r(m_wave_addr++);
						break;
				}
				return ret;
			}

			void write(const u8 addr, const u16 data)
			{
				switch (addr & 3)
				{
					case 0:
						m_reg_index = bitfield(data, 0, 8);
						break;
					case 1:
						reg_w(data);
						break;
					case 2:
						m_wave_addr = data;
						break;
					case 3:
						m_intf.sample_w(m_wave_addr++, data);
						break;
				}
			}

			group_t &group(u8 index) { return m_group[index]; }

			GETTER(s64, lout)
			GETTER(s64, rout)

		private:
			u16 reg_r();
			void reg_w(const u16 data);

			jkms16m24_intf_t &m_intf;
			std::array<group_t, 24> m_group;

			// host interface
			u8 m_reg_index = 0;
			u8 m_group_index = 0;
			u8 m_operator_index = 0;
			u8 m_filter_pole_index = 0;
			u16 m_wave_addr = 0;

			bool m_enable = false;
			s16 m_lvol = 0;
			s16 m_rvol = 0;

			s64 m_lout = 0;
			s64 m_rout = 0;
	};

}; // namespace jkms16m24

#endif // _JKMS16M24_H