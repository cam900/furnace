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

#include "debug.h"
#include "imgui.h"
#include "../engine/platform/genesis.h"
#include "../engine/platform/genesisext.h"
#include "../engine/platform/sms.h"
#include "../engine/platform/gb.h"
#include "../engine/platform/pce.h"
#include "../engine/platform/nes.h"
#include "../engine/platform/c64.h"
#include "../engine/platform/arcade.h"
#include "../engine/platform/ym2610.h"
#include "../engine/platform/ym2610ext.h"
#include "../engine/platform/ym2610b.h"
#include "../engine/platform/ym2610bext.h"
#include "../engine/platform/ay.h"
#include "../engine/platform/ay8930.h"
#include "../engine/platform/tia.h"
#include "../engine/platform/saa.h"
#include "../engine/platform/amiga.h"
#include "../engine/platform/x1_010.h"
#include "../engine/platform/n163.h"
#include "../engine/platform/vrc6.h"
#include "../engine/platform/es5506.h"
#include "../engine/platform/dummy.h"

#define GENESIS_DEBUG \
  DivPlatformGenesis::Channel* ch=(DivPlatformGenesis::Channel*)data; \
  ImGui::Text("> YM2612"); \
  ImGui::Text("- freqHL: %.2x%.2x",ch->freqH,ch->freqL); \
  ImGui::Text("* freq: %d",ch->freq); \
  ImGui::Text(" - base: %d",ch->baseFreq); \
  ImGui::Text(" - pitch: %d",ch->pitch); \
  ImGui::Text("- note: %d",ch->note); \
  ImGui::Text("- ins: %d",ch->ins); \
  ImGui::Text("- vol: %.2x",ch->vol); \
  ImGui::Text("- outVol: %.2x",ch->outVol); \
  ImGui::Text("- pan: %x",ch->pan); \
  ImGui::TextColored(ch->active?colorOn:colorOff,">> Active"); \
  ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged"); \
  ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged"); \
  ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn"); \
  ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff"); \
  ImGui::TextColored(ch->portaPause?colorOn:colorOff,">> PortaPause"); \
  ImGui::TextColored(ch->furnaceDac?colorOn:colorOff,">> FurnaceDAC"); \
  ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");

#define SMS_DEBUG \
  DivPlatformSMS::Channel* ch=(DivPlatformSMS::Channel*)data; \
  ImGui::Text("> SMS"); \
  ImGui::Text("* freq: %d",ch->freq); \
  ImGui::Text(" - base: %d",ch->baseFreq); \
  ImGui::Text(" - pitch: %d",ch->pitch); \
  ImGui::Text("- note: %d",ch->note); \
  ImGui::Text("- ins: %d",ch->ins); \
  ImGui::Text("- vol: %.2x",ch->vol); \
  ImGui::Text("- outVol: %.2x",ch->outVol); \
  ImGui::TextColored(ch->active?colorOn:colorOff,">> Active"); \
  ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged"); \
  ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged"); \
  ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn"); \
  ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");

void putDispatchChan(void* data, int chanNum, int type) {
  ImVec4 colorOn=ImVec4(1.0f,1.0f,0.0f,1.0f);
  ImVec4 colorOff=ImVec4(0.3f,0.3f,0.3f,1.0f);
  switch (type) {
    case DIV_SYSTEM_GENESIS:
    case DIV_SYSTEM_YM2612: {
      if (chanNum>5) {
        SMS_DEBUG;
      } else {
        GENESIS_DEBUG;
      }
      break;
    }
    case DIV_SYSTEM_GENESIS_EXT: {
      if (chanNum>8) {
        SMS_DEBUG;
      } else if (chanNum>=2 && chanNum<=5) {
        // TODO ext ch 3 debug
      } else {
        GENESIS_DEBUG;
      }
      break;
    }
    case DIV_SYSTEM_SMS: {
      SMS_DEBUG;
      break;
    }
    case DIV_SYSTEM_GB: {
      DivPlatformGB::Channel* ch=(DivPlatformGB::Channel*)data;
      ImGui::Text("> GameBoy");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- duty: %d",ch->duty);
      ImGui::Text("- sweep: %.2x",ch->sweep);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->sweepChanged?colorOn:colorOff,">> SweepChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      break;
    }
    case DIV_SYSTEM_PCE: {
      DivPlatformPCE::Channel* ch=(DivPlatformPCE::Channel*)data;
      ImGui::Text("> PCEngine");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("* DAC:");
      ImGui::Text(" - period: %d",ch->dacPeriod);
      ImGui::Text(" - rate: %d",ch->dacRate);
      ImGui::Text(" - pos: %d",ch->dacPos);
      ImGui::Text(" - sample: %d",ch->dacSample);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- pan: %.2x",ch->pan);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->noise?colorOn:colorOff,">> Noise");
      ImGui::TextColored(ch->pcm?colorOn:colorOff,">> DAC");
      ImGui::TextColored(ch->furnaceDac?colorOn:colorOff,">> FurnaceDAC");
      break;
    }
    case DIV_SYSTEM_NES: {
      DivPlatformNES::Channel* ch=(DivPlatformNES::Channel*)data;
      ImGui::Text("> NES");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text(" - prev: %d",ch->prevFreq);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- duty: %d",ch->duty);
      ImGui::Text("- sweep: %.2x",ch->sweep);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->sweepChanged?colorOn:colorOff,">> SweepChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->furnaceDac?colorOn:colorOff,">> FurnaceDAC");
      break;
    }
    case DIV_SYSTEM_C64_6581: case DIV_SYSTEM_C64_8580: {
      DivPlatformC64::Channel* ch=(DivPlatformC64::Channel*)data;
      ImGui::Text("> C64");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text(" - prev: %d",ch->prevFreq);
      ImGui::Text("- testWhen: %d",ch->testWhen);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- duty: %d",ch->duty);
      ImGui::Text("- sweep: %.2x",ch->sweep);
      ImGui::Text("- wave: %.1x",ch->wave);
      ImGui::Text("- ADSR: %.1x %.1x %.1x %.1x",ch->attack,ch->decay,ch->sustain,ch->release);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->sweepChanged?colorOn:colorOff,">> SweepChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->filter?colorOn:colorOff,">> Filter");
      ImGui::TextColored(ch->resetMask?colorOn:colorOff,">> ResetMask");
      ImGui::TextColored(ch->resetFilter?colorOn:colorOff,">> ResetFilter");
      ImGui::TextColored(ch->resetDuty?colorOn:colorOff,">> ResetDuty");
      ImGui::TextColored(ch->ring?colorOn:colorOff,">> Ring");
      ImGui::TextColored(ch->sync?colorOn:colorOff,">> Sync");
      break;
    }
    case DIV_SYSTEM_ARCADE:
    case DIV_SYSTEM_YM2151: {
      DivPlatformArcade::Channel* ch=(DivPlatformArcade::Channel*)data;
      ImGui::Text("> YM2151");
      ImGui::Text("- freqHL: %.2x%.2x",ch->freqH,ch->freqL);
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- KOnCycles: %d",ch->konCycles);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- chVolL: %.2x",ch->chVolL);
      ImGui::Text("- chVolR: %.2x",ch->chVolR);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->portaPause?colorOn:colorOff,">> PortaPause");
      ImGui::TextColored(ch->furnacePCM?colorOn:colorOff,">> FurnacePCM");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      break;
    }
    case DIV_SYSTEM_X1_010: {
      DivPlatformX1_010::Channel* ch=(DivPlatformX1_010::Channel*)data;
      ImGui::Text("> X1-010");
      ImGui::Text("* freq: %.4x",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::Text("- sample: %d",ch->sample);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- pan: %d",ch->pan);
      ImGui::Text("* envelope:");
      ImGui::Text(" - shape: %d",ch->env.shape);
      ImGui::Text(" - period: %.2x",ch->env.period);
      ImGui::Text(" - slide: %.2x",ch->env.slide);
      ImGui::Text(" - slidefrac: %.2x",ch->env.slidefrac);
      ImGui::Text(" - autoEnvNum: %.2x",ch->autoEnvNum);
      ImGui::Text(" - autoEnvDen: %.2x",ch->autoEnvDen);
      ImGui::Text("- WaveBank: %d",ch->waveBank);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- Lvol: %.2x",ch->lvol);
      ImGui::Text("- Rvol: %.2x",ch->rvol);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->envChanged?colorOn:colorOff,">> EnvChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->furnacePCM?colorOn:colorOff,">> FurnacePCM");
      ImGui::TextColored(ch->pcm?colorOn:colorOff,">> PCM");
      ImGui::TextColored(ch->env.flag.envEnable?colorOn:colorOff,">> EnvEnable");
      ImGui::TextColored(ch->env.flag.envOneshot?colorOn:colorOff,">> EnvOneshot");
      ImGui::TextColored(ch->env.flag.envSplit?colorOn:colorOff,">> EnvSplit");
      ImGui::TextColored(ch->env.flag.envHinvR?colorOn:colorOff,">> EnvHinvR");
      ImGui::TextColored(ch->env.flag.envVinvR?colorOn:colorOff,">> EnvVinvR");
      ImGui::TextColored(ch->env.flag.envHinvL?colorOn:colorOff,">> EnvHinvL");
      ImGui::TextColored(ch->env.flag.envVinvL?colorOn:colorOff,">> EnvVinvL");
      break;
    }
    case DIV_SYSTEM_N163: {
      DivPlatformN163::Channel* ch=(DivPlatformN163::Channel*)data;
      ImGui::Text("> N163");
      ImGui::Text("* freq: %.4x",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::Text("- wavepos: %d",ch->wavePos);
      ImGui::Text("- wavelen: %d",ch->waveLen);
      ImGui::Text("- wavemode: %d",ch->waveMode);
      ImGui::Text("- loadwave: %d",ch->loadWave);
      ImGui::Text("- loadpos: %d",ch->loadPos);
      ImGui::Text("- loadlen: %d",ch->loadLen);
      ImGui::Text("- loadmode: %d",ch->loadMode);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- resVol: %.2x",ch->resVol);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->volumeChanged?colorOn:colorOff,">> VolumeChanged");
      ImGui::TextColored(ch->waveChanged?colorOn:colorOff,">> WaveChanged");
      ImGui::TextColored(ch->waveUpdated?colorOn:colorOff,">> WaveUpdated");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      break;
    }
    case DIV_SYSTEM_VRC6: {
      DivPlatformVRC6::Channel* ch=(DivPlatformVRC6::Channel*)data;
      ImGui::Text("> VRC6");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("* DAC:");
      ImGui::Text(" - period: %d",ch->dacPeriod);
      ImGui::Text(" - rate: %d",ch->dacRate);
      ImGui::Text(" - out: %d",ch->dacOut);
      ImGui::Text(" - pos: %d",ch->dacPos);
      ImGui::Text(" - sample: %d",ch->dacSample);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- duty: %d",ch->duty);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->pcm?colorOn:colorOff,">> DAC");
      ImGui::TextColored(ch->furnaceDac?colorOn:colorOff,">> FurnaceDAC");
      break;
    }
    case DIV_SYSTEM_ES5506: {
      DivPlatformES5506::Channel* ch=(DivPlatformES5506::Channel*)data;
      ImGui::Text("> ES5506");
      ImGui::Text("* freq: %d",ch->freq);
      ImGui::Text(" - base: %d",ch->baseFreq);
      ImGui::Text(" - pitch: %d",ch->pitch);
      ImGui::Text("- note: %d",ch->note);
      ImGui::Text("- next sample: %d",ch->sample);
      ImGui::Text("- wave: %d",ch->wave);
      ImGui::Text("- slice: %d",ch->slice);
      ImGui::Text("- CR: %d",ch->cr);
      ImGui::Text("* PCM:");
      ImGui::Text(" - freqOffs: %.f",ch->pcm.freqOffs);
      ImGui::Text(" - current index: %d",ch->pcm.index.curr);
      ImGui::Text(" - next index: %d",ch->pcm.index.next);
      ImGui::Text(" - current start: %d",ch->pcm.start.curr);
      ImGui::Text(" - next start: %d",ch->pcm.start.next);
      ImGui::Text(" - current loop: %d",ch->pcm.loop.curr);
      ImGui::Text(" - next loop: %d",ch->pcm.loop.next);
      ImGui::Text(" - current end: %d",ch->pcm.end.curr);
      ImGui::Text(" - next end: %d",ch->pcm.end.next);
      ImGui::Text(" - current bank: %d",ch->pcm.bank.curr);
      ImGui::Text(" - next bank: %d",ch->pcm.bank.next);
      ImGui::Text(" - current sliceSize: %.4f",ch->pcm.sliceSize.curr);
      ImGui::Text(" - next sliceSize: %.4f",ch->pcm.sliceSize.next);
      ImGui::Text(" - current sliceBound: %.4f",ch->pcm.sliceBound.curr);
      ImGui::Text(" - next sliceBound: %.4f",ch->pcm.sliceBound.next);
      ImGui::Text(" - current sliceEnd: %d",ch->pcm.sliceEnd.curr);
      ImGui::Text(" - next sliceEnd: %d",ch->pcm.sliceEnd.next);
      ImGui::Text(" - current sliceLoop: %d",ch->pcm.sliceLoop.curr);
      ImGui::Text(" - next sliceLoop: %d",ch->pcm.sliceLoop.next);
      ImGui::Text("* Transwave:");
      ImGui::Text(" - index: %d",ch->transWave.index);
      ImGui::Text(" - next: %d",ch->transWave.next);
      ImGui::Text("* Filter:");
      ImGui::Text(" - mode: %d",ch->filter.filterMode);
      ImGui::Text(" - k1: %d",ch->filter.k1);
      ImGui::Text("  - offset: %d",ch->filter.k1Offset);
      ImGui::Text(" - k2: %d",ch->filter.k2);
      ImGui::Text("  - offset: %d",ch->filter.k2Offset);
      ImGui::Text("* Envelope:");
      ImGui::Text(" - envCount: %d",ch->envelope.envCount);
      ImGui::Text(" - k1Ramp: %d",ch->envelope.k1Ramp);
      ImGui::Text(" - k2Ramp: %d",ch->envelope.k2Ramp);
      ImGui::Text("- ins: %d",ch->ins);
      ImGui::Text("- vol: %.2x",ch->vol);
      ImGui::Text("- lvol: %.2x",ch->lvol);
      ImGui::Text("- rvol: %.2x",ch->rvol);
      ImGui::Text("- outVol: %.2x",ch->outVol);
      ImGui::Text("- outLVol: %.2x",ch->outLVol);
      ImGui::Text("- outRVol: %.2x",ch->outRVol);
      ImGui::TextColored(ch->active?colorOn:colorOff,">> Active");
      ImGui::TextColored(ch->insChanged?colorOn:colorOff,">> InsChanged");
      ImGui::TextColored(ch->freqChanged?colorOn:colorOff,">> FreqChanged");
      ImGui::TextColored(ch->volumeChanged?colorOn:colorOff,">> VolumeChanged");
      ImGui::TextColored(ch->sampleChanged?colorOn:colorOff,">> SampleChanged");
      ImGui::TextColored(ch->transWaveChanged?colorOn:colorOff,">> TransWaveChanged");
      ImGui::TextColored(ch->transWave.enable?colorOn:colorOff,">> TransWaveenable");
      ImGui::TextColored(ch->transWave.trigger?colorOn:colorOff,">> TransWavetrigger");
      ImGui::TextColored(ch->filterChanged?colorOn:colorOff,">> FilterChanged");
      ImGui::TextColored(ch->filterRampChanged?colorOn:colorOff,">> FilterRampChanged");
      ImGui::TextColored(ch->envChanged?colorOn:colorOff,">> EnvChanged");
      ImGui::TextColored(ch->envelope.k1Slow?colorOn:colorOff,">> EnvelopeK1slow");
      ImGui::TextColored(ch->envelope.k2Slow?colorOn:colorOff,">> EnvelopeK2slow");
      ImGui::TextColored(ch->keyOn?colorOn:colorOff,">> KeyOn");
      ImGui::TextColored(ch->keyOff?colorOn:colorOff,">> KeyOff");
      ImGui::TextColored(ch->inPorta?colorOn:colorOff,">> InPorta");
      ImGui::TextColored(ch->useWave?colorOn:colorOff,">> UseWave");
      ImGui::TextColored(ch->sliceEnable?colorOn:colorOff,">> SliceEnable");
      ImGui::TextColored(ch->pause?colorOn:colorOff,">> Pause");
      break;
    }
    default:
      ImGui::Text("Unknown system! Help!");
      break;
  }
}
