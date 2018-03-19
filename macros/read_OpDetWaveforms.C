//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TFile.h"
#include "TStyle.h"
#include "TCanvas.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RawData/OpDetWaveform.h"

const raw::ADC_Count_t BASELINE=8000;
const size_t           WAVEFORM_SIZE=1100;
const int              PULSE_POLARITY=-1;

void read_OpDetWaveforms(std::string filename, raw::Channel_t ch_num=0) {

  gStyle->SetOptStat(0);

  
  std::vector<std::string> filenames { filename };
  art::InputTag opdet_tag {"opdaq"};
  
  TH1F *hwvfm_raw = new TH1F("hwvfm_raw","RAW WAVEFORM",WAVEFORM_SIZE,0,WAVEFORM_SIZE);
  TH1F *hwvfm_sub = new TH1F("hwvfm_sub","BASELINE SUBTRACTED WAVEFORM",WAVEFORM_SIZE,0,WAVEFORM_SIZE);

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {

    //if(ev.eventEntry()!=ev_to_process) continue;
    
    //to get run and event info, you use this "eventAuxillary()" object.
    std::cout << "Processing "
	      << "Run " << ev.eventAuxiliary().run() << ", "
	      << "Event " << ev.eventAuxiliary().event() << std::endl;

    auto const& opdet_handle = ev.getValidHandle<vector<raw::OpDetWaveform>>(opdet_tag);
    auto const& opdet_vec(*opdet_handle);

    std::cout << "We have " << opdet_vec.size() << " waveforms." << std::endl;

    for(size_t i_w=0; i_w<opdet_vec.size(); ++i_w){
      auto const& wvfrm = opdet_vec[i_w];
      std::cout << "Channel " << wvfrm.ChannelNumber() 
		<< ", time=" << wvfrm.TimeStamp() 
		<< ": size=" << wvfrm.size() << std::endl;

      if(wvfrm.ChannelNumber()==ch_num){
	for(size_t i_t=0; i_t<wvfrm.size(); ++i_t){
	  hwvfm_raw->SetBinContent(i_t+1,wvfrm[i_t]);
	  hwvfm_sub->SetBinContent(i_t+1,PULSE_POLARITY*(wvfrm[i_t]-BASELINE));
	}
      }
    }

    break;

  } //end loop over events!

  
  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","OpWaveforms");
  canvas->Divide(1,2);
  //canvas->Divide(3,12); //divides the canvas in two!
  canvas->cd(1);
  hwvfm_raw->Draw();
  canvas->cd(2);
  hwvfm_sub->Draw();
  //and ... done!
  
}
