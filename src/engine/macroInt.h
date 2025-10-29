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

#ifndef _MACROINT_H
#define _MACROINT_H

#include "instrument.h"

class DivEngine;

struct DivMacroStruct {
  int pos, lastPos, lfoPos, delay;
  int val;
  bool has, had, actualHad, finished, will, linger, began, masked, activeRelease;
  unsigned int mode, type;
  unsigned short macroType;
  void doMacro(DivInstrumentMacro& source, bool released, bool tick);
  void init() {
    pos=lastPos=lfoPos=mode=type=delay=0;
    has=had=actualHad=will=false;
    linger=false;
    began=true;
    // TODO: test whether this breaks anything?
    val=0;
  }
  void prepare(DivInstrumentMacro& source, DivEngine* e);
  DivMacroStruct(unsigned short mType):
    pos(0),
    lastPos(0),
    lfoPos(0),
    delay(0),
    val(0),
    has(false),
    had(false),
    actualHad(false),
    finished(false),
    will(false),
    linger(false),
    began(true),
    masked(false),
    activeRelease(false),
    mode(0),
    type(0),
    macroType(mType) {}
};

class DivMacroInt {
  DivEngine* e;
  DivInstrument* ins;
  DivMacroStruct* macroList[4096];
  DivInstrumentMacro* macroSource[4096];
  size_t macroListLen;
  int subTick;
  bool released;
  public:
    // common macro
    DivMacroStruct vol;
    DivMacroStruct arp;
    DivMacroStruct duty, wave, pitch, ex1, ex2, ex3;
    DivMacroStruct alg, fb, fms, ams;
    DivMacroStruct panL, panR, phaseReset, ex4, ex5, ex6, ex7, ex8;
    DivMacroStruct ex9, ex10;
  
    // FM operator macro
    struct IntOp {
      DivMacroStruct am, ar, dr, mult;
      DivMacroStruct rr, sl, tl, dt2;
      DivMacroStruct rs, dt, d2r, ssg;
      DivMacroStruct dam, dvb, egt, ksl;
      DivMacroStruct sus, vib, ws, ksr;
      IntOp():
        am(DIV_MACRO_OP_AM),
        ar(DIV_MACRO_OP_AR),
        dr(DIV_MACRO_OP_DR),
        mult(DIV_MACRO_OP_MULT),
        rr(DIV_MACRO_OP_RR),
        sl(DIV_MACRO_OP_SL),
        tl(DIV_MACRO_OP_TL),
        dt2(DIV_MACRO_OP_DT2),
        rs(DIV_MACRO_OP_RS),
        dt(DIV_MACRO_OP_DT),
        d2r(DIV_MACRO_OP_D2R),
        ssg(DIV_MACRO_OP_SSG),
        dam(DIV_MACRO_OP_DAM),
        dvb(DIV_MACRO_OP_DVB),
        egt(DIV_MACRO_OP_EGT),
        ksl(DIV_MACRO_OP_KSL),
        sus(DIV_MACRO_OP_SUS),
        vib(DIV_MACRO_OP_VIB),
        ws(DIV_MACRO_OP_WS),
        ksr(DIV_MACRO_OP_KSR) {}
    } op[4];

    // WM operator macro
    struct IntWm {
      DivMacroStruct envEn;
      DivMacroStruct flfoEn;
      DivMacroStruct alfoEn;
      DivMacroStruct dt;
      DivMacroStruct mult;
      DivMacroStruct outEn;
      DivMacroStruct fmEn;
      DivMacroStruct pmEn;
      DivMacroStruct amEn;
      DivMacroStruct muteEn;
      DivMacroStruct muteBit;
      DivMacroStruct revEn;
      DivMacroStruct revBit;
      DivMacroStruct invEn;
      DivMacroStruct invBit;
      DivMacroStruct intWl;
      DivMacroStruct extWl;
      DivMacroStruct wf;
      DivMacroStruct ew;
      DivMacroStruct arp;
      DivMacroStruct pitch;
      DivMacroStruct duty;
      DivMacroStruct fmInMul;
      DivMacroStruct pmInMul;
      DivMacroStruct amInMul;
      DivMacroStruct fmOutMul;
      DivMacroStruct pmOutMul;
      DivMacroStruct amOutMul;
      DivMacroStruct fmFb;
      DivMacroStruct pmFb;
      DivMacroStruct amFb;
      DivMacroStruct fmMatrix;
      DivMacroStruct pmMatrix;
      DivMacroStruct amMatrix;
      DivMacroStruct spkrVol;
      DivMacroStruct spkrLvol;
      DivMacroStruct spkrRvol;
      DivMacroStruct tl;
      DivMacroStruct noiPitch;
      DivMacroStruct noiILfsr;
      DivMacroStruct noiMask;
      DivMacroStruct filtEn;
      DivMacroStruct filtLp;
      DivMacroStruct filtHp;
      DivMacroStruct filtBp;
      DivMacroStruct filt0F;
      DivMacroStruct filt0Q;
      DivMacroStruct filt1F;
      DivMacroStruct filt1Q;
      DivMacroStruct filt2F;
      DivMacroStruct filt2Q;
      DivMacroStruct filt3F;
      DivMacroStruct filt3Q;
      DivMacroStruct filt4F;
      DivMacroStruct filt4Q;
      DivMacroStruct filt5F;
      DivMacroStruct filt5Q;
      DivMacroStruct filt6F;
      DivMacroStruct filt6Q;
      DivMacroStruct filt7F;
      DivMacroStruct filt7Q;
      DivMacroStruct envAtkT;
      DivMacroStruct envAtkR;
      DivMacroStruct envDecT;
      DivMacroStruct envDecR;
      DivMacroStruct envSusT;
      DivMacroStruct envSusR;
      DivMacroStruct envRelR;
      DivMacroStruct envMul;
      DivMacroStruct flfoT;
      DivMacroStruct flfoL;
      DivMacroStruct flfoM;
      DivMacroStruct flfoNPitch;
      DivMacroStruct flfoNILfsr;
      DivMacroStruct flfoNMask;
      DivMacroStruct alfoT;
      DivMacroStruct alfoL;
      DivMacroStruct alfoM;
      DivMacroStruct alfoNPitch;
      DivMacroStruct alfoNILfsr;
      DivMacroStruct alfoNMask;
      IntWm():
        envEn(DIV_MACRO_WM_ENVEN),
        flfoEn(DIV_MACRO_WM_FLFOEN),
        alfoEn(DIV_MACRO_WM_ALFOEN),
        dt(DIV_MACRO_WM_DT),
        mult(DIV_MACRO_WM_MULT),
        outEn(DIV_MACRO_WM_OUTEN),
        fmEn(DIV_MACRO_WM_FMEN),
        pmEn(DIV_MACRO_WM_PMEN),
        amEn(DIV_MACRO_WM_AMEN),
        muteEn(DIV_MACRO_WM_MUTEEN),
        muteBit(DIV_MACRO_WM_MUTEBIT),
        revEn(DIV_MACRO_WM_REVEN),
        revBit(DIV_MACRO_WM_REVBIT),
        invEn(DIV_MACRO_WM_INVEN),
        invBit(DIV_MACRO_WM_INVBIT),
        intWl(DIV_MACRO_WM_INTWL),
        extWl(DIV_MACRO_WM_EXTWL),
        wf(DIV_MACRO_WM_WF),
        ew(DIV_MACRO_WM_EW),
        arp(DIV_MACRO_WM_ARP),
        pitch(DIV_MACRO_WM_PITCH),
        duty(DIV_MACRO_WM_DUTY),
        fmInMul(DIV_MACRO_WM_FMINMUL),
        pmInMul(DIV_MACRO_WM_PMINMUL),
        amInMul(DIV_MACRO_WM_AMINMUL),
        fmOutMul(DIV_MACRO_WM_FMOUTMUL),
        pmOutMul(DIV_MACRO_WM_PMOUTMUL),
        amOutMul(DIV_MACRO_WM_AMOUTMUL),
        fmFb(DIV_MACRO_WM_FMFB),
        pmFb(DIV_MACRO_WM_PMFB),
        amFb(DIV_MACRO_WM_AMFB),
        fmMatrix(DIV_MACRO_WM_FMMATRIX),
        pmMatrix(DIV_MACRO_WM_PMMATRIX),
        amMatrix(DIV_MACRO_WM_AMMATRIX),
        spkrVol(DIV_MACRO_WM_SPKRVOL),
        spkrLvol(DIV_MACRO_WM_SPKRLVOL),
        spkrRvol(DIV_MACRO_WM_SPKRRVOL),
        tl(DIV_MACRO_WM_TL),
        noiPitch(DIV_MACRO_WM_NOIPITCH),
        noiILfsr(DIV_MACRO_WM_NOIILFSR),
        noiMask(DIV_MACRO_WM_NOIMASK),
        filtEn(DIV_MACRO_WM_FILTEN),
        filtLp(DIV_MACRO_WM_FILTLP),
        filtHp(DIV_MACRO_WM_FILTHP),
        filtBp(DIV_MACRO_WM_FILTBP),
        filt0F(DIV_MACRO_WM_FILT0F),
        filt0Q(DIV_MACRO_WM_FILT0Q),
        filt1F(DIV_MACRO_WM_FILT1F),
        filt1Q(DIV_MACRO_WM_FILT1Q),
        filt2F(DIV_MACRO_WM_FILT2F),
        filt2Q(DIV_MACRO_WM_FILT2Q),
        filt3F(DIV_MACRO_WM_FILT3F),
        filt3Q(DIV_MACRO_WM_FILT3Q),
        filt4F(DIV_MACRO_WM_FILT4F),
        filt4Q(DIV_MACRO_WM_FILT4Q),
        filt5F(DIV_MACRO_WM_FILT5F),
        filt5Q(DIV_MACRO_WM_FILT5Q),
        filt6F(DIV_MACRO_WM_FILT6F),
        filt6Q(DIV_MACRO_WM_FILT6Q),
        filt7F(DIV_MACRO_WM_FILT7F),
        filt7Q(DIV_MACRO_WM_FILT7Q),
        envAtkT(DIV_MACRO_WM_ENVATKT),
        envAtkR(DIV_MACRO_WM_ENVATKR),
        envDecT(DIV_MACRO_WM_ENVDECT),
        envDecR(DIV_MACRO_WM_ENVDECR),
        envSusT(DIV_MACRO_WM_ENVSUST),
        envSusR(DIV_MACRO_WM_ENVSUSR),
        envRelR(DIV_MACRO_WM_ENVRELR),
        envMul(DIV_MACRO_WM_ENVMUL),
        flfoT(DIV_MACRO_WM_FLFOT),
        flfoL(DIV_MACRO_WM_FLFOL),
        flfoM(DIV_MACRO_WM_FLFOM),
        flfoNPitch(DIV_MACRO_WM_FLFONPITCH),
        flfoNILfsr(DIV_MACRO_WM_FLFONILFSR),
        flfoNMask(DIV_MACRO_WM_FLFONMASK),
        alfoT(DIV_MACRO_WM_ALFOT),
        alfoL(DIV_MACRO_WM_ALFOL),
        alfoM(DIV_MACRO_WM_ALFOM),
        alfoNPitch(DIV_MACRO_WM_ALFONPITCH),
        alfoNILfsr(DIV_MACRO_WM_ALFONILFSR),
        alfoNMask(DIV_MACRO_WM_ALFONMASK) {}
    } wm[8];

    // state
    bool hasRelease;

    /**
     * set mask on macro.
     */
    void mask(unsigned short id, bool enabled);

    /**
     * trigger macro release.
     */
    void release();

    /**
     * restart macro.
     */
    void restart(unsigned short id);

    /**
     * trigger next macro tick.
     */
    void next();

    /**
     * set the engine.
     * @param the engine
     */
    void setEngine(DivEngine* eng);

    /**
     * initialize the macro interpreter.
     * @param which an instrument, or NULL.
     */
    void init(DivInstrument* which);

    /**
     * notify this macro interpreter that an instrument has been deleted.
     * @param which the instrument in question.
     */
    void notifyInsDeletion(DivInstrument* which);

    /**
     * get DivMacroStruct by macro type.
     * @param which the macro type.
     * @return a DivMacroStruct, or NULL if none found.
     */
    DivMacroStruct* structByType(unsigned short which);

    DivMacroInt():
      e(NULL),
      ins(NULL),
      macroListLen(0),
      subTick(1),
      released(false),
      vol(DIV_MACRO_VOL),
      arp(DIV_MACRO_ARP),
      duty(DIV_MACRO_DUTY),
      wave(DIV_MACRO_WAVE),
      pitch(DIV_MACRO_PITCH),
      ex1(DIV_MACRO_EX1),
      ex2(DIV_MACRO_EX2),
      ex3(DIV_MACRO_EX3),
      alg(DIV_MACRO_ALG),
      fb(DIV_MACRO_FB),
      fms(DIV_MACRO_FMS),
      ams(DIV_MACRO_AMS),
      panL(DIV_MACRO_PAN_LEFT),
      panR(DIV_MACRO_PAN_RIGHT),
      phaseReset(DIV_MACRO_PHASE_RESET),
      ex4(DIV_MACRO_EX4),
      ex5(DIV_MACRO_EX5),
      ex6(DIV_MACRO_EX6),
      ex7(DIV_MACRO_EX7),
      ex8(DIV_MACRO_EX8),
      ex9(DIV_MACRO_EX9),
      ex10(DIV_MACRO_EX10),
      hasRelease(false) {
      memset(macroList,0,128*sizeof(void*));
      memset(macroSource,0,128*sizeof(void*));
    }
};

#endif
