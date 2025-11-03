/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2025 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "jkms16wm32o8.h"
#include "../engine.h"
#include "../bsr.h"
#include "../../ta-log.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

enum {
  WM_ADDR_CHSEL=0x00,
  WM_ADDR_KEYON=0x01,
  WM_ADDR_LVOL=0x02,
  WM_ADDR_RVOL=0x03,
  WM_ADDR_CTRL=0x04, // envelope/lfo/output/modulator enable/wave
  WM_ADDR_WBIT=0x05, // wave bit enable/position, spaeker output enable
  WM_ADDR_WGEN=0x06, // Waveform size/shape
  WM_ADDR_WADDR=0x07, // external waveform address
  WM_ADDR_PITCH=0x08,
  WM_ADDR_DUTY=0x09, // pulse duty
  WM_ADDR_FMINMUL=0x0a, // FM input multiplier
  WM_ADDR_PMINMUL=0x0b, // PM input multiplier
  WM_ADDR_AMINMUL=0x0c, // AM input multiplier
  WM_ADDR_FMONMUL=0x0d, // FM output multiplier
  WM_ADDR_PMONMUL=0x0e, // PM output multiplier
  WM_ADDR_AMONMUL=0x0f, // AM output multiplier
  WM_ADDR_FMFB=0x10, // FM feedback
  WM_ADDR_PMFB=0x11, // PM feedback
  WM_ADDR_AMFB=0x12, // AM feedback
  WM_ADDR_FMPMMAT=0x13, // FM, PM matrix
  WM_ADDR_AMMAT=0x14, // AM matrix
  WM_ADDR_SOUTMVOL=0x15, // Speaker output master volume
  WM_ADDR_SOUTLVOL=0x16, // Speaker output left volume
  WM_ADDR_SOUTRVOL=0x17, // Speaker output right volume
  WM_ADDR_TL=0x18, // Total level
  WM_ADDR_NPITCH=0x19, // Noise pitch
  WM_ADDR_NILFSR=0x1a, // Noise initial LFSR
  WM_ADDR_NMASK=0x1a, // Noise LFSR mask
  WM_ADDR_FILTEN0=0x1e, // Filter 0-3 enable/low/high/bandpass
  WM_ADDR_FILTEN1=0x1f, // Filter 4-7 enable/low/high/bandpass
  WM_ADDR_FILT0F=0x20, // Filter 0 F
  WM_ADDR_FILT0Q=0x21, // Filter 0 Q
  WM_ADDR_FILT1F=0x22, // Filter 1 F
  WM_ADDR_FILT1Q=0x23, // Filter 1 Q
  WM_ADDR_FILT2F=0x24, // Filter 2 F
  WM_ADDR_FILT2Q=0x25, // Filter 2 Q
  WM_ADDR_FILT3F=0x26, // Filter 3 F
  WM_ADDR_FILT3Q=0x27, // Filter 3 Q
  WM_ADDR_FILT4F=0x28, // Filter 4 F
  WM_ADDR_FILT4Q=0x29, // Filter 4 Q
  WM_ADDR_FILT5F=0x2a, // Filter 5 F
  WM_ADDR_FILT5Q=0x2b, // Filter 5 Q
  WM_ADDR_FILT6F=0x2c, // Filter 6 F
  WM_ADDR_FILT6Q=0x2d, // Filter 6 Q
  WM_ADDR_FILT7F=0x2e, // Filter 7 F
  WM_ADDR_FILT7Q=0x2f, // Filter 7 Q
  WM_ADDR_ENVIN=0x30, // Envelope initial level
  WM_ADDR_ENVDL=0x31, // Envelope delay
  WM_ADDR_ENVAT=0x32, // Envelope attack target
  WM_ADDR_ENVAR=0x33, // Envelope attack rate
  WM_ADDR_ENVDT=0x34, // Envelope decay target
  WM_ADDR_ENVDR=0x35, // Envelope decay rate
  WM_ADDR_ENVST=0x36, // Envelope sustain target
  WM_ADDR_ENVSR=0x37, // Envelope sustain rate
  WM_ADDR_ENVRR=0x38, // Envelope release rate
  WM_ADDR_ENVMUL=0x39, // Envelope multiplier
  WM_ADDR_FLFODL=0x40, // Frequency LFO delay
  WM_ADDR_FLFOT=0x41, // Frequency LFO target
  WM_ADDR_FLFOL=0x42, // Frequency LFO level
  WM_ADDR_FLFOMUL=0x43, // Frequency LFO multiplier
  WM_ADDR_FLFONP=0x44, // Frequency LFO noise pitch
  WM_ADDR_FLFONIL=0x45, // Frequency LFO noise initial LFSR
  WM_ADDR_FLFONM=0x46, // Frequency LFO noise LFSR mask
  WM_ADDR_ALFODL=0x48, // Amplitude LFO delay
  WM_ADDR_ALFOT=0x49, // Amplitude LFO target
  WM_ADDR_ALFOL=0x4a, // Amplitude LFO level
  WM_ADDR_ALFOMUL=0x4b, // Amplitude LFO multiplier
  WM_ADDR_ALFONP=0x4c, // Amplitude LFO noise pitch
  WM_ADDR_ALFONIL=0x4d, // Amplitude LFO noise initial LFSR
  WM_ADDR_ALFONM=0x4e, // Amplitude LFO noise LFSR mask
};

#define rWrite(a,v) { \
    if (!skipRegisterWrites) { \
      regPool[((a)&0x7fff)]=(v)&0xffff; \
      chip.host_w(0,((a)&0x7f)); \
      chip.host_w(1,(v)); \
    } \
  }

#define chWrite(c,a,v) { \
    if (!skipRegisterWrites) { \
      if (curChan!=(c)) { \
        chip.host_w(0,WM_ADDR_CHSEL); \
        chip.host_w(1,0x8000|(((c)&0x1f)<<3)|(curOp&7)); \
        curChan=(c); \
        regPool[0]=0x8000|(((c)&0x1f)<<3)|(curOp&7); \
      } \
      rWrite((((c)&0x1f)<<10)|((a)&0x7f),(v)); \
    } \
  }

#define opWrite(c,o,a,v) { \
    if (!skipRegisterWrites) { \
      if (curChan!=(c) || curOp!=(o)) { \
        chip.host_w(0,WM_ADDR_CHSEL); \
        chip.host_w(1,0x8000|(((c)&0x1f)<<3)|((o)&7)); \
        curChan=(c); \
        curOp=(o); \
        regPool[0]=0x8000|(((c)&0x1f)<<3)|((o)&7); \
      } \
      rWrite((((c)&0x1f)<<10)|(((o)&7)<<7)|((a)&0x7f),(v)); \
    } \
  }

#define envTarget(t) (((t)<0)?((((unsigned short)(t)&0xffff)^0x7fff)+1):(t))

#define CHIP_FREQBASE (1048576.0*4096.0)

void DivPlatformJKMS16WM32O8::acquire(short** buf, size_t len) {
  for (int i=0; i<32; i++) {
    oscBuf[i]->begin(len);
  }
  for (size_t h=0; h<len; h++) {
    chip.tick();
    for (int c=0; c<32; c++) {
      int chOut=(chip.channel(c).lout()+chip.channel(c).rout())>>1;
      oscBuf[c]->putSample(h,chOut);
    }
    int lout=chip.lout();
    if (lout<-32768) lout=-32768;
    if (lout>32767) lout=32767;
    int rout=chip.rout();
    if (rout<-32768) rout=-32768;
    if (rout>32767) rout=32767;

    buf[0][h]=lout;
    buf[1][h]=rout;
  }
  for (int i=0; i<32; i++) {
    oscBuf[i]->end(len);
  }
}

void DivPlatformJKMS16WM32O8::tick(bool sysTick) {
  for (int i=0; i<32; i++) {
    chan[i].std.next();

    if (chan[i].std.vol.had) {
      chan[i].outVol=VOL_SCALE_LINEAR((0x7fff*chan[i].vol)/0xff,chan[i].std.vol.val,0x7fff);
    }

    if (NEW_ARP_STRAT) {
      chan[i].handleArp();
    } else if (chan[i].std.arp.had) {
      if (!chan[i].inPorta) {
        chan[i].baseFreq=NOTE_FREQUENCY(parent->calcArp(chan[i].note,chan[i].std.arp.val));
      }
      chan[i].freqChanged=true;
    }

    if (chan[i].std.panL.had) {
      chan[i].globalLvolOut=VOL_SCALE_LINEAR((0x7fff*chan[i].globalLvol)/0xff,chan[i].std.panL.val,0x7fff);
    }

    if (chan[i].std.panR.had) {
      chan[i].globalRvolOut=VOL_SCALE_LINEAR((0x7fff*chan[i].globalRvol)/0xff,chan[i].std.panR.val,0x7fff);
    }

    if (chan[i].std.pitch.had) {
      if (chan[i].std.pitch.mode) {
        chan[i].pitch2+=chan[i].std.pitch.val;
        CLAMP_VAR(chan[i].pitch2,-65536,65535);
      } else {
        chan[i].pitch2=chan[i].std.pitch.val;
      }
      chan[i].freqChanged=true;
    }

    if (chan[i].std.phaseReset.had) {
      if (chan[i].std.phaseReset.val==1 && chan[i].active) {
        chan[i].keyOn=true;
      }
    }

    bool hasInverted=false;
    if (chan[i].std.ex1.had) {
      if (chan[i].invertL!=(bool)(chan[i].std.ex1.val&2)) {
        chan[i].invertL=chan[i].std.ex1.val&2;
        hasInverted=true;
      }
      if (chan[i].invertR!=(bool)(chan[i].std.ex1.val&1)) {
        chan[i].invertR=chan[i].std.ex1.val&1;
        hasInverted=true;
      }
    }
    if (chan[i].std.ex2.had && chan[i].active) {
      chan[i].opMask=chan[i].std.ex2.val&255;
      chan[i].opMaskChanged=true;
    }
    if (chan[i].std.vol.had || chan[i].std.panL.had || chan[i].std.panR.had || hasInverted) {
      chan[i].shallWriteVol=true;
    }

    for (int o=0; o<8; o++) {
      DivInstrumentWM::WMOperator& op=chan[i].state.op[o];
      DivMacroInt::IntWm& m=chan[i].std.wm[o];

      bool writeCtrl=false;
      bool writeBPos=false;
      bool writeWGen=false;
      bool writeFMPMMat=false;
      bool writeFilt0123=false;
      bool writeFilt4567=false;
      if (m.envEn.had) {
        if (op.env.enable!=(bool)(m.envEn.val&1)) {
          op.env.enable=m.envEn.val&1;
          writeCtrl=true;
        }
        if (op.env.loop!=(bool)(m.envEn.val&2)) {
          op.env.loop=m.envEn.val&2;
          writeCtrl=true;
        }
      }
      if (m.flfoEn.had) {
        if (op.flfo.enable!=(bool)(m.flfoEn.val&1)) {
          op.flfo.enable=m.flfoEn.val&1;
          writeCtrl=true;
        }
        if (op.flfo.wave!=((m.flfoEn.val>>1)&3)) {
          op.flfo.wave=(DivInstrumentWM::WMOperator::WMLfo::WMLfoWaveform)((m.flfoEn.val>>1)&3);
          writeCtrl=true;
        }
      }
      if (m.alfoEn.had) {
        if (op.alfo.enable!=(bool)(m.alfoEn.val&1)) {
          op.alfo.enable=m.alfoEn.val&1;
          writeCtrl=true;
        }
        if (op.alfo.wave!=((m.alfoEn.val>>1)&3)) {
          op.alfo.wave=(DivInstrumentWM::WMOperator::WMLfo::WMLfoWaveform)((m.alfoEn.val>>1)&3);
          writeCtrl=true;
        }
      }

      if (m.dt.had) {
        op.dt=m.dt.val;
        chan[i].freqChanged=true;
      }
      if (m.mult.had) {
        op.pitchMul=m.mult.val;
        chan[i].freqChanged=true;
      }

      if (m.outEn.had) {
        if (op.filtOut!=(bool)(m.outEn.val&1)) {
          op.filtOut=m.outEn.val&1;
          writeCtrl=true;
        }
        if (op.dirOut!=(bool)(m.outEn.val&2)) {
          op.dirOut=m.outEn.val&2;
          writeCtrl=true;
        }
        if (op.spkrEnable!=(bool)(m.outEn.val&4)) {
          op.spkrEnable=m.outEn.val&4;
          writeBPos=true;
        }
      }

      if (m.fmEn.had) {
        if (op.fmIn.enable!=(bool)(m.fmEn.val&1)) {
          op.fmIn.enable=m.fmEn.val&1;
          writeCtrl=true;
        }
        if (op.fmOut.enable!=(bool)(m.fmEn.val&2)) {
          op.fmOut.enable=m.fmEn.val&2;
          writeCtrl=true;
        }
      }
      if (m.pmEn.had) {
        if (op.pmIn.enable!=(bool)(m.pmEn.val&1)) {
          op.pmIn.enable=m.pmEn.val&1;
          writeCtrl=true;
        }
        if (op.pmOut.enable!=(bool)(m.pmEn.val&2)) {
          op.pmOut.enable=m.pmEn.val&2;
          writeCtrl=true;
        }
      }
      if (m.amEn.had) {
        if (op.amIn.enable!=(bool)(m.amEn.val&1)) {
          op.amIn.enable=m.amEn.val&1;
          writeCtrl=true;
        }
        if (op.amOut.enable!=(bool)(m.amEn.val&2)) {
          op.amOut.enable=m.amEn.val&2;
          writeCtrl=true;
        }
      }

      if (m.muteEn.had) {
        if (op.mute.enable!=(bool)(m.muteEn.val&1)) {
          op.mute.enable=m.muteEn.val&1;
          writeBPos=true;
        }
      }
      if (m.muteBit.had) {
        op.mute.bitPos=m.muteBit.val&0xf;
        writeBPos=true;
      }

      if (m.revEn.had) {
        if (op.reverse.enable!=(bool)(m.revEn.val&1)) {
          op.reverse.enable=m.revEn.val&1;
          writeBPos=true;
        }
      }
      if (m.revBit.had) {
        op.reverse.bitPos=m.revBit.val&0xf;
        writeBPos=true;
      }

      if (m.invEn.had) {
        if (op.invert.enable!=(bool)(m.invEn.val&1)) {
          op.invert.enable=m.invEn.val&1;
          writeBPos=true;
        }
      }
      if (m.invBit.had) {
        op.invert.bitPos=m.invBit.val&0xf;
        writeBPos=true;
      }

      if (m.intWl.had) {
        if (op.intWSize!=(unsigned char)(m.intWl.val&0xf)) {
          op.intWSize=m.intWl.val&0xf;
          writeWGen=true;
        }
      }
      if (m.extWl.had) {
        if (op.extWSize!=(unsigned char)(m.extWl.val&0xf)) {
          op.extWSize=m.extWl.val&0xf;
          writeWGen=true;
        }
      }
      if (m.wf.had) {
        if (op.wavBit!=(unsigned char)(m.wf.val&0xff)) {
          op.wavBit=m.wf.val&0xff;
          writeWGen=true;
        }
      }
      if (m.ew.had) {
        if (op.useSample) {
          if (op.initSample!=m.ew.val) {
            op.initSample=m.ew.val;
            chan[i].opsState[o].waveUpdated=true;
          }
        } else {
          if (op.initWave!=m.ew.val) {
            op.initWave=m.ew.val;
            chan[i].opsState[o].waveUpdated=true;
          }
        }
      }

      // detune/fixed pitch
      if (op.fixed) {
        if (!op.pitchCtrl) {
          if (m.pitch.had) {
            op.fixedFreq=m.pitch.val&0xffff;
            chan[i].freqChanged=true;
          }
        } else {
          chan[i].handleArpWmOp(0, o);
          chan[i].handlePitchWmOp(o);
        }
      }

      if (m.duty.had) {
        op.duty=m.duty.val;
        opWrite(i,o,WM_ADDR_DUTY,op.duty);
      }

      if (m.fmInMul.had) {
        op.fmIn.mul=m.fmInMul.val;
        opWrite(i,o,WM_ADDR_FMINMUL,op.fmIn.mul);
      }
      if (m.pmInMul.had) {
        op.pmIn.mul=m.pmInMul.val;
        opWrite(i,o,WM_ADDR_PMINMUL,op.pmIn.mul);
      }
      if (m.amInMul.had) {
        op.amIn.mul=m.amInMul.val;
        opWrite(i,o,WM_ADDR_AMINMUL,op.amIn.mul);
      }

      if (m.fmOutMul.had) {
        op.fmOut.mul=m.fmOutMul.val;
        opWrite(i,o,WM_ADDR_FMINMUL,op.fmOut.mul);
      }
      if (m.pmOutMul.had) {
        op.pmOut.mul=m.pmOutMul.val;
        opWrite(i,o,WM_ADDR_PMINMUL,op.pmOut.mul);
      }
      if (m.amOutMul.had) {
        op.amOut.mul=m.amOutMul.val;
        opWrite(i,o,WM_ADDR_AMINMUL,op.amOut.mul);
      }

      if (m.fmFb.had) {
        op.fmOut.fb=m.fmFb.val;
        opWrite(i,o,WM_ADDR_FMFB,op.fmOut.fb);
      }
      if (m.pmFb.had) {
        op.pmOut.fb=m.pmFb.val;
        opWrite(i,o,WM_ADDR_PMFB,op.pmOut.fb);
      }
      if (m.amFb.had) {
        op.amOut.fb=m.amFb.val;
        opWrite(i,o,WM_ADDR_AMFB,op.amOut.fb);
      }

      if (m.fmMatrix.had) {
        op.fmOut.matrix=m.fmMatrix.val;
        writeFMPMMat=true;
      }
      if (m.pmMatrix.had) {
        op.pmOut.matrix=m.pmMatrix.val;
        writeFMPMMat=true;
      }
      if (m.amMatrix.had) {
        op.amOut.matrix=m.amMatrix.val;
        opWrite(i,o,WM_ADDR_AMMAT,op.amOut.matrix<<8);
      }

      if (m.spkrVol.had) {
        op.spkrVol=m.spkrVol.val;
        opWrite(i,o,WM_ADDR_SOUTMVOL,op.spkrVol);
      }
      if (m.spkrLvol.had) {
        op.spkrLvol=m.spkrLvol.val;
        opWrite(i,o,WM_ADDR_SOUTLVOL,op.spkrLvol);
      }
      if (m.spkrRvol.had) {
        op.spkrRvol=m.spkrRvol.val;
        opWrite(i,o,WM_ADDR_SOUTRVOL,op.spkrRvol);
      }

      if (m.tl.had) {
        op.tl=m.tl.val;
        opWrite(i,o,WM_ADDR_TL,op.tl);
      }

      if (m.noiPitch.had) {
        op.noisePitch=m.noiPitch.val;
        opWrite(i,o,WM_ADDR_NPITCH,op.noisePitch);
      }
      if (m.noiILfsr.had) {
        op.initLfsr=m.noiILfsr.val;
        opWrite(i,o,WM_ADDR_NILFSR,op.initLfsr);
      }
      if (m.noiMask.had) {
        op.lfsrMask=m.noiMask.val;
        opWrite(i,o,WM_ADDR_NMASK,op.lfsrMask);
      }

      if (m.filtEn.had) {
        for (int f=0; f<4; f++) {
          if (op.filter[f].enable!=(bool)(m.filtEn.val&(1<<f))) {
            op.filter[f].enable=m.filtEn.val&(1<<f);
            writeFilt0123=true;
          }
        }
        for (int f=4; f<8; f++) {
          if (op.filter[f].enable!=(bool)(m.filtEn.val&(1<<f))) {
            op.filter[f].enable=m.filtEn.val&(1<<f);
            writeFilt4567=true;
          }
        }
      }
      if (m.filtLp.had) {
        for (int f=0; f<4; f++) {
          if (op.filter[f].lpEnable!=(bool)(m.filtLp.val&(1<<f))) {
            op.filter[f].lpEnable=m.filtLp.val&(1<<f);
            writeFilt0123=true;
          }
        }
        for (int f=4; f<8; f++) {
          if (op.filter[f].lpEnable!=(bool)(m.filtLp.val&(1<<f))) {
            op.filter[f].lpEnable=m.filtLp.val&(1<<f);
            writeFilt4567=true;
          }
        }
      }
      if (m.filtHp.had) {
        for (int f=0; f<4; f++) {
          if (op.filter[f].hpEnable!=(bool)(m.filtHp.val&(1<<f))) {
            op.filter[f].hpEnable=m.filtHp.val&(1<<f);
            writeFilt0123=true;
          }
        }
        for (int f=4; f<8; f++) {
          if (op.filter[f].hpEnable!=(bool)(m.filtHp.val&(1<<f))) {
            op.filter[f].hpEnable=m.filtHp.val&(1<<f);
            writeFilt4567=true;
          }
        }
      }
      if (m.filtBp.had) {
        for (int f=0; f<4; f++) {
          if (op.filter[f].bpEnable!=(bool)(m.filtBp.val&(1<<f))) {
            op.filter[f].bpEnable=m.filtBp.val&(1<<f);
            writeFilt0123=true;
          }
        }
        for (int f=4; f<8; f++) {
          if (op.filter[f].bpEnable!=(bool)(m.filtBp.val&(1<<f))) {
            op.filter[f].bpEnable=m.filtBp.val&(1<<f);
            writeFilt4567=true;
          }
        }
      }
      if (m.filt0F.had) {
        op.filter[0].f=m.filt0F.val;
        opWrite(i,o,WM_ADDR_FILT0F,op.filter[0].f);
      }
      if (m.filt0Q.had) {
        op.filter[0].q=m.filt0Q.val;
        opWrite(i,o,WM_ADDR_FILT0Q,op.filter[0].q);
      }
      if (m.filt1F.had) {
        op.filter[1].f=m.filt1F.val;
        opWrite(i,o,WM_ADDR_FILT1F,op.filter[1].f);
      }
      if (m.filt1Q.had) {
        op.filter[1].q=m.filt1Q.val;
        opWrite(i,o,WM_ADDR_FILT1Q,op.filter[1].q);
      }
      if (m.filt2F.had) {
        op.filter[2].f=m.filt2F.val;
        opWrite(i,o,WM_ADDR_FILT2F,op.filter[2].f);
      }
      if (m.filt2Q.had) {
        op.filter[2].q=m.filt2Q.val;
        opWrite(i,o,WM_ADDR_FILT2Q,op.filter[2].q);
      }
      if (m.filt3F.had) {
        op.filter[3].f=m.filt3F.val;
        opWrite(i,o,WM_ADDR_FILT3F,op.filter[3].f);
      }
      if (m.filt3Q.had) {
        op.filter[3].q=m.filt3Q.val;
        opWrite(i,o,WM_ADDR_FILT3Q,op.filter[3].q);
      }
      if (m.filt4F.had) {
        op.filter[4].f=m.filt4F.val;
        opWrite(i,o,WM_ADDR_FILT4F,op.filter[4].f);
      }
      if (m.filt4Q.had) {
        op.filter[4].q=m.filt4Q.val;
        opWrite(i,o,WM_ADDR_FILT4Q,op.filter[4].q);
      }
      if (m.filt5F.had) {
        op.filter[5].f=m.filt5F.val;
        opWrite(i,o,WM_ADDR_FILT5F,op.filter[5].f);
      }
      if (m.filt5Q.had) {
        op.filter[5].q=m.filt5Q.val;
        opWrite(i,o,WM_ADDR_FILT5Q,op.filter[5].q);
      }
      if (m.filt6F.had) {
        op.filter[6].f=m.filt6F.val;
        opWrite(i,o,WM_ADDR_FILT6F,op.filter[6].f);
      }
      if (m.filt6Q.had) {
        op.filter[6].q=m.filt6Q.val;
        opWrite(i,o,WM_ADDR_FILT6Q,op.filter[6].q);
      }
      if (m.filt7F.had) {
        op.filter[7].f=m.filt7F.val;
        opWrite(i,o,WM_ADDR_FILT7F,op.filter[7].f);
      }
      if (m.filt7Q.had) {
        op.filter[7].q=m.filt7Q.val;
        opWrite(i,o,WM_ADDR_FILT7Q,op.filter[7].q);
      }
      if (m.envAtkT.had) {
        op.env.atkT=m.envAtkT.val;
        opWrite(i,o,WM_ADDR_ENVAT,envTarget(op.env.atkT));
      }
      if (m.envAtkR.had) {
        op.env.atkR=m.envAtkR.val;
        opWrite(i,o,WM_ADDR_ENVAR,op.env.atkR);
      }
      if (m.envDecT.had) {
        op.env.decT=m.envDecT.val;
        opWrite(i,o,WM_ADDR_ENVDT,envTarget(op.env.decT));
      }
      if (m.envDecR.had) {
        op.env.decR=m.envDecR.val;
        opWrite(i,o,WM_ADDR_ENVDR,op.env.decR);
      }
      if (m.envSusT.had) {
        op.env.susT=m.envSusT.val;
        opWrite(i,o,WM_ADDR_ENVST,envTarget(op.env.susT));
      }
      if (m.envSusR.had) {
        op.env.susR=m.envSusR.val;
        opWrite(i,o,WM_ADDR_ENVSR,op.env.susR);
      }
      if (m.envRelR.had) {
        op.env.relR=m.envRelR.val;
        opWrite(i,o,WM_ADDR_ENVRR,op.env.relR);
      }
      if (m.envMul.had) {
        op.env.mul=m.envMul.val;
        opWrite(i,o,WM_ADDR_ENVMUL,op.env.mul);
      }

      if (m.flfoT.had) {
        op.flfo.tgt=m.flfoT.val;
        opWrite(i,o,WM_ADDR_FLFOT,envTarget(op.flfo.tgt));
      }
      if (m.flfoL.had) {
        op.flfo.rate=m.flfoL.val;
        opWrite(i,o,WM_ADDR_FLFOL,op.flfo.rate);
      }
      if (m.flfoM.had) {
        op.flfo.mul=m.flfoM.val;
        opWrite(i,o,WM_ADDR_FLFOMUL,op.flfo.mul);
      }
      if (m.flfoNPitch.had) {
        op.flfo.noisePitch=m.flfoNPitch.val;
        opWrite(i,o,WM_ADDR_FLFONP,op.flfo.noisePitch);
      }
      if (m.flfoNILfsr.had) {
        op.flfo.initLfsr=m.flfoNILfsr.val;
        opWrite(i,o,WM_ADDR_FLFONIL,op.flfo.initLfsr);
      }
      if (m.flfoNMask.had) {
        op.flfo.lfsrMask=m.flfoNMask.val;
        opWrite(i,o,WM_ADDR_FLFONM,op.flfo.lfsrMask);
      }

      if (m.alfoT.had) {
        op.alfo.tgt=m.alfoT.val;
        opWrite(i,o,WM_ADDR_ALFOT,envTarget(op.alfo.tgt));
      }
      if (m.alfoL.had) {
        op.alfo.rate=m.alfoL.val;
        opWrite(i,o,WM_ADDR_ALFOL,op.alfo.rate);
      }
      if (m.alfoM.had) {
        op.alfo.mul=m.alfoM.val;
        opWrite(i,o,WM_ADDR_ALFOMUL,op.alfo.mul);
      }
      if (m.alfoNPitch.had) {
        op.alfo.noisePitch=m.alfoNPitch.val;
        opWrite(i,o,WM_ADDR_ALFONP,op.alfo.noisePitch);
      }
      if (m.alfoNILfsr.had) {
        op.alfo.initLfsr=m.alfoNILfsr.val;
        opWrite(i,o,WM_ADDR_ALFONIL,op.alfo.initLfsr);
      }
      if (m.alfoNMask.had) {
        op.alfo.lfsrMask=m.alfoNMask.val;
        opWrite(i,o,WM_ADDR_ALFONM,op.alfo.lfsrMask);
      }

      if (writeCtrl) {
        writeCtrlVal(i,o);
        writeCtrl=false;
      }
      if (writeBPos) {
        unsigned short bPosval=((op.mute.bitPos&0xf)<<12)|((op.reverse.bitPos&0xf)<<8)|
          ((op.invert.bitPos&0xf)<<4)|(op.mute.enable?0x8:0)|(op.reverse.enable?0x4:0)|
          (op.invert.enable?0x2:0)|(op.spkrEnable?0x1:0);
        opWrite(i,o,WM_ADDR_WBIT,bPosval);
        writeBPos=false;
      }
      if (writeWGen) {
        unsigned short wGenVal=((op.intWSize&0xf)<<12)|((op.extWSize&0xf)<<8)|(op.wavBit&0xff);
        opWrite(i,o,WM_ADDR_WGEN,wGenVal);
        writeWGen=false;
        chan[i].opsState[o].waveUpdated=true;
      }
      if (writeFMPMMat) {
        opWrite(i,o,WM_ADDR_FMPMMAT,((op.fmOut.matrix&0xff)<<8)|((op.pmOut.matrix&0xff)));
        writeFMPMMat=false;
      }
      if (writeFilt0123) {
        unsigned short enVal=0;
        for (int f=0; f<4; f++) {
          if (op.filter[f].enable) enVal|=(8<<(f<<2));
          if (op.filter[f].lpEnable) enVal|=(4<<(f<<2));
          if (op.filter[f].hpEnable) enVal|=(2<<(f<<2));
          if (op.filter[f].bpEnable) enVal|=(1<<(f<<2));
        }
        opWrite(i,o,WM_ADDR_FILTEN0,enVal);
        writeFilt0123=false;
      }
      if (writeFilt4567) {
        unsigned short enVal=0;
        for (int f=4, sv=0; f<8; f++, sv++) {
          if (op.filter[f].enable) enVal|=(8<<(sv<<2));
          if (op.filter[f].lpEnable) enVal|=(4<<(sv<<2));
          if (op.filter[f].hpEnable) enVal|=(2<<(sv<<2));
          if (op.filter[f].bpEnable) enVal|=(1<<(sv<<2));
        }
        opWrite(i,o,WM_ADDR_FILTEN1,enVal);
        writeFilt4567=false;
      }

      if (chan[i].opsState[o].waveUpdated) {
        updateWaveCh(i,o);
        if (chan[i].active) {
          if (!chan[i].keyOff) chan[i].keyOn=true;
        }
        chan[i].opsState[o].waveUpdated=false;
      }
    }
  }

  int hardResetElapsed=0;
  bool mustHardReset=false;

  for (int i=0; i<32; i++) {
    if (chan[i].keyOn || chan[i].keyOff) {
      chWrite(i,WM_ADDR_KEYON,0);
      if (chan[i].hardReset && chan[i].keyOn) {
        mustHardReset=true;
        for (int j=0; j<8; j++) {
          opWrite(i,j,WM_ADDR_CTRL,0);
          hardResetElapsed++;
        }
      }
      chan[i].keyOff=false;
    }
  }

  for (int i=0; i<32; i++) {
    if (chan[i].freqChanged) {
      chan[i].freq=parent->calcFreq(chan[i].baseFreq,chan[i].pitch,chan[i].fixedArp?chan[i].baseNoteOverride:chan[i].arpOff,chan[i].fixedArp,false,2,chan[i].pitch2,chipClock,CHIP_FREQBASE);
      if (chan[i].freq<0x1000) chan[i].freq=0x1000;
      if (chan[i].freq>0xfffffff) chan[i].freq=0xfffffff;

      for (int o=0; o<8; o++) {
        DivInstrumentWM::WMOperator& op=chan[i].state.op[o];
        int dt=(int)op.dt;
        if (op.fixed && !op.pitchCtrl) {
          chan[i].freqL[o]=op.fixedFreq&0xfff;
          chan[i].freqH[o]=(op.fixedFreq>>12)&0xf;
        } else {
          int arp=chan[i].fixedArp?chan[i].baseNoteOverride:chan[i].arpOff;
          int pitch2=chan[i].pitch2+dt;
          int fixedArp=chan[i].fixedArp;
          if(chan[i].opsState[o].hasOpArp) {
            arp=chan[i].opsState[o].fixedArp?chan[i].opsState[o].baseNoteOverride:chan[i].opsState[o].arpOff;
            fixedArp=chan[i].opsState[o].fixedArp;
          }
          if(chan[i].opsState[o].hasOpPitch) {
            pitch2=chan[i].opsState[o].pitch2+dt;
          }
          int opFreq=parent->calcFreq(chan[i].baseFreq,chan[i].pitch,arp,fixedArp,false,2,pitch2,chipClock,CHIP_FREQBASE);
          opFreq=(opFreq*(op.pitchMul?op.pitchMul:1))>>((op.pitchMul)?0:1);
          if (opFreq<0x1000) opFreq=0x1000;
          chan[i].freqH[o]=0;
          if (opFreq>0xfffffff) {
            opFreq=0xfffffff;
            chan[i].freqH[o]=15;
          } else if (opFreq>=0x2000) {
            chan[i].freqH[o]=bsr32(opFreq)-13;
          }
          chan[i].freqL[o]=(opFreq>>chan[i].freqH[o])&0xfff;
        }
        opWrite(i,o,WM_ADDR_PITCH,chan[i].freqL[o]|(chan[i].freqH[o]<<12));
        hardResetElapsed+=2;
      }
      chan[i].freqChanged=false;
    }
    if ((chan[i].keyOn || chan[i].opMaskChanged) && !chan[i].hardReset) {
      chWrite(i,WM_ADDR_KEYON,chan[i].opMask&0xff);
      hardResetElapsed++;
      chan[i].opMaskChanged=false;
      chan[i].keyOn=false;
    }
  }

  // hard reset handling
  if (mustHardReset) {
    for (int i=0; i<32; i++) {
      if ((chan[i].keyOn || chan[i].opMaskChanged) && chan[i].hardReset) {
        // restore ctrl
        for (int j=0; j<8; j++) {
          DivInstrumentWM::WMOperator& op=chan[i].state.op[j];
          unsigned short ctrlVal=(op.env.enable?0x8000:0)|
            (op.env.loop?0x4000:0)|(op.flfo.enable?0x2000:0)|(op.alfo.enable?0x1000:0)|
            ((op.flfo.wave&3)<<10)|((op.alfo.wave&3)<<8)|(op.filtOut?0x80:0)|(op.dirOut?0x40:0)|
            (op.fmIn.enable?0x20:0)|(op.pmIn.enable?0x10:0)|(op.amIn.enable?0x8:0)|
            (op.fmOut.enable?0x4:0)|(op.pmOut.enable?0x2:0)|(op.amOut.enable?0x1:0);
          opWrite(i,j,WM_ADDR_ENVRR,ctrlVal);
        }

        chWrite(i,WM_ADDR_KEYON,chan[i].opMask&0xff);
        chan[i].opMaskChanged=false;
        chan[i].keyOn=false;
      }
    }
  }
  for (int i=0; i<32; i++) {
    if (chan[i].shallWriteVol) {
      writeOutVol(i);
      chan[i].shallWriteVol=false;
    }
  }
}

void DivPlatformJKMS16WM32O8::updateWave(int ch, int op, int wave, int pos, int len) {
  if (ch<0 || ch>=32) {
    return;
  }
  len=65536>>(len&15);
  DivInstrument* ins=parent->getIns(chan[ch].ins,DIV_INS_WM);
  if (ins->wm.op[op].useSample) {
    // load from sample
    if (wave>=0 && wave<parent->song.sampleLen) {
      DivSample* s=parent->getSample(wave);
      if (s!=NULL) {
        chip.host_w(2,pos);
        chip.host_w(4,1);
        for (int i=0; i<len; i++) {
          unsigned int addr=(i*s->length8)/len;
          int data=(unsigned short)(s->data16[addr])^0x8000;
          chip.host_w(3,data&0xffff);
        }
      }
    }
  } else {
    if (wave>=0 && wave<parent->song.waveLen) {
      // load from waveform
      DivWavetable* wt=parent->getWave(wave);
      if (wt!=NULL) {
        chip.host_w(2,pos);
        chip.host_w(4,1);
        for (int i=0; i<len; i++) {
          unsigned int addr=(i*wt->len)/len;
          int data=(unsigned short)((wt->data[addr]*65535)/wt->max);
          chip.host_w(3,data&0xffff);
        }
      }
    }
  }
}

void DivPlatformJKMS16WM32O8::updateWaveCh(int ch, int op) {
  if (ch>=0 && ch<=32) {
    updateWave(ch,op,(chan[ch].state.op[op].useSample)?chan[ch].state.op[op].initSample:chan[ch].state.op[op].initWave,chan[ch].state.op[op].wavBase,chan[ch].state.op[op].extWSize);
  }
}

void DivPlatformJKMS16WM32O8::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
  chan[ch].shallWriteVol=true;
}

void DivPlatformJKMS16WM32O8::commitState(int ch, DivInstrument* ins) {
  if (chan[ch].insChanged) {
    chan[ch].state=ins->wm;
    chan[ch].opMask=
      (chan[ch].state.op[0].enable?1:0)|
      (chan[ch].state.op[1].enable?2:0)|
      (chan[ch].state.op[2].enable?4:0)|
      (chan[ch].state.op[3].enable?8:0)|
      (chan[ch].state.op[4].enable?16:0)|
      (chan[ch].state.op[5].enable?32:0)|
      (chan[ch].state.op[6].enable?64:0)|
      (chan[ch].state.op[7].enable?128:0);
  }

  for (int i=0; i<8; i++) {
    DivInstrumentWM::WMOperator op=chan[ch].state.op[i];
    if (!op.enable) {
      opWrite(ch,i,WM_ADDR_CTRL,0);
      opWrite(ch,i,WM_ADDR_WBIT,0);
      opWrite(ch,i,WM_ADDR_WGEN,0);
    } else {
      if (chan[ch].insChanged) {
        writeCtrlVal(ch,i);
        unsigned short bPosval=((op.mute.bitPos&0xf)<<12)|((op.reverse.bitPos&0xf)<<8)|
          ((op.invert.bitPos&0xf)<<4)|(op.mute.enable?0x8:0)|(op.reverse.enable?0x4:0)|
          (op.invert.enable?0x2:0)|(op.spkrEnable?0x1:0);
        opWrite(ch,i,WM_ADDR_WBIT,bPosval);
        unsigned short wGenVal=((op.intWSize&0xf)<<12)|((op.extWSize&0xf)<<8)|(op.wavBit&0xff);
        opWrite(ch,i,WM_ADDR_WGEN,wGenVal);
        opWrite(ch,i,WM_ADDR_WADDR,op.wavBase);
        chan[ch].opsState[i].waveUpdated=true;
      }
    }
    if (chan[ch].insChanged) {
      opWrite(ch,i,WM_ADDR_FMPMMAT,((op.fmOut.matrix&0xff)<<8)|((op.pmOut.matrix&0xff)));
      unsigned short enVal=0;
      for (int f=0; f<4; f++) {
        if (op.filter[f].enable) enVal|=(8<<(f<<2));
        if (op.filter[f].lpEnable) enVal|=(4<<(f<<2));
        if (op.filter[f].hpEnable) enVal|=(2<<(f<<2));
        if (op.filter[f].bpEnable) enVal|=(1<<(f<<2));
      }
      opWrite(ch,i,WM_ADDR_FILTEN0,enVal);
      for (int f=4, sv=0; f<8; f++, sv++) {
        if (op.filter[f].enable) enVal|=(8<<(sv<<2));
        if (op.filter[f].lpEnable) enVal|=(4<<(sv<<2));
        if (op.filter[f].hpEnable) enVal|=(2<<(sv<<2));
        if (op.filter[f].bpEnable) enVal|=(1<<(sv<<2));
      }
      opWrite(ch,i,WM_ADDR_FILTEN1,enVal);
      opWrite(ch,i,WM_ADDR_DUTY,op.duty);
      opWrite(ch,i,WM_ADDR_FMINMUL,op.fmIn.mul);
      opWrite(ch,i,WM_ADDR_PMINMUL,op.pmIn.mul);
      opWrite(ch,i,WM_ADDR_AMINMUL,op.amIn.mul);
      opWrite(ch,i,WM_ADDR_FMINMUL,op.fmOut.mul);
      opWrite(ch,i,WM_ADDR_PMINMUL,op.pmOut.mul);
      opWrite(ch,i,WM_ADDR_AMINMUL,op.amOut.mul);
      opWrite(ch,i,WM_ADDR_FMFB,op.fmOut.fb);
      opWrite(ch,i,WM_ADDR_PMFB,op.pmOut.fb);
      opWrite(ch,i,WM_ADDR_AMFB,op.amOut.fb);
      opWrite(ch,i,WM_ADDR_AMMAT,op.amOut.matrix<<8);
      opWrite(ch,i,WM_ADDR_SOUTMVOL,op.spkrVol);
      opWrite(ch,i,WM_ADDR_SOUTLVOL,op.spkrLvol);
      opWrite(ch,i,WM_ADDR_SOUTRVOL,op.spkrRvol);
      opWrite(ch,i,WM_ADDR_TL,op.tl);
      opWrite(ch,i,WM_ADDR_NPITCH,op.noisePitch);
      opWrite(ch,i,WM_ADDR_NILFSR,op.initLfsr);
      opWrite(ch,i,WM_ADDR_NMASK,op.lfsrMask);
      opWrite(ch,i,WM_ADDR_FILT0F,op.filter[0].f);
      opWrite(ch,i,WM_ADDR_FILT0Q,op.filter[0].q);
      opWrite(ch,i,WM_ADDR_FILT1F,op.filter[1].f);
      opWrite(ch,i,WM_ADDR_FILT1Q,op.filter[1].q);
      opWrite(ch,i,WM_ADDR_FILT2F,op.filter[2].f);
      opWrite(ch,i,WM_ADDR_FILT2Q,op.filter[2].q);
      opWrite(ch,i,WM_ADDR_FILT3F,op.filter[3].f);
      opWrite(ch,i,WM_ADDR_FILT3Q,op.filter[3].q);
      opWrite(ch,i,WM_ADDR_FILT4F,op.filter[4].f);
      opWrite(ch,i,WM_ADDR_FILT4Q,op.filter[4].q);
      opWrite(ch,i,WM_ADDR_FILT5F,op.filter[5].f);
      opWrite(ch,i,WM_ADDR_FILT5Q,op.filter[5].q);
      opWrite(ch,i,WM_ADDR_FILT6F,op.filter[6].f);
      opWrite(ch,i,WM_ADDR_FILT6Q,op.filter[6].q);
      opWrite(ch,i,WM_ADDR_FILT7F,op.filter[7].f);
      opWrite(ch,i,WM_ADDR_FILT7Q,op.filter[7].q);
      opWrite(ch,i,WM_ADDR_ENVDL,op.env.delR);
      opWrite(ch,i,WM_ADDR_ENVIN,envTarget(op.env.initLv));
      opWrite(ch,i,WM_ADDR_ENVAT,envTarget(op.env.atkT));
      opWrite(ch,i,WM_ADDR_ENVAR,op.env.atkR);
      opWrite(ch,i,WM_ADDR_ENVDT,envTarget(op.env.decT));
      opWrite(ch,i,WM_ADDR_ENVDR,op.env.decR);
      opWrite(ch,i,WM_ADDR_ENVST,envTarget(op.env.susT));
      opWrite(ch,i,WM_ADDR_ENVSR,op.env.susR);
      opWrite(ch,i,WM_ADDR_ENVRR,op.env.relR);
      opWrite(ch,i,WM_ADDR_ENVMUL,op.env.mul);
      opWrite(ch,i,WM_ADDR_FLFODL,op.flfo.delR);
      opWrite(ch,i,WM_ADDR_FLFOT,envTarget(op.flfo.tgt));
      opWrite(ch,i,WM_ADDR_FLFOL,op.flfo.rate);
      opWrite(ch,i,WM_ADDR_FLFOMUL,op.flfo.mul);
      opWrite(ch,i,WM_ADDR_FLFONP,op.flfo.noisePitch);
      opWrite(ch,i,WM_ADDR_FLFONIL,op.flfo.initLfsr);
      opWrite(ch,i,WM_ADDR_FLFONM,op.flfo.lfsrMask);
      opWrite(ch,i,WM_ADDR_ALFODL,op.alfo.delR);
      opWrite(ch,i,WM_ADDR_ALFOT,envTarget(op.alfo.tgt));
      opWrite(ch,i,WM_ADDR_ALFOL,op.alfo.rate);
      opWrite(ch,i,WM_ADDR_ALFOMUL,op.alfo.mul);
      opWrite(ch,i,WM_ADDR_ALFONP,op.alfo.noisePitch);
      opWrite(ch,i,WM_ADDR_ALFONIL,op.alfo.initLfsr);
      opWrite(ch,i,WM_ADDR_ALFONM,op.alfo.lfsrMask);
    }
  }
  if (chan[ch].insChanged) {
    chan[ch].shallWriteVol=true;
  }
}

int DivPlatformJKMS16WM32O8::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[c.chan].ins,DIV_INS_WM);

      chan[c.chan].macroInit(ins);
      memset(chan[c.chan].opsState, 0, sizeof(chan[c.chan].opsState));
      if (!chan[c.chan].std.vol.will) {
        chan[c.chan].outVol=(0x7fff*chan[c.chan].vol)/0xff;
      }
      if (!chan[c.chan].std.panL.will) {
        chan[c.chan].globalLvolOut=(0x7fff*chan[c.chan].globalLvol)/0xff;
      }
      if (!chan[c.chan].std.panR.will) {
        chan[c.chan].globalRvolOut=(0x7fff*chan[c.chan].globalRvol)/0xff;
      }
      commitState(c.chan,ins);
      chan[c.chan].insChanged=false;

      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].baseFreq=NOTE_FREQUENCY(c.value);
        chan[c.chan].note=c.value;
        chan[c.chan].freqChanged=true;
      }
      chan[c.chan].keyOn=true;
      chan[c.chan].active=true;
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].keyOff=true;
      chan[c.chan].keyOn=false;
      chan[c.chan].active=false;
      break;
    case DIV_CMD_NOTE_OFF_ENV:
      chan[c.chan].keyOff=true;
      chan[c.chan].keyOn=false;
      chan[c.chan].active=false;
      chan[c.chan].std.release();
      break;
    case DIV_CMD_ENV_RELEASE:
      chan[c.chan].std.release();
      break;
    case DIV_CMD_VOLUME: {
      if (chan[c.chan].vol!=c.value) {
        chan[c.chan].vol=c.value;
        if (!chan[c.chan].std.vol.has) {
          chan[c.chan].outVol=(0x7fff*c.value)/0xff;
          chan[c.chan].shallWriteVol=true;
        }
      }
      break;
    }
    case DIV_CMD_GET_VOLUME:
      return chan[c.chan].vol;
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[c.chan].ins!=c.value || c.value2==1) {
        chan[c.chan].insChanged=true;
      }
      chan[c.chan].ins=c.value;
      break;
    case DIV_CMD_PANNING: {
      // Left volume
      if (chan[c.chan].globalLvol!=c.value) {
        chan[c.chan].globalLvol=c.value;
        if (!chan[c.chan].std.panL.has) {
          chan[c.chan].globalLvolOut=(0x7fff*c.value)/0xff;
          chan[c.chan].shallWriteVol=true;
        }
      }
      // Right volume
      if (chan[c.chan].globalRvol!=c.value2) {
        chan[c.chan].globalRvol=c.value2;
        if (!chan[c.chan].std.panR.has) {
          chan[c.chan].globalRvolOut=(0x7fff*c.value2)/0xff;
          chan[c.chan].shallWriteVol=true;
        }
      }
      break;
    }
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=NOTE_FREQUENCY(c.value2);
      int newFreq;
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        newFreq=chan[c.chan].baseFreq+c.value;
        if (newFreq>=destFreq) {
          newFreq=destFreq;
          return2=true;
        }
      } else {
        newFreq=chan[c.chan].baseFreq-c.value;
        if (newFreq<=destFreq) {
          newFreq=destFreq;
          return2=true;
        }
      }
      chan[c.chan].baseFreq=newFreq;
      chan[c.chan].freqChanged=true;
      if (return2) {
        chan[c.chan].inPorta=false;
        return 2;
      }
      break;
    }
    case DIV_CMD_LEGATO: {
      if (chan[c.chan].insChanged) {
        DivInstrument* ins=parent->getIns(chan[c.chan].ins,DIV_INS_WM);
        commitState(c.chan,ins);
        chan[c.chan].insChanged=false;
      }
      chan[c.chan].baseFreq=NOTE_FREQUENCY(c.value);
      chan[c.chan].note=c.value;
      chan[c.chan].freqChanged=true;
      break;
    }
    case DIV_CMD_WM_OP_WRITEMASK: {
      chan[c.chan].opWriteMask=c.value&0xff;
      break;
    }
    case DIV_CMD_WM_FILTER_MASK: {
      chan[c.chan].filterWriteMask=c.value&0xff;
      break;
    }
    case DIV_CMD_WM_LOWTEMP: {
      chan[c.chan].lowTemp=c.value&0xff;
      break;
    }
    case DIV_CMD_FM_MULT: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (op.fixed) break;
          op.pitchMul=c.value&15;
        }
        chan[c.chan].freqChanged=true;
      }
      break;
    }
    case DIV_CMD_FM_TL: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].tl=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_TL,chan[c.chan].state.op[o].tl);
        }
      }
      break;
    }
    case DIV_CMD_FM_AR: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].env.atkR=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ENVAR,chan[c.chan].state.op[o].env.atkR);
        }
      }
      break;
    }
    case DIV_CMD_FM_DR: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].env.decR=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ENVDR,chan[c.chan].state.op[o].env.decR);
        }
      }
      break;
    }
    case DIV_CMD_FM_SL: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].env.decT=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_ENVDT,chan[c.chan].state.op[o].env.decT);
        }
      }
      break;
    }
    case DIV_CMD_FM_D2R: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].env.susR=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ENVSR,chan[c.chan].state.op[o].env.susR);
        }
      }
      break;
    }
    case DIV_CMD_FM_RR: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].state.op[o].env.relR=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ENVRR,chan[c.chan].state.op[o].env.relR);
        }
      }
      break;
    }
    case DIV_CMD_FM_AM: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.alfo.enable=c.value&1;
          writeCtrlVal(c.chan,o);
        }
      }
      break;
    }
    case DIV_CMD_FM_VIB: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.flfo.enable=c.value&1;
          writeCtrlVal(c.chan,o);
        }
      }
      break;
    }
    case DIV_CMD_FM_WS: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.wavBit=c.value&0xff;
          unsigned short wGenVal=((op.intWSize&0xf)<<12)|((op.extWSize&0xf)<<8)|(op.wavBit&0xff);
          opWrite(c.chan,o,WM_ADDR_WGEN,wGenVal);
          chan[c.chan].opsState[o].waveUpdated=true;
        }
      }
      break;
    }
    case DIV_CMD_FM_SSG: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (op.env.loop==(bool)c.value) break;
          op.env.loop=c.value;
          writeCtrlVal(c.chan,o);
        }
      }
      break;
    }
    case DIV_CMD_FM_AM_DEPTH: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.alfo.rate=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ALFOL,op.alfo.rate);
        }
      }
      break;
    }
    case DIV_CMD_FM_PM_DEPTH: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.flfo.rate=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_FLFOMUL,op.flfo.rate);
        }
      }
      break;
    }
    case DIV_CMD_FM_AMS: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.alfo.mul=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_ALFOMUL,op.alfo.mul);
        }
      }
      break;
    }
    case DIV_CMD_FM_FMS: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.flfo.mul=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_ALFOMUL,op.flfo.mul);
        }
      }
      break;
    }
    // FLFO waveform
    case DIV_CMD_FM_LFO_WAVE: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.flfo.wave=(DivInstrumentWM::WMOperator::WMLfo::WMLfoWaveform)(c.value&3);
          writeCtrlVal(c.chan,o);
        }
      }
      break;
    }
    // ALFO waveform
    case DIV_CMD_FM_LFO2_WAVE: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.alfo.wave=(DivInstrumentWM::WMOperator::WMLfo::WMLfoWaveform)(c.value&3);
          writeCtrlVal(c.chan,o);
        }
      }
      break;
    }
    case DIV_CMD_FM_OPMASK: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          chan[c.chan].opMask&=~(1<<o);
          if ((c.value>>o)&1) {
            chan[c.chan].opMask|=(1<<o);
          }
        }
      }
      if (chan[c.chan].active) {
        chan[c.chan].opMaskChanged=true;
      }
      break;
    }
    case DIV_CMD_FM_EXTCH: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (op.fixed==(bool)c.value) break;
          op.fixed=c.value;
          chan[c.chan].freqChanged=true;
        }
      }
      break;
    }
    case DIV_CMD_FM_FIXFREQ: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (!op.fixed) break;
          if (!op.pitchCtrl) {
            op.fixedFreq=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
            chan[c.chan].freqChanged=true;
          }
        }
      }
      break;
    }
    case DIV_CMD_FM_FB: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.pmOut.fb=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_PMFB,op.pmOut.fb);
        }
      }
      break;
    }
    case DIV_CMD_ESFM_OP_PANNING: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (c.value&0x100) {
            op.spkrLvol=(signed short)(((c.value&0xff)<<8)|chan[c.chan].lowTemp);
            opWrite(c.chan,o,WM_ADDR_SOUTLVOL,op.spkrLvol);
          }
          if (c.value2&0x100) {
            op.spkrRvol=(signed short)(((c.value2&0xff)<<8)|chan[c.chan].lowTemp);
            opWrite(c.chan,o,WM_ADDR_SOUTRVOL,op.spkrRvol);
          }
        }
      }
      break;
    }
    case DIV_CMD_ESFM_OUTLVL: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.spkrVol=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_SOUTMVOL,op.spkrVol);
        }
      }
      break;
    }
    case DIV_CMD_ESFM_MODIN: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.pmIn.mul=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
          opWrite(c.chan,o,WM_ADDR_PMINMUL,op.pmIn.mul);
        }
      }
      break;
    }
    case DIV_CMD_ESFM_ENV_DELAY: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          op.env.delR=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
          opWrite(c.chan,o,WM_ADDR_ENVDL,op.env.delR);
        }
      }
      break;
    }
    case DIV_CMD_FM_DT: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          if (op.fixed) break;
          op.dt=(signed short)(((c.value&0xff)<<8)|(chan[c.chan].lowTemp));
        }
        chan[c.chan].freqChanged=true;
      }
      break;
    }
    case DIV_CMD_WM_FILTER_EN: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          bool writeFilt[2]={false,false};
          for (int f=0; f<8; f++) {
            if ((chan[c.chan].filterWriteMask>>f)&1) {
              DivInstrumentWM::WMOperator::WMFilter& filt=op.filter[f];
              if (filt.enable!=(bool)(c.value&1)) {
                filt.enable=c.value&1;
                writeFilt[(f>>2)&1]=true;
              }
              if (filt.lpEnable!=(bool)(c.value&2)) {
                filt.lpEnable=c.value&2;
                writeFilt[(f>>2)&1]=true;
              }
              if (filt.hpEnable!=(bool)(c.value&4)) {
                filt.hpEnable=c.value&4;
                writeFilt[(f>>2)&1]=true;
              }
              if (filt.bpEnable!=(bool)(c.value&8)) {
                filt.bpEnable=c.value&8;
                writeFilt[(f>>2)&1]=true;
              }
            }
          }
          if (writeFilt[0]) {
            unsigned short enVal=0;
            for (int j=0; j<4; j++) {
              if (op.filter[j].enable) enVal|=(8<<(j<<2));
              if (op.filter[j].lpEnable) enVal|=(4<<(j<<2));
              if (op.filter[j].hpEnable) enVal|=(2<<(j<<2));
              if (op.filter[j].bpEnable) enVal|=(1<<(j<<2));
            }
            opWrite(c.chan,o,WM_ADDR_FILTEN0,enVal);
          }
          if (writeFilt[1]) {
            unsigned short enVal=0;
            for (int j=4, sv=0; j<8; j++, sv++) {
              if (op.filter[j].enable) enVal|=(8<<(sv<<2));
              if (op.filter[j].lpEnable) enVal|=(4<<(sv<<2));
              if (op.filter[j].hpEnable) enVal|=(2<<(sv<<2));
              if (op.filter[j].bpEnable) enVal|=(1<<(sv<<2));
            }
            opWrite(c.chan,o,WM_ADDR_FILTEN1,enVal);
          }
        }
      }
      break;
    }
    case DIV_CMD_WM_FILTER_F: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          for (int f=0; f<8; f++) {
            if ((chan[c.chan].filterWriteMask>>f)&1) {
              DivInstrumentWM::WMOperator::WMFilter& filt=op.filter[f];
              filt.f=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
              opWrite(c.chan,o,WM_ADDR_FILT0F+(f<<1),filt.f);
            }
          }
        }
      }
      break;
    }
    case DIV_CMD_WM_FILTER_Q: {
      for (int o=0; o<8; o++) {
        if ((chan[c.chan].opWriteMask>>o)&1) {
          DivInstrumentWM::WMOperator& op=chan[c.chan].state.op[o];
          for (int f=0; f<8; f++) {
            if ((chan[c.chan].filterWriteMask>>f)&1) {
              DivInstrumentWM::WMOperator::WMFilter& filt=op.filter[f];
              filt.q=((c.value&0xff)<<8)|(chan[c.chan].lowTemp);
              opWrite(c.chan,o,WM_ADDR_FILT0Q+(f<<1),filt.f);
            }
          }
        }
      }
      break;
    }
    case DIV_CMD_FM_HARD_RESET:
      chan[c.chan].hardReset=c.value;
      break;
    case DIV_CMD_MACRO_OFF:
      chan[c.chan].std.mask(c.value,true);
      break;
    case DIV_CMD_MACRO_ON:
      chan[c.chan].std.mask(c.value,false);
      break;
    case DIV_CMD_MACRO_RESTART:
      chan[c.chan].std.restart(c.value);
      break;
    case DIV_CMD_GET_VOLMAX:
      return 255;
      break;
    case DIV_CMD_PRE_PORTA:
      if (!chan[c.chan].inPorta && c.value && !parent->song.brokenPortaArp && chan[c.chan].std.arp.will && !NEW_ARP_STRAT) {
        chan[c.chan].baseFreq=NOTE_FREQUENCY(chan[c.chan].note);
      }
      chan[c.chan].inPorta=c.value;
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformJKMS16WM32O8::writeOutVol(int ch) {
  int outL=0;
  int outR=0;
  if (!isMuted[ch]) {
    outL=(chan[ch].outVol*chan[ch].globalLvolOut)/32767;
    outR=(chan[ch].outVol*chan[ch].globalRvolOut)/32767;
    if (chan[ch].invertL) outL=-outL;
    if (chan[ch].invertR) outR=-outR;
  }
  chWrite(ch,WM_ADDR_LVOL,(outL&0xffff));
  chWrite(ch,WM_ADDR_RVOL,(outR&0xffff));
}

void DivPlatformJKMS16WM32O8::writeCtrlVal(int ch, int o) {
  DivInstrumentWM::WMOperator& op=chan[ch].state.op[o];
  unsigned short ctrlVal=(op.env.enable?0x8000:0)|
    (op.env.loop?0x4000:0)|(op.flfo.enable?0x2000:0)|(op.alfo.enable?0x1000:0)|
    ((op.flfo.wave&3)<<10)|((op.alfo.wave&3)<<8)|(op.filtOut?0x80:0)|(op.dirOut?0x40:0)|
    (op.fmIn.enable?0x20:0)|(op.pmIn.enable?0x10:0)|(op.amIn.enable?0x8:0)|
    (op.fmOut.enable?0x4:0)|(op.pmOut.enable?0x2:0)|(op.amOut.enable?0x1:0);
  opWrite(ch,o,WM_ADDR_CTRL,ctrlVal);
}

u16 DivPlatformJKMS16WM32O8::read_wave(u16 addr) {
  if (waveRAM!=NULL) {
    return waveRAM[addr&0xffff];
  }
  return 0;
}

void DivPlatformJKMS16WM32O8::write_wave(u16 addr, u16 wave) {
  if (waveRAM!=NULL) {
    waveRAM[addr&0xffff]=wave;
  }
}

void DivPlatformJKMS16WM32O8::forceIns() {
  for (int i=0; i<32; i++) {
    chan[i].insChanged=true;
    chan[i].freqChanged=true;
    chan[i].shallWriteVol=true;
    chan[i].opMaskChanged=true;
    for (int o=0; o<8; o++) {
      chan[i].opsState[o].waveUpdated=true;
    }
  }
}

void DivPlatformJKMS16WM32O8::toggleRegisterDump(bool enable) {
  DivDispatch::toggleRegisterDump(enable);
}

void* DivPlatformJKMS16WM32O8::getChanState(int ch) {
  return &chan[ch];
}

DivMacroInt* DivPlatformJKMS16WM32O8::getChanMacroInt(int ch) {
  return &chan[ch].std;
}

unsigned short DivPlatformJKMS16WM32O8::getPan(int ch) {
  return (((chan[ch].globalLvolOut)&0x7f80)<<1)|(((chan[ch].globalRvolOut)&0x7f80)>>7);
}

DivDispatchOscBuffer* DivPlatformJKMS16WM32O8::getOscBuffer(int ch) {
  return oscBuf[ch];
}

unsigned char* DivPlatformJKMS16WM32O8::getRegisterPool() {
  return (unsigned char*)regPool;
}

int DivPlatformJKMS16WM32O8::getRegisterPoolSize() {
  return WM_REG_POOL_SIZE;
}

int DivPlatformJKMS16WM32O8::getRegisterPoolDepth() {
  return 16;
}

void DivPlatformJKMS16WM32O8::reset() {
  chip.reset();
  for (int i=0; i<WM_REG_POOL_SIZE; i++) {
    regPool[i]=0;
  }

  for (int i=0; i<32; i++) {
    chan[i]=DivPlatformJKMS16WM32O8::Channel();
    chan[i].std.setEngine(parent);
    chan[i].vol=0xff;
    chan[i].outVol=0x7fff;
    chan[i].globalLvol=0xff;
    chan[i].globalLvolOut=0x7fff;
    chan[i].globalRvol=0xff;
    chan[i].globalRvolOut=0x7fff;
  }

  oldOut[0]=0;
  oldOut[1]=0;
  rWrite(WM_ADDR_CHSEL,0x8000); //enable sound output
  curChan=0;
  curOp=0;
}

int DivPlatformJKMS16WM32O8::getOutputCount() {
  return 2;
}

bool DivPlatformJKMS16WM32O8::keyOffAffectsArp(int ch) {
  return false;
}

bool DivPlatformJKMS16WM32O8::keyOffAffectsPorta(int ch) {
  return false;
}

bool DivPlatformJKMS16WM32O8::getLegacyAlwaysSetVolume() {
  return false;
}

void DivPlatformJKMS16WM32O8::notifyInsChange(int ins) {
  for (int i=0; i<32; i++) {
    if (chan[i].ins==ins) {
      chan[i].insChanged=true;
    }
  }
}

void DivPlatformJKMS16WM32O8::notifyInsDeletion(void* ins) {
  for (int i=0; i<32; i++) {
    chan[i].std.notifyInsDeletion((DivInstrument*)ins);
  }
}

void DivPlatformJKMS16WM32O8::poke(unsigned int addr, unsigned short val) {
  rWrite(addr,val);
}

void DivPlatformJKMS16WM32O8::poke(std::vector<DivRegWrite>& wlist) {
  for (DivRegWrite& i: wlist) rWrite(i.addr,i.val);
}

void DivPlatformJKMS16WM32O8::setFlags(const DivConfig& flags) {
  chipClock=1<<28;
  rate=(int)((double)chipClock/4096.0);
  for (int i=0; i<32; i++) {
    oscBuf[i]->setRate(rate);
  }
}

int DivPlatformJKMS16WM32O8::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  waveRAM=new unsigned short[0x10000];
  for (int i=0; i<32; i++) {
    isMuted[i]=false;
    oscBuf[i]=new DivDispatchOscBuffer;
  }
  setFlags(flags);
  reset();

  return 32;
}

void DivPlatformJKMS16WM32O8::quit() {
  delete[] waveRAM;
  for (int i=0; i<32; i++) {
    delete oscBuf[i];
  }
}

DivPlatformJKMS16WM32O8::~DivPlatformJKMS16WM32O8() {
}
