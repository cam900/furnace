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
void DivMacroIntStruct::doMacro(DivMacroSTD<T>& source) {
  if (finished) finished=false;
  if (had!=has) {
    finished=true;
  }
  had=has;
  if (has) {
    val=source.val[pos++];
    if (source.rel>=0 && pos>source.rel && !released) {
      if (source.loop<source.len && source.loop>=0 && source.loop<source.rel) {
        pos=source.loop;
      } else {
        pos--;
      }
    }
    if (pos>=source.len) {
      if (source.loop<source.len && source.loop>=0 && (source.loop>=source.rel || source.rel>=source.len)) {
        pos=source.loop;
      } else {
        has=false;
      }
    }
  }
}

// CPU hell
void DivMacroInt::next() {
  if (ins==NULL) return;

  vol.doMacro(ins->std.volMacro);
  arp.doMacro(ins->std.arpMacro);
  duty.doMacro(ins->std.dutyMacro);
  wave.doMacro(ins->std.waveMacro);
  
  pitch.doMacro(ins->std.pitchMacro);
  ex1.doMacro(ins->std.ex1Macro);
  ex2.doMacro(ins->std.ex2Macro);
  ex3.doMacro(ins->std.ex3Macro);

  alg.doMacro(ins->std.algMacro);
  fb.doMacro(ins->std.fbMacro);
  fms.doMacro(ins->std.fmsMacro);
  ams.doMacro(ins->std.amsMacro);

  for (int i=0; i<4; i++) {
    DivInstrumentSTD::OpMacro& m=ins->std.opMacros[i];
    IntOp& o=op[i];
    o.am.doMacro(m.amMacro);
    o.ar.doMacro(m.arMacro);
    o.dr.doMacro(m.drMacro);
    o.mult.doMacro(m.multMacro);

    o.rr.doMacro(m.rrMacro);
    o.sl.doMacro(m.slMacro);
    o.tl.doMacro(m.tlMacro);
    o.dt2.doMacro(m.dt2Macro);

    o.rs.doMacro(m.rsMacro);
    o.dt.doMacro(m.dtMacro);
    o.d2r.doMacro(m.d2rMacro);
    o.ssg.doMacro(m.ssgMacro);

    o.dam.doMacro(m.damMacro);
    o.dvb.doMacro(m.dvbMacro);
    o.egt.doMacro(m.egtMacro);
    o.ksl.doMacro(m.kslMacro);

    o.sus.doMacro(m.susMacro);
    o.vib.doMacro(m.vibMacro);
    o.ws.doMacro(m.wsMacro);
    o.ksr.doMacro(m.ksrMacro);
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