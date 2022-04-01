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

#include "macroInt.h"
#include "instrument.h"

template<typename T>
void DivMacroInt::doMacro(DivMacroIntStruct& macro, DivMacroSTD<T>& source) {
  if (macro.finished) macro.finished=false;
  if (macro.had!=macro.has) {
    macro.finished=true;
  }
  macro.had=macro.has;
  if (macro.has) {
    macro.val=source.val[macro.pos++];
    if (source.rel>=0 && macro.pos>source.rel && !released) {
      if (source.loop<source.len && source.loop>=0 && source.loop<source.rel) {
        macro.pos=source.loop;
      } else {
        macro.pos--;
      }
    }
    if (macro.pos>=source.len) {
      if (source.loop<source.len && source.loop>=0 && (source.loop>=source.rel || source.rel>=source.len)) {
        macro.pos=source.loop;
      } else {
        macro.has=false;
      }
    }
  }
}

// CPU hell
void DivMacroInt::next() {
  if (ins==NULL) return;

  doMacro(vol,ins->std.volMacro);
  doMacro(arp,ins->std.arpMacro);
  doMacro(duty,ins->std.dutyMacro);
  doMacro(wave,ins->std.waveMacro);
  
  doMacro(pitch,ins->std.pitchMacro);
  doMacro(ex1,ins->std.ex1Macro);
  doMacro(ex2,ins->std.ex2Macro);
  doMacro(ex3,ins->std.ex3Macro);

  doMacro(alg,ins->std.algMacro);
  doMacro(fb,ins->std.fbMacro);
  doMacro(fms,ins->std.fmsMacro);
  doMacro(ams,ins->std.amsMacro);

  for (int i=0; i<4; i++) {
    DivInstrumentSTD::OpMacro& m=ins->std.opMacros[i];
    IntOp& o=op[i];
    doMacro(o.am,m.amMacro);
    doMacro(o.ar,m.arMacro);
    doMacro(o.dr,m.drMacro);
    doMacro(o.mult,m.multMacro);

    doMacro(o.rr,m.rrMacro);
    doMacro(o.sl,m.slMacro);
    doMacro(o.tl,m.tlMacro);
    doMacro(o.dt2,m.dt2Macro);

    doMacro(o.rs,m.rsMacro);
    doMacro(o.dt,m.dtMacro);
    doMacro(o.d2r,m.d2rMacro);
    doMacro(o.ssg,m.ssgMacro);

    doMacro(o.dam,m.damMacro);
    doMacro(o.dvb,m.dvbMacro);
    doMacro(o.egt,m.egtMacro);
    doMacro(o.ksl,m.kslMacro);

    doMacro(o.sus,m.susMacro);
    doMacro(o.vib,m.vibMacro);
    doMacro(o.ws,m.wsMacro);
    doMacro(o.ksr,m.ksrMacro);
  }
}

void DivMacroInt::release() {
  released=true;
}

void DivMacroInt::init(DivInstrument* which) {
  ins=which;
  vol.reset();
  arp.reset();
  duty.reset();
  wave.reset();
  pitch.reset();
  ex1.reset();
  ex2.reset();
  ex3.reset();
  alg.reset();
  fb.reset();
  fms.reset();
  ams.reset();

  released=false;

  op[0]=IntOp();
  op[1]=IntOp();
  op[2]=IntOp();
  op[3]=IntOp();

  arpMode=false;

  if (ins==NULL) return;

  if (ins->std.volMacro.len>0) {
    vol.init();
  }
  if (ins->std.arpMacro.len>0) {
    arp.init();
  }
  if (ins->std.dutyMacro.len>0) {
    duty.init();
  }
  if (ins->std.waveMacro.len>0) {
    wave.init();
  }
  if (ins->std.pitchMacro.len>0) {
    pitch.init();
  }
  if (ins->std.ex1Macro.len>0) {
    ex1.init();
  }
  if (ins->std.ex2Macro.len>0) {
    ex2.init();
  }
  if (ins->std.ex3Macro.len>0) {
    ex3.init();
  }
  if (ins->std.algMacro.len>0) {
    alg.init();
  }
  if (ins->std.fbMacro.len>0) {
    fb.init();
  }
  if (ins->std.fmsMacro.len>0) {
    fms.init();
  }
  if (ins->std.amsMacro.len>0) {
    ams.init();
  }

  if (ins->std.arpMacroMode) {
    arpMode=true;
  }

  for (int i=0; i<4; i++) {
    DivInstrumentSTD::OpMacro& m=ins->std.opMacros[i];
    IntOp& o=op[i];

    if (m.amMacro.len>0) {
      o.am.init();
    }
    if (m.arMacro.len>0) {
      o.ar.init();
    }
    if (m.drMacro.len>0) {
      o.dr.init();
    }
    if (m.multMacro.len>0) {
      o.mult.init();
    }
    if (m.rrMacro.len>0) {
      o.rr.init();
    }
    if (m.slMacro.len>0) {
      o.sl.init();
    }
    if (m.tlMacro.len>0) {
      o.tl.init();
    }
    if (m.dt2Macro.len>0) {
      o.dt2.init();
    }
    if (m.rsMacro.len>0) {
      o.rs.init();
    }
    if (m.dtMacro.len>0) {
      o.dt.init();
    }
    if (m.d2rMacro.len>0) {
      o.d2r.init();
    }
    if (m.ssgMacro.len>0) {
      o.ssg.init();
    }

    if (m.damMacro.len>0) {
      o.dam.init();
    }
    if (m.dvbMacro.len>0) {
      o.dvb.init();
    }
    if (m.egtMacro.len>0) {
      o.egt.init();
    }
    if (m.kslMacro.len>0) {
      o.ksl.init();
    }
    if (m.susMacro.len>0) {
      o.sus.init();
    }
    if (m.vibMacro.len>0) {
      o.vib.init();
    }
    if (m.wsMacro.len>0) {
      o.ws.init();
    }
    if (m.ksrMacro.len>0) {
      o.ksr.init();
    }
  }
}

void DivMacroInt::notifyInsDeletion(DivInstrument* which) {
  if (ins==which) {
    init(NULL);
  }
}