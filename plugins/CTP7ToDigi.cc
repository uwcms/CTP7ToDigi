// -*- C++ -*-
//
// Package:    TestProducer/CTP7ToDigi
// Class:      CTP7ToDigi
// 
/**\class CTP7ToDigi CTP7ToDigi.cc TestProducer/CTP7ToDigi/plugins/CTP7ToDigi.cc

   Description: [one line class summary]

   Implementation:
   [Notes on implementation]
*/
//
// Original Author:  Sridhara Dasu
//         Created:  Sat, 04 Oct 2014 05:07:24 GMT
//
//


// system include files

#include <memory>
#include <iostream>
#include <string>

using namespace std;

// Framework stuff

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

// CTP7 access providers

#include "CTP7Client.hh"
#include "RCTInfoFactory.hh"

// RCT data formats

#include "DataFormats/L1CaloTrigger/interface/L1CaloEmCand.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegion.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegionDetId.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloCollections.h"

//
// class declaration
//

class CTP7ToDigi : public edm::EDProducer {
public:
  explicit CTP7ToDigi(const edm::ParameterSet&);
  ~CTP7ToDigi();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  virtual void beginJob() override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;
  virtual void endJob() override;
      
  virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
  virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
  virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
  virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

  // ----------member data ---------------------------

  std::string ctp7Host;
  std::string ctp7Port;
  
  CTP7Client *ctp7Client;
  
  uint32_t buffer[NILinks][NIntsPerLink];

  int NEventsPerCapture;

};

//
// constants, enums and typedefs
//

const uint32_t NIntsPerFrame = 6;

//
// static data member definitions
//

//
// constructors and destructor
//
CTP7ToDigi::CTP7ToDigi(const edm::ParameterSet& iConfig)
{

  // Get host and port
  ctp7Host = iConfig.getUntrackedParameter<std::string>("ctp7Host");
  ctp7Port = iConfig.getUntrackedParameter<std::string>("ctp7Port");
  NEventsPerCapture = iConfig.getUntrackedParameter<int>("NEventsPerCapture",170);

  // Create CTP7Client to communicate with specified host/port 
  ctp7Client = new CTP7Client(ctp7Host.c_str(), ctp7Port.c_str());


  //register your products
  produces<L1CaloEmCollection>();
  produces<L1CaloRegionCollection>();

}


CTP7ToDigi::~CTP7ToDigi()
{
  // Close CTP7Client connection
  if(ctp7Client != 0) delete ctp7Client;
}



//
// member functions
//

// ------------ method called to produce the data  ------------
void
CTP7ToDigi::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  static uint32_t index = 0;

  if(index == 0) {

    ctp7Client->capture();

    for(uint32_t link = 0; link < NILinks; link++) {
      if(!ctp7Client->dumpContiguousBuffer(CTP7::inputBuffer, link, 0, NIntsPerLink, buffer[link])) {
	cerr << "CTP7ToDigi::produce() Error reading from CTP7" << endl;
      }
    }
  }

  // Take six ints at a time from even and odd fibers, assumed to be neighboring
  // channels to make rctInfo buffer, and from that make rctEMCands and rctRegions

  std::auto_ptr<L1CaloEmCollection> rctEMCands(new L1CaloEmCollection);
  std::auto_ptr<L1CaloRegionCollection> rctRegions(new L1CaloRegionCollection);

  RCTInfoFactory rctInfoFactory;
  for(uint32_t link = 0; link < NILinks; link+=2) {
    vector <uint32_t> evenFiberData;
    vector <uint32_t> oddFiberData;
    vector <RCTInfo> rctInfo;
    for(uint32_t i = 0; i < 6; i++) {
      evenFiberData.push_back(buffer[link][index+i]);
      oddFiberData.push_back(buffer[link+1][index+i]);
    }
    rctInfoFactory.produce(evenFiberData, oddFiberData, rctInfo);
    rctInfoFactory.printRCTInfo(rctInfo);
    for(int j = 0; j < 4; j++) {
      rctEMCands->push_back(L1CaloEmCand(rctInfo[0].neRank[j], rctInfo[0].neRegn[j], rctInfo[0].neCard[j], link/2, false));
    }
    for(int j = 0; j < 4; j++) {
      rctEMCands->push_back(L1CaloEmCand(rctInfo[0].ieRank[j], rctInfo[0].ieRegn[j], rctInfo[0].ieCard[j], link/2, true));
    }
    for(int j = 0; j < 7; j++) {
      for(int k = 0; k < 2; k++) {
	bool o = (((rctInfo[0].oBits >> (j * 7 + k)) && 0x1) == 0x1);
	bool t = (((rctInfo[0].tBits >> (j * 7 + k)) && 0x1) == 0x1);
	bool m = (((rctInfo[0].mBits >> (j * 7 + k)) && 0x1) == 0x1);
	bool q = (((rctInfo[0].qBits >> (j * 7 + k)) && 0x1) == 0x1);
	rctRegions->push_back(L1CaloRegion(rctInfo[0].rgnEt[j][k], o, t, m, q, link/2, j, k));
      }
    }
    for(int j = 0; j < 2; j++) {
      for(int k = 0; k < 4; k++) {
	rctRegions->push_back(L1CaloRegion(rctInfo[0].hfEt[j][k], 0, link/2, (j * 2 +  k)));
      }
    }
  }

  iEvent.put(rctEMCands);
  iEvent.put(rctRegions);

  cout << "CTP7ToDigi::produce() " << index << endl;

  index += NIntsPerFrame;
  if(index >= std::min(NIntsPerLink, NEventsPerCapture)) index = 0;  
}

// ------------ method called once each job just before starting event loop  ------------
void 
CTP7ToDigi::beginJob()
{
  cout << "CTP7ToDigi::beginJob()" << endl;
}

// ------------ method called once each job just after ending the event loop  ------------
void 
CTP7ToDigi::endJob() {
  cout << "CTP7ToDigi::endJob()" << endl;
}

// ------------ method called when starting to processes a run  ------------

void
CTP7ToDigi::beginRun(edm::Run const&, edm::EventSetup const&)
{
  cout << "CTP7ToDigi::beginRun()" << endl;
}

 
// ------------ method called when ending the processing of a run  ------------

void
CTP7ToDigi::endRun(edm::Run const&, edm::EventSetup const&)
{
  cout << "CTP7ToDigi::endRun()" << endl;
}

 
// ------------ method called when starting to processes a luminosity block  ------------

void
CTP7ToDigi::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
  cout << "CTP7ToDigi::beginLuminosityBlock()" << endl;
}

 
// ------------ method called when ending the processing of a luminosity block  ------------

void
CTP7ToDigi::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
  cout << "CTP7ToDigi::endLuminosityBlock()" << endl;
}
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
CTP7ToDigi::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setComment("Creates events using data captured in the CTP7 buffers");
  desc.addUntracked<std::string>("ctp7Host", "localhost")->setComment("CTP7 TCP/IP host name");
  desc.addUntracked<std::string>("ctp7Port", "5555")->setComment("CTP7 TCP/IP port name");
}

//define this as a plug-in
DEFINE_FWK_MODULE(CTP7ToDigi);
