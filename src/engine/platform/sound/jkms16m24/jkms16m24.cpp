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

JKMS16M24 Fantasy sound generator chip

Recommend input clock: 98.304MHz
Output rate: Input clock / 384 (24 groups / 4 operators / 4 cycle)

Features:
- 24 operator groups (act as channels)
- 4 operator per group
- FM, AM, PM in/output to/from each operators
- Built in volume envelope, vibrato, tremolo, 4 pole filter per operator
	- Lowpass, Highpass, Bandpass support for each filter poles
- Internal/External waveform support
	- Up to 64Kword x 16bit external RAM for external waveform
- Stereo output support

*/

#include "jkms16m24.hpp"

namespace jkms16m24
{
	/*
		Register map

		Register Bit   Description
		Global:
		0x00     15    Sound output enable
		         7-8   Filter pole index in operator
		         5-6   Operator index in group for register 0x06 onward
		         0-4   Group index for register 0x03 onward (up to 23 (0x17))
		0x01     15-0  Global left volume (16 bit signed)
		0x02     15-0  Global right volume (16 bit signed)
		Group: (index specified from register 0x00)
		0x03     N     Operator N keyon(rising edge)/off(falling edge) (Read: Keyon status, N = 0...3)
		0z04     15-0  Group left volume (16 bit signed)
		0x05     15-0  Group right volume (16 bit signed)
		Operator: (index specified from register 0x00)
		0x06     15    Speaker output enable
		         14    FM output enable
		         13    AM output enable
		         12    PM output enable
		         11    FM input enable
		         10    AM input enable
		         9     PM input enable
		         8     Envelope enable
		         7     Vibrato enable
		         6     Tremolo enable
		         5     Envelope loop (repeat decay and sustain stage)
		         4     Vibrato invert
		         3     Tremolo invert
		         2     Envelope sync to pitch
		         1     Vibrato sync to pitch
		         0     Tremolo sync to pitch
		0x07     15-0  Operator pitch (Period = Output rate / (value + 1))
		0x08     15-0  Operator output scale (16 bit signed)
		0x09     15-0  Operator left speaker output scale (16 bit signed)
		0x0a     15-0  Operator right speaker output scale (16 bit signed)
		0x0b     15    Internal waveform enable
		         14    Delta noise
		         13    White noise
		         12    Periodic noise
		         11    Pulse wave
		         10    Inverted pulse wave
		         9     Sawtooth wave
		         8     Inverted sawtooth wave
		         7     Triangle wave
		         6     Inverted triangle wave
		         3-0   Internal waveform length (65536 >> value)
		0x0c     15-0  Pulse width for pulse wave
		0x0d     15-0  LFSR initial value
		0x0e     15-0  LFSR update mask
		0x0f     15-0  LFSR update scale
		0x10     15    External waveform enable
		         14    External waveform horizontal invert bit enable
		         13    External waveform vertical invert bit enable
		         12    External waveform mute bit enable
		         11-8  External waveform horizontal invert bit position (in phase)
		         7-4   External waveform vertical invert bit position (in phase)
		         3-0   External waveform mute bit position (in phase)
		0x11     15-0  External waveform base address (in external RAM)
		0x12     3-0   External waveform length (65536 >> value)
		0x13     15-0  Waveform output scale (16 bit signed)
		0x14     15-0  Envelope delay time*
		0x15     15-0  Envelope attack rate*
		0x16     15-0  Envelope attack target**
		0x17     15-0  Envelope decay rate*
		0x18     15-0  Envelope decay target**
		0x19     15-0  Envelope sustain rate*
		0x1a     15-0  Envelope sustain target**
		0x1b     15-0  Envelope release rate*
		0x1c     15-0  Envelope output scale (16 bit signed)
		0x1d     1-0   Vibrato waveform (Off, Triangle, Sawtooth, Square)
		0x1e     15-0  Vibrato delay time*
		0x1f     15-0  Vibrato speed*
		0x20     15-0  Vibrato output scale (16 bit signed)
		0x21     1-0   Tremolo waveform (Off, Triangle, Sawtooth, Square)
		0x22     15-0  Tremolo delay time*
		0x23     15-0  Tremolo speed*
		0x24     15-0  Tremolo output scale (16 bit signed)
		0x25     15-0  Filter output scale (16 bit signed)
		Filter pole: (index specified from register 0x00)
		0x26     15    Filter pole enable
		         14    Add lowpass output
		         13    Add highpass output
		         12    Add bandpass output
		         11-0  Resonance (4096 - value)
		0x27     15-0  Cutoff frequency (16 bit signed)
		0x28     15-0  Filter pole output scale (16 bit signed)
		0x29     N+8   FM output to operator N (N = 0...3)
		         N+4   AM output to operator N (N = 0...3)
		         N+0   PM output to operator N (N = 0...3)
		0x2a     15-0  FM output scale (16 bit signed)
		0x2b     15-0  AM output scale (16 bit signed)
		0x2c     15-0  PM output scale (16 bit signed)
		0x2d     15-0  FM input scale (16 bit signed)
		0x2e     15-0  AM input scale (16 bit signed)
		0x2f     15-0  PM input scale (16 bit signed)
		Status
		9z30     15-0  Waveform address/phase (Read only)

		* Envelope delay time/Envelope stage rate/LFO speed calculation:
		- Exponent: Top 4 bit of register
		- Mantissa: Bottom 12 bit of register
		if Exponent == 0 then
			output = Mantissa;
		end else then
			output = (0x1000 | Mantissa) << (Exponent - 1);
		end
		return output;

		** Envelope stage target calculation:
		- Exponent: Top 5 bit of register, MSB is sign
		- Mantissa: Bottom 11 bit of register
		if (Exponent & 0xf) == 0 then
			output = Mantissa;
		end else then
			output = (0x800 | Mantissa) << ((Exponent & 0xf) - 1);
		end
		return sign ? -output : output;
		Min/Max value is -0x2000000 to 0x2000000
	*/
	u16 jkms16m24_t::reg_r()
	{
		if ((m_reg_index >= 0x03) && (m_group_index >= 24))
			return 0;
		group_t &group = m_group[(m_group_index >= 24) ? 0 : m_group_index];
		u16 ret = 0;
		switch (m_reg_index)
		{
			// global
			case 0x00:
				ret = (m_enable ? 0x8000 : 0) |
						(m_filter_pole_index << 7) |
						(m_operator_index << 5) |
						(m_group_index << 0);
				break;
			case 0x01:
				ret = m_lvol;
				break;
			case 0x02:
				ret = m_rvol;
				break;
			// group
			case 0x03:
			{
				for (u8 op = 0; op < 4; op++)
				{
					if (group.op(op).get_keyon())
					{
						ret |= 1 << op;
					}
				}
				break;
			}
			case 0x04:
				ret = group.get_lvol();
				break;
			case 0x05:
				ret = group.get_rvol();
				break;
			// operator
			case 0x06:
				ret = (group.op(m_operator_index).get_speaker_output_en() ? 0x8000 : 0) |
						(group.op(m_operator_index).get_fm_output_en() ? 0x4000 : 0) |
						(group.op(m_operator_index).get_am_output_en() ? 0x2000 : 0) |
						(group.op(m_operator_index).get_pm_output_en() ? 0x1000 : 0) |
						(group.op(m_operator_index).get_fm_input_en() ? 0x800 : 0) |
						(group.op(m_operator_index).get_am_input_en() ? 0x400 : 0) |
						(group.op(m_operator_index).get_pm_input_en() ? 0x200 : 0) |
						(group.op(m_operator_index).envelope().get_enable() ? 0x100 : 0) |
						(group.op(m_operator_index).vibrato().get_enable() ? 0x80 : 0) |
						(group.op(m_operator_index).tremolo().get_enable() ? 0x40 : 0) |
						(group.op(m_operator_index).envelope().get_loop() ? 0x20 : 0) |
						(group.op(m_operator_index).vibrato().get_invert() ? 0x10 : 0) |
						(group.op(m_operator_index).tremolo().get_invert() ? 0x8 : 0) |
						(group.op(m_operator_index).get_env_sync() ? 0x4 : 0) |
						(group.op(m_operator_index).get_vibrato_sync() ? 0x2 : 0) |
						(group.op(m_operator_index).get_tremolo_sync() ? 0x1 : 0);
				break;
			case 0x07:
				ret = group.op(m_operator_index).get_pitch();
				break;
			case 0x08:
				ret = group.op(m_operator_index).get_output_scale();
				break;
			case 0x09:
				ret = group.op(m_operator_index).get_loutput_scale();
				break;
			case 0x0a:
				ret = group.op(m_operator_index).get_routput_scale();
				break;
			// waveform generator
			case 0x0b:
				ret = (group.op(m_operator_index).wavegen().get_int_wave_enable() ? 0x8000 : 0) |
						(group.op(m_operator_index).wavegen().get_delta_noise() ? 0x4000 : 0) |
						(group.op(m_operator_index).wavegen().get_white_noise() ? 0x2000 : 0) |
						(group.op(m_operator_index).wavegen().get_periodic_noise() ? 0x1000 : 0) |
						(group.op(m_operator_index).wavegen().get_pulse() ? 0x800 : 0) |
						(group.op(m_operator_index).wavegen().get_inv_pulse() ? 0x400 : 0) |
						(group.op(m_operator_index).wavegen().get_sawtooth() ? 0x200 : 0) |
						(group.op(m_operator_index).wavegen().get_inv_sawtooth() ? 0x100 : 0) |
						(group.op(m_operator_index).wavegen().get_triangle() ? 0x80 : 0) |
						(group.op(m_operator_index).wavegen().get_inv_triangle() ? 0x40 : 0) |
						(group.op(m_operator_index).wavegen().get_wave_len() << 0);
				break;
			case 0x0c:
				ret = group.op(m_operator_index).wavegen().get_pulse_width();
				break;
			case 0x0d:
				ret = group.op(m_operator_index).wavegen().get_lfsr_init();
				break;
			case 0x0e:
				ret = group.op(m_operator_index).wavegen().get_lfsr_mask();
				break;
			case 0x0f:
				ret = group.op(m_operator_index).wavegen().get_lfsr_scale();
				break;
			case 0x10:
				ret = (group.op(m_operator_index).wavegen().get_ext_wave_enable() ? 0x8000 : 0) |
						(group.op(m_operator_index).wavegen().get_hinv_enable() ? 0x4000 : 0) |
						(group.op(m_operator_index).wavegen().get_vinv_enable() ? 0x2000 : 0) |
						(group.op(m_operator_index).wavegen().get_mute_enable() ? 0x1000 : 0) |
						(group.op(m_operator_index).wavegen().get_hinv_bitoffs() << 8) |
						(group.op(m_operator_index).wavegen().get_vinv_bitoffs() << 4) |
						(group.op(m_operator_index).wavegen().get_mute_bitoffs() << 0);
				break;
			case 0x11:
				ret = group.op(m_operator_index).wavegen().get_base_addr();
				break;
			case 0x12:
				ret = group.op(m_operator_index).wavegen().get_ext_wave_len();
				break;
			case 0x13:
				ret = group.op(m_operator_index).get_wave_scale();
				break;
			// envelope
			case 0x14:
				ret = group.op(m_operator_index).envelope().get_delay_time();
				break;
			case 0x15:
				ret = group.op(m_operator_index).envelope().get_attack_rate();
				break;
			case 0x16:
				ret = group.op(m_operator_index).envelope().get_attack_target();
				break;
			case 0x17:
				ret = group.op(m_operator_index).envelope().get_decay_rate();
				break;
			case 0x18:
				ret = group.op(m_operator_index).envelope().get_decay_target();
				break;
			case 0x19:
				ret = group.op(m_operator_index).envelope().get_sustain_rate();
				break;
			case 0x1a:
				ret = group.op(m_operator_index).envelope().get_sustain_target();
				break;
			case 0x1b:
				ret = group.op(m_operator_index).envelope().get_release_rate();
				break;
			case 0x1c:
				ret = group.op(m_operator_index).get_envelope_scale();
				break;
			// vibrato
			case 0x1d:
				ret = group.op(m_operator_index).vibrato().get_waveform();
				break;
			case 0x1e:
				ret = group.op(m_operator_index).vibrato().get_lfo_delay_time();
				break;
			case 0x1f:
				ret = group.op(m_operator_index).vibrato().get_lfo_rate();
				break;
			case 0x20:
				ret = group.op(m_operator_index).get_vibrato_scale();
				break;
			// tremolo
			case 0x21:
				ret = group.op(m_operator_index).tremolo().get_waveform();
				break;
			case 0x22:
				ret = group.op(m_operator_index).tremolo().get_lfo_delay_time();
				break;
			case 0x23:
				ret = group.op(m_operator_index).tremolo().get_lfo_rate();
				break;
			case 0x24:
				ret = group.op(m_operator_index).get_tremolo_scale();
				break;
			// filter
			case 0x25:
				ret = group.op(m_operator_index).filter().get_scale();
				break;
			case 0x26:
				ret = (group.op(m_operator_index).filter().pole(m_filter_pole_index).get_enable() ? 0x8000 : 0) |
						(group.op(m_operator_index).filter().pole(m_filter_pole_index).get_add_lp() ? 0x4000 : 0) |
						(group.op(m_operator_index).filter().pole(m_filter_pole_index).get_add_hp() ? 0x2000 : 0) |
						(group.op(m_operator_index).filter().pole(m_filter_pole_index).get_add_bp() ? 0x1000 : 0) |
						(group.op(m_operator_index).filter().pole(m_filter_pole_index).get_resonance() << 0);
				break;
			case 0x27:
				ret = group.op(m_operator_index).filter().pole(m_filter_pole_index).get_cutoff();
				break;
			case 0x28:
				ret = group.op(m_operator_index).filter().pole(m_filter_pole_index).get_scale();
				break;
			// modulator
			case 0x29:
				ret = (group.op(m_operator_index).get_fm_output_matrix() << 8) |
						(group.op(m_operator_index).get_am_output_matrix() << 4) |
						(group.op(m_operator_index).get_pm_output_matrix() << 0);
				break;
			case 0x2a:
				ret = group.op(m_operator_index).get_fm_output_scale();
				break;
			case 0x2b:
				ret = group.op(m_operator_index).get_am_output_scale();
				break;
			case 0x2c:
				ret = group.op(m_operator_index).get_pm_output_scale();
				break;
			case 0x2d:
				ret = group.op(m_operator_index).get_fm_input_scale();
				break;
			case 0x2e:
				ret = group.op(m_operator_index).get_am_input_scale();
				break;
			case 0x2f:
				ret = group.op(m_operator_index).get_pm_input_scale();
				break;
			case 0x30:
				ret = group.op(m_operator_index).get_addr();
				break;
		}
		return ret;
	}

	void jkms16m24_t::reg_w(const u16 data)
	{
		if ((m_reg_index >= 0x03) && (m_group_index >= 24))
			return;
		group_t &group = m_group[(m_group_index >= 24) ? 0 : m_group_index];
		switch (m_reg_index)
		{
			// global
			case 0x00:
				m_enable = bitfield(data, 15);
				m_filter_pole_index = bitfield(data, 7, 2);
				m_operator_index = bitfield(data, 5, 2);
				m_group_index = bitfield(data, 0, 5);
				break;
			case 0x01:
				m_lvol = data;
				break;
			case 0x02:
				m_rvol = data;
				break;
			// group
			case 0x03:
			{
				for (u8 op = 0; op < 4; op++)
				{
					// rising edge: keyon
					if (!group.op(op).get_keyon() && bitfield(data, op))
					{
						group.op(op).keyon();
					}
					// falling edge: keyoff
					if (group.op(op).get_keyon() && !bitfield(data, op))
					{
						group.op(op).keyoff();
					}
				}
				break;
			}
			case 0x04:
				group.set_lvol(data);
				break;
			case 0x05:
				group.set_rvol(data);
				break;
			// operator
			case 0x06:
				group.op(m_operator_index).set_speaker_output_en(bitfield(data, 15));
				group.op(m_operator_index).set_fm_output_en(bitfield(data, 14));
				group.op(m_operator_index).set_am_output_en(bitfield(data, 13));
				group.op(m_operator_index).set_pm_output_en(bitfield(data, 12));
				group.op(m_operator_index).set_fm_input_en(bitfield(data, 11));
				group.op(m_operator_index).set_am_input_en(bitfield(data, 10));
				group.op(m_operator_index).set_pm_input_en(bitfield(data, 9));
				group.op(m_operator_index).envelope().set_enable(bitfield(data, 8));
				group.op(m_operator_index).vibrato().set_enable(bitfield(data, 7));
				group.op(m_operator_index).tremolo().set_enable(bitfield(data, 6));
				group.op(m_operator_index).envelope().set_loop(bitfield(data, 5));
				group.op(m_operator_index).vibrato().set_invert(bitfield(data, 4));
				group.op(m_operator_index).tremolo().set_invert(bitfield(data, 3));
				group.op(m_operator_index).set_env_sync(bitfield(data, 2));
				group.op(m_operator_index).set_vibrato_sync(bitfield(data, 1));
				group.op(m_operator_index).set_tremolo_sync(bitfield(data, 0));
				break;
			case 0x07:
				group.op(m_operator_index).set_pitch(data);
				break;
			case 0x08:
				group.op(m_operator_index).set_output_scale(data);
				break;
			case 0x09:
				group.op(m_operator_index).set_loutput_scale(data);
				break;
			case 0x0a:
				group.op(m_operator_index).set_routput_scale(data);
				break;
			// waveform generator
			case 0x0b:
				group.op(m_operator_index).wavegen().set_int_wave_enable(bitfield(data, 15));
				group.op(m_operator_index).wavegen().set_delta_noise(bitfield(data, 14));
				group.op(m_operator_index).wavegen().set_white_noise(bitfield(data, 13));
				group.op(m_operator_index).wavegen().set_periodic_noise(bitfield(data, 12));
				group.op(m_operator_index).wavegen().set_pulse(bitfield(data, 11));
				group.op(m_operator_index).wavegen().set_inv_pulse(bitfield(data, 10));
				group.op(m_operator_index).wavegen().set_sawtooth(bitfield(data, 9));
				group.op(m_operator_index).wavegen().set_inv_sawtooth(bitfield(data, 8));
				group.op(m_operator_index).wavegen().set_triangle(bitfield(data, 7));
				group.op(m_operator_index).wavegen().set_inv_triangle(bitfield(data, 6));
				group.op(m_operator_index).wavegen().set_wave_len(bitfield(data, 0, 4));
				break;
			case 0x0c:
				group.op(m_operator_index).wavegen().set_pulse_width(data);
				break;
			case 0x0d:
				group.op(m_operator_index).wavegen().set_lfsr_init(data);
				break;
			case 0x0e:
				group.op(m_operator_index).wavegen().set_lfsr_mask(data);
				break;
			case 0x0f:
				group.op(m_operator_index).wavegen().set_lfsr_scale(data);
				break;
			case 0x10:
				group.op(m_operator_index).wavegen().set_ext_wave_enable(bitfield(data, 15));
				group.op(m_operator_index).wavegen().set_hinv_enable(bitfield(data, 14));
				group.op(m_operator_index).wavegen().set_vinv_enable(bitfield(data, 13));
				group.op(m_operator_index).wavegen().set_mute_enable(bitfield(data, 12));
				group.op(m_operator_index).wavegen().set_hinv_bitoffs(bitfield(data, 8, 4));
				group.op(m_operator_index).wavegen().set_vinv_bitoffs(bitfield(data, 4, 4));
				group.op(m_operator_index).wavegen().set_mute_bitoffs(bitfield(data, 0, 4));
				break;
			case 0x11:
				group.op(m_operator_index).wavegen().set_base_addr(data);
				break;
			case 0x12:
				group.op(m_operator_index).wavegen().set_ext_wave_len(bitfield(data, 0, 4));
				break;
			case 0x13:
				group.op(m_operator_index).set_wave_scale(data);
				break;
			// envelope
			case 0x14:
				group.op(m_operator_index).envelope().set_delay_time(data);
				break;
			case 0x15:
				group.op(m_operator_index).envelope().set_attack_rate(data);
				break;
			case 0x16:
				group.op(m_operator_index).envelope().set_attack_target(data);
				break;
			case 0x17:
				group.op(m_operator_index).envelope().set_decay_rate(data);
				break;
			case 0x18:
				group.op(m_operator_index).envelope().set_decay_target(data);
				break;
			case 0x19:
				group.op(m_operator_index).envelope().set_sustain_rate(data);
				break;
			case 0x1a:
				group.op(m_operator_index).envelope().set_sustain_target(data);
				break;
			case 0x1b:
				group.op(m_operator_index).envelope().set_release_rate(data);
				break;
			case 0x1c:
				group.op(m_operator_index).set_envelope_scale(data);
				break;
			// vibrato
			case 0x1d:
				group.op(m_operator_index).vibrato().set_waveform(bitfield(data, 0, 2));
				break;
			case 0x1e:
				group.op(m_operator_index).vibrato().set_lfo_delay_time(data);
				break;
			case 0x1f:
				group.op(m_operator_index).vibrato().set_lfo_rate(data);
				break;
			case 0x20:
				group.op(m_operator_index).set_vibrato_scale(data);
				break;
			// tremolo
			case 0x21:
				group.op(m_operator_index).tremolo().set_waveform(bitfield(data, 0, 2));
				break;
			case 0x22:
				group.op(m_operator_index).tremolo().set_lfo_delay_time(data);
				break;
			case 0x23:
				group.op(m_operator_index).tremolo().set_lfo_rate(data);
				break;
			case 0x24:
				group.op(m_operator_index).set_tremolo_scale(data);
				break;
			// filter
			case 0x25:
				group.op(m_operator_index).filter().set_scale(data);
				break;
			case 0x26:
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_enable(bitfield(data, 15));
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_add_lp(bitfield(data, 14));
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_add_hp(bitfield(data, 13));
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_add_bp(bitfield(data, 12));
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_resonance(bitfield(data, 0, 12));
				break;
			case 0x27:
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_cutoff(data);
				break;
			case 0x28:
				group.op(m_operator_index).filter().pole(m_filter_pole_index).set_scale(data);
				break;
			// modulator
			case 0x29:
				group.op(m_operator_index).set_fm_output_matrix(bitfield(data, 8, 4));
				group.op(m_operator_index).set_am_output_matrix(bitfield(data, 4, 4));
				group.op(m_operator_index).set_pm_output_matrix(bitfield(data, 0, 4));
				break;
			case 0x2a:
				group.op(m_operator_index).set_fm_output_scale(data);
				break;
			case 0x2b:
				group.op(m_operator_index).set_am_output_scale(data);
				break;
			case 0x2c:
				group.op(m_operator_index).set_pm_output_scale(data);
				break;
			case 0x2d:
				group.op(m_operator_index).set_fm_input_scale(data);
				break;
			case 0x2e:
				group.op(m_operator_index).set_am_input_scale(data);
				break;
			case 0x2f:
				group.op(m_operator_index).set_pm_input_scale(data);
				break;
		}
	}
}; // namespace jkms16m24
