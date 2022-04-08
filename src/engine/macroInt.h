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

class DivMacroInt {
  DivInstrument* ins;
  bool released;
  public:
    template<typename T>
    struct DivMacroIntStruct {
      DivMacroSTD<T>* source;
      int pos, val;
      bool has, had, finished, will;
      bool mode;
      void reset() {
        source=NULL;
        pos=0;
        has=had=finished=will=false;
        mode=false;
      }
      void init(DivMacroSTD<T>* src) {
        if (src==NULL) {
          return;
        }
        source=src;
        has=had=finished=will=true;
        mode=src->mode;
      }
	    void doMacro(bool released);
      DivMacroIntStruct<T>():
        source(NULL),
        pos(0),
        val(0),
        has(false),
        had(false),
        finished(false),
        will(false),
        mode(false) {}
    };

    DivMacroIntStruct<int> vol;
    DivMacroIntStruct<int> arp;
    DivMacroIntStruct<int> duty, wave, pitch, ex1, ex2, ex3;
    DivMacroIntStruct<int> alg, fb, fms, ams;
    DivMacroIntStruct<int> panL, panR, phaseReset, ex4, ex5, ex6, ex7, ex8;
    struct IntOp {
      DivMacroIntStruct<unsigned char> am, ar, dr, mult;
      DivMacroIntStruct<unsigned char> rr, sl, tl, dt2;
      DivMacroIntStruct<unsigned char> rs, dt, d2r, ssg;
      DivMacroIntStruct<unsigned char> dam, dvb, egt, ksl;
      DivMacroIntStruct<unsigned char> sus, vib, ws, ksr;
      void reset() {
        am.reset();
        ar.reset();
        dr.reset();
        mult.reset();
        rr.reset();
        sl.reset();
        tl.reset();
        dt2.reset();
        rs.reset();
        dt.reset();
        d2r.reset();
        ssg.reset();
        dam.reset();
        dvb.reset();
        egt.reset();
        ksl.reset();
        sus.reset();
        vib.reset();
        ws.reset();
        ksr.reset();
      }
      IntOp():
        am(DivMacroIntStruct<unsigned char>()),
        ar(DivMacroIntStruct<unsigned char>()),
        dr(DivMacroIntStruct<unsigned char>()),
        mult(DivMacroIntStruct<unsigned char>()),
        rr(DivMacroIntStruct<unsigned char>()),
        sl(DivMacroIntStruct<unsigned char>()),
        tl(DivMacroIntStruct<unsigned char>()),
        dt2(DivMacroIntStruct<unsigned char>()),
        rs(DivMacroIntStruct<unsigned char>()),
        dt(DivMacroIntStruct<unsigned char>()),
        d2r(DivMacroIntStruct<unsigned char>()),
        ssg(DivMacroIntStruct<unsigned char>()),
        dam(DivMacroIntStruct<unsigned char>()),
        dvb(DivMacroIntStruct<unsigned char>()),
        egt(DivMacroIntStruct<unsigned char>()),
        ksl(DivMacroIntStruct<unsigned char>()),
        sus(DivMacroIntStruct<unsigned char>()),
        vib(DivMacroIntStruct<unsigned char>()),
        ws(DivMacroIntStruct<unsigned char>()),
        ksr(DivMacroIntStruct<unsigned char>()) {}
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
      vol(DivMacroIntStruct<int>()),
      arp(DivMacroIntStruct<int>()),
      duty(DivMacroIntStruct<int>()),
      wave(DivMacroIntStruct<int>()),
      pitch(DivMacroIntStruct<int>()),
      ex1(DivMacroIntStruct<int>()),
      ex2(DivMacroIntStruct<int>()),
      ex3(DivMacroIntStruct<int>()),
      alg(DivMacroIntStruct<int>()),
      fb(DivMacroIntStruct<int>()),
      fms(DivMacroIntStruct<int>()),
      ams(DivMacroIntStruct<int>()),
      panL(DivMacroIntStruct<int>()),
      panR(DivMacroIntStruct<int>()),
      phaseReset(DivMacroIntStruct<int>()),
      ex4(DivMacroIntStruct<int>()),
      ex5(DivMacroIntStruct<int>()),
      ex6(DivMacroIntStruct<int>()),
      ex7(DivMacroIntStruct<int>()),
      ex8(DivMacroIntStruct<int>()),
      op{IntOp(),IntOp(),IntOp(),IntOp()} {}
};

#endif
