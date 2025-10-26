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

#include "macroInt.h"
#include "instrument.h"
#include "engine.h"
#include "../ta-log.h"

#define ADSR_LOW source.val[0]
#define ADSR_HIGH source.val[1]
#define ADSR_AR source.val[2]
#define ADSR_HT source.val[3]
#define ADSR_DR source.val[4]
#define ADSR_SL source.val[5]
#define ADSR_ST source.val[6]
#define ADSR_SR source.val[7]
#define ADSR_RR source.val[8]

#define LFO_SPEED source.val[11]
#define LFO_WAVE source.val[12]
#define LFO_PHASE source.val[13]
#define LFO_LOOP source.val[14]
#define LFO_GLOBAL source.val[15]

void DivMacroStruct::prepare(DivInstrumentMacro& source, DivEngine* e) {
  has=had=actualHad=will=true;
  mode=source.mode;
  type=(source.open>>1)&3;
  activeRelease=source.open&8;
  linger=(source.macroType==DIV_MACRO_VOL && e->song.volMacroLinger);
  lfoPos=LFO_PHASE;
}

void DivMacroStruct::doMacro(DivInstrumentMacro& source, bool released, bool tick) {
  if (!tick) {
    had=false;
    return;
  }
  if (masked) {
    had=false;
    has=false;
    return;
  }
  if (released && type==1 && lastPos<3) delay=0;
  if (released && type==0 && pos<source.rel && source.rel<source.len && activeRelease) {
    delay=0;
    pos=source.rel;
  }
  if (delay>0) {
    delay--;
    if (!linger) had=false;
    return;
  }
  if (began && source.delay>0) {
    delay=source.delay;
  } else {
    delay=source.speed-1;
  }
  if (began) {
    began=false;
  }
  if (finished) {
    finished=false;
  }
  if (actualHad!=has) {
    finished=true;
  }
  actualHad=has;
  had=actualHad;

  if (has) {
    if (type==0) { // sequence
      lastPos=pos;
      val=source.val[pos++];
      if (pos>source.rel && !released) {
        if (source.loop<source.len && source.loop<source.rel) {
          pos=source.loop;
        } else {
          pos--;
        }
      }
      if (pos>=source.len) {
        if (source.loop<source.len && (source.loop>=source.rel || source.rel>=source.len)) {
          pos=source.loop;
        } else if (linger) {
          pos--;
        } else {
          has=false;
        }
      }
    }
    if (type==1) { // ADSR
      if (released && lastPos<3) lastPos=3;
      switch (lastPos) {
        case 0: // attack
          pos+=ADSR_AR;
          if (pos>255) {
            pos=255;
            lastPos=1;
            delay=ADSR_HT;
          }
          break;
        case 1: // decay
          pos-=ADSR_DR;
          if (pos<=ADSR_SL) {
            pos=ADSR_SL;
            lastPos=2;
            delay=ADSR_ST;
          }
          break;
        case 2: // sustain
          pos-=ADSR_SR;
          if (pos<0) {
            pos=0;
            lastPos=4;
          }
          break;
        case 3: // release
          pos-=ADSR_RR;
          if (pos<0) {
            pos=0;
            lastPos=4;
          }
          break;
        case 4: // end
          pos=0;
          if (!linger) has=false;
          break;
      }
      if (ADSR_HIGH>ADSR_LOW) {
        val=ADSR_LOW+((pos+(ADSR_HIGH-ADSR_LOW)*pos)>>8);
      } else {
        val=ADSR_HIGH+(((255-pos)+(ADSR_LOW-ADSR_HIGH)*(255-pos))>>8);
      }
    }
    if (type==2) { // LFO
      lfoPos+=LFO_SPEED;
      lfoPos&=1023;

      int lfoOut=0;
      switch (LFO_WAVE&3) {
        case 0: // triangle
          lfoOut=((lfoPos&512)?(1023-lfoPos):(lfoPos))>>1;
          break;
        case 1: // saw
          lfoOut=lfoPos>>2;
          break;
        case 2: // pulse
          lfoOut=(lfoPos&512)?255:0;
          break;
      }
      if (ADSR_HIGH>ADSR_LOW) {
        val=ADSR_LOW+((lfoOut+(ADSR_HIGH-ADSR_LOW)*lfoOut)>>8);
      } else {
        val=ADSR_HIGH+(((255-lfoOut)+(ADSR_LOW-ADSR_HIGH)*(255-lfoOut))>>8);
      }
    }
  }
}

void DivMacroInt::next() {
  if (ins==NULL) return;
  // run macros
  // TODO: potentially get rid of list to avoid allocations
  subTick--;
  for (size_t i=0; i<macroListLen; i++) {
    if (macroList[i]!=NULL && macroSource[i]!=NULL) {
      macroList[i]->doMacro(*macroSource[i],released,subTick==0);
    }
  }
  if (subTick<=0) {
    if (e==NULL) {
      subTick=1;
    } else {
      subTick=e->tickMult;
    }
  }
}

#define CONSIDER(x,y) \
  case y: \
    x.masked=enabled; \
    break;

#define CONSIDER_OP(oi,o) \
  CONSIDER(op[oi].am,0+o) \
  CONSIDER(op[oi].ar,1+o) \
  CONSIDER(op[oi].dr,2+o) \
  CONSIDER(op[oi].mult,3+o) \
  CONSIDER(op[oi].rr,4+o) \
  CONSIDER(op[oi].sl,5+o) \
  CONSIDER(op[oi].tl,6+o) \
  CONSIDER(op[oi].dt2,7+o) \
  CONSIDER(op[oi].rs,8+o) \
  CONSIDER(op[oi].dt,9+o) \
  CONSIDER(op[oi].d2r,10+o) \
  CONSIDER(op[oi].ssg,11+o) \
  CONSIDER(op[oi].dam,12+o) \
  CONSIDER(op[oi].dvb,13+o) \
  CONSIDER(op[oi].egt,14+o) \
  CONSIDER(op[oi].ksl,15+o) \
  CONSIDER(op[oi].sus,16+o) \
  CONSIDER(op[oi].vib,17+o) \
  CONSIDER(op[oi].ws,18+o) \
  CONSIDER(op[oi].ksr,19+o)

#define CONSIDER_WM(oi,o) \
  CONSIDER(wm[oi].envEn,0+o) \
  CONSIDER(wm[oi].flfoEn,1+o) \
  CONSIDER(wm[oi].alfoEn,2+o) \
  CONSIDER(wm[oi].dt,3+o) \
  CONSIDER(wm[oi].mult,4+o) \
  CONSIDER(wm[oi].outEn,5+o) \
  CONSIDER(wm[oi].fmEn,6+o) \
  CONSIDER(wm[oi].pmEn,7+o) \
  CONSIDER(wm[oi].amEn,8+o) \
  CONSIDER(wm[oi].muteEn,9+o) \
  CONSIDER(wm[oi].muteBit,10+o) \
  CONSIDER(wm[oi].revEn,11+o) \
  CONSIDER(wm[oi].revBit,12+o) \
  CONSIDER(wm[oi].invEn,13+o) \
  CONSIDER(wm[oi].invBit,14+o) \
  CONSIDER(wm[oi].intWl,15+o) \
  CONSIDER(wm[oi].extWl,16+o) \
  CONSIDER(wm[oi].wf,17+o) \
  CONSIDER(wm[oi].ew,18+o) \
  CONSIDER(wm[oi].arp,19+o) \
  CONSIDER(wm[oi].pitch,20+o) \
  CONSIDER(wm[oi].duty,21+o) \
  CONSIDER(wm[oi].fmInMul,22+o) \
  CONSIDER(wm[oi].pmInMul,23+o) \
  CONSIDER(wm[oi].amInMul,24+o) \
  CONSIDER(wm[oi].fmOutMul,25+o) \
  CONSIDER(wm[oi].pmOutMul,26+o) \
  CONSIDER(wm[oi].amOutMul,27+o) \
  CONSIDER(wm[oi].fmFb,28+o) \
  CONSIDER(wm[oi].pmFb,29+o) \
  CONSIDER(wm[oi].amFb,30+o) \
  CONSIDER(wm[oi].fmMatrix,31+o) \
  CONSIDER(wm[oi].pmMatrix,32+o) \
  CONSIDER(wm[oi].amMatrix,33+o) \
  CONSIDER(wm[oi].spkrVol,34+o) \
  CONSIDER(wm[oi].spkrLVol,35+o) \
  CONSIDER(wm[oi].spkrRVol,36+o) \
  CONSIDER(wm[oi].tl,37+o) \
  CONSIDER(wm[oi].noiPitch,38+o) \
  CONSIDER(wm[oi].noiILfsr,39+o) \
  CONSIDER(wm[oi].noiMask,40+o) \
  CONSIDER(wm[oi].filtEn,41+o) \
  CONSIDER(wm[oi].filtLp,42+o) \
  CONSIDER(wm[oi].filtHp,43+o) \
  CONSIDER(wm[oi].filtBp,44+o) \
  CONSIDER(wm[oi].filt0F,45+o) \
  CONSIDER(wm[oi].filt0Q,46+o) \
  CONSIDER(wm[oi].filt1F,47+o) \
  CONSIDER(wm[oi].filt1Q,48+o) \
  CONSIDER(wm[oi].filt2F,49+o) \
  CONSIDER(wm[oi].filt2Q,50+o) \
  CONSIDER(wm[oi].filt3F,51+o) \
  CONSIDER(wm[oi].filt3Q,52+o) \
  CONSIDER(wm[oi].filt4F,53+o) \
  CONSIDER(wm[oi].filt4Q,54+o) \
  CONSIDER(wm[oi].filt5F,55+o) \
  CONSIDER(wm[oi].filt5Q,56+o) \
  CONSIDER(wm[oi].filt6F,57+o) \
  CONSIDER(wm[oi].filt6Q,58+o) \
  CONSIDER(wm[oi].filt7F,59+o) \
  CONSIDER(wm[oi].filt7Q,60+o) \
  CONSIDER(wm[oi].envAtkT,61+o) \
  CONSIDER(wm[oi].envAtkR,62+o) \
  CONSIDER(wm[oi].envDecT,63+o) \
  CONSIDER(wm[oi].envDecR,64+o) \
  CONSIDER(wm[oi].envSusT,65+o) \
  CONSIDER(wm[oi].envSusR,66+o) \
  CONSIDER(wm[oi].envRelR,67+o) \
  CONSIDER(wm[oi].envMul,68+o) \
  CONSIDER(wm[oi].flfoT,69+o) \
  CONSIDER(wm[oi].flfoL,70+o) \
  CONSIDER(wm[oi].flfoM,71+o) \
  CONSIDER(wm[oi].flfoNPitch,72+o) \
  CONSIDER(wm[oi].flfoNILfsr,73+o) \
  CONSIDER(wm[oi].flfoNMask,74+o) \
  CONSIDER(wm[oi].alfoT,75+o) \
  CONSIDER(wm[oi].alfoL,76+o) \
  CONSIDER(wm[oi].alfoM,77+o) \
  CONSIDER(wm[oi].alfoNPitch,78+o) \
  CONSIDER(wm[oi].alfoNILfsr,79+o) \
  CONSIDER(wm[oi].alfoNMask,80+o)

void DivMacroInt::mask(unsigned short id, bool enabled) {
  switch (id) {
    CONSIDER(vol,0)
    CONSIDER(arp,1)
    CONSIDER(duty,2)
    CONSIDER(wave,3)
    CONSIDER(pitch,4)
    CONSIDER(ex1,5)
    CONSIDER(ex2,6)
    CONSIDER(ex3,7)
    CONSIDER(alg,8)
    CONSIDER(fb,9)
    CONSIDER(fms,10)
    CONSIDER(ams,11)
    CONSIDER(panL,12)
    CONSIDER(panR,13)
    CONSIDER(phaseReset,14)
    CONSIDER(ex4,15)
    CONSIDER(ex5,16)
    CONSIDER(ex6,17)
    CONSIDER(ex7,18)
    CONSIDER(ex8,19)
    CONSIDER(ex9,20)
    CONSIDER(ex10,21)

    CONSIDER_OP(0,0x20)
    CONSIDER_OP(2,0x40)
    CONSIDER_OP(1,0x60)
    CONSIDER_OP(3,0x80)

    CONSIDER_WM(0,0x400)
    CONSIDER_WM(1,0x480)
    CONSIDER_WM(2,0x500)
    CONSIDER_WM(3,0x580)
    CONSIDER_WM(4,0x600)
    CONSIDER_WM(5,0x680)
    CONSIDER_WM(6,0x700)
    CONSIDER_WM(7,0x780)
  }
}

#undef CONSIDER_WM
#undef CONSIDER_OP
#undef CONSIDER

#define CONSIDER(x,y,z) \
  case z: \
    macroState=&x; \
    macro=&ins->std.y; \
    break;

#define CONSIDER_OP(oi,o) \
  CONSIDER(op[oi].am,opMacros[oi].amMacro,0+o) \
  CONSIDER(op[oi].ar,opMacros[oi].arMacro,1+o) \
  CONSIDER(op[oi].dr,opMacros[oi].drMacro,2+o) \
  CONSIDER(op[oi].mult,opMacros[oi].multMacro,3+o) \
  CONSIDER(op[oi].rr,opMacros[oi].rrMacro,4+o) \
  CONSIDER(op[oi].sl,opMacros[oi].slMacro,5+o) \
  CONSIDER(op[oi].tl,opMacros[oi].tlMacro,6+o) \
  CONSIDER(op[oi].dt2,opMacros[oi].dt2Macro,7+o) \
  CONSIDER(op[oi].rs,opMacros[oi].rsMacro,8+o) \
  CONSIDER(op[oi].dt,opMacros[oi].dtMacro,9+o) \
  CONSIDER(op[oi].d2r,opMacros[oi].d2rMacro,10+o) \
  CONSIDER(op[oi].ssg,opMacros[oi].ssgMacro,11+o) \
  CONSIDER(op[oi].dam,opMacros[oi].damMacro,12+o) \
  CONSIDER(op[oi].dvb,opMacros[oi].dvbMacro,13+o) \
  CONSIDER(op[oi].egt,opMacros[oi].egtMacro,14+o) \
  CONSIDER(op[oi].ksl,opMacros[oi].kslMacro,15+o) \
  CONSIDER(op[oi].sus,opMacros[oi].susMacro,16+o) \
  CONSIDER(op[oi].vib,opMacros[oi].vibMacro,17+o) \
  CONSIDER(op[oi].ws,opMacros[oi].wsMacro,18+o) \
  CONSIDER(op[oi].ksr,opMacros[oi].ksrMacro,19+o)

#define CONSIDER_WM(oi,o) \
  CONSIDER(wm[oi].envEn,wmMacros[oi].envEnMacro,0+o) \
  CONSIDER(wm[oi].flfoEn,wmMacros[oi].flfoEnMacro,1+o) \
  CONSIDER(wm[oi].alfoEn,wmMacros[oi].alfoEnMacro,2+o) \
  CONSIDER(wm[oi].dt,wmMacros[oi].dtMacro,3+o) \
  CONSIDER(wm[oi].mult,wmMacros[oi].multMacro,4+o) \
  CONSIDER(wm[oi].outEn,wmMacros[oi].outEnMacro,5+o) \
  CONSIDER(wm[oi].fmEn,wmMacros[oi].fmEnMacro,6+o) \
  CONSIDER(wm[oi].pmEn,wmMacros[oi].pmEnMacro,7+o) \
  CONSIDER(wm[oi].amEn,wmMacros[oi].amEnMacro,8+o) \
  CONSIDER(wm[oi].muteEn,wmMacros[oi].muteEnMacro,9+o) \
  CONSIDER(wm[oi].muteBit,wmMacros[oi].muteBitMacro,10+o) \
  CONSIDER(wm[oi].revEn,wmMacros[oi].revEnMacro,11+o) \
  CONSIDER(wm[oi].revBit,wmMacros[oi].revBitMacro,12+o) \
  CONSIDER(wm[oi].invEn,wmMacros[oi].invEnMacro,13+o) \
  CONSIDER(wm[oi].invBit,wmMacros[oi].invBitMacro,14+o) \
  CONSIDER(wm[oi].intWl,wmMacros[oi].intWlMacro,15+o) \
  CONSIDER(wm[oi].extWl,wmMacros[oi].extWlMacro,16+o) \
  CONSIDER(wm[oi].wf,wmMacros[oi].wfMacro,17+o) \
  CONSIDER(wm[oi].ew,wmMacros[oi].ewMacro,18+o) \
  CONSIDER(wm[oi].arp,wmMacros[oi].arpMacro,19+o) \
  CONSIDER(wm[oi].pitch,wmMacros[oi].pitchMacro,20+o) \
  CONSIDER(wm[oi].duty,wmMacros[oi].dutyMacro,21+o) \
  CONSIDER(wm[oi].fmInMul,wmMacros[oi].fmInMulMacro,22+o) \
  CONSIDER(wm[oi].pmInMul,wmMacros[oi].pmInMulMacro,23+o) \
  CONSIDER(wm[oi].amInMul,wmMacros[oi].amInMulMacro,24+o) \
  CONSIDER(wm[oi].fmOutMul,wmMacros[oi].fmOutMulMacro,25+o) \
  CONSIDER(wm[oi].pmOutMul,wmMacros[oi].pmOutMulMacro,26+o) \
  CONSIDER(wm[oi].amOutMul,wmMacros[oi].amOutMulMacro,27+o) \
  CONSIDER(wm[oi].fmFb,wmMacros[oi].fmFbMacro,28+o) \
  CONSIDER(wm[oi].pmFb,wmMacros[oi].pmFbMacro,29+o) \
  CONSIDER(wm[oi].amFb,wmMacros[oi].amFbMacro,30+o) \
  CONSIDER(wm[oi].fmMatrix,wmMacros[oi].fmMatrixMacro,31+o) \
  CONSIDER(wm[oi].pmMatrix,wmMacros[oi].pmMatrixMacro,32+o) \
  CONSIDER(wm[oi].amMatrix,wmMacros[oi].amMatrixMacro,33+o) \
  CONSIDER(wm[oi].spkrVol,wmMacros[oi].spkrVolMacro,34+o) \
  CONSIDER(wm[oi].spkrLVol,wmMacros[oi].spkrLVolMacro,35+o) \
  CONSIDER(wm[oi].spkrRVol,wmMacros[oi].spkrRVolMacro,36+o) \
  CONSIDER(wm[oi].tl,wmMacros[oi].tlMacro,37+o) \
  CONSIDER(wm[oi].noiPitch,wmMacros[oi].noiPitchMacro,38+o) \
  CONSIDER(wm[oi].noiILfsr,wmMacros[oi].noiILfsrMacro,39+o) \
  CONSIDER(wm[oi].noiMask,wmMacros[oi].noiMaskMacro,40+o) \
  CONSIDER(wm[oi].filtEn,wmMacros[oi].filtEnMacro,41+o) \
  CONSIDER(wm[oi].filtLp,wmMacros[oi].filtLpMacro,42+o) \
  CONSIDER(wm[oi].filtHp,wmMacros[oi].filtHpMacro,43+o) \
  CONSIDER(wm[oi].filtBp,wmMacros[oi].filtBpMacro,44+o) \
  CONSIDER(wm[oi].filt0F,wmMacros[oi].filt0FMacro,45+o) \
  CONSIDER(wm[oi].filt0Q,wmMacros[oi].filt0QMacro,46+o) \
  CONSIDER(wm[oi].filt1F,wmMacros[oi].filt1FMacro,47+o) \
  CONSIDER(wm[oi].filt1Q,wmMacros[oi].filt1QMacro,48+o) \
  CONSIDER(wm[oi].filt2F,wmMacros[oi].filt2FMacro,49+o) \
  CONSIDER(wm[oi].filt2Q,wmMacros[oi].filt2QMacro,50+o) \
  CONSIDER(wm[oi].filt3F,wmMacros[oi].filt3FMacro,51+o) \
  CONSIDER(wm[oi].filt3Q,wmMacros[oi].filt3QMacro,52+o) \
  CONSIDER(wm[oi].filt4F,wmMacros[oi].filt4FMacro,53+o) \
  CONSIDER(wm[oi].filt4Q,wmMacros[oi].filt4QMacro,54+o) \
  CONSIDER(wm[oi].filt5F,wmMacros[oi].filt5FMacro,55+o) \
  CONSIDER(wm[oi].filt5Q,wmMacros[oi].filt5QMacro,56+o) \
  CONSIDER(wm[oi].filt6F,wmMacros[oi].filt6FMacro,57+o) \
  CONSIDER(wm[oi].filt6Q,wmMacros[oi].filt6QMacro,58+o) \
  CONSIDER(wm[oi].filt7F,wmMacros[oi].filt7FMacro,59+o) \
  CONSIDER(wm[oi].filt7Q,wmMacros[oi].filt7QMacro,60+o) \
  CONSIDER(wm[oi].envAtkT,wmMacros[oi].envAtkTMacro,61+o) \
  CONSIDER(wm[oi].envAtkR,wmMacros[oi].envAtkRMacro,62+o) \
  CONSIDER(wm[oi].envDecT,wmMacros[oi].envDecTMacro,63+o) \
  CONSIDER(wm[oi].envDecR,wmMacros[oi].envDecRMacro,64+o) \
  CONSIDER(wm[oi].envSusT,wmMacros[oi].envSusTMacro,65+o) \
  CONSIDER(wm[oi].envSusR,wmMacros[oi].envSusRMacro,66+o) \
  CONSIDER(wm[oi].envRelR,wmMacros[oi].envRelRMacro,67+o) \
  CONSIDER(wm[oi].envMul,wmMacros[oi].envMulMacro,68+o) \
  CONSIDER(wm[oi].flfoT,wmMacros[oi].flfoTMacro,69+o) \
  CONSIDER(wm[oi].flfoL,wmMacros[oi].flfoLMacro,70+o) \
  CONSIDER(wm[oi].flfoM,wmMacros[oi].flfoMMacro,71+o) \
  CONSIDER(wm[oi].flfoNPitch,wmMacros[oi].flfoNPitchMacro,72+o) \
  CONSIDER(wm[oi].flfoNILfsr,wmMacros[oi].flfoNILfsrMacro,73+o) \
  CONSIDER(wm[oi].flfoNMask,wmMacros[oi].flfoNMaskMacro,74+o) \
  CONSIDER(wm[oi].alfoT,wmMacros[oi].alfoTMacro,75+o) \
  CONSIDER(wm[oi].alfoL,wmMacros[oi].alfoLMacro,76+o) \
  CONSIDER(wm[oi].alfoM,wmMacros[oi].alfoMMacro,77+o) \
  CONSIDER(wm[oi].alfoNPitch,wmMacros[oi].alfoNPitchMacro,78+o) \
  CONSIDER(wm[oi].alfoNILfsr,wmMacros[oi].alfoNILfsrMacro,79+o) \
  CONSIDER(wm[oi].alfoNMask,wmMacros[oi].alfoNMaskMacro,80+o)

void DivMacroInt::restart(unsigned short id) {
  DivMacroStruct* macroState=NULL;
  DivInstrumentMacro* macro=NULL;

  if (e==NULL) return;
  if (ins==NULL) return;
  
  switch (id) {
    CONSIDER(vol,volMacro,0)
    CONSIDER(arp,arpMacro,1)
    CONSIDER(duty,dutyMacro,2)
    CONSIDER(wave,waveMacro,3)
    CONSIDER(pitch,pitchMacro,4)
    CONSIDER(ex1,ex1Macro,5)
    CONSIDER(ex2,ex2Macro,6)
    CONSIDER(ex3,ex3Macro,7)
    CONSIDER(alg,algMacro,8)
    CONSIDER(fb,fbMacro,9)
    CONSIDER(fms,fmsMacro,10)
    CONSIDER(ams,amsMacro,11)
    CONSIDER(panL,panLMacro,12)
    CONSIDER(panR,panRMacro,13)
    CONSIDER(phaseReset,phaseResetMacro,14)
    CONSIDER(ex4,ex4Macro,15)
    CONSIDER(ex5,ex5Macro,16)
    CONSIDER(ex6,ex6Macro,17)
    CONSIDER(ex7,ex7Macro,18)
    CONSIDER(ex8,ex8Macro,19)
    CONSIDER(ex9,ex9Macro,20)
    CONSIDER(ex10,ex10Macro,21)

    CONSIDER_OP(0,0x20)
    CONSIDER_OP(2,0x40)
    CONSIDER_OP(1,0x60)
    CONSIDER_OP(3,0x80)

    CONSIDER_WM(0,0x400)
    CONSIDER_WM(1,0x480)
    CONSIDER_WM(2,0x500)
    CONSIDER_WM(3,0x580)
    CONSIDER_WM(4,0x600)
    CONSIDER_WM(5,0x680)
    CONSIDER_WM(6,0x700)
    CONSIDER_WM(7,0x780)
  }

  if (macroState==NULL || macro==NULL) return;

  if (macro->len<=0) return;
  if (macroState->masked) return;

  macroState->init();
  macroState->prepare(*macro,e);
}

#undef CONSIDER_WM
#undef CONSIDER_OP
#undef CONSIDER

void DivMacroInt::release() {
  released=true;
}

void DivMacroInt::setEngine(DivEngine* eng) {
  e=eng;
}

#define ADD_MACRO(m,s) \
  if (!m.masked) { \
    macroList[macroListLen]=&m; \
    macroSource[macroListLen++]=&s; \
  }

void DivMacroInt::init(DivInstrument* which) {
  ins=which;
  // initialize
  for (size_t i=0; i<macroListLen; i++) {
    if (macroList[i]!=NULL) macroList[i]->init();
  }
  macroListLen=0;
  subTick=1;

  hasRelease=false;
  released=false;

  if (ins==NULL) return;

  // prepare common macro
  if (ins->std.volMacro.len>0) {
    ADD_MACRO(vol,ins->std.volMacro);
  }
  if (ins->std.arpMacro.len>0) {
    ADD_MACRO(arp,ins->std.arpMacro);
  }
  if (ins->std.dutyMacro.len>0) {
    ADD_MACRO(duty,ins->std.dutyMacro);
  }
  if (ins->std.waveMacro.len>0) {
    ADD_MACRO(wave,ins->std.waveMacro);
  }
  if (ins->std.pitchMacro.len>0) {
    ADD_MACRO(pitch,ins->std.pitchMacro);
  }
  if (ins->std.ex1Macro.len>0) {
    ADD_MACRO(ex1,ins->std.ex1Macro);
  }
  if (ins->std.ex2Macro.len>0) {
    ADD_MACRO(ex2,ins->std.ex2Macro);
  }
  if (ins->std.ex3Macro.len>0) {
    ADD_MACRO(ex3,ins->std.ex3Macro);
  }
  if (ins->std.algMacro.len>0) {
    ADD_MACRO(alg,ins->std.algMacro);
  }
  if (ins->std.fbMacro.len>0) {
    ADD_MACRO(fb,ins->std.fbMacro);
  }
  if (ins->std.fmsMacro.len>0) {
    ADD_MACRO(fms,ins->std.fmsMacro);
  }
  if (ins->std.amsMacro.len>0) {
    ADD_MACRO(ams,ins->std.amsMacro);
  }

  if (ins->std.panLMacro.len>0) {
    ADD_MACRO(panL,ins->std.panLMacro);
  }
  if (ins->std.panRMacro.len>0) {
    ADD_MACRO(panR,ins->std.panRMacro);
  }
  if (ins->std.phaseResetMacro.len>0) {
    ADD_MACRO(phaseReset,ins->std.phaseResetMacro);
  }
  if (ins->std.ex4Macro.len>0) {
    ADD_MACRO(ex4,ins->std.ex4Macro);
  }
  if (ins->std.ex5Macro.len>0) {
    ADD_MACRO(ex5,ins->std.ex5Macro);
  }
  if (ins->std.ex6Macro.len>0) {
    ADD_MACRO(ex6,ins->std.ex6Macro);
  }
  if (ins->std.ex7Macro.len>0) {
    ADD_MACRO(ex7,ins->std.ex7Macro);
  }
  if (ins->std.ex8Macro.len>0) {
    ADD_MACRO(ex8,ins->std.ex8Macro);
  }
  if (ins->std.ex9Macro.len>0) {
    ADD_MACRO(ex9,ins->std.ex9Macro);
  }
  if (ins->std.ex10Macro.len>0) {
    ADD_MACRO(ex10,ins->std.ex10Macro);
  }

  // prepare FM operator macros
  for (int i=0; i<4; i++) {
    DivInstrumentSTD::OpMacro& m=ins->std.opMacros[i];
    IntOp& o=op[i];
    if (m.amMacro.len>0) {
      ADD_MACRO(o.am,m.amMacro);
    }
    if (m.arMacro.len>0) {
      ADD_MACRO(o.ar,m.arMacro);
    }
    if (m.drMacro.len>0) {
      ADD_MACRO(o.dr,m.drMacro);
    }
    if (m.multMacro.len>0) {
      ADD_MACRO(o.mult,m.multMacro);
    }
    if (m.rrMacro.len>0) {
      ADD_MACRO(o.rr,m.rrMacro);
    }
    if (m.slMacro.len>0) {
      ADD_MACRO(o.sl,m.slMacro);
    }
    if (m.tlMacro.len>0) {
      ADD_MACRO(o.tl,m.tlMacro);
    }
    if (m.dt2Macro.len>0) {
      ADD_MACRO(o.dt2,m.dt2Macro);
    }
    if (m.rsMacro.len>0) {
      ADD_MACRO(o.rs,m.rsMacro);
    }
    if (m.dtMacro.len>0) {
      ADD_MACRO(o.dt,m.dtMacro);
    }
    if (m.d2rMacro.len>0) {
      ADD_MACRO(o.d2r,m.d2rMacro);
    }
    if (m.ssgMacro.len>0) {
      ADD_MACRO(o.ssg,m.ssgMacro);
    }

    if (m.damMacro.len>0) {
      ADD_MACRO(o.dam,m.damMacro);
    }
    if (m.dvbMacro.len>0) {
      ADD_MACRO(o.dvb,m.dvbMacro);
    }
    if (m.egtMacro.len>0) {
      ADD_MACRO(o.egt,m.egtMacro);
    }
    if (m.kslMacro.len>0) {
      ADD_MACRO(o.ksl,m.kslMacro);
    }
    if (m.susMacro.len>0) {
      ADD_MACRO(o.sus,m.susMacro);
    }
    if (m.vibMacro.len>0) {
      ADD_MACRO(o.vib,m.vibMacro);
    }
    if (m.wsMacro.len>0) {
      ADD_MACRO(o.ws,m.wsMacro);
    }
    if (m.ksrMacro.len>0) {
      ADD_MACRO(o.ksr,m.ksrMacro);
    }
  }

  // prepare WM operator macros
  for (int i=0; i<8; i++) {
    DivInstrumentSTD::WmMacro& m=ins->std.wmMacros[i];
    IntWm& o=wm[i];
    if (m.envEnMacro.len>0) {
      ADD_MACRO(o.envEn,m.envEnMacro);
    }
    if (m.flfoEnMacro.len>0) {
      ADD_MACRO(o.flfoEn,m.flfoEnMacro);
    }
    if (m.alfoEnMacro.len>0) {
      ADD_MACRO(o.alfoEn,m.alfoEnMacro);
    }
    if (m.dtMacro.len>0) {
      ADD_MACRO(o.dt,m.dtMacro);
    }
    if (m.multMacro.len>0) {
      ADD_MACRO(o.mult,m.multMacro);
    }
    if (m.outEnMacro.len>0) {
      ADD_MACRO(o.outEn,m.outEnMacro);
    }
    if (m.fmEnMacro.len>0) {
      ADD_MACRO(o.fmEn,m.fmEnMacro);
    }
    if (m.pmEnMacro.len>0) {
      ADD_MACRO(o.pmEn,m.pmEnMacro);
    }
    if (m.amEnMacro.len>0) {
      ADD_MACRO(o.amEn,m.amEnMacro);
    }
    if (m.muteEnMacro.len>0) {
      ADD_MACRO(o.muteEn,m.muteEnMacro);
    }
    if (m.muteBitMacro.len>0) {
      ADD_MACRO(o.muteBit,m.muteBitMacro);
    }
    if (m.revEnMacro.len>0) {
      ADD_MACRO(o.revEn,m.revEnMacro);
    }
    if (m.revBitMacro.len>0) {
      ADD_MACRO(o.revBit,m.revBitMacro);
    }
    if (m.invEnMacro.len>0) {
      ADD_MACRO(o.invEn,m.invEnMacro);
    }
    if (m.invBitMacro.len>0) {
      ADD_MACRO(o.invBit,m.invBitMacro);
    }
    if (m.intWlMacro.len>0) {
      ADD_MACRO(o.intWl,m.intWlMacro);
    }
    if (m.extWlMacro.len>0) {
      ADD_MACRO(o.extWl,m.extWlMacro);
    }
    if (m.wfMacro.len>0) {
      ADD_MACRO(o.wf,m.wfMacro);
    }
    if (m.ewMacro.len>0) {
      ADD_MACRO(o.ew,m.ewMacro);
    }
    if (m.arpMacro.len>0) {
      ADD_MACRO(o.arp,m.arpMacro);
    }
    if (m.pitchMacro.len>0) {
      ADD_MACRO(o.pitch,m.pitchMacro);
    }
    if (m.dutyMacro.len>0) {
      ADD_MACRO(o.duty,m.dutyMacro);
    }
    if (m.fmInMulMacro.len>0) {
      ADD_MACRO(o.fmInMul,m.fmInMulMacro);
    }
    if (m.pmInMulMacro.len>0) {
      ADD_MACRO(o.pmInMul,m.pmInMulMacro);
    }
    if (m.amInMulMacro.len>0) {
      ADD_MACRO(o.amInMul,m.amInMulMacro);
    }
    if (m.fmOutMulMacro.len>0) {
      ADD_MACRO(o.fmOutMul,m.fmOutMulMacro);
    }
    if (m.pmOutMulMacro.len>0) {
      ADD_MACRO(o.pmOutMul,m.pmOutMulMacro);
    }
    if (m.amOutMulMacro.len>0) {
      ADD_MACRO(o.amOutMul,m.amOutMulMacro);
    }
    if (m.fmMatrixMacro.len>0) {
      ADD_MACRO(o.fmMatrix,m.fmMatrixMacro);
    }
    if (m.pmMatrixMacro.len>0) {
      ADD_MACRO(o.pmMatrix,m.pmMatrixMacro);
    }
    if (m.amMatrixMacro.len>0) {
      ADD_MACRO(o.amMatrix,m.amMatrixMacro);
    }
    if (m.fmFbMacro.len>0) {
      ADD_MACRO(o.fmFb,m.fmFbMacro);
    }
    if (m.pmFbMacro.len>0) {
      ADD_MACRO(o.pmFb,m.pmFbMacro);
    }
    if (m.amFbMacro.len>0) {
      ADD_MACRO(o.amFb,m.amFbMacro);
    }
    if (m.spkrVolMacro.len>0) {
      ADD_MACRO(o.spkrVol,m.spkrVolMacro);
    }
    if (m.spkrLVolMacro.len>0) {
      ADD_MACRO(o.spkrLVol,m.spkrLVolMacro);
    }
    if (m.spkrRVolMacro.len>0) {
      ADD_MACRO(o.spkrRVol,m.spkrRVolMacro);
    }
    if (m.tlMacro.len>0) {
      ADD_MACRO(o.tl,m.tlMacro);
    }
    if (m.noiPitchMacro.len>0) {
      ADD_MACRO(o.noiPitch,m.noiPitchMacro);
    }
    if (m.noiILfsrMacro.len>0) {
      ADD_MACRO(o.noiILfsr,m.noiILfsrMacro);
    }
    if (m.noiMaskMacro.len>0) {
      ADD_MACRO(o.noiMask,m.noiMaskMacro);
    }
    if (m.filtEnMacro.len>0) {
      ADD_MACRO(o.filtEn,m.filtEnMacro);
    }
    if (m.filtLpMacro.len>0) {
      ADD_MACRO(o.filtLp,m.filtLpMacro);
    }
    if (m.filtHpMacro.len>0) {
      ADD_MACRO(o.filtHp,m.filtHpMacro);
    }
    if (m.filtBpMacro.len>0) {
      ADD_MACRO(o.filtBp,m.filtBpMacro);
    }
    if (m.filt0FMacro.len>0) {
      ADD_MACRO(o.filt0F,m.filt0FMacro);
    }
    if (m.filt0QMacro.len>0) {
      ADD_MACRO(o.filt0Q,m.filt0QMacro);
    }
    if (m.filt1FMacro.len>0) {
      ADD_MACRO(o.filt1F,m.filt1FMacro);
    }
    if (m.filt1QMacro.len>0) {
      ADD_MACRO(o.filt1Q,m.filt1QMacro);
    }
    if (m.filt2FMacro.len>0) {
      ADD_MACRO(o.filt2F,m.filt2FMacro);
    }
    if (m.filt2QMacro.len>0) {
      ADD_MACRO(o.filt2Q,m.filt2QMacro);
    }
    if (m.filt3FMacro.len>0) {
      ADD_MACRO(o.filt3F,m.filt3FMacro);
    }
    if (m.filt3QMacro.len>0) {
      ADD_MACRO(o.filt3Q,m.filt3QMacro);
    }
    if (m.filt4FMacro.len>0) {
      ADD_MACRO(o.filt4F,m.filt4FMacro);
    }
    if (m.filt4QMacro.len>0) {
      ADD_MACRO(o.filt4Q,m.filt4QMacro);
    }
    if (m.filt5FMacro.len>0) {
      ADD_MACRO(o.filt5F,m.filt5FMacro);
    }
    if (m.filt5QMacro.len>0) {
      ADD_MACRO(o.filt5Q,m.filt5QMacro);
    }
    if (m.filt6FMacro.len>0) {
      ADD_MACRO(o.filt6F,m.filt6FMacro);
    }
    if (m.filt6QMacro.len>0) {
      ADD_MACRO(o.filt6Q,m.filt6QMacro);
    }
    if (m.filt7FMacro.len>0) {
      ADD_MACRO(o.filt7F,m.filt7FMacro);
    }
    if (m.filt7QMacro.len>0) {
      ADD_MACRO(o.filt7Q,m.filt7QMacro);
    }
    if (m.envAtkTMacro.len>0) {
      ADD_MACRO(o.envAtkT,m.envAtkTMacro);
    }
    if (m.envAtkRMacro.len>0) {
      ADD_MACRO(o.envAtkR,m.envAtkRMacro);
    }
    if (m.envDecTMacro.len>0) {
      ADD_MACRO(o.envDecT,m.envDecTMacro);
    }
    if (m.envDecRMacro.len>0) {
      ADD_MACRO(o.envDecR,m.envDecRMacro);
    }
    if (m.envSusTMacro.len>0) {
      ADD_MACRO(o.envSusT,m.envSusTMacro);
    }
    if (m.envSusRMacro.len>0) {
      ADD_MACRO(o.envSusR,m.envSusRMacro);
    }
    if (m.envRelRMacro.len>0) {
      ADD_MACRO(o.envRelR,m.envRelRMacro);
    }
    if (m.envMulMacro.len>0) {
      ADD_MACRO(o.envMul,m.envMulMacro);
    }
    if (m.flfoTMacro.len>0) {
      ADD_MACRO(o.flfoT,m.flfoTMacro);
    }
    if (m.flfoLMacro.len>0) {
      ADD_MACRO(o.flfoL,m.flfoLMacro);
    }
    if (m.flfoMMacro.len>0) {
      ADD_MACRO(o.flfoM,m.flfoMMacro);
    }
    if (m.flfoNPitchMacro.len>0) {
      ADD_MACRO(o.flfoNPitch,m.flfoNPitchMacro);
    }
    if (m.flfoNILfsrMacro.len>0) {
      ADD_MACRO(o.flfoNILfsr,m.flfoNILfsrMacro);
    }
    if (m.flfoNMaskMacro.len>0) {
      ADD_MACRO(o.flfoNMask,m.flfoNMaskMacro);
    }
    if (m.alfoTMacro.len>0) {
      ADD_MACRO(o.alfoT,m.alfoTMacro);
    }
    if (m.alfoLMacro.len>0) {
      ADD_MACRO(o.alfoL,m.alfoLMacro);
    }
    if (m.alfoMMacro.len>0) {
      ADD_MACRO(o.alfoM,m.alfoMMacro);
    }
    if (m.alfoNPitchMacro.len>0) {
      ADD_MACRO(o.alfoNPitch,m.alfoNPitchMacro);
    }
    if (m.alfoNILfsrMacro.len>0) {
      ADD_MACRO(o.alfoNILfsr,m.alfoNILfsrMacro);
    }
    if (m.alfoNMaskMacro.len>0) {
      ADD_MACRO(o.alfoNMask,m.alfoNMaskMacro);
    }
  }

  for (size_t i=0; i<macroListLen; i++) {
    if (macroSource[i]!=NULL) {
      macroList[i]->prepare(*macroSource[i],e);
      // check ADSR mode
      if ((macroSource[i]->open&6)==2) {
        if (macroSource[i]->val[8]>0) {
          hasRelease=true;
        }
      } else if (macroSource[i]->rel<macroSource[i]->len) {
        hasRelease=true;
      }
    }
  }
}

void DivMacroInt::notifyInsDeletion(DivInstrument* which) {
  if (ins==which) {
    init(NULL);
  }
}

#define CONSIDERS(x,y) case (y&0x7f): return &x; break;
#define CONSIDER(x,y) case (y&0x1f): return &x; break;

DivMacroStruct* DivMacroInt::structByType(unsigned short type) {
  if (type>=0x400 && type<0x800) {
    unsigned char o=((type>>7))&7;
    switch (type&0x7f) {
      CONSIDERS(wm[o].envEn,DIV_MACRO_WM_ENVEN)
      CONSIDERS(wm[o].flfoEn,DIV_MACRO_WM_FLFOEN)
      CONSIDERS(wm[o].alfoEn,DIV_MACRO_WM_ALFOEN)
      CONSIDERS(wm[o].dt,DIV_MACRO_WM_DT)
      CONSIDERS(wm[o].mult,DIV_MACRO_WM_MULT)
      CONSIDERS(wm[o].outEn,DIV_MACRO_WM_OUTEN)
      CONSIDERS(wm[o].fmEn,DIV_MACRO_WM_FMEN)
      CONSIDERS(wm[o].pmEn,DIV_MACRO_WM_PMEN)
      CONSIDERS(wm[o].amEn,DIV_MACRO_WM_AMEN)
      CONSIDERS(wm[o].muteEn,DIV_MACRO_WM_MUTEEN)
      CONSIDERS(wm[o].muteBit,DIV_MACRO_WM_MUTEBIT)
      CONSIDERS(wm[o].revEn,DIV_MACRO_WM_REVEN)
      CONSIDERS(wm[o].revBit,DIV_MACRO_WM_REVBIT)
      CONSIDERS(wm[o].invEn,DIV_MACRO_WM_INVEN)
      CONSIDERS(wm[o].invBit,DIV_MACRO_WM_INVBIT)
      CONSIDERS(wm[o].intWl,DIV_MACRO_WM_INTWL)
      CONSIDERS(wm[o].extWl,DIV_MACRO_WM_EXTWL)
      CONSIDERS(wm[o].wf,DIV_MACRO_WM_WF)
      CONSIDERS(wm[o].ew,DIV_MACRO_WM_EW)
      CONSIDERS(wm[o].arp,DIV_MACRO_WM_ARP)
      CONSIDERS(wm[o].pitch,DIV_MACRO_WM_PITCH)
      CONSIDERS(wm[o].duty,DIV_MACRO_WM_DUTY)
      CONSIDERS(wm[o].fmInMul,DIV_MACRO_WM_FMINMUL)
      CONSIDERS(wm[o].pmInMul,DIV_MACRO_WM_PMINMUL)
      CONSIDERS(wm[o].amInMul,DIV_MACRO_WM_AMINMUL)
      CONSIDERS(wm[o].fmOutMul,DIV_MACRO_WM_FMOUTMUL)
      CONSIDERS(wm[o].pmOutMul,DIV_MACRO_WM_PMOUTMUL)
      CONSIDERS(wm[o].amOutMul,DIV_MACRO_WM_AMOUTMUL)
      CONSIDERS(wm[o].fmFb,DIV_MACRO_WM_FMFB)
      CONSIDERS(wm[o].pmFb,DIV_MACRO_WM_PMFB)
      CONSIDERS(wm[o].amFb,DIV_MACRO_WM_AMFB)
      CONSIDERS(wm[o].fmMatrix,DIV_MACRO_WM_FMMATRIX)
      CONSIDERS(wm[o].pmMatrix,DIV_MACRO_WM_PMMATRIX)
      CONSIDERS(wm[o].amMatrix,DIV_MACRO_WM_AMMATRIX)
      CONSIDERS(wm[o].spkrVol,DIV_MACRO_WM_SPKRVOL)
      CONSIDERS(wm[o].spkrLVol,DIV_MACRO_WM_SPKRLVOL)
      CONSIDERS(wm[o].spkrRVol,DIV_MACRO_WM_SPKRRVOL)
      CONSIDERS(wm[o].tl,DIV_MACRO_WM_TL)
      CONSIDERS(wm[o].noiPitch,DIV_MACRO_WM_NOIPITCH)
      CONSIDERS(wm[o].noiILfsr,DIV_MACRO_WM_NOIILFSR)
      CONSIDERS(wm[o].noiMask,DIV_MACRO_WM_NOIMASK)
      CONSIDERS(wm[o].filtEn,DIV_MACRO_WM_FILTEN)
      CONSIDERS(wm[o].filtLp,DIV_MACRO_WM_FILTLP)
      CONSIDERS(wm[o].filtHp,DIV_MACRO_WM_FILTHP)
      CONSIDERS(wm[o].filtBp,DIV_MACRO_WM_FILTBP)
      CONSIDERS(wm[o].filt0F,DIV_MACRO_WM_FILT0F)
      CONSIDERS(wm[o].filt0Q,DIV_MACRO_WM_FILT0Q)
      CONSIDERS(wm[o].filt1F,DIV_MACRO_WM_FILT1F)
      CONSIDERS(wm[o].filt1Q,DIV_MACRO_WM_FILT1Q)
      CONSIDERS(wm[o].filt2F,DIV_MACRO_WM_FILT2F)
      CONSIDERS(wm[o].filt2Q,DIV_MACRO_WM_FILT2Q)
      CONSIDERS(wm[o].filt3F,DIV_MACRO_WM_FILT3F)
      CONSIDERS(wm[o].filt3Q,DIV_MACRO_WM_FILT3Q)
      CONSIDERS(wm[o].filt4F,DIV_MACRO_WM_FILT4F)
      CONSIDERS(wm[o].filt4Q,DIV_MACRO_WM_FILT4Q)
      CONSIDERS(wm[o].filt5F,DIV_MACRO_WM_FILT5F)
      CONSIDERS(wm[o].filt5Q,DIV_MACRO_WM_FILT5Q)
      CONSIDERS(wm[o].filt6F,DIV_MACRO_WM_FILT6F)
      CONSIDERS(wm[o].filt6Q,DIV_MACRO_WM_FILT6Q)
      CONSIDERS(wm[o].filt7F,DIV_MACRO_WM_FILT7F)
      CONSIDERS(wm[o].filt7Q,DIV_MACRO_WM_FILT7Q)
      CONSIDERS(wm[o].envAtkT,DIV_MACRO_WM_ENVATKT)
      CONSIDERS(wm[o].envAtkR,DIV_MACRO_WM_ENVATKR)
      CONSIDERS(wm[o].envDecT,DIV_MACRO_WM_ENVDECT)
      CONSIDERS(wm[o].envDecR,DIV_MACRO_WM_ENVDECR)
      CONSIDERS(wm[o].envSusT,DIV_MACRO_WM_ENVSUST)
      CONSIDERS(wm[o].envSusR,DIV_MACRO_WM_ENVSUSR)
      CONSIDERS(wm[o].envRelR,DIV_MACRO_WM_ENVRELR)
      CONSIDERS(wm[o].envMul,DIV_MACRO_WM_ENVMUL)
      CONSIDERS(wm[o].flfoT,DIV_MACRO_WM_FLFOT)
      CONSIDERS(wm[o].flfoL,DIV_MACRO_WM_FLFOL)
      CONSIDERS(wm[o].flfoM,DIV_MACRO_WM_FLFOM)
      CONSIDERS(wm[o].flfoNPitch,DIV_MACRO_WM_FLFONPITCH)
      CONSIDERS(wm[o].flfoNILfsr,DIV_MACRO_WM_FLFONILFSR)
      CONSIDERS(wm[o].flfoNMask,DIV_MACRO_WM_FLFONMASK)
      CONSIDERS(wm[o].alfoT,DIV_MACRO_WM_ALFOT)
      CONSIDERS(wm[o].alfoL,DIV_MACRO_WM_ALFOL)
      CONSIDERS(wm[o].alfoM,DIV_MACRO_WM_ALFOM)
      CONSIDERS(wm[o].alfoNPitch,DIV_MACRO_WM_ALFONPITCH)
      CONSIDERS(wm[o].alfoNILfsr,DIV_MACRO_WM_ALFONILFSR)
      CONSIDERS(wm[o].alfoNMask,DIV_MACRO_WM_ALFONMASK)
    }

    return NULL;
  } else if (type>=0x20 && type<0xa0) {
    unsigned char o=((type>>5)-1)&3;
    switch (type&0x1f) {
      CONSIDER(op[o].am,DIV_MACRO_OP_AM)
      CONSIDER(op[o].ar,DIV_MACRO_OP_AR)
      CONSIDER(op[o].dr,DIV_MACRO_OP_DR)
      CONSIDER(op[o].mult,DIV_MACRO_OP_MULT)
      CONSIDER(op[o].rr,DIV_MACRO_OP_RR)
      CONSIDER(op[o].sl,DIV_MACRO_OP_SL)
      CONSIDER(op[o].tl,DIV_MACRO_OP_TL)
      CONSIDER(op[o].dt2,DIV_MACRO_OP_DT2)
      CONSIDER(op[o].rs,DIV_MACRO_OP_RS)
      CONSIDER(op[o].dt,DIV_MACRO_OP_DT)
      CONSIDER(op[o].d2r,DIV_MACRO_OP_D2R)
      CONSIDER(op[o].ssg,DIV_MACRO_OP_SSG)
      CONSIDER(op[o].dam,DIV_MACRO_OP_DAM)
      CONSIDER(op[o].dvb,DIV_MACRO_OP_DVB)
      CONSIDER(op[o].egt,DIV_MACRO_OP_EGT)
      CONSIDER(op[o].ksl,DIV_MACRO_OP_KSL)
      CONSIDER(op[o].sus,DIV_MACRO_OP_SUS)
      CONSIDER(op[o].vib,DIV_MACRO_OP_VIB)
      CONSIDER(op[o].ws,DIV_MACRO_OP_WS)
      CONSIDER(op[o].ksr,DIV_MACRO_OP_KSR)
    }

    return NULL;
  }

  switch (type) {
    CONSIDER(vol,DIV_MACRO_VOL)
    CONSIDER(arp,DIV_MACRO_ARP)
    CONSIDER(duty,DIV_MACRO_DUTY)
    CONSIDER(wave,DIV_MACRO_WAVE)
    CONSIDER(pitch,DIV_MACRO_PITCH)
    CONSIDER(ex1,DIV_MACRO_EX1)
    CONSIDER(ex2,DIV_MACRO_EX2)
    CONSIDER(ex3,DIV_MACRO_EX3)
    CONSIDER(alg,DIV_MACRO_ALG)
    CONSIDER(fb,DIV_MACRO_FB)
    CONSIDER(fms,DIV_MACRO_FMS)
    CONSIDER(ams,DIV_MACRO_AMS)
    CONSIDER(panL,DIV_MACRO_PAN_LEFT)
    CONSIDER(panR,DIV_MACRO_PAN_RIGHT)
    CONSIDER(phaseReset,DIV_MACRO_PHASE_RESET)
    CONSIDER(ex4,DIV_MACRO_EX4)
    CONSIDER(ex5,DIV_MACRO_EX5)
    CONSIDER(ex6,DIV_MACRO_EX6)
    CONSIDER(ex7,DIV_MACRO_EX7)
    CONSIDER(ex8,DIV_MACRO_EX8)
    CONSIDER(ex9,DIV_MACRO_EX9)
    CONSIDER(ex10,DIV_MACRO_EX10)
  }

  return NULL;
}

#undef CONSIDER
