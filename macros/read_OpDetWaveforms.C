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
#include "TMath.h"
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
//const raw::ADC_Count_t fThresholdADC=20;
const float            fPretrigFraction=0.3;
const size_t           fReadoutWindowSize=100;
const bool             fCombineWindows=true;
const float            fMicrosecondsPerTick=0.002;

void read_OpDetWaveforms(std::string filename, raw::Channel_t ch_num=0, raw::ADC_Count_t fThresholdADC=20) {

  gStyle->SetOptStat(0);  

  const size_t pretrig_size  = fPretrigFraction*fReadoutWindowSize;
  const size_t posttrig_size = fReadoutWindowSize-pretrig_size;
  
  std::vector<std::string> filenames { filename };
  art::InputTag opdet_tag {"opdaq"};
  
  TH1F *hwvfm_raw = new TH1F("hwvfm_raw","RAW WAVEFORM",WAVEFORM_SIZE,-0.5,WAVEFORM_SIZE-0.5);
  TH1F *hwvfm_sub = new TH1F("hwvfm_sub","BASELINE SUBTRACTED WAVEFORM",WAVEFORM_SIZE,-0.5,WAVEFORM_SIZE-0.5);

  std::vector<raw::OpDetWaveform> opdetpulses;
  double my_ch_start_time=0;
  
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
	my_ch_start_time = wvfrm.TimeStamp();
	for(size_t i_t=0; i_t<wvfrm.size(); ++i_t){
	  hwvfm_raw->SetBinContent(i_t+1,wvfrm[i_t]);
	  hwvfm_sub->SetBinContent(i_t+1,PULSE_POLARITY*(wvfrm[i_t]-BASELINE));
	}
      }

      //loop over waveform to find trigger regions
      //trigger is when baseline-subtracted positive-polarity waveform
      //goes above threshold.
      short val,prev_val=0;
      bool above_thresh=false, in_pulse=false;
      //std::vector<short> pulse_wvfm;
      size_t trig_start=0,trig_stop=wvfrm.size();

      for(size_t i_t=0; i_t<wvfrm.size(); ++i_t){
	val = PULSE_POLARITY*(wvfrm[i_t]-BASELINE);

	//if we're not above threshold already, test if we are...
	if(!above_thresh && val>=fThresholdADC){
	  above_thresh=true;
	  if(!in_pulse){
	    in_pulse=true;
	    //pulse_wvfm.clear();
	    trig_start = i_t>pretrig_size ? i_t-pretrig_size : 0;
	    trig_stop  = (wvfrm.size()-1-i_t)>posttrig_size ? i_t+posttrig_size : wvfrm.size();

	    std::cout << "\tTrigger found at " << i_t
		      << ". Window set to be [ " << trig_start << ","
		      << trig_stop << ")"
		      << std::endl;
	  }
	  else if(in_pulse){
	    trig_stop  = (wvfrm.size()-1-i_t)>posttrig_size ? i_t+posttrig_size : wvfrm.size();
	    std::cout << "\tRetrigger found at " << i_t
		      << ". Window extended to be [ " << trig_start << ","
		      << trig_stop << ")"
		      << std::endl;
	  }
	}
	else if(above_thresh && val<fThresholdADC){
	  above_thresh=false;
	}

	if(in_pulse && i_t==trig_stop-1){
	  opdetpulses.emplace_back(wvfrm.TimeStamp()+fMicrosecondsPerTick*trig_start,
				   wvfrm.ChannelNumber(),
				   trig_stop-trig_start);
	  opdetpulses.back().insert(opdetpulses.back().begin(),wvfrm.begin()+trig_start,wvfrm.begin()+trig_stop);
	  std::cout << "\tInserted waveform [ " << trig_start << "," << trig_stop << ")"
		    << " timestamp = " << opdetpulses.back().TimeStamp()
		    << " (original = " << wvfrm.TimeStamp() << ")"
		    << std::endl;
	  in_pulse=false;

	}

      }
      
    }

    break;

  } //end loop over events!

  std::vector<TH1F*> h_readout_wvfms;
  for(auto const& wvfrm : opdetpulses){
    size_t i_rw=0;
    if(wvfrm.ChannelNumber()==ch_num){
      TString hname; hname.Form("h_wvfm_ch%u_rw%lu",wvfrm.ChannelNumber(),i_rw);
      TString htitle; htitle.Form("Readout Waveform: Ch %u, Window %lu",wvfrm.ChannelNumber(),i_rw);
      int start_tick = (wvfrm.TimeStamp()-my_ch_start_time)*TMath::Nint(1./fMicrosecondsPerTick);
      TH1F* htmp = new TH1F(hname,htitle,wvfrm.size(),start_tick-0.5,start_tick-0.5+wvfrm.size());
      h_readout_wvfms.push_back(htmp);
      for(size_t i_twt=0; i_twt<wvfrm.size(); ++i_twt)
	h_readout_wvfms.back()->SetBinContent(i_twt+1,PULSE_POLARITY*(wvfrm[i_twt]-BASELINE));
      h_readout_wvfms.back()->SetLineColor(kRed);
      i_rw++;
    }
  }

  
  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","OpWaveforms");
  canvas->Divide(1,2);
  //canvas->Divide(3,12); //divides the canvas in two!
  canvas->cd(1);
  hwvfm_raw->Draw();
  canvas->cd(2);
  hwvfm_sub->Draw();
  for(auto h : h_readout_wvfms)
    h->Draw("same");
  //and ... done!
  
}
