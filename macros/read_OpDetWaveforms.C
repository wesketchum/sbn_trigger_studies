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

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RawData/OpDetWaveform.h"

void read_OpDetWaveforms(std::string filename) {

  gStyle->SetOptStat(0);

  
  std::vector<std::string> filenames { filename };
  art::InputTag opdet_tag {"opdaq"};
  
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
      std::cout << "Channel " << wvfrm.ChannelNumber() << ", time=" << wvfrm.TimeStamp() << ": size=" << wvfrm.size() << std::endl;
    }

  } //end loop over events!

  /*
  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","OpFlash Info!",1500,1200);
  canvas->Divide(3,12); //divides the canvas in two!

  for(size_t i_w=0; i_w<36; ++i_w){
    canvas->cd(i_w+1);
    hist_vec[i_w]->Draw();
  }
  //and ... done!
  */
}
