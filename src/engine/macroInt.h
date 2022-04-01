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

#ifndef _MACROINT_H
#define _MACROINT_H

#include "instrument.h"

struct DivMacroIntStruct {
  int pos, val;
  bool has, had, finished, will;
  void reset() {
    pos=0;
    has=had=finished=will=false;
  }
  void init() {
    has=had=finished=will=true;
  }
  DivMacroIntStruct():
    pos(0),
    val(0),
    has(false),
    had(false),
    finished(false),
    will(false) {}
};

class DivMacroInt {
  DivInstrument* ins;
  bool released;
  public:
    DivMacroIntStruct vol;
    DivMacroIntStruct arp;
    DivMacroIntStruct duty, wave, pitch, ex1, ex2, ex3;
    DivMacroIntStruct alg, fb, fms, ams;
    bool arpMode;
    struct IntOp {
      DivMacroIntStruct am, ar, dr, mult;
      DivMacroIntStruct rr, sl, tl, dt2;
      DivMacroIntStruct rs, dt, d2r, ssg;
      DivMacroIntStruct dam, dvb, egt, ksl;
      DivMacroIntStruct sus, vib, ws, ksr;
      IntOp():
        am(DivMacroIntStruct()),
        ar(DivMacroIntStruct()),
        dr(DivMacroIntStruct()),
        mult(DivMacroIntStruct()),
        rr(DivMacroIntStruct()),
        sl(DivMacroIntStruct()),
        tl(DivMacroIntStruct()),
        dt2(DivMacroIntStruct()),
        rs(DivMacroIntStruct()),
        dt(DivMacroIntStruct()),
        d2r(DivMacroIntStruct()),
        ssg(DivMacroIntStruct()),
        dam(DivMacroIntStruct()),
        dvb(DivMacroIntStruct()),
        egt(DivMacroIntStruct()),
        ksl(DivMacroIntStruct()),
        sus(DivMacroIntStruct()),
        vib(DivMacroIntStruct()),
        ws(DivMacroIntStruct()),
        ksr(DivMacroIntStruct()) {}
    } op[4];

    /**
     * trigger macro release.
     */
    void release();

    /**
     * trigger next macro tick.
     */
    void next();

    /**
     * execute macro routine.
     * @param val an macro struct.
     * @param source an instrument.
     */
    template<typename T>void doMacro(DivMacroIntStruct& macro, DivMacroSTD<T>& source);

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

    DivMacroInt():
      ins(NULL),
      released(false),
      vol(DivMacroIntStruct()),
      arp(DivMacroIntStruct()),
      duty(DivMacroIntStruct()),
      wave(DivMacroIntStruct()),
      pitch(DivMacroIntStruct()),
      ex1(DivMacroIntStruct()),
      ex2(DivMacroIntStruct()),
      ex3(DivMacroIntStruct()),
      alg(DivMacroIntStruct()),
      fb(DivMacroIntStruct()),
      fms(DivMacroIntStruct()),
      ams(DivMacroIntStruct()),
      arpMode(false) {}
};

#endif
