/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2026 tildearrow and contributors
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

#include "insEditCommon.h"
#include "../intConst.h"
#include "../util.h"
#include "../engine/platform/sound/jkms16wm32o8.hpp"
#include "IconsFontAwesome4.h"

#define OP_DRAG_POINT_WM \
  if (ImGui::Button(ICON_FA_ARROWS)) { \
  } \
  if (ImGui::BeginDragDropSource()) { \
    opToMove=i; \
    ImGui::SetDragDropPayload("FUR_WMOP",NULL,0,ImGuiCond_Once); \
    ImGui::Button(ICON_FA_ARROWS "##SysDragWM"); \
    ImGui::SameLine(); \
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
      ImGui::Text(_("(copying)")); \
    } else { \
      ImGui::Text(_("(swapping)")); \
    } \
    ImGui::EndDragDropSource(); \
  } else if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip(_("- drag to swap operator\n- shift-drag to copy operator")); \
  } \
  if (ImGui::BeginDragDropTarget()) { \
    const ImGuiPayload* dragItem=ImGui::AcceptDragDropPayload("FUR_WMOP"); \
    if (dragItem!=NULL) { \
      if (dragItem->IsDataType("FUR_WMOP")) { \
        if (opToMove!=i && opToMove>=0) { \
          int destOp=i; \
          int sourceOp=opToMove; \
          if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              ins->wm.op[destOp]=ins->wm.op[sourceOp]; \
            }); \
          } else { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              DivInstrumentWM::WMOperator origOp=ins->wm.op[sourceOp]; \
              ins->wm.op[sourceOp]=ins->wm.op[destOp]; \
              ins->wm.op[destOp]=origOp; \
            }); \
          } \
          PARAMETER; \
        } \
        opToMove=-1; \
      } \
    } \
    ImGui::EndDragDropTarget(); \
  }

const char* wmEnvEnableBits[3]={
  "enable", "loop", NULL
};

const char* wmLfoEnableBits[4]={
  "enable", "wave0", "wave1", NULL
};

const char* wmOutEnableBits[4]={
  "filtered", "direct", "speaker", NULL
};

const char* wmModEnableBits[3]={
  "input", "output", NULL
};

const char* wmOperatorBits[9]={
  "op1", "op2", "op3", "op4", "op5", "op6", "op7", "op8", NULL
};

const char* wmShapeBits[9]={
  _N("pulse"),
  _N("saw"),
  _N("triangle"),
  _N("noise"),
  _N("invpulse"),
  _N("invsaw"),
  _N("invtriangle"),
  _N("external"),
  NULL
};

const char* wmMatrixBits[9]={
  _N("to op1"),
  _N("to op2"),
  _N("to op3"),
  _N("to op4"),
  _N("to op5"),
  _N("to op6"),
  _N("to op7"),
  _N("to op8"),
  NULL
};

const char* wmFilterBits[9]={
  _N("filter 1"),
  _N("filter 2"),
  _N("filter 3"),
  _N("filter 4"),
  _N("filter 5"),
  _N("filter 6"),
  _N("filter 7"),
  _N("filter 8"),
  NULL
};

const char* wmModeBits[3]={
  _N("invert right"),
  _N("invert left"),
  NULL
};

const char* wmLfoWaveTypes[4]={
  _N("Sawtooth"),
  _N("Triangle"),
  _N("Square"),
  _N("Noise")
};

String macroWMLfoWave(int id, float val, void* u) {
  const char* label="???";
  if (((int)val)&1) {
    switch ((((int)val)>>1)&3) {
      case 0:
        label=_("Saw");
        break;
      case 1:
        label=_("Triangle");
        break;
      case 2:
        label=_("Square");
        break;
      case 3:
        label=_("Noise");
        break;
      default: break;
    }
    return fmt::sprintf("%d: %s (%d, %x)",id,label,((int)val&7),((int)val&7));
  }
  return fmt::sprintf("%d: disabled (%d, %x)",id,((int)val&7),((int)val&7));
}

String macroWMPitchMul(int id, float val, void* u) {
  return fmt::sprintf("%d: x%.1f (%d, %x)",id,(((int)val)==0)?0.5f:(float)(((int)val)&15),((int)val&15),((int)val&15));
}

String macroWMBitPos(int id, float val, void* u) {
  return fmt::sprintf("%d: %d (%d, %x)",id,16-(((int)val)&15),((int)val&15),((int)val&15));
}

String macroWMWaveSize(int id, float val, void* u) {
  return fmt::sprintf("%d: %d samples (%d, %x)",id,65536>>(((int)val)&15),((int)val&15),((int)val&15));
}

void FurnaceGUI::insEditWM(DivInstrument* ins) {
  if (ImGui::BeginTabItem("JKMS16WM32O8")) {
    DivInstrumentWM& wmOrigin=ins->wm;

    int columns=2;
    switch (settings.fmLayout) {
      case 1: // 2x4
        columns=2;
        break;
      case 2: // 1x8
        columns=1;
        break;
      case 3: // 4x2
        columns=4;
        break;
    }
    if (ImGui::BeginTable("WMOperators",columns,ImGuiTableFlags_SizingStretchSame)) {
      for (int i=0; i<8; i++) {
        DivInstrumentWM::WMOperator& op=wmOrigin.op[i];
        if ((settings.fmLayout==3 && ((i&3)==0)) || (settings.fmLayout!=3 && ((i+1)&1)) || i==0 || settings.fmLayout==2) ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Separator();
        ImGui::PushID(fmt::sprintf("wmop%d",i).c_str());

        ImGui::Dummy(ImVec2(dpiScale,dpiScale));
        String opNameLabel;
        String matLabel;
        OP_DRAG_POINT_WM;
        ImGui::SameLine();
        opNameLabel=fmt::sprintf("OP%d",i+1);
        pushToggleColors(op.enable);
        if (ImGui::Button(opNameLabel.c_str())) {
          op.enable=!op.enable;
          PARAMETER;
        }
        popToggleColors();

        bool isFixed=op.fixed;
        if (ImGui::Checkbox(_("Fixed frequency"),&isFixed)) { PARAMETER
          op.fixed=isFixed;
        }

        bool spkrEnable=op.spkrEnable;
        if (ImGui::Checkbox(_("Speaker output"),&spkrEnable)) { PARAMETER
          op.spkrEnable=spkrEnable;
        }

        bool dirOut=op.dirOut;
        if (ImGui::Checkbox(_("Direct output"),&dirOut)) { PARAMETER
          op.dirOut=dirOut;
        }

        bool filtOut=op.filtOut;
        if (ImGui::Checkbox(_("Filtered output"),&filtOut)) { PARAMETER
          op.filtOut=filtOut;
        }
        ImGui::Separator();

        int tl=op.tl;
        ImGui::Text(_("Total Level"));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderInt("##WMOPTL",&tl,-32768,32767)) { PARAMETER
          if (tl<-32768) tl=-32768;
          if (tl>32767) tl=32767;
          op.tl=tl;
        } rightClickable

        ImGui::Separator();
        bool isPitchControl=op.pitchCtrl;
        if (isFixed) {
          ImGui::SameLine();
          pushWarningColor(isPitchControl && !e->song.compatFlags.linearPitch);
          if (ImGui::Checkbox(_("Pitch control"),&isPitchControl)) { PARAMETER
            op.pitchCtrl=isPitchControl;
          }
          popWarningColor();
          if (ImGui::IsItemHovered()) {
            if (isPitchControl && !e->song.compatFlags.linearPitch) {
              ImGui::SetTooltip(_("only works on linear pitch! go to Compatibility Flags > Pitch/Playback and set Pitch linearity to Full."));
            } else {
              ImGui::SetTooltip(_("use op's arpeggio and pitch macros control instead of frequency macros"));
            }
          }
          if (!isPitchControl) {
            int freqNum=op.fixedFreq;

            ImGui::Text(_("Frequency"));
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (CWSliderInt("##WMOPFixedFreq",&freqNum,0,65535)) { PARAMETER
              if (freqNum<0) freqNum=0;
              if (freqNum>65535) freqNum=65535;
              op.fixedFreq=freqNum;
            } rightClickable
          }
        } else {
          ImGui::Text(_("Detune"));
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          P(CWSliderScalar("##WMOPDT",ImGuiDataType_S16,&op.dt,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable

          ImGui::Text(_("Pitch Multiplier"));
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          P(CWSliderScalar("##WMOPPMUL",ImGuiDataType_U8,&op.pitchMul,&_ZERO,&_FIFTEEN,fmt::sprintf("x%.1f",((op.pitchMul&15)==0)?0.5f:(float)(op.pitchMul&15)).c_str())); rightClickable
        }
        ImGui::Separator();
        if (ImGui::TreeNode(_("Waveform generator"))) {
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Waveform"));
          pushToggleColors(op.wavBit&1);
          if (ImGui::Button(_("pulse"))) { PARAMETER
            op.wavBit^=1;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&2);
          if (ImGui::Button(_("saw"))) { PARAMETER
            op.wavBit^=2;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&4);
          if (ImGui::Button(_("tri"))) { PARAMETER
            op.wavBit^=4;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&8);
          if (ImGui::Button(_("noise"))) { PARAMETER
            op.wavBit^=8;
          }
          popToggleColors();
          pushToggleColors(op.wavBit&16);
          if (ImGui::Button(_("invpulse"))) { PARAMETER
            op.wavBit^=16;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&32);
          if (ImGui::Button(_("invsaw"))) { PARAMETER
            op.wavBit^=32;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&64);
          if (ImGui::Button(_("invtri"))) { PARAMETER
            op.wavBit^=64;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(op.wavBit&128);
          if (ImGui::Button(_("external"))) { PARAMETER
            op.wavBit^=128;
          }
          popToggleColors();

          bool useExtWave=op.wavBit&128;
          ImGui::BeginDisabled(!useExtWave);
          bool useSample=op.useSample;
          if (ImGui::Checkbox(_("Use Sample"),&useSample)) { PARAMETER
            op.useSample=useSample;
          }
          ImGui::EndDisabled();

          if (ImGui::BeginTable("wmWavParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMWavDuty",ImGuiDataType_U16,&op.duty,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Pulse Duty"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMIntWSize",ImGuiDataType_U8,&op.intWSize,&_ZERO,&_FIFTEEN,fmt::sprintf("%d samples",65536>>(op.intWSize&15)).c_str())); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Internal Waveform Size"));

            // external waveform
            ImGui::BeginDisabled(!useExtWave);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMExtWSize",ImGuiDataType_U8,&op.extWSize,&_ZERO,&_FIFTEEN,fmt::sprintf("%d samples",65536>>(op.extWSize&15)).c_str())); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("External Waveform Size"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMExtWBase",ImGuiDataType_U16,&op.wavBase,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("External Waveform base address"));

            if (useSample) {
              String sName;
              if (op.initSample<0 || op.initSample>=e->song.sampleLen) {
                sName=_("none selected");
              } else {
                sName=e->song.sample[op.initSample]->name;
              }
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##WMISample",sName.c_str())) {
                String id;
                for (int i=0; i<e->song.sampleLen; i++) {
                  id=fmt::sprintf("%d: %s",i,e->song.sample[i]->name);
                  if (ImGui::Selectable(id.c_str(),op.initSample==i)) { PARAMETER
                    op.initSample=i;
                    notifySampleChange=true;
                  }
                }
                ImGui::EndCombo();
              }
              ImGui::TableNextColumn();
              ImGui::TextUnformatted(_("External sample"));
            } else {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::InputInt("##WMIWave",&op.initWave,1,4)) { PARAMETER
                if (op.initWave<0) op.initWave=0;
                if (op.initWave>=(int)e->song.wave.size()) op.initWave=e->song.wave.size()-1;
              }
              if (ins->std.wmMacros[i].ewMacro.len>0) {
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("waveform macro is controlling external waveform!\nthis value will be ineffective."));
                }
              }
              ImGui::TableNextColumn();
              if (ins->std.wmMacros[i].ewMacro.len>0) {
                ImGui::PushStyleColor(ImGuiCol_Text,uiColors[GUI_COLOR_WARNING]);
                ImGui::AlignTextToFramePadding();
                ImGui::Text(_("External waveform"));
                ImGui::SameLine();
                ImGui::Text(ICON_FA_EXCLAMATION_TRIANGLE);
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("waveform macro is controlling external waveform!\nthis value will be ineffective."));
                }
              } else {
                ImGui::AlignTextToFramePadding();
                ImGui::Text(_("External waveform"));
              }
            }

            // modulator
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool muteEn=op.mute.enable;
            if (ImGui::Checkbox(_("Mute bit enable"),&muteEn)) { PARAMETER
              op.mute.enable=muteEn;
            }

            ImGui::BeginDisabled(!muteEn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMMuteBitPos",ImGuiDataType_U8,&op.mute.bitPos,&_ZERO,&_FIFTEEN,fmt::sprintf("%d",16-(op.mute.bitPos&15)).c_str())); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Mute bit position"));
            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool revEn=op.reverse.enable;
            if (ImGui::Checkbox(_("Reverse bit enable"),&revEn)) { PARAMETER
              op.reverse.enable=revEn;
            }

            ImGui::BeginDisabled(!revEn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMRevBitPos",ImGuiDataType_U8,&op.reverse.bitPos,&_ZERO,&_FIFTEEN,fmt::sprintf("%d",16-(op.reverse.bitPos&15)).c_str())); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Reverse bit position"));
            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool invEn=op.invert.enable;
            if (ImGui::Checkbox(_("Invert bit enable"),&invEn)) { PARAMETER
              op.invert.enable=invEn;
            }

            ImGui::BeginDisabled(!invEn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMInvBitPos",ImGuiDataType_U8,&op.invert.bitPos,&_ZERO,&_FIFTEEN,fmt::sprintf("%d",16-(op.invert.bitPos&15)).c_str())); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Invert bit position"));
            ImGui::EndDisabled();

            ImGui::EndDisabled();

            // noise
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMWavNPitch",ImGuiDataType_U16,&op.noisePitch,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Noise Pitch"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMWavNILfsr",ImGuiDataType_U16,&op.initLfsr,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Noise Initial LFSR"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMWavNMask",ImGuiDataType_U16,&op.lfsrMask,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Noise LFSR Mask"));

            ImGui::EndTable();
          }
          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode(_("Modulator"))) {
          if (ImGui::BeginTable("wmModParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool fmIn=op.fmIn.enable;
            if (ImGui::Checkbox(_("FM input enable"),&fmIn)) { PARAMETER
              op.fmIn.enable=fmIn;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool pmIn=op.pmIn.enable;
            if (ImGui::Checkbox(_("PM input enable"),&pmIn)) { PARAMETER
              op.pmIn.enable=pmIn;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool amIn=op.amIn.enable;
            if (ImGui::Checkbox(_("AM input enable"),&amIn)) { PARAMETER
              op.amIn.enable=amIn;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool fmOut=op.fmOut.enable;
            if (ImGui::Checkbox(_("FM output enable"),&fmOut)) { PARAMETER
              op.fmOut.enable=fmOut;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool pmOut=op.pmOut.enable;
            if (ImGui::Checkbox(_("PM output enable"),&pmOut)) { PARAMETER
              op.pmOut.enable=pmOut;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool amOut=op.amOut.enable;
            if (ImGui::Checkbox(_("AM output enable"),&amOut)) { PARAMETER
              op.amOut.enable=amOut;
            }

            ImGui::BeginDisabled(!fmIn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFMInMul",ImGuiDataType_S16,&op.fmIn.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("FM input Multiplier"));
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!pmIn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMPMInMul",ImGuiDataType_S16,&op.pmIn.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("PM input Multiplier"));
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!amIn);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAMInMul",ImGuiDataType_S16,&op.amIn.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("AM input Multiplier"));
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!fmOut);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFMOutMul",ImGuiDataType_S16,&op.fmOut.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("FM output Multiplier"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFMOutFB",ImGuiDataType_S16,&op.fmOut.fb,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("FM feedback"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (int m=0; m<8; m++) {
              matLabel=fmt::sprintf("##FMOutMat%d",m);
              int matShift=1<<m;
              bool fmMat=op.fmOut.matrix&matShift;
              if (m!=0) {
                ImGui::SameLine();
              }
              if (ImGui::Checkbox(matLabel.c_str(),&fmMat)) { PARAMETER
                op.fmOut.matrix=(op.fmOut.matrix&~(matShift))|(fmMat?matShift:0);
              }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("FM output matrix (Op1 to 8)"));
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!pmOut);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMPMOutMul",ImGuiDataType_S16,&op.pmOut.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("PM output Multiplier"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMPMOutFB",ImGuiDataType_S16,&op.pmOut.fb,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("PM feedback"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (int m=0; m<8; m++) {
              matLabel=fmt::sprintf("##PMOutMat%d",m);
              int matShift=1<<m;
              bool pmMat=op.pmOut.matrix&matShift;
              if (m!=0) {
                ImGui::SameLine();
              }
              if (ImGui::Checkbox(matLabel.c_str(),&pmMat)) { PARAMETER
                op.pmOut.matrix=(op.pmOut.matrix&~(matShift))|(pmMat?matShift:0);
              }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("PM output matrix (Op1 to 8)"));
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!amOut);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAMOutMul",ImGuiDataType_S16,&op.amOut.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("AM output Multiplier"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAMOutFB",ImGuiDataType_S16,&op.amOut.fb,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("AM feedback"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (int m=0; m<8; m++) {
              matLabel=fmt::sprintf("##AMOutMat%d",m);
              int matShift=1<<m;
              bool amMat=op.amOut.matrix&matShift;
              if (m!=0) {
                ImGui::SameLine();
              }
              if (ImGui::Checkbox(matLabel.c_str(),&amMat)) { PARAMETER
                op.amOut.matrix=(op.amOut.matrix&~(matShift))|(amMat?matShift:0);
              }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("AM output matrix (Op1 to 8)"));
            ImGui::EndDisabled();

            ImGui::EndTable();
          }
          ImGui::TreePop();
        }
        ImGui::Separator();

        if (ImGui::TreeNode(_("Envelope"))) {
          bool envOn=op.env.enable;
          if (ImGui::Checkbox(_("Enable"),&envOn)) { PARAMETER
            op.env.enable=envOn;
          }

          ImGui::BeginDisabled(!op.env.enable);
          ImGui::SameLine();
          bool envLoop=op.env.loop;
          if (ImGui::Checkbox(_("Loop"),&envLoop)) { PARAMETER
            op.env.loop=envLoop;
          }

          if (ImGui::BeginTable("wmEnvParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvInitLV",ImGuiDataType_S16,&op.env.initLv,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Initial level"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvDelay",ImGuiDataType_U16,&op.env.delR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Delay length"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvAtkT",ImGuiDataType_S16,&op.env.atkT,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Attack target"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvAtkR",ImGuiDataType_U16,&op.env.atkR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Attack rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvDecT",ImGuiDataType_S16,&op.env.decT,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Decay target"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvDecR",ImGuiDataType_U16,&op.env.decR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Decay rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvSusT",ImGuiDataType_S16,&op.env.susT,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Sustain target"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvSusR",ImGuiDataType_U16,&op.env.susR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Sustain rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvRelR",ImGuiDataType_U16,&op.env.relR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Release rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMEnvMul",ImGuiDataType_S16,&op.env.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Envelope multipler"));

            ImGui::EndTable();
          }
          ImGui::EndDisabled();
          ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode(_("Frequency LFO"))) {
          bool flfoOn=op.flfo.enable;
          if (ImGui::Checkbox(_("Enable"),&flfoOn)) { PARAMETER
            op.flfo.enable=flfoOn;
          }
          ImGui::BeginDisabled(!op.flfo.enable);
          if (ImGui::BeginTable("wmFlfoParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFLFOWS",ImGuiDataType_U8,&op.flfo.wave,&_ZERO,&_THREE,wmLfoWaveTypes[op.flfo.wave&3])); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Waveform"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoDelay",ImGuiDataType_U16,&op.flfo.delR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Delay length"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoTarget",ImGuiDataType_S16,&op.flfo.tgt,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Target"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoRate",ImGuiDataType_U16,&op.flfo.rate,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoMul",ImGuiDataType_S16,&op.flfo.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Multiplier"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoNPitch",ImGuiDataType_U16,&op.flfo.noisePitch,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Noise Pitch"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoNInit",ImGuiDataType_U16,&op.flfo.initLfsr,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Initial LFSR"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMFlfoNMask",ImGuiDataType_U16,&op.flfo.lfsrMask,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("LFSR Mask"));

            ImGui::EndTable();
          }
          ImGui::EndDisabled();
          ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode(_("Amplitude LFO"))) {
          bool alfoOn=op.alfo.enable;
          if (ImGui::Checkbox(_("Enable"),&alfoOn)) { PARAMETER
            op.alfo.enable=alfoOn;
          }
          ImGui::BeginDisabled(!op.alfo.enable);
          if (ImGui::BeginTable("wmAlfoParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMALFOWS",ImGuiDataType_U8,&op.alfo.wave,&_ZERO,&_THREE,wmLfoWaveTypes[op.alfo.wave&3])); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Waveform"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoDelay",ImGuiDataType_U16,&op.alfo.delR,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Delay length"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoTarget",ImGuiDataType_S16,&op.alfo.tgt,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Target"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoRate",ImGuiDataType_U16,&op.alfo.rate,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Rate"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoMul",ImGuiDataType_S16,&op.alfo.mul,&_MINUS_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_EIGHT,&_THRTY_TWO_THOUSAND_SEVEN_HUNDRED_SIXTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Multiplier"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoNPitch",ImGuiDataType_U16,&op.alfo.noisePitch,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Noise Pitch"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoNInit",ImGuiDataType_U16,&op.alfo.initLfsr,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Initial LFSR"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WMAlfoNMask",ImGuiDataType_U16,&op.alfo.lfsrMask,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("LFSR Mask"));

            ImGui::EndTable();
          }
          ImGui::EndDisabled();
          ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::BeginDisabled(!filtOut);
        if (ImGui::TreeNode(_("Filter"))) {
          String filterLabel;
          for (int f=0; f<8; f++) {
            filterLabel=fmt::sprintf("Filter %d",f);
            if (ImGui::TreeNode(_(filterLabel.c_str()))) {
              bool filterOn=op.filter[f].enable;
              if (ImGui::Checkbox(_("Enable"),&filterOn)) { PARAMETER
                op.filter[f].enable=filterOn;
              }

              ImGui::BeginDisabled(!filterOn);
              bool lpOn=op.filter[f].lpEnable;
              if (ImGui::Checkbox(_("Lowpass output Enable"),&lpOn)) { PARAMETER
                op.filter[f].lpEnable=lpOn;
              }

              bool hpOn=op.filter[f].hpEnable;
              if (ImGui::Checkbox(_("Highpass output Enable"),&hpOn)) { PARAMETER
                op.filter[f].hpEnable=hpOn;
              }

              bool bpOn=op.filter[f].bpEnable;
              if (ImGui::Checkbox(_("Bandpass output Enable"),&bpOn)) { PARAMETER
                op.filter[f].bpEnable=bpOn;
              }

              if (ImGui::BeginTable(fmt::sprintf("wmFilter%dParams",f).c_str(),2,ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
                ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar(fmt::sprintf("##WMFilter%dF",f).c_str(),ImGuiDataType_U16,&op.filter[f].f,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("F parameter"));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar(fmt::sprintf("##WMFilter%dQ",f).c_str(),ImGuiDataType_U16,&op.filter[f].q,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("Q parameter"));

                ImGui::EndTable();
              }
              ImGui::EndDisabled();
              ImGui::TreePop();
            }
          }
          ImGui::TreePop();
        }
        ImGui::EndDisabled();

        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    ImGui::EndTabItem();
  }

  if (!ins->amiga.useSample) {
    insTabWavetable(ins);
  }
  insTabSample(ins);

  std::vector<FurnaceGUIMacroDesc> macroList;

  char label[32];
  int waveCount=MAX(1,e->song.waveLen-1);
  int sampleCount=MAX(1,e->song.sampleLen-1);
  for (int i=0; i<8; i++) {
    snprintf(label,31,_("OP%d Macros"),i+1);
    if (ImGui::BeginTabItem(label)) {
      ImGui::PushID(i);
      macroList.push_back(FurnaceGUIMacroDesc(_("Total Level"),&ins->std.wmMacros[i].tlMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Speaker master lv."),&ins->std.wmMacros[i].spkrVolMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Speaker left lv."),&ins->std.wmMacros[i].spkrLVolMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Speaker right lv."),&ins->std.wmMacros[i].spkrRVolMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
      if (ins->wm.op[i].fixed) {
        if (!ins->wm.op[i].pitchCtrl) {
          macroList.push_back(FurnaceGUIMacroDesc(_("Frequency"),&ins->std.wmMacros[i].pitchMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_PITCH]));
        } else {
          macroList.push_back(FurnaceGUIMacroDesc(_("Op. Arpeggio"),&ins->std.wmMacros[i].arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.wmMacros[i].arpMacro.val,true));
          macroList.push_back(FurnaceGUIMacroDesc(_("Op. Pitch"),&ins->std.wmMacros[i].pitchMacro,-65536,65535,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode,NULL,false,NULL,false,NULL,false,true));
        }
      }
      macroList.push_back(FurnaceGUIMacroDesc(_("Env. enable"),&ins->std.wmMacros[i].envEnMacro,0,2,64,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,wmEnvEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO enable"),&ins->std.wmMacros[i].flfoEnMacro,0,3,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMLfoWave,true,wmLfoEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO enable"),&ins->std.wmMacros[i].alfoEnMacro,0,3,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMLfoWave,true,wmLfoEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Detune"),&ins->std.wmMacros[i].dtMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Pitch Mul."),&ins->std.wmMacros[i].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMPitchMul));
      macroList.push_back(FurnaceGUIMacroDesc(_("Out. enable"),&ins->std.wmMacros[i].outEnMacro,0,3,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmOutEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("FM enable"),&ins->std.wmMacros[i].fmEnMacro,0,2,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmModEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("PM enable"),&ins->std.wmMacros[i].pmEnMacro,0,2,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmModEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("AM enable"),&ins->std.wmMacros[i].amEnMacro,0,2,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmModEnableBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Mute enable"),&ins->std.wmMacros[i].muteEnMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Mute bit pos."),&ins->std.wmMacros[i].muteBitMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMBitPos));
      macroList.push_back(FurnaceGUIMacroDesc(_("Reverse enable"),&ins->std.wmMacros[i].revEnMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Reverse bit pos."),&ins->std.wmMacros[i].revBitMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMBitPos));
      macroList.push_back(FurnaceGUIMacroDesc(_("Invert enable"),&ins->std.wmMacros[i].invEnMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Invert bit pos."),&ins->std.wmMacros[i].invBitMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMBitPos));
      macroList.push_back(FurnaceGUIMacroDesc(_("Int. wave size"),&ins->std.wmMacros[i].intWlMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMWaveSize));
      macroList.push_back(FurnaceGUIMacroDesc(_("Ext. wave size"),&ins->std.wmMacros[i].extWlMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroWMWaveSize));
      macroList.push_back(FurnaceGUIMacroDesc(_("Waveform shape"),&ins->std.wmMacros[i].wfMacro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmShapeBits));
      if (ins->wm.op[i].useSample) {
        macroList.push_back(FurnaceGUIMacroDesc(_("External sample"),&ins->std.wmMacros[i].ewMacro,0,sampleCount,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      } else {
        macroList.push_back(FurnaceGUIMacroDesc(_("External waveform"),&ins->std.wmMacros[i].ewMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      }
      macroList.push_back(FurnaceGUIMacroDesc(_("Pulse duty"),&ins->std.wmMacros[i].dutyMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FM input mul."),&ins->std.wmMacros[i].fmInMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("PM input mul."),&ins->std.wmMacros[i].pmInMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("AM input mul."),&ins->std.wmMacros[i].amInMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FM output mul."),&ins->std.wmMacros[i].fmOutMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("PM output mul."),&ins->std.wmMacros[i].pmOutMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("AM output mul."),&ins->std.wmMacros[i].amOutMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FM feedback"),&ins->std.wmMacros[i].fmFbMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("PM feedback"),&ins->std.wmMacros[i].pmFbMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("AM feedback"),&ins->std.wmMacros[i].amFbMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FM output matrix"),&ins->std.wmMacros[i].fmMatrixMacro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmMatrixBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("PM output matrix"),&ins->std.wmMacros[i].pmMatrixMacro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmMatrixBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("AM output matrix"),&ins->std.wmMacros[i].amMatrixMacro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmMatrixBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Noise pitch"),&ins->std.wmMacros[i].noiPitchMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_NOISE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Noise init. LFSR"),&ins->std.wmMacros[i].noiILfsrMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Noise mask"),&ins->std.wmMacros[i].noiMaskMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Attack target"),&ins->std.wmMacros[i].envAtkTMacro,-32767,32767,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Attack rate"),&ins->std.wmMacros[i].envAtkRMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Decay target"),&ins->std.wmMacros[i].envDecTMacro,-32767,32767,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Decay rate"),&ins->std.wmMacros[i].envDecRMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Sustain target"),&ins->std.wmMacros[i].envSusTMacro,-32767,32767,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Sustain rate"),&ins->std.wmMacros[i].envSusRMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Release rate"),&ins->std.wmMacros[i].envRelRMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Envelope mul."),&ins->std.wmMacros[i].envMulMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO target"),&ins->std.wmMacros[i].flfoTMacro,-32767,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO level"),&ins->std.wmMacros[i].flfoLMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO mul."),&ins->std.wmMacros[i].flfoMMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO Noise pitch"),&ins->std.wmMacros[i].flfoNPitchMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_NOISE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO Noise init. LFSR"),&ins->std.wmMacros[i].flfoNILfsrMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("FLFO Noise mask"),&ins->std.wmMacros[i].flfoNMaskMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO target"),&ins->std.wmMacros[i].alfoTMacro,-32767,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO level"),&ins->std.wmMacros[i].alfoLMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO mul."),&ins->std.wmMacros[i].alfoMMacro,-32768,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO Noise pitch"),&ins->std.wmMacros[i].alfoNPitchMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_NOISE]));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO Noise init. LFSR"),&ins->std.wmMacros[i].alfoNILfsrMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("ALFO Noise mask"),&ins->std.wmMacros[i].alfoNMaskMacro,0,16,128,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter enable"),&ins->std.wmMacros[i].filtEnMacro,0,8,160,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,wmFilterBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter lowpass"),&ins->std.wmMacros[i].filtLpMacro,0,8,160,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,wmFilterBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter highpass"),&ins->std.wmMacros[i].filtHpMacro,0,8,160,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,wmFilterBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter bandpass"),&ins->std.wmMacros[i].filtBpMacro,0,8,160,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,wmFilterBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 0 F"),&ins->std.wmMacros[i].filt0FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 0 Q"),&ins->std.wmMacros[i].filt0QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 1 F"),&ins->std.wmMacros[i].filt1FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 1 Q"),&ins->std.wmMacros[i].filt1QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 2 F"),&ins->std.wmMacros[i].filt2FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 2 Q"),&ins->std.wmMacros[i].filt2QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 3 F"),&ins->std.wmMacros[i].filt3FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 3 Q"),&ins->std.wmMacros[i].filt3QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 4 F"),&ins->std.wmMacros[i].filt4FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 4 Q"),&ins->std.wmMacros[i].filt4QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 5 F"),&ins->std.wmMacros[i].filt5FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 5 Q"),&ins->std.wmMacros[i].filt5QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 6 F"),&ins->std.wmMacros[i].filt6FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 6 Q"),&ins->std.wmMacros[i].filt6QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 7 F"),&ins->std.wmMacros[i].filt7FMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter 7 Q"),&ins->std.wmMacros[i].filt7QMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));

      drawMacros(macroList,macroEditStateOP[i],ins);
      ImGui::PopID();
      ImGui::EndTabItem();
    }
  }

  if (ImGui::BeginTabItem(_("Macros"))) {
    macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,32767,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
    macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
    macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,32767,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
    macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
    macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-65536,65535,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
    macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
    macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex1Macro,0,2,96,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmModeBits));
    macroList.push_back(FurnaceGUIMacroDesc(_("OpMask"),&ins->std.ex2Macro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,wmOperatorBits));

    drawMacros(macroList,macroEditStateMacros,ins);
    ImGui::EndTabItem();
  }
}
