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

#ifndef _INSTRUMENT_H
#define _INSTRUMENT_H
#include "safeWriter.h"
#include "dataErrors.h"
#include "../ta-utils.h"

// NOTICE!
// before adding new instrument types to this struct, please ask me first.
// absolutely zero support granted to conflicting formats.
enum DivInstrumentType {
  DIV_INS_STD=0,
  DIV_INS_FM=1,
  DIV_INS_GB=2,
  DIV_INS_C64=3,
  DIV_INS_AMIGA=4,
  DIV_INS_PCE=5,
  DIV_INS_AY=6,
  DIV_INS_AY8930=7,
  DIV_INS_TIA=8,
  DIV_INS_SAA1099=9,
  DIV_INS_VIC=10,
  DIV_INS_PET=11,
  DIV_INS_VRC6=12,
  DIV_INS_OPLL=13,
  DIV_INS_OPL=14,
  DIV_INS_FDS=15,
  DIV_INS_VBOY=16,
  DIV_INS_N163=17,
  DIV_INS_SCC=18,
  DIV_INS_OPZ=19,
  DIV_INS_POKEY=20,
  DIV_INS_BEEPER=21,
  DIV_INS_SWAN=22,
  DIV_INS_MIKEY=23,
  DIV_INS_VERA=24,
  DIV_INS_X1_010=25,
  DIV_INS_ES5506=26,
  DIV_INS_MAX,
};

// FM operator structure:
// - OPN:
//   - AM, AR, DR, MULT, RR, SL, TL, RS, DT, D2R, SSG-EG
// - OPM:
//   - AM, AR, DR, MULT, RR, SL, TL, DT2, RS, DT, D2R
// - OPLL:
//   - AM, AR, DR, MULT, RR, SL, TL, SSG-EG&8 = EG-S
//   - KSL, VIB, KSR
// - OPL:
//   - AM, AR, DR, MULT, RR, SL, TL, SSG-EG&8 = EG-S
//   - KSL, VIB, WS (OPL2/3), KSR
// - OPZ: NOT FINAL!
//   - AM, AR, DR, MULT (CRS), RR, SL, TL, DT2, RS, DT, D2R
//   - KSL = LS, WS, DVB = MULT (FINE), DAM = REV, EGT = EGShift

struct DivInstrumentFM {
  unsigned char alg, fb, fms, ams, ops, opllPreset;
  bool fixedDrums;
  unsigned short kickFreq, snareHatFreq, tomTopFreq;
  struct Operator {
    unsigned char am, ar, dr, mult, rr, sl, tl, dt2, rs, dt, d2r, ssgEnv;
    unsigned char dam, dvb, egt, ksl, sus, vib, ws, ksr; // YMU759/OPL/OPZ
    Operator():
      am(0),
      ar(0),
      dr(0),
      mult(0),
      rr(0),
      sl(0),
      tl(0),
      dt2(0),
      rs(0),
      dt(0),
      d2r(0),
      ssgEnv(0),
      dam(0),
      dvb(0),
      egt(0),
      ksl(0),
      sus(0),
      vib(0),
      ws(0),
      ksr(0) {}
  } op[4];
  DivInstrumentFM():
    alg(0),
    fb(0),
    fms(0),
    ams(0),
    ops(2),
    opllPreset(0),
    fixedDrums(false),
    kickFreq(0x520),
    snareHatFreq(0x550),
    tomTopFreq(0x1c0) {
    // default instrument
    fb=4;
    op[0].tl=42;
    op[0].ar=31;
    op[0].dr=8;
    op[0].sl=15;
    op[0].rr=3;
    op[0].mult=5;
    op[0].dt=5;

    op[2].tl=18;
    op[2].ar=31;
    op[2].dr=10;
    op[2].sl=15;
    op[2].rr=4;
    op[2].mult=1;
    op[2].dt=0;

    op[1].tl=48;
    op[1].ar=31;
    op[1].dr=4;
    op[1].sl=11;
    op[1].rr=1;
    op[1].mult=1;
    op[1].dt=5;

    op[3].tl=2;
    op[3].ar=31;
    op[3].dr=9;
    op[3].sl=15;
    op[3].rr=9;
    op[3].mult=1;
    op[3].dt=0;
  }
};

template<typename T> struct DivMacroSTD {
  T val[256];
  unsigned char height;
  bool open;
  unsigned char len;
  signed char loop;
  signed char rel;
  DivMacroSTD(unsigned char h=~0):
    val{0},
    height(h),
    open(false),
    len(0),
    loop(-1),
    rel(-1) {}
};
struct DivInstrumentSTD {
  DivMacroSTD<int> volMacro;
  DivMacroSTD<int> arpMacro;
  DivMacroSTD<int> dutyMacro;
  DivMacroSTD<int> waveMacro;
  DivMacroSTD<int> pitchMacro;
  DivMacroSTD<int> ex1Macro;
  DivMacroSTD<int> ex2Macro;
  DivMacroSTD<int> ex3Macro;
  DivMacroSTD<int> algMacro;
  DivMacroSTD<int> fbMacro;
  DivMacroSTD<int> fmsMacro;
  DivMacroSTD<int> amsMacro;
  bool arpMacroMode;
  struct OpMacro {
    // ar, dr, mult, rr, sl, tl, dt2, rs, dt, d2r, ssgEnv;
    DivMacroSTD<unsigned char> amMacro;
    DivMacroSTD<unsigned char> arMacro;
    DivMacroSTD<unsigned char> drMacro;
    DivMacroSTD<unsigned char> multMacro;
    DivMacroSTD<unsigned char> rrMacro;
    DivMacroSTD<unsigned char> slMacro;
    DivMacroSTD<unsigned char> tlMacro;
    DivMacroSTD<unsigned char> dt2Macro;
    DivMacroSTD<unsigned char> rsMacro;
    DivMacroSTD<unsigned char> dtMacro;
    DivMacroSTD<unsigned char> d2rMacro;
    DivMacroSTD<unsigned char> ssgMacro;
    DivMacroSTD<unsigned char> damMacro;
    DivMacroSTD<unsigned char> dvbMacro;
    DivMacroSTD<unsigned char> egtMacro;
    DivMacroSTD<unsigned char> kslMacro;
    DivMacroSTD<unsigned char> susMacro;
    DivMacroSTD<unsigned char> vibMacro;
    DivMacroSTD<unsigned char> wsMacro;
    DivMacroSTD<unsigned char> ksrMacro;
    OpMacro():
      amMacro(DivMacroSTD<unsigned char>()), arMacro(DivMacroSTD<unsigned char>()),
      drMacro(DivMacroSTD<unsigned char>()), multMacro(DivMacroSTD<unsigned char>()),
      rrMacro(DivMacroSTD<unsigned char>()), slMacro(DivMacroSTD<unsigned char>()),
      tlMacro(DivMacroSTD<unsigned char>()), dt2Macro(DivMacroSTD<unsigned char>()),
      rsMacro(DivMacroSTD<unsigned char>()), dtMacro(DivMacroSTD<unsigned char>()),
      d2rMacro(DivMacroSTD<unsigned char>()), ssgMacro(DivMacroSTD<unsigned char>()),
      damMacro(DivMacroSTD<unsigned char>()), dvbMacro(DivMacroSTD<unsigned char>()),
      egtMacro(DivMacroSTD<unsigned char>()), kslMacro(DivMacroSTD<unsigned char>()),
      susMacro(DivMacroSTD<unsigned char>()), vibMacro(DivMacroSTD<unsigned char>()),
      wsMacro(DivMacroSTD<unsigned char>()), ksrMacro(DivMacroSTD<unsigned char>()) { }
  } opMacros[4];
  DivInstrumentSTD():
    volMacro(DivMacroSTD<int>(15)),
    arpMacro(DivMacroSTD<int>()),
    dutyMacro(DivMacroSTD<int>(3)),
    waveMacro(DivMacroSTD<int>(63)),
    pitchMacro(DivMacroSTD<int>()),
    ex1Macro(DivMacroSTD<int>()),
    ex2Macro(DivMacroSTD<int>()),
    ex3Macro(DivMacroSTD<int>()),
    algMacro(DivMacroSTD<int>()),
    fbMacro(DivMacroSTD<int>()),
    fmsMacro(DivMacroSTD<int>()),
    amsMacro(DivMacroSTD<int>()),
    arpMacroMode(false) { }
};

struct DivInstrumentGB {
  unsigned char envVol, envDir, envLen, soundLen;
  DivInstrumentGB():
    envVol(15),
    envDir(0),
    envLen(2),
    soundLen(64) {}
};

struct DivInstrumentC64 {
  bool triOn, sawOn, pulseOn, noiseOn;
  unsigned char a, d, s, r;
  unsigned short duty;
  unsigned char ringMod, oscSync;
  bool toFilter, volIsCutoff, initFilter, dutyIsAbs, filterIsAbs;
  unsigned char res;
  unsigned short cut;
  bool hp, lp, bp, ch3off;

  DivInstrumentC64():
    triOn(false),
    sawOn(true),
    pulseOn(false),
    noiseOn(false),
    a(0),
    d(8),
    s(0),
    r(0),
    duty(2048),
    ringMod(0),
    oscSync(0),
    toFilter(false),
    volIsCutoff(false),
    initFilter(false),
    dutyIsAbs(false),
    filterIsAbs(false),
    res(0),
    cut(0),
    hp(false),
    lp(false),
    bp(false),
    ch3off(false) {}
};

struct DivInstrumentAmiga {
  short initSample;
  bool useNoteMap;
  int noteFreq[120];
  short noteMap[120];

  DivInstrumentAmiga():
    initSample(0),
    useNoteMap(false) {
    memset(noteMap,-1,120*sizeof(short));
    memset(noteFreq,0,120*sizeof(int));
  }
};

struct DivInstrumentN163 {
  int wave, wavePos, waveLen;
  unsigned char waveMode;

  DivInstrumentN163():
    wave(-1),
    wavePos(0),
    waveLen(32),
    waveMode(3) {}
};

struct DivInstrumentES5506 {
  struct Sample {
    struct TransWave {
      int index, loopStart;
      double loopEnd;
      TransWave():
        index(-1),
        loopStart(0),
        loopEnd(16777216) {}
    };
    int init;
    bool transWaveEnable;
    std::vector<TransWave> transWaveTable;
    Sample():
      init(-1),
      transWaveEnable(false),
      transWaveTable{DivInstrumentES5506::Sample::TransWave()} {}
  };
  struct Filter {
    signed int k1, k2;
    unsigned char filterMode;
    Filter():
      k1(65535),
      k2(65535),
      filterMode(3) {}
  };
  struct Envelope {
    unsigned short envCount;
    signed short k1Ramp, k2Ramp;
    bool k1Slow, k2Slow;
    Envelope():
      envCount(0),
      k1Ramp(0),
      k2Ramp(0),
      k1Slow(false),
      k2Slow(false) {}
  };
  Sample sample;
  Filter filter;
  Envelope envelope;
  DivInstrumentES5506():
    sample(DivInstrumentES5506::Sample()),
    filter(DivInstrumentES5506::Filter()),
    envelope(DivInstrumentES5506::Envelope()) {}
};

struct DivInstrument {
  String name;
  bool mode;
  DivInstrumentType type;
  DivInstrumentFM fm;
  DivInstrumentSTD std;
  DivInstrumentGB gb;
  DivInstrumentC64 c64;
  DivInstrumentAmiga amiga;
  DivInstrumentN163 n163;
  DivInstrumentES5506 es5506;
  
  /**
   * save the instrument to a SafeWriter.
   * @param w the SafeWriter in question.
   */
  void putInsData(SafeWriter* w);

  /**
   * read instrument data in .fui format.
   * @param reader the reader.
   * @param version the format version.
   * @return a DivDataErrors.
   */
  DivDataErrors readInsData(SafeReader& reader, short version);

  /**
   * save this instrument to a file.
   * @param path file path.
   * @return whether it was successful.
   */
  bool save(const char* path);
  DivInstrument():
    name(""),
    mode(false),
    type(DIV_INS_STD) {
  }
};
#endif
