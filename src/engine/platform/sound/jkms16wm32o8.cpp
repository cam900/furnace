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

Input clock: ~300MHz
Output rate: Input clock / 4096 (32 (sound channels) * 8 (operators) * 16 (tick per operators));
- 65536Hz at 268.435456MHz
32 total sound channels:
- 8 opetators per channel
  - Stereo output per operator
  - Envelope, Frequency LFO, Amplitude LFO, 8 pole filters
  - Internal and external waveform support
    - Pulse, Sawtooth, Triangle internal wave support, can be accumulated simultaneously
	- External waveform can be modified run-time - Mute, Horizontal/Vertical reverse/invert
	- Variable size (power of 2)
  - Frequency/Phase/Amplitude modulator
    - Independent matrix per operator, Can be modulated per operator simultaneously

Host interface map (16 bit data)

Offset Bit                 Description
       fedc ba98 7654 3210 
00     x--- ---- ---- ---- Busy (not implemented currently)
       ---- ---- xxxx xxxx Register select
02     xxxx xxxx xxxx xxxx Register data
04     xxxx xxxx xxxx xxxx RAM address
06     xxxx xxxx xxxx xxxx RAM data
08     xxxx xxxx xxxx xxxx RAM modulo (RAM address increment after RAM access)

Register map (16 bit data)

Register Bit                 Description
         fedc ba98 7654 3210 
00       x--- ---- ---- ---- Sound enable
         ---- ---- xxxx x--- Channel select
         ---- ---- ---- -xxx Opearator select
01-03: Per-channel registers
01       ---- ---- 7654 3210 Operator on/off
02       xxxx xxxx xxxx xxxx Left volume (16 bit signed)
03       xxxx xxxx xxxx xxxx Right volume (16 bit signed)
04 onwards: Per-operator registers
04       x--- ---- ---- ---- Envelope enable
         -x-- ---- ---- ---- Envelope loop
         --x- ---- ---- ---- Frequency LFO enable
         ---x ---- ---- ---- Amplitude LFO enable
         ---- xx-- ---- ---- Frequency LFO waveform (0 = Sawtooth, 1 = Triangle, 2 = Square, 3 = noise)
         ---- --xx ---- ---- Amplitude LFO waveform (0 = Sawtooth, 1 = Triangle, 2 = Square, 3 = noise)
         ---- ---- x--- ---- Enable filter output
         ---- ---- -x-- ---- Enable direct output
         ---- ---- --x- ---- FM input enable
         ---- ---- ---x ---- PM input enable
         ---- ---- ---- x--- AM input enable
         ---- ---- ---- -x-- FM output enable
         ---- ---- ---- --x- PM output enable
         ---- ---- ---- ---x AM output enable
05       xxxx ---- ---- ---- Mute bit position
         ---- xxxx ---- ---- Reverse bit position
         ---- ---- xxxx ---- Invert bit position
         ---- ---- ---- x--- Mute bit enable
         ---- ---- ---- -x-- Reverse bit enable
         ---- ---- ---- --x- Invert bit enable
         ---- ---- ---- ---x Enable speaker output
06       xxxx ---- ---- ---- Internal waveform size (2^(16-x))
         ---- xxxx ---- ---- External waveform size (2^(16-x))
         ---- ---- x--- ---- Enable external waveform
         ---- ---- -x-- ---- Enable inverted triangle waveform
         ---- ---- --x- ---- Enable inverted sawtooth waveform
         ---- ---- ---x ---- Enable inverted pulse waveform
         ---- ---- ---- x--- Enable noise
         ---- ---- ---- -x-- Enable triangle waveform
         ---- ---- ---- --x- Enable sawtooth waveform
         ---- ---- ---- ---x Enable pulse waveform
07       xxxx xxxx xxxx xxxx External waveform base address
08       eeee mmmm mmmm mmmm Pitch ((0x1000|m) * 2^e)
09       xxxx xxxx xxxx xxxx Pulse duty
0a       xxxx xxxx xxxx xxxx FM input multiplier (16 bit signed)
0b       xxxx xxxx xxxx xxxx PM input multiplier (16 bit signed)
0c       xxxx xxxx xxxx xxxx AM input multiplier (16 bit signed)
0d       xxxx xxxx xxxx xxxx FM output multiplier (16 bit signed)
0e       xxxx xxxx xxxx xxxx PM output multiplier (16 bit signed)
0f       xxxx xxxx xxxx xxxx AM output multiplier (16 bit signed)
10       xxxx xxxx xxxx xxxx FM feedback (16 bit signed)
11       xxxx xxxx xxxx xxxx PM feedback (16 bit signed)
12       xxxx xxxx xxxx xxxx AM feedback (16 bit signed)
13       7654 3210 ---- ---- FM output operator enable bit
         ---- ---- 7654 3210 PM output operator enable bit
14       7654 3210 ---- ---- AM output operator enable bit
15       xxxx xxxx xxxx xxxx Speaker output master volume (16 bit signed)
16       xxxx xxxx xxxx xxxx Speaker output left volume (16 bit signed)
17       xxxx xxxx xxxx xxxx Speaker output right volume (16 bit signed)
18       xxxx xxxx xxxx xxxx Total level (16 bit signed)
19       xxxx xxxx xxxx xxxx Noise pitch
1a       xxxx xxxx xxxx xxxx Noise initial LFSR
1b       xxxx xxxx xxxx xxxx Noise LFSR mask
1e       3--- 2--- 1--- 0--- Filter enable bit
         -3-- -2-- -1-- -0-- Filter lowpass output enable
         --3- --2- --1- --0- Filter highpass output enable
         ---3 ---2 ---1 ---0 Filter bandpass output enable
1f       7--- 6--- 5--- 4--- Filter enable bit
         -7-- -6-- -5-- -4-- Filter lowpass output enable
         --7- --6- --5- --4- Filter highpass output enable
         ---7 ---6 ---5 ---4 Filter bandpass output enable
20       xxxx xxxx xxxx xxxx Filter #0 F parameter
21       xxxx xxxx xxxx xxxx Filter #0 Q parameter
22       xxxx xxxx xxxx xxxx Filter #1 F parameter
23       xxxx xxxx xxxx xxxx Filter #1 Q parameter
24       xxxx xxxx xxxx xxxx Filter #2 F parameter
25       xxxx xxxx xxxx xxxx Filter #2 Q parameter
26       xxxx xxxx xxxx xxxx Filter #3 F parameter
27       xxxx xxxx xxxx xxxx Filter #3 Q parameter
28       xxxx xxxx xxxx xxxx Filter #4 F parameter
29       xxxx xxxx xxxx xxxx Filter #4 Q parameter
2a       xxxx xxxx xxxx xxxx Filter #5 F parameter
2b       xxxx xxxx xxxx xxxx Filter #5 Q parameter
2c       xxxx xxxx xxxx xxxx Filter #6 F parameter
2d       xxxx xxxx xxxx xxxx Filter #6 Q parameter
2e       xxxx xxxx xxxx xxxx Filter #7 F parameter
2f       xxxx xxxx xxxx xxxx Filter #7 Q parameter
30       seee emmm mmmm mmmm Envelope initial level*
31       eeee mmmm mmmm mmmm Envelope delay length**
32       seee emmm mmmm mmmm Envelope attack target*
33       eeee mmmm mmmm mmmm Envelope attack rate**
34       seee emmm mmmm mmmm Envelope decay target*
35       eeee mmmm mmmm mmmm Envelope decay rate**
36       seee emmm mmmm mmmm Envelope sustain target*
37       eeee mmmm mmmm mmmm Envelope sustain rate**
38       eeee mmmm mmmm mmmm Envelope release rate**
39       xxxx xxxx xxxx xxxx Envelope multiplier (16 bit signed)
40       eeee mmmm mmmm mmmm Frequency LFO delay length**
41       seee emmm mmmm mmmm Frequency LFO target**
42       eeee mmmm mmmm mmmm Frequency LFO level*
43       xxxx xxxx xxxx xxxx Frequency LFO multiplier
44       xxxx xxxx xxxx xxxx Frequency LFO noise pitch
45       xxxx xxxx xxxx xxxx Frequency LFO noise initial LFSR
46       xxxx xxxx xxxx xxxx Frequency LFO noise LFSR mask
48       eeee mmmm mmmm mmmm Amplitude LFO delay length**
49       seee emmm mmmm mmmm Amplitude LFO target**
4a       eeee mmmm mmmm mmmm Amplitude LFO level*
4b       xxxx xxxx xxxx xxxx Amplitude LFO multiplier
4c       xxxx xxxx xxxx xxxx Amplitude LFO noise pitch
4d       xxxx xxxx xxxx xxxx Amplitude LFO noise initial LFSR
4e       xxxx xxxx xxxx xxxx Amplitude LFO noise LFSR mask

* level and target formula: (((e == 0) ? m : ((0x800|m) ^ (2*(e-1)))) * (s ? -1 : 1))
** delay and rate formula: ((e == 0) ? m : ((0x1000|m) ^ (2*(e-1))))
*/

#include "jkms16wm32o8.hpp"

namespace jkms16wm32o8
{
	void jkms16wm32o8_t::channel_t::operator_t::envelope_t::tick()
	{
		if (m_enable)
		{
			switch (m_state)
			{
				case ENV_STATE_DELAY:
					if (m_delay_counter > 0)
						m_delay_counter--;
					else
					{
						m_delay_counter = 0;
						m_state = ENV_STATE_ATTACK;
					}
					break;
				case ENV_STATE_ATTACK:
				{
					const s32 target = get_target(m_attack_target);
					const s32 rate = get_rate(m_attack_rate);
					if (m_env_level > target)
						m_env_level = std::max(m_env_level - rate, target);
					else if (m_env_level < target)
						m_env_level = std::min(m_env_level + rate, target);
					if (m_env_level == target)
						m_state = ENV_STATE_DECAY;
					break;
				}
				case ENV_STATE_DECAY:
				{
					const s32 target = get_target(m_decay_target);
					const s32 rate = get_rate(m_decay_rate);
					if (m_env_level > target)
						m_env_level = std::max(m_env_level - rate, target);
					else if (m_env_level < target)
						m_env_level = std::min(m_env_level + rate, target);
					if (m_env_level == target)
						m_state = ENV_STATE_SUSTAIN;
					break;
				}
				case ENV_STATE_SUSTAIN:
				{
					const s32 target = get_target(m_sustain_target);
					const s32 rate = get_rate(m_sustain_rate);
					if (m_env_level > target)
						m_env_level = std::max(m_env_level - rate, target);
					else if (m_env_level < target)
						m_env_level = std::min(m_env_level + rate, target);
					if (m_env_level == target)
						m_state = m_loop ? ENV_STATE_DECAY : ENV_STATE_IDLE;
					break;
				}
				case ENV_STATE_RELEASE:
				{
					const s32 rate = get_rate(m_release_rate);
					if (m_env_level > 0)
						m_env_level = std::max(m_env_level - rate, 0);
					else if (m_env_level < 0)
						m_env_level = std::min(m_env_level + rate, 0);
					if (m_env_level == 0)
						m_state = ENV_STATE_IDLE;
					break;
				}
				default:
					break;
			}
		}
	}

	void jkms16wm32o8_t::channel_t::operator_t::lfo_t::tick()
	{
		m_lfo_out = 0;
		if (m_enable)
		{
			switch (m_state)
			{
				case LFO_STATE_DELAY:
					if (m_delay_counter > 0)
						m_delay_counter--;
					else
					{
						m_delay_counter = 0;
						m_state = LFO_STATE_RUN;
					}
					break;
				case LFO_STATE_RUN:
				{
					const s32 target = get_target(m_target) * m_lfo_sign;
					const s32 rate = get_rate(m_rate);
					if (m_lfo_level > target)
						m_lfo_level = std::max(m_lfo_level - rate, target);
					else if (m_lfo_level < target)
						m_lfo_level = std::min(m_lfo_level + rate, target);
					switch (m_wave)
					{
						case 0: // sawtooth
							m_lfo_out = m_lfo_level;
							if (m_lfo_level == target)
								m_lfo_level = -m_lfo_level;
							break;
						case 1: // triangle
							m_lfo_out = m_lfo_level;
							if (m_lfo_level == target)
								m_lfo_sign = -m_lfo_sign;
							break;
						case 2: // square
							m_lfo_out = get_target(0x7fff) * m_lfo_sign;
							if (m_lfo_level == target)
								m_lfo_sign = -m_lfo_sign;
							break;
						case 3: // noise
							m_lfo_out = s16(m_lfsr);
							break;
					}
					if (m_noise_counter >= m_noise_pitch)
					{
						u32 xor_out = 0;
						for (int b = 0; b < 16; b++)
						{
							if (bitfield(m_lfsr_mask, b))
								xor_out ^= bitfield(m_lfsr, b);
						}
						m_lfsr = (m_lfsr >> 1) ^ (xor_out << 16);
						if (!m_lfsr)
							m_lfsr |= (1 << 16);
						m_noise_counter = 0;
					}
					else
					{
						m_noise_counter++;
					}
					break;
				}
				default:
					break;
			}
		}
	}

	void jkms16wm32o8_t::channel_t::operator_t::filter_t::tick(const s32 input)
	{
		m_l = m_d[1] + ((m_f * m_d[0]) >> 16);
		m_h = input - m_l - ((m_q * m_d[0]) >> 16);
		m_b = ((m_f * m_h) >> 16) + m_d[0];

		m_d[0] = m_b;
		m_d[1] = m_l;
	}

	void jkms16wm32o8_t::channel_t::operator_t::reset()
	{
		m_env.reset();
		m_flfo.reset();
		m_alfo.reset();
		for (filter_t &filter : m_filter)
		{
			filter.reset();
		}
		m_fm_in.reset();
		m_pm_in.reset();
		m_am_in.reset();
		m_fm_out.reset();
		m_pm_out.reset();
		m_am_out.reset();
		m_mute.reset();
		m_reverse.reset();
		m_invert.reset();
		m_is_keyon = false;
		m_wave_addr = 0;
		m_frac = 0;
		m_wave_out = 0;
		m_op_out = 0;
		m_lout = 0;
		m_rout = 0;
		m_noise_counter = 0;
		m_lfsr = 1;
		m_busy = false;
		m_speaker_out_enable = false;
		m_direct_out = false;
		m_filter_out = false;
		m_total_level = 0;
		m_wave_base = 0;
		m_wave_bit = 0;
		m_int_wave_size = 0;
		m_ext_wave_size = 0;
		m_pitch = 0;
		m_duty = 0;
		m_speaker_vol = 0;
		m_speaker_lvol = 0;
		m_speaker_rvol = 0;
		m_noise_pitch = 0;
		m_initial_lfsr = 0;
		m_lfsr_mask = 0;
	}

	void jkms16wm32o8_t::channel_t::operator_t::tick()
	{
		m_op_out = m_wave_out = m_lout = m_rout = 0;
		if (m_busy)
		{
			m_env.tick();
			m_flfo.tick();
			m_alfo.tick();
			u32 calculated_freq = m_fm_in.enable() ? ((m_pitch + (m_fm_in.get_input())) & 0xffff) : m_pitch;
			u32 calculated_addr = m_pm_in.enable() ? ((m_wave_addr + (m_pm_in.get_input())) & 0xffff) : m_wave_addr;
			bool is_muted = false;
			bool is_inverted = false;
			const u32 internal_addr = (calculated_addr << (m_int_wave_size & 0xf)) & 0xffff;
			u32 external_addr = calculated_addr & (0xffff >> (m_ext_wave_size & 0xf));
			if (m_mute.enable() && bitfield(calculated_addr, 16 - m_mute.bitpos()))
				is_muted = true;
			if (m_reverse.enable() && bitfield(calculated_addr, 16 - m_reverse.bitpos()))
				external_addr ^= 0xffff >> (m_ext_wave_size & 0xf);
			if (m_invert.enable() && bitfield(calculated_addr, 16 - m_invert.bitpos()))
				is_inverted = true;
			if (bitfield(m_wave_bit, 0)) // pulse
				m_wave_out += (internal_addr >= m_duty) ? -0x7fff : 0x7fff;
			if (bitfield(m_wave_bit, 1)) // sawtooth
				m_wave_out += s16(internal_addr);
			if (bitfield(m_wave_bit, 2)) // triangle
				m_wave_out += s16(((internal_addr & 0x8000) ? (0xffff - ((internal_addr & 0x7fff) << 1)) : ((internal_addr & 0x7fff) << 1)) ^ 0x8000);
			if (bitfield(m_wave_bit, 3)) // noise
				m_wave_out += (bitfield(m_lfsr, 0)) ? -0x7fff : 0x7fff;
			if (bitfield(m_wave_bit, 4)) // inverted pulse
				m_wave_out += (internal_addr <= m_duty) ? -0x7fff : 0x7fff;
			if (bitfield(m_wave_bit, 5)) // inverted sawtooth
				m_wave_out += s16(-internal_addr);
			if (bitfield(m_wave_bit, 6)) // inverted triangle
				m_wave_out += s16((((~internal_addr) & 0x8000) ? (0xffff - ((internal_addr & 0x7fff) << 1)) : ((internal_addr & 0x7fff) << 1)) ^ 0x8000);
			if (bitfield(m_wave_bit, 7) && (!is_muted)) // external
				m_wave_out += s16((m_host.m_host.m_intf.read_wave((m_wave_base + external_addr) & 0xffff) ^ (is_inverted ? 0xffff : 0)) - 0x8000);

			if (m_filter_out)
			{
				s32 prev = m_wave_out;
				for (int pole = 0; pole < JKMS16WM32_MAX_FILTERS; pole++)
				{
					filter_t &filter = m_filter[pole];
					if (filter.enable())
					{
						filter.tick(prev);
						prev = 0;
						if (filter.lp_enable())
							prev += filter.lowpass_output();
						if (filter.hp_enable())
							prev += filter.highpass_output();
						if (filter.bp_enable())
							prev += filter.bandpass_output();
					}
				}
				m_op_out += clamp(prev, -0x8000, 0x7fff);
			}
			if (m_direct_out)
				m_op_out += m_wave_out;
			if (m_am_in.enable())
				m_op_out = clamp(m_op_out + m_am_in.get_input(), -0x8000, 0x7fff);
			if (m_env.enable())
				m_op_out = (m_op_out * m_env.result()) >> 26;
			if (m_alfo.enable())
				m_op_out = clamp<s64>((m_op_out + m_alfo.result()) >> 26, -0x8000, 0x7fff);

			m_op_out = clamp((m_op_out * m_total_level) >> 15, -0x8000, 0x7fff);
			if (m_speaker_out_enable)
			{
				const s32 out = clamp((m_op_out * m_speaker_vol) >> 15, -0x8000, 0x7fff);
				m_lout = clamp((out * m_speaker_lvol) >> 15, -0x8000, 0x7fff);
				m_rout = clamp((out * m_speaker_rvol) >> 15, -0x8000, 0x7fff);
			}

			m_fm_in.reset();
			m_pm_in.reset();
			m_am_in.reset();
			if (m_fm_out.enable())
			{
				for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
				{
					if (bitfield(m_fm_out.matrix(), op))
						m_host.m_op[op].fm_in().add_input(m_fm_out.get_output(m_op_out));
				}
				m_fm_in.add_input(m_fm_out.get_feedback(m_op_out));
			}
			if (m_pm_out.enable())
			{
				for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
				{
					if (bitfield(m_pm_out.matrix(), op))
						m_host.m_op[op].pm_in().add_input(m_pm_out.get_output(m_op_out));
				}
				m_pm_in.add_input(m_pm_out.get_feedback(m_op_out));
			}
			if (m_am_out.enable())
			{
				for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
				{
					if (bitfield(m_am_out.matrix(), op))
						m_host.m_op[op].am_in().add_input(m_am_out.get_output(m_op_out));
				}
				m_am_in.add_input(m_am_out.get_feedback(m_op_out));
			}
			if (m_flfo.enable())
				calculated_freq += m_flfo.result();

			m_frac += get_pitch(calculated_freq);
			if (m_frac >= 0xfffff)
			{
				m_wave_addr += (m_frac >> 20);
				m_frac &= 0xfffff;
			}
			if (m_noise_counter >= m_noise_pitch)
			{
				u32 xor_out = 0;
				for (int b = 0; b < 16; b++)
				{
					if (bitfield(m_lfsr_mask, b))
						xor_out ^= bitfield(m_lfsr, b);
				}
				m_lfsr = (m_lfsr >> 1) ^ (xor_out << 16);
				if (!m_lfsr)
					m_lfsr |= (1 << 16);
				m_noise_counter = 0;
			}
			else
			{
				m_noise_counter++;
			}
		}
		else
		{
			m_fm_in.reset();
			m_pm_in.reset();
			m_am_in.reset();
		}
	}

	void jkms16wm32o8_t::channel_t::tick()
	{
		m_lout = m_rout = 0;
		for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
		{
			m_op[op].tick();
			if (m_op[op].busy())
			{
				m_lout += m_op[op].lout();
				m_rout += m_op[op].rout();
			}
		}
		m_lout = clamp((m_lout * m_lvol) >> 15, -0x8000, 0x7fff);
		m_rout = clamp((m_rout * m_rvol) >> 15, -0x8000, 0x7fff);
	}

	void jkms16wm32o8_t::reset()
	{
		for (channel_t &channel : m_channel)
		{
			channel.reset();
		}
		m_lout = 0;
		m_rout = 0;
		m_register_select = 0;
		m_sound_enable = true;
		m_op_select = 0;
		m_channel_select = 0;
	}

	void jkms16wm32o8_t::tick()
	{
		m_lout = m_rout = 0;
		if (m_sound_enable)
		{
			for (int ch = 0; ch < JKMS16WM32_MAX_CHANNELS; ch++)
			{
				m_channel[ch].tick();
				m_lout += m_channel[ch].lout();
				m_rout += m_channel[ch].rout();
			}
			m_lout = clamp(m_lout, -0x8000, 0x7fff);
			m_rout = clamp(m_rout, -0x8000, 0x7fff);
		}
	}

	u16 jkms16wm32o8_t::host_r(const u8 addr)
	{
		u16 ret = 0;
		switch (addr & 7)
		{
			case 0:
				ret = m_register_select;
				break;
			case 1:
				ret = regs_r();
				break;
			case 2:
				ret = m_waveram_addr;
				break;
			case 3:
				ret = m_intf.read_wave(m_waveram_addr);
				m_waveram_addr += m_waveram_modulo;
				break;
			case 4:
				ret = m_waveram_modulo;
				break;
		}
		return ret;
	}

	void jkms16wm32o8_t::host_w(const u8 addr, const u16 data)
	{
		switch (addr & 7)
		{
			case 0:
				m_register_select = data & 0xff;
				break;
			case 1:
				regs_w(data);
				break;
			case 2:
				m_waveram_addr = data;
				break;
			case 3:
				m_intf.write_wave(m_waveram_addr, data);
				m_waveram_addr += m_waveram_modulo;
				break;
			case 4:
				m_waveram_modulo = data;
				break;
		}
	}

	u16 jkms16wm32o8_t::regs_r()
	{
		u16 ret = 0;
		channel_t &channel = m_channel[m_channel_select];
		channel_t::operator_t &op = channel.op(m_op_select);
		switch (m_register_select)
		{
			case 0x00:
				ret = (m_sound_enable ? (1 << 15) : 0) |
					(m_channel_select << 3) |
					m_op_select;
				break;
			case 0x01:
			{
				for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
				{
					ret |= channel.op(op).busy() ? (1 << op) : 0;
				}
				break;
			}
			case 0x02:
				ret = u16(channel.lvol());
				break;
			case 0x03:
				ret = u16(channel.rvol());
				break;
			case 0x04:
				ret = (op.env().enable() ? (1 << 15) : 0) |
					(op.env().loop() ? (1 << 14) : 0) |
					(op.flfo().enable() ? (1 << 13) : 0) |
					(op.alfo().enable() ? (1 << 12) : 0) |
					(op.flfo().wave() << 10) |
					(op.alfo().wave() << 8) |
					(op.filter_out() ? (1 << 7) : 0) |
					(op.direct_out() ? (1 << 6) : 0) |
					(op.fm_in().enable() ? (1 << 5) : 0) |
					(op.pm_in().enable() ? (1 << 4) : 0) |
					(op.am_in().enable() ? (1 << 3) : 0) |
					(op.fm_out().enable() ? (1 << 2) : 0) |
					(op.pm_out().enable() ? (1 << 1) : 0) |
					(op.am_out().enable() ? (1 << 0) : 0);
				break;
			case 0x05:
				ret = (op.mute().bitpos() << 12) |
					(op.reverse().bitpos() << 8) |
					(op.invert().bitpos() << 4) |
					(op.mute().enable() ? (1 << 3) : 0) |
					(op.reverse().enable() ? (1 << 2) : 0) |
					(op.invert().enable() ? (1 << 1) : 0) |
					(op.speaker_out_enable() ? (1 << 0) : 0);
				break;
			case 0x06:
				ret = (op.int_wave_size() << 12) |
					(op.ext_wave_size() << 8) |
					(op.wave_bit());
				break;
			case 0x07:
				ret = op.wave_base();
				break;
			case 0x08:
				ret = op.pitch();
				break;
			case 0x09:
				ret = op.duty();
				break;
			case 0x0a:
				ret = u16(op.fm_in().multiplier());
				break;
			case 0x0b:
				ret = u16(op.pm_in().multiplier());
				break;
			case 0x0c:
				ret = u16(op.am_in().multiplier());
				break;
			case 0x0d:
				ret = u16(op.fm_out().multiplier());
				break;
			case 0x0e:
				ret = u16(op.pm_out().multiplier());
				break;
			case 0x0f:
				ret = u16(op.am_out().multiplier());
				break;
			case 0x10:
				ret = u16(op.fm_out().feedback());
				break;
			case 0x11:
				ret = u16(op.pm_out().feedback());
				break;
			case 0x12:
				ret = u16(op.am_out().feedback());
				break;
			case 0x13:
				ret = (op.fm_out().matrix() << 8) |
					op.pm_out().matrix();
				break;
			case 0x14:
				ret = (op.am_out().matrix() << 8);
				break;
			case 0x15:
				ret = u16(op.speaker_vol());
				break;
			case 0x16:
				ret = u16(op.speaker_lvol());
				break;
			case 0x17:
				ret = u16(op.speaker_rvol());
				break;
			case 0x18:
				ret = u16(op.total_level());
				break;
			case 0x19:
				ret = op.noise_pitch();
				break;
			case 0x1a:
				ret = op.initial_lfsr();
				break;
			case 0x1b:
				ret = op.lfsr_mask();
				break;
			case 0x1e:
			{
				for (int f = 0; f < 4; f++)
				{
					ret |= (op.filter(f).enable() ? (8 << (f << 2)) : 0) |
						(op.filter(f).lp_enable() ? (4 << (f << 2)) : 0) |
						(op.filter(f).hp_enable() ? (2 << (f << 2)) : 0) |
						(op.filter(f).bp_enable() ? (1 << (f << 2)) : 0);
				}
				break;
			}
			case 0x1f:
			{
				for (int f = 4, b = 0; f < JKMS16WM32_MAX_FILTERS; f++, b++)
				{
					ret |= (op.filter(f).enable() ? (8 << (b << 2)) : 0) |
						(op.filter(f).lp_enable() ? (4 << (b << 2)) : 0) |
						(op.filter(f).hp_enable() ? (2 << (b << 2)) : 0) |
						(op.filter(f).bp_enable() ? (1 << (b << 2)) : 0);
				}
				break;
			}
			case 0x20:
			case 0x22:
			case 0x24:
			case 0x26:
			case 0x28:
			case 0x2a:
			case 0x2c:
			case 0x2e:
				ret = u16(op.filter(bitfield(m_register_select, 1, JKMS16WM32_FILTER_BITS)).f());
				break;
			case 0x21:
			case 0x23:
			case 0x25:
			case 0x27:
			case 0x29:
			case 0x2b:
			case 0x2d:
			case 0x2f:
				ret = u16(op.filter(bitfield(m_register_select, 1, JKMS16WM32_FILTER_BITS)).q());
				break;
			case 0x30:
				ret = op.env().initial_level();
				break;
			case 0x31:
				ret = op.env().delay_rate();
				break;
			case 0x32:
				ret = op.env().attack_target();
				break;
			case 0x33:
				ret = op.env().attack_rate();
				break;
			case 0x34:
				ret = op.env().decay_target();
				break;
			case 0x35:
				ret = op.env().decay_rate();
				break;
			case 0x36:
				ret = op.env().sustain_target();
				break;
			case 0x37:
				ret = op.env().sustain_rate();
				break;
			case 0x38:
				ret = op.env().release_rate();
				break;
			case 0x39:
				ret = u16(op.env().multiplier());
				break;
			case 0x40:
				ret = op.flfo().delay_rate();
				break;
			case 0x41:
				ret = op.flfo().target();
				break;
			case 0x42:
				ret = op.flfo().rate();
				break;
			case 0x43:
				ret = u16(op.flfo().multiplier());
				break;
			case 0x44:
				ret = op.flfo().noise_pitch();
				break;
			case 0x45:
				ret = op.flfo().initial_lfsr();
				break;
			case 0x46:
				ret = op.flfo().lfsr_mask();
				break;
			case 0x48:
				ret = op.alfo().delay_rate();
				break;
			case 0x49:
				ret = op.alfo().target();
				break;
			case 0x4a:
				ret = op.alfo().rate();
				break;
			case 0x4b:
				ret = u16(op.alfo().multiplier());
				break;
			case 0x4c:
				ret = op.alfo().noise_pitch();
				break;
			case 0x4d:
				ret = op.alfo().initial_lfsr();
				break;
			case 0x4e:
				ret = op.alfo().lfsr_mask();
				break;
		}
		return ret;
	}
	void jkms16wm32o8_t::regs_w(const u16 data)
	{
		channel_t &channel = m_channel[m_channel_select];
		channel_t::operator_t &op = channel.op(m_op_select);
		switch (m_register_select)
		{
			case 0x00:
				m_sound_enable = bitfield(data, 15);
				m_channel_select = bitfield(data, 3, JKMS16WM32_CHANNEL_BITS);
				m_op_select = bitfield(data, 0, JKMS16WM32_OPERATOR_BITS);
				break;
			case 0x01:
			{
				for (int op = 0; op < JKMS16WM32_MAX_OPERATORS; op++)
				{
					if (bitfield(data, op))
						channel.op(op).keyon();
					else
						channel.op(op).keyoff();
				}
				break;
			}
			case 0x02:
				channel.set_lvol(s16(data));
				break;
			case 0x03:
				channel.set_rvol(s16(data));
				break;
			case 0x04:
			{
				const bool prev_env_enable = op.env().enable();
				const bool prev_flfo_enable = op.flfo().enable();
				const bool prev_alfo_enable = op.alfo().enable();
				op.env().set_enable(bitfield(data, 15));
				op.env().set_loop(bitfield(data, 14));
				op.flfo().set_enable(bitfield(data, 13));
				op.alfo().set_enable(bitfield(data, 12));
				op.flfo().set_wave(bitfield(data, 10, 2));
				op.alfo().set_wave(bitfield(data, 8, 2));
				op.set_filter_out(bitfield(data, 7));
				op.set_direct_out(bitfield(data, 6));
				op.fm_in().set_enable(bitfield(data, 5));
				op.pm_in().set_enable(bitfield(data, 4));
				op.am_in().set_enable(bitfield(data, 3));
				op.fm_out().set_enable(bitfield(data, 2));
				op.pm_out().set_enable(bitfield(data, 1));
				op.am_out().set_enable(bitfield(data, 0));
				if ((!op.is_keyon()) && (prev_env_enable && !op.env().enable()))
					op.set_busy(false);
				if (op.is_keyon() && (!prev_env_enable && op.env().enable()))
					op.env().keyon();
				if (op.is_keyon() && (!prev_flfo_enable && op.flfo().enable()))
					op.flfo().keyon();
				if (op.is_keyon() && (!prev_alfo_enable && op.alfo().enable()))
					op.alfo().keyon();
				break;
			}
			case 0x05:
				op.mute().set_bitpos(bitfield(data, 12, 4));
				op.reverse().set_bitpos(bitfield(data, 8, 4));
				op.invert().set_bitpos(bitfield(data, 4, 4));
				op.mute().set_enable(bitfield(data, 3));
				op.reverse().set_enable(bitfield(data, 2));
				op.invert().set_enable(bitfield(data, 1));
				op.set_speaker_out_enable(bitfield(data, 0));
				break;
			case 0x06:
				op.set_int_wave_size(bitfield(data, 12, 4));
				op.set_ext_wave_size(bitfield(data, 8, 4));
				op.set_wave_bit(bitfield(data, 0, 8));
				break;
			case 0x07:
				op.set_wave_base(data);
				break;
			case 0x08:
				op.set_pitch(data);
				break;
			case 0x09:
				op.set_duty(data);
				break;
			case 0x0a:
				op.fm_in().set_multiplier(s16(data));
				break;
			case 0x0b:
				op.pm_in().set_multiplier(s16(data));
				break;
			case 0x0c:
				op.am_in().set_multiplier(s16(data));
				break;
			case 0x0d:
				op.fm_out().set_multiplier(s16(data));
				break;
			case 0x0e:
				op.pm_out().set_multiplier(s16(data));
				break;
			case 0x0f:
				op.am_out().set_multiplier(s16(data));
				break;
			case 0x10:
				op.fm_out().set_feedback(s16(data));
				break;
			case 0x11:
				op.pm_out().set_feedback(s16(data));
				break;
			case 0x12:
				op.am_out().set_feedback(s16(data));
				break;
			case 0x13:
				op.fm_out().set_matrix(bitfield(data, 8, JKMS16WM32_MAX_OPERATORS));
				op.pm_out().set_matrix(bitfield(data, 0, JKMS16WM32_MAX_OPERATORS));
				break;
			case 0x14:
				op.am_out().set_matrix(bitfield(data, 8, JKMS16WM32_MAX_OPERATORS));
				break;
			case 0x15:
				op.set_speaker_vol(s16(data));
				break;
			case 0x16:
				op.set_speaker_lvol(s16(data));
				break;
			case 0x17:
				op.set_speaker_rvol(s16(data));
				break;
			case 0x18:
				op.set_total_level(s16(data));
				break;
			case 0x19:
				op.set_noise_pitch(data);
				break;
			case 0x1a:
				op.set_initial_lfsr(data);
				break;
			case 0x1b:
				op.set_lfsr_mask(data);
				break;
			case 0x1e:
			{
				for (int f = 0; f < 4; f++)
				{
					const bool prev_filter_enable = op.filter(f).enable();
					op.filter(f).set_enable(bitfield(data, (f << 2) | 3));
					op.filter(f).set_lp_enable(bitfield(data, (f << 2) | 2));
					op.filter(f).set_hp_enable(bitfield(data, (f << 2) | 1));
					op.filter(f).set_bp_enable(bitfield(data, (f << 2) | 0));
					if (op.is_keyon() && (!prev_filter_enable && op.filter(f).enable()))
						op.filter(f).keyon();
				}
				break;
			}
			case 0x1f:
			{
				for (int f = 4, b = 0; f < JKMS16WM32_MAX_FILTERS; f++, b++)
				{
					const bool prev_filter_enable = op.filter(f).enable();
					op.filter(f).set_enable(bitfield(data, (b << 2) | 3));
					op.filter(f).set_lp_enable(bitfield(data, (b << 2) | 2));
					op.filter(f).set_hp_enable(bitfield(data, (b << 2) | 1));
					op.filter(f).set_bp_enable(bitfield(data, (b << 2) | 0));
					if (op.is_keyon() && (!prev_filter_enable && op.filter(f).enable()))
						op.filter(f).keyon();
				}
				break;
			}
			case 0x20:
			case 0x22:
			case 0x24:
			case 0x26:
				op.filter(bitfield(m_register_select, 1, JKMS16WM32_FILTER_BITS)).set_f(u16(data));
				break;
			case 0x21:
			case 0x23:
			case 0x25:
			case 0x27:
				op.filter(bitfield(m_register_select, 1, JKMS16WM32_FILTER_BITS)).set_q(u16(data));
				break;
			case 0x30:
				op.env().set_initial_level(data);
				break;
			case 0x31:
				op.env().set_delay_rate(data);
				break;
			case 0x32:
				op.env().set_attack_target(data);
				break;
			case 0x33:
				op.env().set_attack_rate(data);
				break;
			case 0x34:
				op.env().set_decay_target(data);
				break;
			case 0x35:
				op.env().set_decay_rate(data);
				break;
			case 0x36:
				op.env().set_sustain_target(data);
				break;
			case 0x37:
				op.env().set_sustain_rate(data);
				break;
			case 0x38:
				op.env().set_release_rate(data);
				break;
			case 0x39:
				op.env().set_multiplier(s16(data));
				break;
			case 0x40:
				op.flfo().set_delay_rate(data);
				break;
			case 0x41:
				op.flfo().set_target(data);
				break;
			case 0x42:
				op.flfo().set_rate(data);
				break;
			case 0x43:
				op.flfo().set_multiplier(s16(data));
				break;
			case 0x44:
				op.flfo().set_noise_pitch(data);
				break;
			case 0x45:
				op.flfo().set_initial_lfsr(data);
				break;
			case 0x46:
				op.flfo().set_lfsr_mask(data);
				break;
			case 0x48:
				op.alfo().set_delay_rate(data);
				break;
			case 0x49:
				op.alfo().set_target(data);
				break;
			case 0x4a:
				op.alfo().set_rate(data);
				break;
			case 0x4b:
				op.alfo().set_multiplier(s16(data));
				break;
			case 0x4c:
				op.flfo().set_noise_pitch(data);
				break;
			case 0x4d:
				op.flfo().set_initial_lfsr(data);
				break;
			case 0x4e:
				op.flfo().set_lfsr_mask(data);
				break;
		}
	}
}; // namespace jkms16wm32o8
