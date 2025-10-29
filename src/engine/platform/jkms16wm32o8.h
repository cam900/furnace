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

#include "../dispatch.h"
#include "../../fixedQueue.h"
#include "sound/jkms16wm32o8.hpp"
using namespace jkms16wm32o8;

#define WM_REG_POOL_SIZE 0x8000

class DivPlatformJKMS16WM32O8: public DivDispatch, public jkms16wm32o8_intf_t {
  const unsigned int hardResetCycles=127;

  struct Channel: public SharedChannel<int> {
    DivInstrumentWM state;
    unsigned short freqL[8], freqH[8];
    bool hardReset;
    signed int globalLvol, globalRvol, globalLvolOut, globalRvolOut;
    bool invertL, invertR, shallWriteVol, opMaskChanged;
    unsigned char opMask, opWriteMask, filterWriteMask, lowTemp;
    struct {
      int baseNoteOverride;
      bool fixedArp;
      int arpOff;
      int pitch2;
      bool hasOpArp;
      bool hasOpPitch;
      bool waveUpdated;
    } opsState[8];

    void handleArpWmOp(int offset=0, int o=0) {
      DivMacroInt::IntWm& m=this->std.wm[o];
      if (m.arp.had) {
        opsState[o].hasOpArp=true;

        if (m.arp.val<0) {
          if (!(m.arp.val&0x40000000)) {
            opsState[o].baseNoteOverride=(m.arp.val|0x40000000)+offset;
            opsState[o].fixedArp=true;
          } else {
            opsState[o].arpOff=m.arp.val;
            opsState[o].fixedArp=false;
          }
        } else {
          if (m.arp.val&0x40000000) {
            opsState[o].baseNoteOverride=(m.arp.val&(~0x40000000))+offset;
            opsState[o].fixedArp=true;
          } else {
            opsState[o].arpOff=m.arp.val;
            opsState[o].fixedArp=false;
          }
        }
        freqChanged=true;
      } else {
        opsState[o].hasOpArp=false;
      }
    }

    void handlePitchWmOp(int o) {
      DivMacroInt::IntWm& m=this->std.wm[o];

      if (m.pitch.had) {
        opsState[o].hasOpPitch=true;

        if (m.pitch.mode) {
          opsState[o].pitch2+=m.pitch.val;
          CLAMP_VAR(opsState[o].pitch2,-65536,65535);
        } else {
          opsState[o].pitch2=m.pitch.val;
        }
        this->freqChanged=true;
      }

      else {
        opsState[o].hasOpPitch=false;
      }
    }

    Channel():
      SharedChannel<int>(32767),
      freqL{0},
      freqH{0},
      hardReset(false),
      globalLvol(32767),
      globalRvol(32767),
      globalLvolOut(32767),
      globalRvolOut(32767),
      invertL(false),
      invertR(false),
      shallWriteVol(false),
      opMaskChanged(false),
      opMask(255),
      opWriteMask(255),
      filterWriteMask(1),
      lowTemp(0) {
        memset(opsState, 0, sizeof(opsState));
      }
  };
  Channel chan[32];
  DivDispatchOscBuffer* oscBuf[32];
  bool isMuted[32];
  int curChan, curOp;
  jkms16wm32o8_t chip;
  short oldOut[2];
  unsigned short *waveRAM;

  unsigned short regPool[WM_REG_POOL_SIZE];

  int octave(int freq, int fixedBlock);
  int toFreq(int freq, int fixedBlock);
  void commitState(int ch, DivInstrument* ins);

  friend void putDispatchChip(void*,int);
  friend void putDispatchChan(void*,int,int);

  void updateWave(int ch, int op, int wave, int pos, int len);
  void updateWaveCh(int ch, int op);
  void writeOutVol(int ch);
  void writeCtrlVal(int ch, int o);
  public:
    virtual u16 read_wave(u16 addr) override;
    virtual void write_wave(u16 addr, u16 wave) override;

    virtual void acquire(short** buf, size_t len) override;
    virtual int dispatch(DivCommand c) override;
    virtual void* getChanState(int chan) override;
    virtual DivMacroInt* getChanMacroInt(int ch) override;
    virtual unsigned short getPan(int ch) override;
    virtual DivDispatchOscBuffer* getOscBuffer(int chan) override;
    virtual unsigned char* getRegisterPool() override;
    virtual int getRegisterPoolSize() override;
    virtual int getRegisterPoolDepth() override;
    virtual int getOutputCount() override;
    virtual void reset() override;
    virtual void forceIns() override;
    virtual void tick(bool sysTick=true) override;
    virtual void muteChannel(int ch, bool mute) override;
    virtual bool keyOffAffectsArp(int ch) override;
    virtual bool keyOffAffectsPorta(int ch) override;
    virtual bool hasAcquireDirect() override;
    virtual bool getLegacyAlwaysSetVolume() override;
    virtual void toggleRegisterDump(bool enable) override;
    virtual void notifyInsChange(int ins) override;
    virtual void notifyInsDeletion(void* ins) override;
    virtual void poke(unsigned int addr, unsigned short val) override;
    virtual void poke(std::vector<DivRegWrite>& wlist) override;
    virtual int getClockRangeMin() override;
    virtual int getClockRangeMax() override;
    virtual void setFlags(const DivConfig& flags) override;
    virtual int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags) override;
    virtual void quit() override;
    DivPlatformJKMS16WM32O8():
      DivDispatch(),
      jkms16wm32o8_intf_t(),
      chip(*this) {}
    ~DivPlatformJKMS16WM32O8();
};
