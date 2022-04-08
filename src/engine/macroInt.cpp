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
void DivMacroInt::DivMacroIntStruct<T>::doMacro(bool released) {
  if (source==NULL) {
    return;
  }
  if (finished) finished=false;
  if (had!=has) {
    finished=true;
  }
  had=has;
  if (has) {
    val=source->val[pos++];
    if (source->rel>=0 && pos>source->rel && !released) {
      if (source->loop<source->len && source->loop>=0 && source->loop<source->rel) {
        pos=source->loop;
      } else {
        pos--;
      }
    }
    if (pos>=source->len) {
      if (source->loop<source->len && source->loop>=0 && (source->loop>=source->rel || source->rel>=source->len)) {
        pos=source->loop;
      } else {
        has=false;
      }
    }
  }
}

// CPU hell
void DivMacroInt::next() {
  if (ins==NULL) return;

  vol.doMacro(released);
  arp.doMacro(released);
  duty.doMacro(released);
  wave.doMacro(released);
  
  pitch.doMacro(released);
  ex1.doMacro(released);
  ex2.doMacro(released);
  ex3.doMacro(released);

  alg.doMacro(released);
  fb.doMacro(released);
  fms.doMacro(released);
  ams.doMacro(released);

  panL.doMacro(released);
  panR.doMacro(released);
  phaseReset.doMacro(released);
  ex4.doMacro(released);
  ex5.doMacro(released);
  ex6.doMacro(released);
  ex7.doMacro(released);
  ex8.doMacro(released);

  for (int i=0; i<4; i++) {
    IntOp& o=op[i];
    o.am.doMacro(released);
    o.ar.doMacro(released);
    o.dr.doMacro(released);
    o.mult.doMacro(released);

    o.rr.doMacro(released);
    o.sl.doMacro(released);
    o.tl.doMacro(released);
    o.dt2.doMacro(released);

    o.rs.doMacro(released);
    o.dt.doMacro(released);
    o.d2r.doMacro(released);
    o.ssg.doMacro(released);

    o.dam.doMacro(released);
    o.dvb.doMacro(released);
    o.egt.doMacro(released);
    o.ksl.doMacro(released);

    o.sus.doMacro(released);
    o.vib.doMacro(released);
    o.ws.doMacro(released);
    o.ksr.doMacro(released);
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
  panL.reset();
  panR.reset();
  phaseReset.reset();
  ex4.reset();
  ex5.reset();
  ex6.reset();
  ex7.reset();
  ex8.reset();

  released=false;

  op[0].reset();
  op[1].reset();
  op[2].reset();
  op[3].reset();

  if (ins==NULL) return;

  if (ins->std.volMacro.len>0) {
    vol.init(&ins->std.volMacro);
  }
  if (ins->std.arpMacro.len>0) {
    arp.init(&ins->std.arpMacro);
  }
  if (ins->std.dutyMacro.len>0) {
    duty.init(&ins->std.dutyMacro);
  }
  if (ins->std.waveMacro.len>0) {
    wave.init(&ins->std.waveMacro);
  }
  if (ins->std.pitchMacro.len>0) {
    pitch.init(&ins->std.pitchMacro);
  }
  if (ins->std.ex1Macro.len>0) {
    ex1.init(&ins->std.ex1Macro);
  }
  if (ins->std.ex2Macro.len>0) {
    ex2.init(&ins->std.ex2Macro);
  }
  if (ins->std.ex3Macro.len>0) {
    ex3.init(&ins->std.ex3Macro);
  }
  if (ins->std.algMacro.len>0) {
    alg.init(&ins->std.algMacro);
  }
  if (ins->std.fbMacro.len>0) {
    fb.init(&ins->std.fbMacro);
  }
  if (ins->std.fmsMacro.len>0) {
    fms.init(&ins->std.fmsMacro);
  }
  if (ins->std.amsMacro.len>0) {
    ams.init(&ins->std.amsMacro);
  }

  // TODO: other macros
  if (ins->std.panLMacro.len>0) {
    panL.init(&ins->std.panLMacro);
  }
  if (ins->std.panRMacro.len>0) {
    panR.init(&ins->std.panRMacro);
  }
  if (ins->std.phaseResetMacro.len>0) {
    phaseReset.init(&ins->std.phaseResetMacro);
  }
  if (ins->std.ex4Macro.len>0) {
    ex4.init(&ins->std.ex4Macro);
  }
  if (ins->std.ex5Macro.len>0) {
    ex5.init(&ins->std.ex5Macro);
  }
  if (ins->std.ex6Macro.len>0) {
    ex6.init(&ins->std.ex6Macro);
  }
  if (ins->std.ex7Macro.len>0) {
    ex7.init(&ins->std.ex7Macro);
  }
  if (ins->std.ex8Macro.len>0) {
    ex8.init(&ins->std.ex8Macro);
  }

  for (int i=0; i<4; i++) {
    DivInstrumentSTD::OpMacro& m=ins->std.opMacros[i];
    IntOp& o=op[i];

    if (m.amMacro.len>0) {
      o.am.init(&m.amMacro);
    }
    if (m.arMacro.len>0) {
      o.ar.init(&m.arMacro);
    }
    if (m.drMacro.len>0) {
      o.dr.init(&m.drMacro);
    }
    if (m.multMacro.len>0) {
      o.mult.init(&m.multMacro);
    }
    if (m.rrMacro.len>0) {
      o.rr.init(&m.rrMacro);
    }
    if (m.slMacro.len>0) {
      o.sl.init(&m.slMacro);
    }
    if (m.tlMacro.len>0) {
      o.tl.init(&m.tlMacro);
    }
    if (m.dt2Macro.len>0) {
      o.dt2.init(&m.dt2Macro);
    }
    if (m.rsMacro.len>0) {
      o.rs.init(&m.rsMacro);
    }
    if (m.dtMacro.len>0) {
      o.dt.init(&m.dtMacro);
    }
    if (m.d2rMacro.len>0) {
      o.d2r.init(&m.d2rMacro);
    }
    if (m.ssgMacro.len>0) {
      o.ssg.init(&m.ssgMacro);
    }

    if (m.damMacro.len>0) {
      o.dam.init(&m.damMacro);
    }
    if (m.dvbMacro.len>0) {
      o.dvb.init(&m.dvbMacro);
    }
    if (m.egtMacro.len>0) {
      o.egt.init(&m.egtMacro);
    }
    if (m.kslMacro.len>0) {
      o.ksl.init(&m.kslMacro);
    }
    if (m.susMacro.len>0) {
      o.sus.init(&m.susMacro);
    }
    if (m.vibMacro.len>0) {
      o.vib.init(&m.vibMacro);
    }
    if (m.wsMacro.len>0) {
      o.ws.init(&m.wsMacro);
    }
    if (m.ksrMacro.len>0) {
      o.ksr.init(&m.ksrMacro);
    }
  }
}

void DivMacroInt::notifyInsDeletion(DivInstrument* which) {
  if (ins==which) {
    init(NULL);
  }
}
