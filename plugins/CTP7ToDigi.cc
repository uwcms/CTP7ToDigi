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

// Scan in file

#include <fstream>

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
  int getLinkNumber(bool even, int crate);
  bool scanInLink(uint32_t link, uint32_t tempBuffer[NIntsPerLink]);
  void printLinksToFile();
  bool waitForCaptureSuccess();
  virtual void endJob() override;      
  virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
  virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
  virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
  virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

  // ----------member data ---------------------------

  std::string ctp7Host;
  std::string ctp7Port;
  std::string testFile;
  
  CTP7Client *ctp7Client;
  
  uint32_t buffer[NILinks][NIntsPerLink];

  int NEventsPerCapture;
  bool test;
  bool createLinkFile;

  char fileName[40];

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
  test = iConfig.getUntrackedParameter<bool>("test",false);
  createLinkFile = iConfig.getUntrackedParameter<bool>("createLinkFile",false);
  testFile = iConfig.getUntrackedParameter<std::string>("testFile","testFile.txt");
  // Create CTP7Client to communicate with specified host/port 
  ctp7Client = new CTP7Client(ctp7Host.c_str(), ctp7Port.c_str());

  //set test file name here, shoudl be added as an untrackedParamater
  sprintf(fileName,"testFile.txt");

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
  static uint32_t countCycles = 0;
  static uint32_t loopEvents = 0;

  if(index == 0) {

    countCycles++;
    cout<<"Capture number: "<<dec<<countCycles<<endl;

    if(!ctp7Client->checkConnection()){
      cout<<"CTP7 Check Connection FAILED!!!! If you are trying "; 
      cout<<"to capture data from CTP7, think again!"<<endl;}
    
    ctp7Client->capture();

    if(!waitForCaptureSuccess())
      cout<<"Capture Not Successful!!!"<<endl;
    

    if(!test) // normal mode
      for(uint32_t link = 0; link < NILinks; link++) {
	unsigned int addressOffset = link * NIntsPerLink * 4;
	if(!ctp7Client->getValues(CTP7::inputBuffer,addressOffset,NIntsPerLink,buffer[link])){
	  cerr << "CTP7ToDigi::produce() Error reading from CTP7" << endl;
	}
      }
    else {// test mode
      cout <<"TESTING MODE"<<endl;
      for(uint32_t link = 0; link < NILinks; link++) {
	if(!scanInLink(link,buffer[link])){
	  cerr << "CTP7ToDigi::produce() Error reading from file: " << testFile << endl;
	}
      }
    }

    if(createLinkFile)
      printLinksToFile();
  }  

  // Take six ints at a time from even and odd fibers, assumed to be neighboring
  // channels to make rctInfo buffer, and from that make rctEMCands and rctRegions

  std::auto_ptr<L1CaloEmCollection> rctEMCands(new L1CaloEmCollection);
  std::auto_ptr<L1CaloRegionCollection> rctRegions(new L1CaloRegionCollection);

  RCTInfoFactory rctInfoFactory;

 for(uint32_t link = 0; link < NILinks; link+=2){
//  for(uint32_t link = 0; link < NILinks/2; link++) {
    vector <uint32_t> evenFiberData;
    vector <uint32_t> oddFiberData;
    vector <RCTInfo> rctInfo;

    cout<<endl<<dec<<"Crate Number? --> "<<link/2<<endl;
 
    int CTP7link;
    for(uint32_t i = 0; i < 6; i++) {
      //Order for filling the links is 0 to 18, however, the links are not ordered in the CTP7
      //getLinkNumber method provides a temporary mapping; a long term getLinkID and match
      //Needs to be implemented (currently in place in the CTP7 Unpacker)
    
      CTP7link = getLinkNumber(true,link/2);
      evenFiberData.push_back(buffer[CTP7link][index+i]);
      CTP7link = getLinkNumber(false,link/2);
      oddFiberData.push_back(buffer[CTP7link][index+i]);
    }

    cout<<"Print evenFiberData : ";
    for (uint32_t i=0; i<evenFiberData.size(); i++){           cout<<hex<<evenFiberData.at(i)<<",";    }
    cout<<endl<<"Print oddFiberData :";
    for (uint32_t i=0; i<evenFiberData.size(); i++){          cout<<hex<<oddFiberData.at(i)<<",";     }
    cout<<endl;

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
	bool o = (((rctInfo[0].oBits >> (j * 2 + k)) & 0x1) == 0x1);
	bool t = (((rctInfo[0].tBits >> (j * 2 + k)) & 0x1) == 0x1);
	bool m = (((rctInfo[0].mBits >> (j * 2 + k)) & 0x1) == 0x1);
	bool q = (((rctInfo[0].qBits >> (j * 2 + k)) & 0x1) == 0x1);
	rctRegions->push_back(L1CaloRegion(rctInfo[0].rgnEt[j][k], o, t, m, q, link/2, j, k));
      }
    }
    for(int j = 0; j < 2; j++) {
      for(int k = 0; k < 4; k++) {
        // bool fineGrain = hfFineGrainBits.at(hfRgn);
        bool fg=(((rctInfo[0].hfQBits>> (j * 4 + k)) & 0x1)  == 0x1); 
	rctRegions->push_back(L1CaloRegion(rctInfo[0].hfEt[j][k], fg, link/2, (j * 4 +  k)));
      }
    }
  }


  iEvent.put(rctEMCands);
  iEvent.put(rctRegions);

  cout <<dec<< "CTP7ToDigi::produce() " << index << endl;

  index += NIntsPerFrame;

  // index and "loopEvents" cannot be the same. loopEvents needs to increase by one, while index is used in evenFiberData and is increased by NIntsPerFrame 
  // this part needs debugging!

  uint32_t MINIMUM= NEventsPerCapture ;   // The min was a mistake like it was (it made us capture too often for the pattern, we repeat events).
                                          // Make sure for pattern tests only 64 events are run, but set in the configuration file, not only here
                                          // To be revised: MINIMUM=std::min( (int) NIntsPerLink, NEventsPerCapture);
  if(loopEvents >= MINIMUM) loopEvents = 0;  
  else loopEvents++;
}

int CTP7ToDigi::getLinkNumber(bool even, int crate){

  //even is even and odd is odd
  if(even){
    if(crate==0)return  15;//LinkID 15: a   
    if(crate==1)return  18;//LinkID 18: 10a 
    if(crate==2)return  20;//LinkID 20: 20a 
    if(crate==3)return  12;//LinkID 12: 30a 
    if(crate==4)return  13;//LinkID 13: 40a 
    if(crate==5)return  17;//LinkID 17: 50a 
    if(crate==6)return   2;//LinkID 2: 60a  
    if(crate==7)return   5;//LinkID 5: 70a  
    if(crate==8)return  10;//LinkID 10: 80a 
    if(crate==9)return   0;//LinkID 0: 90a  
    if(crate==10)return  1;//LinkID 1: a0a  
    if(crate==11)return  6;//LinkID 6: b0a  
    if(crate==12)return 27;//LinkID 27: c0a 
    if(crate==13)return 30;//LinkID 30: d0a 
    if(crate==14)return 32;//LinkID 32: e0a 
    if(crate==15)return 24;//LinkID 24: f0a 
    if(crate==16)return 25;//LinkID 25: 100a
    if(crate==17)return 29;//LinkID 29: 110a
    else{
      std::cout<<"Failed to find odd crate; since we don't check the linkIDs from CTP7 this must be a software bug! (check with Isobel)"<<std::endl;
      return 0;}
  }
  else{
    if(crate==0)return  16;//LinkID 16: b   
    if(crate==1)return  19;//LinkID 19: 10b 
    if(crate==2)return  21;//LinkID 21: 20b 
    if(crate==3)return  14;//LinkID 14: 30b 
    if(crate==4)return  23;//LinkID 23: 40b 
    if(crate==5)return  22;//LinkID 22: 50b 
    if(crate==6)return   4;//LinkID 4: 60b  
    if(crate==7)return   7;//LinkID 7: 70b  
    if(crate==8)return   8;//LinkID 8: 80b   
    if(crate==9)return   3;//LinkID 3: 90b  
    if(crate==10)return  9;//LinkID 9: a0b  
    if(crate==11)return 11;//LinkID 11: b0b 
    if(crate==12)return 28;//LinkID 28: c0b 
    if(crate==13)return 31;//LinkID 31: d0b 
    if(crate==14)return 33;//LinkID 33: e0b 
    if(crate==15)return 26;//LinkID 26: f0b 
    if(crate==16)return 35;//LinkID 35: 100b
    if(crate==17)return 34;//LinkID 34: 110b
    else{
      std::cout<<"Failed to find odd crate; since we don't check the linkIDs from CTP7 this must be a software bug! (check with Isobel, as it is likely her fault)"<<std::endl;
      return 0;
    }
  }

}

/*
 * This reads in a file of form testFile.txt (see test directory for example)
 * and outputs a dump of the same form that would be received by a CTP7 Capture
 */
 
bool CTP7ToDigi::scanInLink(uint32_t link, uint32_t tempBuffer[NIntsPerLink]){

  FILE *fptr = fopen(fileName, "r");

  if(fptr == NULL) {
    cout<<"Error: Could not open emulator input file "<<endl;
    return false;
  } 

  char line[100];
  char searchTerm[100];
  int ret = 0; 

  char wordLink[20];
  sprintf(wordLink,"link");
  sprintf(searchTerm,"%u",(unsigned int) link);

  unsigned int j = 0, i = 0;
  unsigned int tInt = 0;
  bool foundlink = false;

  while (1) {
    ret = fscanf(fptr, "%s", line);
    j++;

    if(ret == EOF)
      break;
    
    if(i == NIntsPerLink)
      break;

    
    if( foundlink && i < NIntsPerLink){
      //std::cout << line <<" " << std::endl;
      //if(i%6==0)
      //std::cout<<endl;
      sscanf(line, "%x", &tInt);
      tempBuffer[i] = tInt;
      i++;
    }

    if(strcmp(line,wordLink)==0){
      ret = fscanf(fptr, "%s", line);
      if (strcmp(line,searchTerm)==0){
	foundlink = true;
	//std::cout<<"FOUND LINK!!!!!! "<< searchTerm<< " "<< line<< " At line "<< j <<std::endl;
      }
    }
  }

  fclose(fptr);
  return true;
  
}

void CTP7ToDigi::printLinksToFile(){
  char outputFile[40];
  sprintf(outputFile,"outputFile.txt");
  FILE *fptr = fopen(outputFile, "w");
  for(unsigned int j = 0; j < NILinks; j++){
    fprintf( fptr, "\nlink %i\n", j );
    for(unsigned int i = 0; i< NIntsPerLink; i++){
      fprintf( fptr, "%x ", buffer[j][i] );
      if(i%6==5)
	fputs("\n",fptr);
    }
  }

  fclose(fptr);

}

/*
 * Check CTP7 Capture Status to see if Capture was Successful
 * After 5 attempts return false. TCP/IP communication takes 
 */

bool CTP7ToDigi::waitForCaptureSuccess(){

  int nAttempts = 0;
  CTP7::CaptureStatus captureStatusTemp;
  CTP7::CaptureStatus *captureStatus;
  captureStatus = &captureStatusTemp;

  while(nAttempts<5){

    ctp7Client->getCaptureStatus(captureStatus);

    if(*captureStatus == CTP7::Done)
      return true;

    nAttempts++;
  }

  return false;
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
  desc.addUntracked<bool>("test", false)->setComment("Test or normal running?");
}

//define this as a plug-in
DEFINE_FWK_MODULE(CTP7ToDigi);
