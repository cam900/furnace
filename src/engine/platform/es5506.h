/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
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

#ifndef _ES5506_H
#define _ES5506_H

#include "../dispatch.h"
#include <queue>
#include "../macroInt.h"
#include "sound/es550x/es550x.hpp"

class DivES5506Interface: public es550x_intf {
  public:
    DivEngine* parent;
    bool irq=false;
    int sampleBank;
    virtual void irqb(bool state) { irq=state; }
    virtual s16 read_sample(u8 voice, u8 bank, u32 address) override {
      if (parent->es5506Mem==NULL) return 0;
      return parent->es5506Mem[(u32(bank)<<21)|(address & 0x1fffff)];
    }
    DivES5506Interface(): parent(NULL), sampleBank(0) {}
};

class DivPlatformES5506: public DivDispatch {
  struct Channel {
    struct Sample {
      template<typename T> struct SampleState {
        T init, curr, next;
        void reset() {
          curr=next=init;
        }
        void pop() {
          next=curr;
        }
        SampleState(T i):
          init(i), curr(i), next(i) {}
      }
      double freqOffs;
      SampleState<int> sample;
      SampleState<unsigned int> start, loop, end, bank;
      SampleState<double> sliceSize, sliceStart;
      SampleState<unsigned int> sliceLoop, sliceEnd;
      Sample():
        freqOffs(0),
        sample(-1),
        start(0),
        loop(0),
        end(0),
        bank(0),
        sliceSize(0.0),
        sliceStart(0.0),
        sliceLoop(0),
        sliceEnd(0) {}
    };
    struct TransWave {
      int index, next;
      bool enable;
      bool trigger;
      TransWave():
        index(-1),
        next(-1),
        enable(false),
        trigger(false) {}
    };
    struct Filter : public DivInstrumentES5506::Filter {
      unsigned short k1Offset, k2Offset;
      Filter():
        DivInstrumentES5506::Filter(),
        k1Offset(0),
        k2Offset(0) {}
    };
    int freq, baseFreq, pitch;
    unsigned short audLen;
    unsigned int audPos;
    int sample, wave, slice;
    unsigned char ins;
    int note;
    int panning;
    unsigned short cr;
    bool active, insChanged, freqChanged, volumeChanged, sampleChanged, transWaveChanged, filterChanged, filterRampChanged, envChanged, keyOn, keyOff, inPorta, useWave, pause, sliceEnable;
    int vol, outVol, lvol, rvol;
    Sample pcm;
    TransWave transWave;
    Filter filter;
    DivInstrumentES5506::Envelope envelope;
    DivMacroInt std;
    Channel(DivDispatch &p):
      freq(0),
      baseFreq(0),
      pitch(0),
      audLen(0),
      audPos(0),
      sample(-1),
      slice(0),
      ins(-1),
      note(0),
      panning(0xff),
      cr(0x0003),
      active(false),
      insChanged(true),
      freqChanged(false),
      volumeChanged(false),
      sampleChanged(false),
      transWaveChanged(false),
      filterChanged(false),
      filterRampChanged(false),
      envChanged(false),
      keyOn(false),
      keyOff(false),
      inPorta(false),
      useWave(false),
      pause(false),
      sliceEnable(false),
      vol(255<<8),
      outVol(255<<8),
      lvol(255<<8),
      rvol(255<<8),
      pcm(DivPlatformES5506::Channel::Sample()),
      transWave(DivPlatformES5506::Channel::TransWave()),
      filter(DivInstrumentES5506::Filter()),
      envelope(DivInstrumentES5506::Envelope()),
      std(p) {}
  };
  Channel chan[32];
  bool isMuted[32];
  struct QueuedWrite {
      unsigned char addr;
      unsigned int val;
      unsigned int mask;
      int delay;
      QueuedWrite(unsigned char a, unsigned int v, unsigned int m=~0, int d=2): addr(a), val(v), mask(m), delay(d) {} // delay for simulate 8->32 bit bottleneck
  };
  std::queue<QueuedWrite> writes;
  unsigned char lastPan;
  unsigned char initChanMax, chanMax;

  int cycles, curPage, delay;

  DivES5506Interface intf;
  es5506_core es5506;
  unsigned char regPool[4*16*128]; // 7 page bits, 16x32 bit registers per page (big endian format)
  void updateIRQ(unsigned char ch);
  friend void putDispatchChan(void*,int,int);

  public:
    void acquire(short* bufL, short* bufR, size_t start, size_t len);
    int dispatch(DivCommand c);
    void* getChanState(int chan);
    unsigned char* getRegisterPool();
    int getRegisterPoolSize();
    void reset();
    void forceIns();
    void tick();
    void muteChannel(int ch, bool mute);
    bool isStereo();
    bool keyOffAffectsArp(int ch);
    void setFlags(unsigned int flags);
    void notifyInsChange(int ins);
    void notifyWaveChange(int wave);
    void notifyInsDeletion(void* ins);
    void poke(unsigned int addr, unsigned short val);
    void poke(std::vector<DivRegWrite>& wlist);
    const char** getRegisterSheet();
    const char* getEffectName(unsigned char effect);
    int init(DivEngine* parent, int channels, int sugRate, unsigned int flags);
    void quit();
  DivPlatformES5506(DivDispatch &p):
    DivDispatch(),
    es5506(intf),
    chan{*this,*this,*this,*this,*this,*this,*this,*this,
         *this,*this,*this,*this,*this,*this,*this,*this,
         *this,*this,*this,*this,*this,*this,*this,*this,
         *this,*this,*this,*this,*this,*this,*this,*this} {} // up to 32 PCM channels
};

#endif
