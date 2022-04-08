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

#include "es5506.h"
#include "../engine.h"
#include "../../ta-log.h"
#include <math.h>
#include <map>

#define CHIP_FREQBASE (16*2048)

#define rWrite(a,v) {if(!skipRegisterWrites) {writes.emplace(a,v); }}
#define rWriteMask(a,v,m) {if(!skipRegisterWrites) {writes.emplace(a,v,m); }}
#define rWriteDelay(a,v,d) {if(!skipRegisterWrites) {writes.emplace(a,v,~0,d); }}
#define rWriteMaskDelay(a,v,m,d) {if(!skipRegisterWrites) {writes.emplace(a,v,m,d); }}
#define pageWrite(p,a,v) \
  if (!skipRegisterWrites) { \
    if (curPage!=(p)) { \
      curPage=(p); \
      rWrite(0x0f,curPage); \
    } \
    rWrite(a,v); \
  }

#define pageWriteDelay(p,a,v,d) \
  if (!skipRegisterWrites) { \
    if (curPage!=(p)) { \
      curPage=(p); \
      rWrite(0x0f,curPage); \
    } \
    rWriteDelay(a,v,d); \
  }

#define pageWriteMask(p,a,v,m) \
  if (!skipRegisterWrites) { \
    if (curPage!=(p)) { \
      curPage=(p); \
      rWrite(0x0f,curPage); \
    } \
    rWriteMask(a,v,m); \
  }

#define commonPageWrite(p,pm,a,v) \
  if (!skipRegisterWrites) { \
    if ((curPage&(pm))!=((p)&(pm))) { \
      curPage=(curPage&~(pm))|((p)&(pm)); \
      rWriteMask(0x0f,curPage,pm); \
    } \
    rWrite(a,v); \
  }

#define commonPageWriteMask(p,pm,a,v,vm) \
  if (!skipRegisterWrites) { \
    if ((curPage&(pm))!=((p)&(pm))) { \
      curPage=(curPage&~(pm))|((p)&(pm)); \
      rWriteMask(0x0f,curPage,pm); \
    } \
    rWriteMask(a,v,vm); \
  }

#define pageWriteMaskDelay(p,a,v,m,d) \
  if (!skipRegisterWrites) { \
    if (curPage!=(p)) { \
      curPage=(p); \
      rWrite(0x0f,curPage); \
    } \
    rWriteMaskDelay(a,v,m,d); \
  }

#define crWriteMask(c,v,m) \
  if (!skipRegisterWrites) { \
    if ((chan[c].cr&(m))!=((v)&(m))) { \
      chan[c].cr=(chan[c].cr&~(m))|((v)&(m)); \
      commonPageWriteMask((c)&0x1f,0x5f,0x00,v,m) \
    } \
  }

#define crRefresh(c) \
  if (!skipRegisterWrites) { \
    if ((curPage&(0x5f))!=((c)&(0x1f))) { \
      unsigned int pagemask=0; \
      for (int byte=0; byte<4; byte++) { \
        pagemask|=es5506.read((0x0f<<2)+byte,true)<<(24-(byte<<3)); \
      } \
      cycles+=4; \
      for (int byte=0; byte<4; byte++) { \
        unsigned char addr=(0x0f<<2)+byte; \
        unsigned char val=((pagemask&~(0x5f))|((c)&(0x1f)))>>(24-(byte<<3)); \
        es5506.write(addr,val,true); \
        if (dumpWrites) { \
          addWrite(addr,val); \
        } \
      } \
      curPage=(curPage&~(0x5f))|((c)&(0x1f)); \
      cycles+=4; \
    } \
    chan[c].cr=0; \
    for (int byte=0; byte<4; byte++) { \
      chan[c].cr|=es5506.read((0x00<<2)+byte,true)<<(24-(byte<<3)); \
    } \
    cycles+=4; \
  }


const char* regCheatSheetES5506[]={
  // Page 00-1F
  "CR", "00|00",
  "FC", "00|01",
  "LVOL", "00|02",
  "LVRAMP", "00|03",
  "RVOL", "00|04",
  "RVRAMP", "00|05",
  "ECOUNT", "00|06",
  "K2", "00|07",
  "K2RAMP", "00|08",
  "K1", "00|09",
  "K1RAMP", "00|0A",
  "ACTV", "00|0B",
  "MODE", "00|0C",
  "POT", "00|0D",
  "IRQV", "00|0E",
  "PAGE", "00|0F",
  // Page 20-3F
  "CR", "20|00",
  "START", "20|01",
  "END", "20|02",
  "ACCUM", "20|03",
  "O4(n-1)", "20|04",
  "O3(n-2)", "20|05",
  "O3(n-1)", "20|06",
  "O2(n-2)", "20|07",
  "O2(n-1)", "20|08",
  "O1(n-1)", "20|09",
  "W_ST", "20|0A",
  "W_END", "20|0B",
  "LR_END", "20|0C",
  "POT", "20|0D",
  "IRQV", "20|0E",
  "PAGE", "20|0F",
  // Page 40~
  "CH0L", "40|00",
  "CH0R", "40|01",
  "CH1L", "40|02",
  "CH1R", "40|03",
  "CH2L", "40|04",
  "CH2R", "40|05",
  "CH3L", "40|06",
  "CH3R", "40|07",
  "CH4L", "40|08",
  "CH4R", "40|09",
  "CH5L", "40|0A",
  "CH5R", "40|0B",
  NULL
};

const char** DivPlatformES5506::getRegisterSheet() {
  return regCheatSheetES5506;
}

const char* DivPlatformES5506::getEffectName(unsigned char effect) {
  switch (effect) {
    case 0x10:
      return "10xx: Set Sample index/Transwave sequence index";
      break;
    case 0x11:
      return "11xx: Set filter 4, 3 mode (0: HP/K2 HP/K2, 1: HP/K2, LP/K1, 2: LP/K2, LP/K2, 3: LP/K2, LP/K1)";
      break;
    case 0x12:
      return "120x: Set pause/resume sample (0: Resume, 1: Pause)";
      break;
    case 0x13:
      return "130x: Set transwave slice mode (0: Enable, 1: Disable)";
      break;
    case 0x20:
      return "20xx: Set envelope counter (000 to 0FF)";
      break;
    case 0x21:
      return "21xx: Set envelope counter (100 to 1FF)";
      break;
    case 0x22:
      return "22xx: Set filter K1 envelope ramp (normal speed, signed 8 bit)";
      break;
    case 0x23:
      return "23xx: Set filter K1 envelope ramp (slowed down, signed 8 bit)";
      break;
    case 0x24:
      return "24xx: Set filter K2 envelope ramp (normal speed, signed 8 bit)";
      break;
    case 0x25:
      return "25xx: Set filter K2 envelope ramp (slowed down, signed 8 bit)";
      break;
    default:
      if ((effect&0xf0)==0x30) {
        return "3xxx: Set transwave slice position";
      } else if ((effect&0xf0)==0x40) {
        return "4xxx: Set filter K1 (bit 15 to 4)";
      } else if ((effect&0xf0)==0x50) {
        return "5xxx: Set filter K2 (bit 15 to 4)";
      }
      break;
  }
  return NULL;
}
void DivPlatformES5506::acquire(short* bufL, short* bufR, size_t start, size_t len) {
  for (size_t h=start; h<start+len; h++) {
    // Command handler
    while (!writes.empty()) {
      QueuedWrite w=writes.front();
      // convert 32 bit access to 8 bit access in chip
      for (int byte=0; byte<4; byte++) {
        unsigned char a=(w.addr<<2)+byte;
        unsigned char v=w.val>>(24-(byte<<3));
        unsigned char m=w.mask>>(24-(byte<<3));
        writes8.emplace(a,v,m,(byte==3)?w.delay:0);
      }
      writes.pop();
    }
    es5506.tick();
    if (es5506.e_rising_edge()) {
      if ((!writes8.empty())) {
        QueuedWrite8 w=writes8.front();
        if ((w.mask!=((unsigned char)(~0))) && (!w.masked)) {
          w.val=(es5506.host_r(w.addr)&~w.mask)|(w.val&w.mask);
          w.masked=true;
        } else {
          es5506.host_w(w.addr,w.val);
          if (dumpWrites) {
            addWrite(w.addr,w.val);
          }
          cycles+=w.delay;
          writes8.pop();
        }
      } else if (cycles>0) { // wait until cycles
        cycles--;
      }
    }
    bufL[h]=es5506.lout(0);
    bufR[h]=es5506.rout(0);
    // Reversed loop, Transwave uses IRQ
    if (intf.irq)
    {
      while (!intf.irq) {
        // get irqv
        unsigned int irqv=0;
        for (int byte=0; byte<4; byte++) {
          irqv|=es5506.read((0x0e<<2)+byte,true)<<(24-(byte<<3));
        }
        // end if irqv is cleared
        if (irqv&0x80) {
          break;
        }
        // get voice from IRQV register
        updateIRQ(irqv);
      }
    }
  }
}

void DivPlatformES5506::updateIRQ(unsigned char ch)
{
  crRefresh(ch);
  // Reversed Loop: DIR, IRQE, BLE, LPE flag enabled
  if ((chan[ch].cr&0x0078)==0x0078) {
    // Set DIR, clear IRQE, BLE
    crWriteMask(ch,0x0040,0x00f0);
  }
  if (chan[ch].transWave.trigger) {
    if ((chan[ch].cr&0x0024)==0x0024) { // Transwave: IRQE, LEI flag enabled
      unsigned int cr=chan[ch].cr;
      // clear IRQE, IRQ, LEI
      cr&=~0x00a4;
      if (chan[ch].pcm.index.next>=0 && chan[ch].pcm.index.next<parent->song.sampleLen) { // sample change
        chan[ch].pcm.index.curr=chan[ch].pcm.index.next;
        DivSample* s=parent->getSample(chan[ch].pcm.index.curr);
        if (chan[ch].sliceEnable) {
          if (cr&0x0040) { // Backward
            pageWrite(0x20+ch,0x01,chan[ch].pcm.sliceLoop.next); // Set loop start address
            pageWrite(0x20+ch,0x03,chan[ch].pcm.sliceEnd.next); // initial accumulator
          } else { // Foward
            pageWrite(0x20+ch,0x02,chan[ch].pcm.sliceEnd.next); // Set loop end address
            pageWrite(0x20+ch,0x03,chan[ch].pcm.sliceLoop.next); // initial accumulator
          }
        }
        if (cr&0x0040) { // Backward
          pageWrite(0x20+ch,0x01,chan[ch].pcm.loop.next); // Set loop start address
          pageWrite(0x20+ch,0x03,chan[ch].pcm.end.next); // initial accumulator
        } else { // Foward
          pageWrite(0x20+ch,0x02,chan[ch].pcm.end.next); // Set loop end address
          pageWrite(0x20+ch,0x03,chan[ch].pcm.loop.next); // initial accumulator
        }
        if (s->centerRate<1) {
          chan[ch].pcm.freqOffs=1.0;
        } else {
          chan[ch].pcm.freqOffs=8363.0/(double)s->centerRate;
        }
        chan[ch].freq=parent->calcFreq((chan[ch].pcm.freqOffs*(double)(chan[ch].baseFreq))*(chanMax+1),chan[ch].pitch,false);
        if (chan[ch].freq<0) chan[ch].freq=0;
        if (chan[ch].freq>0x1ffff) chan[ch].freq=0x1ffff;
        pageWrite(0x00+ch,0x01,chan[ch].freq);
        chan[ch].pcm.bank.pop();
        chan[ch].pcm.start.pop();
        chan[ch].pcm.loop.pop();
        chan[ch].pcm.end.pop();
        chan[ch].pcm.sliceSize.pop();
        chan[ch].pcm.sliceBound.pop();
        chan[ch].pcm.sliceLoop.pop();
        chan[ch].pcm.sliceEnd.pop();
        cr=(cr&~0xc0ff)|(chan[ch].pcm.bank.curr<<14)|((s->loopMode==DIV_SAMPLE_LOOP_PINGPONG)?0x0018:((s->loopMode==DIV_SAMPLE_LOOP_BACKWARD)?0x0038:0x0008));
        chan[ch].freqChanged=true;
        chan[ch].pcm.index.next=-1;
      } else { // just loop position changed
        if (cr&0x0040) { // Backward
          pageWrite(0x20+ch,0x01,chan[ch].pcm.loop.curr); // Set loop start address
          pageWrite(0x20+ch,0x03,chan[ch].pcm.end.curr); // initial accumulator
        } else { // Foward
          pageWrite(0x20+ch,0x02,chan[ch].pcm.end.curr); // Set loop end address
          pageWrite(0x20+ch,0x03,chan[ch].pcm.start.curr); // initial accumulator
        }
        DivSample* s=parent->getSample(chan[ch].pcm.index.curr);
        if (s->loopMode!=DIV_SAMPLE_LOOP_PINGPONG) {
          cr=(cr&~0x0078)|((s->loopMode==DIV_SAMPLE_LOOP_BACKWARD)?0x0038:0x0008);
        }
        pageWrite(0x20+ch,0x01,chan[ch].pcm.loop.curr);
      }
      crWriteMask(ch,cr,0x3c00); // update CR
    }
    chan[ch].transWave.trigger=false;
  }
}

void DivPlatformES5506::tick() {
  // ES5506 PCMs
  for (int i=0; i<=chanMax; i++) {
    chan[i].std.next();
    if (chan[i].std.vol.had) {
      chan[i].outVol=((chan[i].vol&0xff)*MIN(65535,chan[i].std.vol.val))/255;
      // Check if enabled and write volume
      if (chan[i].active && !isMuted[i]) {
        chan[i].volumeChanged=true;
      }
    }
    if (chan[i].std.panL.had) {
      chan[i].lvol=chan[i].std.panL.val;
      // Check if enabled and write volume
      if (chan[i].active && !isMuted[i]) {
        chan[i].volumeChanged=true;
      }
    }
    if (chan[i].std.panR.had) {
      chan[i].rvol=chan[i].std.panR.val;
      // Check if enabled and write volume
      if (chan[i].active && !isMuted[i]) {
        chan[i].volumeChanged=true;
      }
    }
    if (chan[i].std.arp.had) {
      if (!chan[i].inPorta) {
        if (chan[i].std.arp.mode) {
          chan[i].baseFreq=NOTE_FREQUENCY(chan[i].std.arp.val);
        } else {
          chan[i].baseFreq=NOTE_FREQUENCY(chan[i].note+chan[i].std.arp.val);
        }
      }
      chan[i].freqChanged=true;
    } else {
      if (chan[i].std.arp.mode && chan[i].std.arp.finished) {
        chan[i].baseFreq=NOTE_FREQUENCY(chan[i].note);
        chan[i].freqChanged=true;
      }
    }
    if (chan[i].std.duty.had) { // Filter mode
      if (chan[i].filter.filterMode!=chan[i].std.duty.val) {
        chan[i].filter.filterMode=chan[i].std.duty.val;
        chan[i].filterChanged=true;
      }
    }
    if (chan[i].std.wave.had) {
      if (chan[i].transWave.enable) { // Transwave Index
        if (chan[i].transWave.next!=chan[i].std.wave.val) {
          chan[i].transWave.next=chan[i].std.wave.val;
          chan[i].sliceEnable=false;
          chan[i].slice=0;
          if (chan[i].active) {
            chan[i].transWaveChanged=true;
          }
        }
      } else { // Sample index
        if (chan[i].sample!=chan[i].std.wave.val) {
          chan[i].sample=chan[i].std.wave.val;
          if (chan[i].active) {
            chan[i].sampleChanged=true;
          }
        }
      }
    }
    if (chan[i].std.ex2.had) { // K1
      if (!chan[i].std.ex2.mode) {
        if (chan[i].filter.k1Offset!=(unsigned short)chan[i].std.ex2.val) {
          chan[i].filter.k1Offset=(unsigned short)chan[i].std.ex2.val;
          chan[i].filterChanged=true;
        }
      } else {
        if (chan[i].filter.k1!=chan[i].std.ex2.val) {
          chan[i].filter.k1=chan[i].std.ex2.val;
          chan[i].filter.k1Offset=0;
          chan[i].filterChanged=true;
        }
      }
    }
    if (chan[i].std.ex3.had) { // K2
      if (!chan[i].std.ex3.mode) {
        if (chan[i].filter.k2Offset!=(unsigned short)chan[i].std.ex3.val) {
          chan[i].filter.k2Offset=(unsigned short)chan[i].std.ex3.val;
          chan[i].filterChanged=true;
        }
      } else {
        if (chan[i].filter.k2!=chan[i].std.ex3.val) {
          chan[i].filter.k2=chan[i].std.ex3.val;
          chan[i].filter.k2Offset=0;
          chan[i].filterChanged=true;
        }
      }
    }
    if (chan[i].std.alg.had) { // Extra control
      if (chan[i].pause!=(chan[i].std.alg.val&1)) { // pause
        chan[i].pause=(chan[i].std.alg.val&1);
        crWriteMask(i,chan[i].pause?0x0002:0x0000,0x0002); // Set STOP1 to pause flag
      }
      if (chan[i].sliceEnable!=(chan[i].std.alg.val&2)) {
        chan[i].sliceEnable=(chan[i].std.alg.val&2);
        chan[i].transWaveChanged=true;
      }
      if (chan[i].envelope.k1Slow!=(chan[i].std.alg.val&4)) { // K1 slow bit
        chan[i].envelope.k1Slow=(chan[i].std.alg.val&4);
        chan[i].filterRampChanged=true;
      }
      if (chan[i].envelope.k2Slow!=(chan[i].std.alg.val&8)) { // K2 slow bit
        chan[i].envelope.k2Slow=(chan[i].std.alg.val&8);
        chan[i].filterRampChanged=true;
      }
      if (chan[i].std.alg.val&16) { // Envelope force update
        chan[i].envChanged=true;
      }
    }
    if (chan[i].std.fb.had) { // Envelope count
      if (chan[i].envelope.envCount!=chan[i].std.fb.val) {
        chan[i].envelope.envCount=chan[i].std.fb.val;
        chan[i].envChanged=true;
      }
    }
    if (chan[i].std.fms.had) { // K1RAMP
      if (chan[i].envelope.k1Ramp!=chan[i].std.fms.val) {
        chan[i].envelope.k1Ramp=chan[i].std.fms.val;
        chan[i].filterRampChanged=true;
      }
    }
    if (chan[i].std.ams.had) { // K2RAMP
      if (chan[i].envelope.k2Ramp!=chan[i].std.ams.val) {
        chan[i].envelope.k2Ramp=chan[i].std.ams.val;
        chan[i].filterRampChanged=true;
      }
    }
    if (chan[i].insChanged) {
      DivInstrument* ins=parent->getIns(chan[i].ins);
      if (ins->es5506.sample.transWaveEnable) { // transwave
        chan[i].transWave.next=chan[i].sample=ins->es5506.sample.init;
      } else { // amiga format
        chan[i].sample=ins->amiga.initSample;
        if (chan[i].sample<0 || chan[i].sample>=parent->song.sampleLen) {
          chan[i].sample=-1;
        }
      }
      chan[i].sampleChanged=true;
      chan[i].insChanged=false;
    }
    if (chan[i].sampleChanged) { // Sample change routine
      crWriteMask(i,0x0003,0x00e3); // Set Stop bits
      bool vaild=false;
      bool loopEnable=false;
      if (chan[i].transWave.enable) { // transwave
        chan[i].pcm.index.curr=chan[i].transWave.next;
        if (chan[i].pcm.index.curr>=0 && chan[i].pcm.index.curr<parent->song.sampleLen) {
          DivInstrument* ins=parent->getIns(chan[i].ins);
          DivSample* s=parent->getSample(chan[i].pcm.index.curr);
          DivInstrumentES5506::Sample::TransWave& table=ins->es5506.sample.transWaveTable[chan[i].transWave.next];
          chan[i].pcm.loop.curr=((unsigned int)((double)(s->offES5506*1024.0)+(table.loopStart*2048.0)))&0xfffff800;
          chan[i].pcm.end.curr=((unsigned int)((double)(s->offES5506*1024.0)+((table.loopEnd-1)*2048.0)))&0xffffff80;
          chan[i].pcm.sliceSize.curr=table.sliceSize;
          chan[i].pcm.sliceBound.curr=table.sliceBound;
          loopEnable=true;
          vaild=true;
        }
      } else { // amiga format
        chan[i].pcm.index.curr=chan[i].sample;
        if (chan[i].pcm.index.curr>=0 && chan[i].pcm.index.curr<parent->song.sampleLen) {
          DivSample* s=parent->getSample(chan[i].pcm.index.curr);
          chan[i].pcm.loop.curr=(s->loopStart>=0)?(((s->offES5506<<10)+(s->loopStart<<11))&0xfffff800):s->loopStart;
          chan[i].pcm.end.curr=(((s->offES5506<<10)+((s->length16)<<10))-0x800)&0xffffff80;
          loopEnable=(s->loopStart>=0);
          vaild=true;
        }
      }
      if (vaild) {
        DivSample* s=parent->getSample(chan[i].pcm.index.curr);
        if (s->centerRate<1) {
          chan[i].pcm.freqOffs=1.0;
        } else {
          chan[i].pcm.freqOffs=8363.0/(double)s->centerRate;
        }
        chan[i].pcm.bank.curr=(s->offES5506>>22)&3;
        chan[i].pcm.start.curr=(s->offES5506<<10)&0xffffffff;
        pageWrite(0x00+i,0x06,0x000); // Clear ECOUNT
        crWriteMask(i,(chan[i].pcm.bank.curr<<14)|0x0300,0xc300); // Set Bank, LP3, LP4 bit to 1 (Lowpass filter mode)
        pageWrite(0x20+i,0x03,chan[i].pcm.start.curr); // Set accumulator to Start address
        pageWrite(0x00+i,0x07,0xffff); // Initialize filter (4 sample period delay needs)
        pageWriteDelay(0x00+i,0x09,0xffff,(chanMax+1)*4);
        if (loopEnable) {
          pageWrite(0x20+i,0x01,chan[i].pcm.loop.curr); // Set loop start address
          crWriteMask(i,((s->loopMode==DIV_SAMPLE_LOOP_PINGPONG)?0x0018:((s->loopMode==DIV_SAMPLE_LOOP_BACKWARD)?0x0038:0x0008)),0x3cfc); // Set loop mode
        } else {
          crWriteMask(i,0x0000,0x3cfc); // Set loop disable
        }
        pageWrite(0x20+i,0x02,chan[i].pcm.end.curr); // Set loop end address
        if (chan[i].active) {
          if (!chan[i].keyOff) {
            chan[i].keyOn=true;
          }
          chan[i].freqChanged=true;
        }
      } else {
        chan[i].pcm.freqOffs=0;
        chan[i].pcm.start.reset();
        chan[i].pcm.loop.reset();
        chan[i].pcm.end.reset();
        chan[i].pcm.sliceBound.reset();
        chan[i].pcm.sliceSize.reset();
        chan[i].pcm.sliceLoop.reset();
        chan[i].pcm.sliceEnd.reset();
      }
      chan[i].sampleChanged=false;
    }
    if (chan[i].transWaveChanged) { // transwave routine
      DivInstrument* ins=parent->getIns(chan[i].ins);
      if (chan[i].active && chan[i].transWave.enable && ((chan[i].transWave.next>=0) && (chan[i].transWave.next<ins->es5506.sample.transWaveTable.size()))) {
        if (chan[i].transWave.index!=chan[i].transWave.next) {
          chan[i].transWave.index=chan[i].transWave.next;
          crRefresh(i);
          DivInstrumentES5506::Sample::TransWave& table=ins->es5506.sample.transWaveTable[chan[i].transWave.index];
          if (chan[i].pcm.index.curr!=table.index) { // change sample index
            if (table.index>=0 && table.index<parent->song.sampleLen) {
              crWriteMask(i,0x0030,0x00bc); // Set BLE, IRQE (Set Transwave mode)
              DivSample* s=parent->getSample(table.index);
              chan[i].pcm.index.next=table.index;
              chan[i].pcm.bank.next=(s->offES5506>>22)&3;
              chan[i].pcm.start.next=(s->offES5506<<10)&0xffffffff;
              chan[i].pcm.loop.next=((unsigned int)((double)(s->offES5506*1024.0)+(table.loopStart*2048.0)))&0xfffff800;
              chan[i].pcm.end.next=((unsigned int)((double)(s->offES5506*1024.0)+((table.loopEnd-1)*2048.0)))&0xffffff80;
              chan[i].pcm.sliceSize.next=table.sliceSize;
              chan[i].pcm.sliceBound.next=table.sliceBound;
              if (chan[i].sliceEnable) { // sliced?
                chan[i].pcm.sliceLoop.next=(unsigned int)((double)(s->offES5506*1024.0)+(((double)(chan[i].slice*chan[i].pcm.sliceBound.next)*2048.0)/4095.0))&0xfffff800;
                chan[i].pcm.sliceEnd.next=(unsigned int)((double)(s->offES5506*1024.0)+((chan[i].pcm.sliceSize.next-1.0)*2048.0)+((double)((chan[i].slice*chan[i].pcm.sliceBound.next)*2048.0)/4095.0))&0xffffff80;
                if (chan[i].cr&0x0040) { // Backward
                  pageWrite(0x20+i,0x02,chan[i].pcm.sliceEnd.next); // Set loop end address
                } else { // Foward
                  pageWrite(0x20+i,0x01,chan[i].pcm.sliceLoop.next); // Set loop start address
                }
              } else {
                if (chan[i].cr&0x0040) { // Backward
                  pageWrite(0x20+i,0x02,chan[i].pcm.end.next); // Set loop end address
                } else { // Foward
                  pageWrite(0x20+i,0x01,chan[i].pcm.loop.next); // Set loop start address
                }
              }
            }
          } else { // just loop postiion changes
            DivSample* s=parent->getSample(chan[i].pcm.index.curr);
            chan[i].pcm.loop.curr=((unsigned int)((double)(s->offES5506*1024.0)+(table.loopStart*2048.0)))&0xfffff800;
            chan[i].pcm.end.curr=((unsigned int)((double)(s->offES5506*1024.0)+((table.loopEnd-1)*2048.0)))&0xffffff80;
            if (chan[i].sliceEnable) { // sliced?
              chan[i].pcm.sliceLoop.curr=(unsigned int)((double)(s->offES5506*1024.0)+(((double)(chan[i].slice*chan[i].pcm.sliceBound.curr)*2048.0)/4095.0))&0xfffff800;
              chan[i].pcm.sliceEnd.curr=(unsigned int)((double)(s->offES5506*1024.0)+((chan[i].pcm.sliceSize.curr-1)*2048.0)+((double)((chan[i].slice*chan[i].pcm.sliceBound.curr)*2048.0)/4095.0))&0xffffff80;
              pageWrite(0x20+i,0x01,chan[i].pcm.sliceLoop.curr); // Set loop start address
              pageWrite(0x20+i,0x02,chan[i].pcm.sliceEnd.curr); // Set loop end address
            } else {
              pageWrite(0x20+i,0x01,chan[i].pcm.loop.curr); // Set loop start address
              pageWrite(0x20+i,0x02,chan[i].pcm.end.curr); // Set loop end address
            }
          }
        }
        chan[i].transWave.trigger=true;
      }
      chan[i].transWaveChanged=false;
    }
    if (chan[i].filterChanged) {
      crWriteMask(i,(chan[i].filter.filterMode&3)<<8,0x0300); // set filter mode LP3, LP4
      pageWrite(0x00+i,0x07,MAX(0,MIN(65535,(chan[i].filter.k1)+(chan[i].filter.k1Offset)))&0xffff);
      pageWrite(0x00+i,0x09,MAX(0,MIN(65535,(chan[i].filter.k2)+(chan[i].filter.k2Offset)))&0xffff);
      chan[i].filterChanged=false;
    }
    if (chan[i].volumeChanged) {
      if (!isMuted[i]) {
        chan[i].outLVol=MAX(0,MIN(65535,((chan[i].outVol*(chan[i].lvol*((chan[i].panning>>4)&0xf)))/0xfff)/0xf));
        chan[i].outRVol=MAX(0,MIN(65535,((chan[i].outVol*(chan[i].rvol*((chan[i].panning>>0)&0xf)))/0xfff)/0xf));
      } else {
        chan[i].outLVol=0;
        chan[i].outRVol=0;
      }
      pageWrite(0x00+i,0x02,chan[i].outLVol);
      pageWrite(0x00+i,0x04,chan[i].outRVol);
      chan[i].volumeChanged=false;
    }
    if (chan[i].filterRampChanged) {
      pageWrite(0x00+i,0x08,(((unsigned short)(chan[i].envelope.k1Ramp&0xff))<<8)|(chan[i].envelope.k1Slow?1:0));
      pageWrite(0x00+i,0x0a,(((unsigned short)(chan[i].envelope.k2Ramp&0xff))<<8)|(chan[i].envelope.k2Slow?1:0));
      chan[i].filterRampChanged=false;
    }
    if (chan[i].envChanged) {
      pageWrite(0x00+i,0x06,chan[i].envelope.envCount&0x1ff);
      chan[i].envChanged=false;
    }
    if (chan[i].freqChanged || chan[i].keyOn || chan[i].keyOff) {
      chan[i].freq=parent->calcFreq((chan[i].pcm.freqOffs*(double)(chan[i].baseFreq))*(chanMax+1),chan[i].pitch,false);
      if (chan[i].freq<0) chan[i].freq=0;
      if (chan[i].freq>0x1ffff) chan[i].freq=0x1ffff;
      if (chan[i].keyOn) {
        crWriteMask(i,0x0000,0x0003); // clear Stop bit
      }
      if (chan[i].keyOff) {
        crWriteMask(i,0x0003,0x00e3); // set Stop bit
      } else if (chan[i].active) {
        pageWrite(0x00+i,0x01,chan[i].freq);
      }
      if (chan[i].keyOn) chan[i].keyOn=false;
      if (chan[i].keyOff) chan[i].keyOff=false;
      chan[i].freqChanged=false;
    }
  }
}

int DivPlatformES5506::dispatch(DivCommand c) {
  if (c.chan>chanMax) {
    return 1;
  }
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[c.chan].ins);
      if (ins->es5506.sample.transWaveEnable) { // transwave
        chan[c.chan].transWave.next=chan[c.chan].sample=ins->es5506.sample.init;
        chan[c.chan].sliceEnable=ins->es5506.sample.sliceEnable;
        chan[c.chan].slice=ins->es5506.sample.sliceInit;
      } else { // amiga format
        chan[c.chan].sample=ins->amiga.initSample;
        if (chan[c.chan].sample<0 || chan[c.chan].sample>=parent->song.sampleLen) {
          chan[c.chan].sample=-1;
        }
      }
      // filter
      chan[c.chan].filter.k1=ins->es5506.filter.k1;
      chan[c.chan].filter.k2=ins->es5506.filter.k2;
      chan[c.chan].filter.filterMode=ins->es5506.filter.filterMode;
      chan[c.chan].filter.k1Offset=(chan[c.chan].std.ex2.will && (!chan[c.chan].std.ex2.mode))?(unsigned short)(chan[c.chan].std.ex2.val):0;
      chan[c.chan].filter.k2Offset=(chan[c.chan].std.ex3.will && (!chan[c.chan].std.ex3.mode))?(unsigned short)(chan[c.chan].std.ex3.val):0;
      // envelope
      chan[c.chan].envelope=ins->es5506.envelope;
      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].baseFreq=NOTE_FREQUENCY(c.value);
        chan[c.chan].freqChanged=true;
        chan[c.chan].note=c.value;
      }
      chan[c.chan].active=true;
      chan[c.chan].keyOn=true;
      chan[c.chan].sampleChanged=true;
      chan[c.chan].transWaveChanged=true;
      chan[c.chan].filterChanged=true;
      chan[c.chan].filterRampChanged=true;
      chan[c.chan].envChanged=true;
      if (!chan[c.chan].std.vol.will) {
        chan[c.chan].outVol=chan[c.chan].vol;
      }
      if (!isMuted[c.chan]) {
        chan[c.chan].volumeChanged=true;
      }
      chan[c.chan].pause=false;
      chan[c.chan].std.init(ins);
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].sample=-1;
      chan[c.chan].active=false;
      chan[c.chan].keyOff=true;
      chan[c.chan].std.init(NULL);
      break;
    case DIV_CMD_NOTE_OFF_ENV:
    case DIV_CMD_ENV_RELEASE:
      chan[c.chan].std.release();
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[c.chan].ins!=c.value || c.value2==1) {
        chan[c.chan].ins=c.value;
        chan[c.chan].insChanged=true;
      }
      break;
    case DIV_CMD_VOLUME:
      if (chan[c.chan].vol!=c.value) {
        chan[c.chan].vol=c.value;
        if (!chan[c.chan].std.vol.has) {
          // Check if enabled and write volume
          chan[c.chan].outVol=c.value;
          if (chan[c.chan].active && !isMuted[c.chan]) {
            chan[c.chan].volumeChanged=true;
          }
        }
      }
      break;
    case DIV_CMD_GET_VOLUME:
      if (chan[c.chan].std.vol.has) {
        return chan[c.chan].vol;
      }
      return chan[c.chan].outVol;
      break;
    // Sample commands
    case DIV_CMD_WAVE:
      if (chan[c.chan].pcm.index.curr>=0 && chan[c.chan].pcm.index.curr<parent->song.sampleLen) {
        chan[c.chan].sliceEnable=false;
        chan[c.chan].slice=0;
        if (chan[c.chan].transWave.enable) { // transwave
          chan[c.chan].transWave.next=c.value;
          chan[c.chan].transWaveChanged=true;
        } else { // amiga format
          chan[c.chan].sample=c.value;
          chan[c.chan].sampleChanged=true;
        } // waveform not implemented
      }
      break;
    case DIV_CMD_SAMPLE_POS: {
      if (chan[c.chan].pcm.index.curr>=0 && chan[c.chan].pcm.index.curr<parent->song.sampleLen) {
        unsigned int pos=(c.value<<11); // sample position
        if (chan[c.chan].transWave.enable) { // transwave
          if (chan[c.chan].sliceEnable) { // slice mode?
            pageWrite(0x20+c.chan,0x03,MAX(chan[c.chan].pcm.sliceLoop.curr,MIN(chan[c.chan].pcm.sliceEnd.curr,chan[c.chan].pcm.sliceLoop.curr+pos)));
          } else {
            pageWrite(0x20+c.chan,0x03,MAX(chan[c.chan].pcm.loop.curr,MIN(chan[c.chan].pcm.end.curr,chan[c.chan].pcm.loop.curr+pos)));
          }
        } else { // amiga format
          pageWrite(0x20+c.chan,0x03,MAX(chan[c.chan].pcm.start.curr,MIN(chan[c.chan].pcm.end.curr,chan[c.chan].pcm.start.curr+pos)));
        }
      }
      break;
    }
    case DIV_CMD_ES5506_SLICE: // toggle slice
      if (chan[c.chan].pcm.index.curr>=0 && chan[c.chan].pcm.index.curr<parent->song.sampleLen) {
        if (chan[c.chan].sliceEnable!=(c.value&1)) {
          chan[c.chan].sliceEnable=c.value&1;
          chan[c.chan].transWaveChanged=true;
        }
      }
      break;
    case DIV_CMD_ES5506_SLICE_POS: // adjust slice position
      if (chan[c.chan].pcm.index.curr>=0 && chan[c.chan].pcm.index.curr<parent->song.sampleLen) {
        chan[c.chan].slice=c.value2;
        chan[c.chan].transWaveChanged=true;
      }
      break;
    case DIV_CMD_ES5506_PAUSE: // toggle pause
      if (chan[c.chan].pcm.index.curr>=0 && chan[c.chan].pcm.index.curr<parent->song.sampleLen) {
        if (chan[c.chan].pause!=(c.value&1)) {
          chan[c.chan].pause=c.value&1;
          crWriteMask(c.chan,chan[c.chan].pause?0x0002:0x0000,0x0002); // Set STOP1 to pause flag
        }
      }
      break;
    // Filter commands
    case DIV_CMD_ES5506_K1:
      chan[c.chan].filter.k1=(chan[c.chan].filter.k1&0xf)|((c.value&0xfff)<<4); // only bit 4-15 is changeable via command
      chan[c.chan].filter.k1Offset=0; // reset offset
      chan[c.chan].filterChanged=true;
      break;
    case DIV_CMD_ES5506_K2:
      chan[c.chan].filter.k2=(chan[c.chan].filter.k2&0xf)|((c.value&0xfff)<<4); // only bit 4-15 is changeable via command
      chan[c.chan].filter.k2Offset=0; // reset offset
      chan[c.chan].filterChanged=true;
      break;
    case DIV_CMD_ES5506_FILTER_MODE:
      chan[c.chan].filter.filterMode=c.value&3;
      chan[c.chan].filterChanged=true;
      break;
    // Envelope commands
    case DIV_CMD_ES5506_K1_RAMP:
      chan[c.chan].envelope.k1Slow=c.value2&1;
      chan[c.chan].envelope.k1Ramp=(signed char)(c.value&0xff);
      chan[c.chan].filterRampChanged=true;
      break;
    case DIV_CMD_ES5506_K2_RAMP:
      chan[c.chan].envelope.k2Slow=c.value2&1;
      chan[c.chan].envelope.k2Ramp=(signed char)(c.value&0xff);
      chan[c.chan].filterRampChanged=true;
      break;
    case DIV_CMD_ES5506_ENVELOPE_COUNT:
      chan[c.chan].envelope.envCount=c.value&0x1ff;
      chan[c.chan].envChanged=true;
      break;
    case DIV_CMD_PANNING:
      chan[c.chan].panning=c.value;
      if (!isMuted[c.chan]) {
        chan[c.chan].volumeChanged=true;
      }
      break;
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=NOTE_FREQUENCY(c.value2);
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        chan[c.chan].baseFreq+=c.value;
        if (chan[c.chan].baseFreq>=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[c.chan].baseFreq-=c.value;
        if (chan[c.chan].baseFreq<=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[c.chan].freqChanged=true;
      if (return2) {
        chan[c.chan].inPorta=false;
        return 2;
      }
      break;
    }
    case DIV_CMD_LEGATO: {
      chan[c.chan].baseFreq=NOTE_FREQUENCY(c.value+((chan[c.chan].std.arp.will && !chan[c.chan].std.arp.mode)?(chan[c.chan].std.arp.val-12):(0)));
      chan[c.chan].freqChanged=true;
      chan[c.chan].note=c.value;
      break;
    }
    case DIV_CMD_PRE_PORTA:
      if (chan[c.chan].active && c.value2) {
        if (parent->song.resetMacroOnPorta) chan[c.chan].std.init(parent->getIns(chan[c.chan].ins));
      }
      chan[c.chan].inPorta=c.value;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 255;//4095
      break;
    case DIV_ALWAYS_SET_VOLUME:
      return 1;
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformES5506::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
  if (ch<=chanMax) {
    chan[ch].volumeChanged=true;
  }
}

void DivPlatformES5506::forceIns() {
  for (int i=0; i<=chanMax; i++) {
    chan[i].insChanged=true;
    chan[i].freqChanged=true;
    chan[i].volumeChanged=true;
    chan[i].sampleChanged=true;
    chan[i].transWaveChanged=true;
    chan[i].filterChanged=true;
    chan[i].filterRampChanged=true;
    chan[i].envChanged=true;
    chan[i].pcm.index.reset();
  }
}

void* DivPlatformES5506::getChanState(int ch) {
  return &chan[ch];
}

void DivPlatformES5506::reset() {
  while (!writes.empty()) writes.pop();
  for (int i=0; i<32; i++) {
    chan[i]=DivPlatformES5506::Channel();
  }
  es5506.reset();
  commonPageWrite(0x20,0x60,0x0a,0x01); // W_ST
  commonPageWrite(0x20,0x60,0x0b,0x11); // W_END
  commonPageWrite(0x20,0x60,0x0c,0x20); // LR_END
  commonPageWrite(0x00,0x00,0x0b,chanMax); // Set max channel number
  commonPageWrite(0x00,0x00,0x0c,0x08); // Enable serial output, Single/Master mode
}

bool DivPlatformES5506::isStereo() {
  return true;
}

bool DivPlatformES5506::keyOffAffectsArp(int ch) {
  return true;
}

void DivPlatformES5506::notifyInsChange(int ins) {
  for (int i=0; i<=chanMax; i++) {
    if (chan[i].ins==ins) {
      chan[i].insChanged=true;
    }
  }
}

void DivPlatformES5506::notifyWaveChange(int wave) {
  // TODO when wavetables are added
  // TODO they probably won't be added unless the samples reside in RAM
}

void DivPlatformES5506::notifyInsDeletion(void* ins) {
  for (int i=0; i<32; i++) {
    chan[i].std.notifyInsDeletion((DivInstrument*)ins);
  }
}

void DivPlatformES5506::setFlags(unsigned int flags) {
  chipClock=16000000; //mostly used clock
  rate=chipClock*2; // clock pulse, 4 BCLK per voice, 4 clock per BCLK (16 clock per voice)
  initChanMax=MAX(4,(flags>>4)&0x1f);
  if (chanMax!=initChanMax) {
    chanMax=initChanMax;
    commonPageWrite(0x00,0x60,0x0b,chanMax);
    forceIns();
  }
}

void DivPlatformES5506::poke(unsigned int addr, unsigned short val) {
  rWrite(addr, val);
}

void DivPlatformES5506::poke(std::vector<DivRegWrite>& wlist) {
  for (DivRegWrite& i: wlist) rWrite(i.addr,i.val);
}

unsigned char* DivPlatformES5506::getRegisterPool() {
  for (int page=0; page<128; page++) {
    for (int reg=0; reg<16; reg++) {
      for (int byte=0; byte<4; byte++) {
        regPool[byte+(reg<<2)+(page<<6)]=es5506.regs_r(page,reg,false); // register is 32 bit, host access is 8 bit
      }
    }
  }
  return regPool;
}

int DivPlatformES5506::getRegisterPoolSize() {
  return 4*16*128;
}

int DivPlatformES5506::init(DivEngine* p, int channels, int sugRate, unsigned int flags) {
  parent=p;
  intf.parent=parent;
  dumpWrites=false;
  skipRegisterWrites=false;

  for (int i=0; i<32; i++) {
    isMuted[i]=false;
  }
  setFlags(flags);
  reset();
  return 32;
}

void DivPlatformES5506::quit() {
}
