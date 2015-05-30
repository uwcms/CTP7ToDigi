// -*- C++ -*-
//
// Package:    TestProducer/RCTToDigi
// Class:      RCTToDigi
// 
/**\class RCTToDigi RCTToDigi.cc TestProducer/RCTToDigi/plugins/RCTToDigi.cc

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
#include <sys/time.h>

// Framework stuff

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

// CTP7 access providers

#include "CTP7Client.hh"
#include "RCTInfoFactory.hh"
//#include "../src/L1CaloBXCollections.hh"

// RCT data formats
#include "DataFormats/L1CaloTrigger/interface/L1CaloEmCand.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegion.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegionDetId.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloCollections.h"

#include "DataFormats/L1TCalorimeter/interface/CaloEmCand.h"
#include "DataFormats/L1TCalorimeter/interface/CaloRegion.h"
#include "DataFormats/L1TCalorimeter/interface/CaloTower.h"

// Link Monitor Class

#include "CTP7Tests/LinkMonitor/interface/LinkMonitor.h"
#include "CTP7Tests/TimeMonitor/interface/TimeMonitor.h"

//utility
#include "Math/LorentzVector.h"
#include "DataFormats/L1Trigger/interface/L1Candidate.h"
using namespace std;

#define EVENT_HEADER_WORDS 6
#define CHANNEL_HEADER_WORDS 2
#define CHANNEL_DATA_WORDS_PER_BX 6
#define NIntsBRAMDAQ 1024*2
#define NLinks 36

// Scan in file

#include <fstream>

//
// class declaration
//


class RCTToDigi : public edm::EDProducer {
public:
  explicit RCTToDigi(const edm::ParameterSet&);
  ~RCTToDigi();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  virtual void beginJob() override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;
  int getLinkNumber(bool even, int crate);
  bool scanInDAQData(uint32_t tempBuffer[NIntsBRAMDAQ]);
  void printDAQToFile();
  bool waitForCaptureSuccess();
  bool decodeCapturedLinkID(uint32_t capturedValue, uint32_t &crateNumber, uint32_t &linkNumber, bool &even);
  bool getBXNumbers(const uint32_t L1aBCID, const uint32_t BXsInCapture, unsigned int BCs[5], uint32_t firstBX, uint32_t lastBX);
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
  
  uint32_t buffer[NIntsBRAMDAQ];

  int NEventsPerCapture;
  bool test;
  bool createDAQFile;

  char fileName[40];

};

//
// constants, enums and typedefs
//

const uint32_t NIntsPerFrame = 6;

//Fill a vector to fill LinkMonitorCollection later
typedef std::vector<uint32_t> LinkMonitorTmp;

struct crate_data{
  unsigned int crateID;
  unsigned int BXID;

  unsigned int ctp7LinkNumberEven;
  unsigned int ctp7LinkNumberOdd;

  //full 32-bit linkID
  unsigned int capturedLinkIDEven;
  unsigned int capturedLinkIDOdd;  

  std::vector <unsigned int> uintEven;
  std::vector <unsigned int> uintOdd;
};

struct link_data{
  unsigned int ctp7LinkNumber;
  //full 32-bit linkID
  unsigned int capturedLinkID;
  //decoded linkID from the oRSC
  unsigned int capturedLinkNumber;
  bool even;
  unsigned int crateID;
  std::vector <unsigned int> uint;
};


//
// static data member definitions
//

//
// constructors and destructor
//
RCTToDigi::RCTToDigi(const edm::ParameterSet& iConfig)
{

  // Get host and port
  ctp7Host = iConfig.getUntrackedParameter<std::string>("ctp7Host");
  ctp7Port = iConfig.getUntrackedParameter<std::string>("ctp7Port");
  NEventsPerCapture = iConfig.getUntrackedParameter<int>("NEventsPerCapture",5);
  test = iConfig.getUntrackedParameter<bool>("test",false);
  createDAQFile = iConfig.getUntrackedParameter<bool>("createDAQFile",false);
  testFile = iConfig.getUntrackedParameter<std::string>("testFile","testFile.txt");
  // Create CTP7Client to communicate with specified host/port 
  if(!test)
    ctp7Client = new CTP7Client(ctp7Host.c_str(), ctp7Port.c_str());

  //set test file name here, shoudl be added as an untrackedParamater
  sprintf(fileName,testFile.c_str());

  //register your products
  produces<L1CaloEmCollection>();
  produces<L1CaloRegionCollection>();
  produces<LinkMonitorCollection>();
  produces<TimeMonitorCollection>();
}


RCTToDigi::~RCTToDigi()
{
  // Close CTP7Client connection
  if(ctp7Client != 0) delete ctp7Client;
}



//
// member functions
//

// ------------ method called to produce the data  ------------
// NOTICE: A Number of data checks are performed, if data appears
//         corrupted then this will produce nothing.
//

void
RCTToDigi::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  RCTInfoFactory rctInfoFactory;

  std::auto_ptr<L1CaloEmCollection> rctEMCands(new L1CaloEmCollection);
  std::auto_ptr<L1CaloRegionCollection> rctRegions(new L1CaloRegionCollection);

  //LinkMonitorCollection Final Output Collection
  std::auto_ptr<LinkMonitorCollection> rctLinkMonitor(new LinkMonitorCollection);

  //Grab the link vector by first filling a different class-less type
  std::auto_ptr<LinkMonitorTmp> rctLinksTmp(new LinkMonitorTmp);


  static uint32_t index = 0;
  static uint32_t countCycles = 0;
  static uint32_t loopEvents = 0;

  countCycles++;
  cout<<"Capture number: "<<dec<<countCycles<<endl;

  if(!test){ // normal mode
    if(!ctp7Client->checkConnection()){
      cout<<"CTP7 Check Connection FAILED!!!! If you are trying "; 
      cout<<"to capture data from CTP7, think again!"<<endl;
      cout<<"Exiting..."<<endl; exit(0);
    }
    ctp7Client->setValue( CTP7::daqSpyCaptureRegisters, 0,1);
    
    int nAttempts = 0;
    while(1 != ctp7Client->getValue(CTP7::daqSpyCaptureRegisters, 1)){
      usleep(5);
      if(nAttempts>20){
	cout<<"--------- Capture Failed, check if Run is going, Exiting. ----------"<<endl;
	  exit(0);
      }
    }

    if(!ctp7Client->getValues(CTP7::daqBuffer,0,2047,buffer)){
      cerr << "RCTToDigi::produce() Error reading DAQ from CTP7" << endl;
    }

    //Fill the rctLinksTmp 
    ctp7Client->dumpStatus(*rctLinksTmp);
    
    for (uint32_t i = 0; i < rctLinksTmp->size() ; i++){
      rctLinkMonitor->push_back(LinkMonitor(rctLinksTmp->at(i)));
    }
    
  }
  else { // test mode
    cout <<"TESTING MODE"<<endl;
    if(!scanInDAQData(buffer)){
      cerr << "RCTToDigi::produce() Error reading from file: " << testFile << endl;
    }
  }
  
  if(createDAQFile)
    printDAQToFile();


  // Dump DAQ Buffer and decode into individual crate even and odd link data
  // channels to make rctInfo buffer, and from that make rctEMCands and rctRegions
  uint32_t nBX = 0; 
  uint32_t ctp7FWVersion;
  uint32_t L1ID, L1aBCID;
  uint32_t BCs[5] = {0}; //5 BCs is max readout
     
  L1ID          =  buffer[1];                      // extract the L1 ID number
  L1aBCID       =  buffer[5] & 0x00000FFF;         // extract the BCID number of L1A
  nBX           = (buffer[5] & 0x00FF0000) >> 16;  // extract number of BXs readout per L1A 
  ctp7FWVersion =  buffer[4];
  
  cout << "L1ID = " << L1ID << " L1A BCID = " << L1aBCID << " BXs in capture = " << nBX << " CTP7 DAQ FW = " << ctp7FWVersion<<endl;
  
  //getBXNumbers handles special cases, for example:
  //if L1A is 3563, nBX = 3 then BCs = 3562, 3563, 0
  uint32_t firstBX = 0; uint32_t lastBX = 1; 
  getBXNumbers(L1aBCID, nBX, BCs, firstBX, lastBX);
  
  //rctRegions->setBXRange(0, nBX);
  //rctEMCands->setBXRange(0, nBX);
  
  std::vector<RCTInfo> allCrateRCTInfo[nBX];
  
  link_data allLinks[nBX][NLinks];
  
  uint32_t iDAQBuffer = 0;
  
  //Step 1: Grab all data from ctp7 buffer and put into link format
  for(unsigned int iLink = 0; iLink < NLinks; iLink++ ){
    
    iDAQBuffer = EVENT_HEADER_WORDS + iLink * (CHANNEL_HEADER_WORDS + 
					       nBX * CHANNEL_DATA_WORDS_PER_BX);
    
    //first decode linkID 
    uint32_t linkID     = buffer[iDAQBuffer++];
    uint32_t tmp        = buffer[iDAQBuffer++];
    uint32_t CRCErrCnt  =  tmp & 0x0000FFFF;
    //uint32_t linkStatus = (tmp & 0xFFFF0000) >> 16;
    //cout<<"linkID "<<std::hex<<linkID<<" CRCErrCnt "<<CRCErrCnt<<endl;
    if(CRCErrCnt!=0)
      std::cout<<"WARNING CRC ErrorFound"<<std::endl;
    
    uint32_t capturedLinkID = 0;
    uint32_t crateID = 0;
    uint32_t capturedLinkNumber = 0;
    bool even = false;
    
    //Using normal method for decoding oRSC Captured LinkID
    //Input linkID, output: crateID, capturedLinkNumber, Even or Odd
    decodeCapturedLinkID(linkID, crateID, capturedLinkNumber, even);
    
    // Loop over multiple BX                                                                        
    for (uint32_t iBX=0; iBX<nBX; iBX++){
      //std::cout<<"iBX "<<iBX<<endl;
      allLinks[iBX][iLink].uint.reserve(6);
      allLinks[iBX][iLink].ctp7LinkNumber     = iLink;
      allLinks[iBX][iLink].capturedLinkID     = capturedLinkID;
      allLinks[iBX][iLink].crateID            = crateID;
      allLinks[iBX][iLink].capturedLinkNumber = capturedLinkNumber;
      allLinks[iBX][iLink].even               = even;
      //std::cout<<std::dec<<"iLink "<<iLink<<" crateID "<<crateID<<" even "<<even<<std::endl;   

      for(unsigned int iWord = 0; iWord < 6 ; iWord++ ){
	//printf("0x%8.8X ", buffer[iDAQBuffer+iWord+iBX*6]);    
	allLinks[iBX][iLink].uint.push_back(buffer[iDAQBuffer+iWord+iBX*6]);
	//allLinks[iLink][iBX].uint.push_back(buffer[iDAQBuffer+iWord]);
      }
      //cout<<endl;
    }
  }

  //Step 2: Dynamically match links and create RCTInfo Objects 
  uint32_t nCratesFound = 0;
  for(unsigned int iCrate = 0; iCrate < 18 ; iCrate++){
    
    bool foundEven = false, foundOdd = false;
    link_data even[nBX];
    link_data odd[nBX];
    
    for(unsigned int iLink = 0; iLink < NLinks; iLink++){
      
      if( (allLinks[0][iLink].crateID==iCrate) && (allLinks[0][iLink].even == true) ){
	
	foundEven = true;
	for (unsigned int iBX=0; iBX<nBX; iBX++)
	  even[iBX] = allLinks[iBX][iLink];
	
      }
      else if( (allLinks[0][iLink].crateID==iCrate) && (allLinks[0][iLink].even == false) ){
	
	foundOdd = true;
	for (unsigned int iBX=0; iBX<nBX; iBX++)
	  odd[iBX] = allLinks[iBX][iLink];
	
      } 
      //if success then create RCTInfoVector and fill output object
      if(foundEven && foundOdd){
	nCratesFound++;
	   
	//fill rctInfoVector for all BX read out
	for (unsigned int iBX=0; iBX<nBX; iBX++){
	  RCTInfoFactory rctInfoFactory;
	  std::vector <RCTInfo> rctInfoData;
	  if(!rctInfoFactory.produce(even[iBX].uint, odd[iBX].uint, rctInfoData)){
	    std::cout<<"Failed to produce data; corrupted data? Exiting."<<std::endl;
	    return;
	  }
	  rctInfoFactory.setRCTInfoCrateID(rctInfoData, iCrate);
	  allCrateRCTInfo[iBX].push_back(rctInfoData.at(0));
	}
	break;
      }
    }
  }
  
  if(nCratesFound != 18)
    cerr << "Warning -- only found "<< nCratesFound << " valid crates";
  
  //Step 3: Create Collections from RCTInfo Objects  
  for (uint32_t iBX=0; iBX<nBX; iBX++){
    
    int bx = BCs[iBX];
    for(unsigned int iCrate = 0; iCrate < nCratesFound; iCrate++ ){

      RCTInfo rctInfo = allCrateRCTInfo[iBX].at(iCrate);
      RCTInfoFactory rctInfoFactory;

      rctInfoFactory.printRCTInfo(allCrateRCTInfo[iBX]);
      //Use Crate ID to identify eta/phi of candidate
      for(int j = 0; j < 4; j++) {

	L1CaloEmCand em = L1CaloEmCand(rctInfo.neRank[j], 
				       rctInfo.neRegn[j], 
				       rctInfo.neCard[j], 
				       rctInfo.crateID, 
				       false);
	em.setBx(iBX);
	rctEMCands->push_back(em);
	
      }
      
      for(int j = 0; j < 4; j++) {
	
	L1CaloEmCand em = L1CaloEmCand(rctInfo.ieRank[j], 
				       rctInfo.ieRegn[j], 
				       rctInfo.ieCard[j], 
				       rctInfo.crateID, 
				       true);
	
	em.setBx(iBX);
	rctEMCands->push_back( em);
      }

	 for(int j = 0; j < 7; j++) {

	   for(int k = 0; k < 2; k++) {
	     
	     bool o = (((rctInfo.oBits >> (j * 2 + k)) & 0x1) == 0x1);
	     bool t = (((rctInfo.tBits >> (j * 2 + k)) & 0x1) == 0x1);
	     bool m = (((rctInfo.mBits >> (j * 2 + k)) & 0x1) == 0x1);
	     bool q = (((rctInfo.qBits >> (j * 2 + k)) & 0x1) == 0x1);

	     L1CaloRegion rgn = L1CaloRegion(rctInfo.rgnEt[j][k], o, t, m, q, rctInfo.crateID , j, k);
	     std::cout<<"rgn pt "<< rgn.et()<<std::endl;
	     rgn.setBx(iBX);
	     rctRegions->push_back( rgn);
	   }
	 }

	 for(int j = 0; j < 2; j++) {

	   for(int k = 0; k < 4; k++) {

	     bool fg=(((rctInfo.hfQBits>> (j * 4 + k)) & 0x1)  == 0x1); 

	     L1CaloRegion rgn = L1CaloRegion(rctInfo.hfEt[j][k], fg,  rctInfo.crateID , (j * 4 +  k));
	     rgn.setBx(iBX);
	     rctRegions->push_back(rgn);
	   }
	 }
    }
  }
     

  iEvent.put(rctEMCands);
  iEvent.put(rctRegions);
  //iEvent.put(rctLinkMonitor);
  //iEvent.put(rctTime);

  cout <<dec<< "RCTToDigi::produce() " << index << endl;

  index += NIntsPerFrame;

  // index and "loopEvents" cannot be the same. loopEvents needs to increase by one, while index is used in evenFiberData and is increased by NIntsPerFrame 
  // this part needs debugging!

  uint32_t MINIMUM= NEventsPerCapture ;   // The min was a mistake like it was (it made us capture too often for the pattern, we repeat events).
                                          // Make sure for pattern tests only 64 events are run, but set in the configuration file, not only here
                                          // To be revised: MINIMUM=std::min( (int) NIntsBRAMDAQ, NEventsPerCapture);
  if(loopEvents >= MINIMUM) loopEvents = 0;  
  else loopEvents++;
}


  bool 
  RCTToDigi::getBXNumbers(const uint32_t L1aBCID, const uint32_t BXsInCapture, unsigned int BCs[5], uint32_t firstBX, uint32_t lastBX) {

  if (BXsInCapture > 3) {
    if (L1aBCID == 0){
      BCs[0]=3562; BCs[1]=3563; BCs[2]=0; BCs[3]=1; BCs[4]=2;
    }
    else if (L1aBCID == 1){
      BCs[0]=3563; BCs[1]=0; BCs[2]=1; BCs[3]=2; BCs[4]=3;
    }
    else{
      BCs[0]= L1aBCID - 2;
      BCs[1]= L1aBCID - 1;
      BCs[2]= L1aBCID - 0;
      BCs[3]= L1aBCID + 1;
      BCs[4]= L1aBCID + 2;
    }
  } else if (BXsInCapture > 1) {
    if (L1aBCID == 0){
      BCs[0]=3563;
      BCs[1]=0;
      BCs[2]=1;

    }
    else{
      BCs[0]= L1aBCID - 1;
      BCs[1]= L1aBCID - 0;
      BCs[2]= L1aBCID + 1;

    }
  }
  else{
    BCs[0]= L1aBCID;
  }

  firstBX = BCs[0];

  //check if BXs in Capture greater than 5
  if(BXsInCapture<5) lastBX  = BCs[BXsInCapture-1];
  else lastBX = 0;

  return true;
}

  bool RCTToDigi::decodeCapturedLinkID(uint32_t capturedValue, uint32_t &crateNumber, uint32_t &linkNumber, bool &even)
  {
    
    //if crateNumber not valid set to 0xFF
    crateNumber = ( capturedValue >> 8 ) & 0xFF;
    if(crateNumber > 17)
      crateNumber = 0xFF;
    
    //if linkNumber not valid set to 0xFF
    linkNumber = ( capturedValue ) & 0xFF;

    if(linkNumber > 12)
      linkNumber = 0xFF;
    
    if((linkNumber&0x1) == 0)
      even=true;
    else
      even=false;
    
    return true;
  };

/*
 * This reads in a file of form testFile.txt (see test directory for example)
 * and outputs a dump of the same form that would be received by a CTP7 Capture
 */
 
bool RCTToDigi::scanInDAQData(uint32_t tempBuffer[NIntsBRAMDAQ]){

  FILE *fptr = fopen(fileName, "r");

  if(fptr == NULL) {
    cout<<"Error: Could not open emulator input file "<<endl;
    return false;
  } 


  int ret = 0; 

  int i = 0;
  unsigned int tInt = 0;
  char line[100];

  while (1) {
    ret = fscanf(fptr, "%s", line);

    if(ret == EOF)
      break;
    
    if(i == NIntsBRAMDAQ)
      break;

    sscanf(line, "%x", &tInt);
    tempBuffer[i] = tInt;
    //std::cout<<"i "<< i<< " "<<std::hex<< tempBuffer[i] <<endl;
    i++;
  }

  fclose(fptr);
  return true;
  
}

void RCTToDigi::printDAQToFile(){
  char outputFile[40];
  sprintf(outputFile,"outputFileDAQ.txt");
  FILE *fptr = fopen(outputFile, "w");
  for(unsigned int i = 0; i< NIntsBRAMDAQ; i++){
    fprintf( fptr, "%08x ", buffer[i] );
    //printf("%08x ", buffer[i]);
    if(i%6==5)
      fputs("\n",fptr);
  }

  fclose(fptr);

}

/*
 * Check CTP7 Capture Status to see if Capture was Successful
 * After 5 attempts return false. TCP/IP communication takes 
 */

bool RCTToDigi::waitForCaptureSuccess(){

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
RCTToDigi::beginJob()
{
  cout << "RCTToDigi::beginJob()" << endl;
}

// ------------ method called once each job just after ending the event loop  ------------
void 
RCTToDigi::endJob() {
  cout << "RCTToDigi::endJob()" << endl;
}

// ------------ method called when starting to processes a run  ------------

void
RCTToDigi::beginRun(edm::Run const&, edm::EventSetup const&)
{
  cout << "RCTToDigi::beginRun()" << endl;
}

 
// ------------ method called when ending the processing of a run  ------------

void
RCTToDigi::endRun(edm::Run const&, edm::EventSetup const&)
{
  cout << "RCTToDigi::endRun()" << endl;
}

 
// ------------ method called when starting to processes a luminosity block  ------------

void
RCTToDigi::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
  cout << "RCTToDigi::beginLuminosityBlock()" << endl;
}

 
// ------------ method called when ending the processing of a luminosity block  ------------

void
RCTToDigi::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
  cout << "RCTToDigi::endLuminosityBlock()" << endl;
}
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
RCTToDigi::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setComment("Creates events using data captured in the CTP7 buffers");
  desc.addUntracked<std::string>("ctp7Host", "localhost")->setComment("CTP7 TCP/IP host name");
  desc.addUntracked<std::string>("ctp7Port", "5555")->setComment("CTP7 TCP/IP port name");
  desc.addUntracked<bool>("test", false)->setComment("Test or normal running?");
}

//define this as a plug-in
DEFINE_FWK_MODULE(RCTToDigi);
