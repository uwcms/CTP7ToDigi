#ifndef RCTInfoFactory_hh
#define RCTInfoFactory_hh

#include "RCTInfo.hh"
#include <iostream>
#include <fstream>
#include <vector>

class RCTInfo;

class RCTInfoFactory {

public:

  enum EGError {NONE=0, RANK_SATURATED, RANK, ORDER, MISSING};

  RCTInfoFactory() : verbose(false) {;}
  ~RCTInfoFactory() {;}

  bool decodeCapturedLinkID(unsigned int capturedValue, unsigned int & crateNumber, unsigned int & linkNumber, bool & even);
  bool setRCTInfoCrateID(std::vector<RCTInfo> &rctInfoVector, unsigned int crateID);
  bool produce(const std::vector <unsigned int> evenFiberData, 
	       const std::vector <unsigned int> oddFiberData,
	       std::vector <RCTInfo> &rctInfo);

  bool produce(const std::vector < std::vector <unsigned int> > cableData,
	       std::vector <RCTInfo> &rctInfo);

  bool printRCTInfo(const std::vector<RCTInfo> &rctInfo);

  bool printRCTInfoToFile(const std::vector<RCTInfo> &rctInfo, std::ofstream &fileName);

  //Helper Functions
  unsigned int GetRegTenBits(RCTInfo rctInfo, unsigned int j, unsigned int k);
  unsigned int GetElectronTenBits(unsigned int Card, unsigned int Region, unsigned int Rank);

  void setVerbose() {verbose = true;}
  void setQuiet() {verbose = false;}

  bool timeStampChar( char timeStamp[80] );
  bool timeStampCharTime( char  timeStamp[80] );
  bool timeStampCharDate( char  timeStamp[80] );
  bool GetUserName(char * username);
  bool verbose;

private:

  // No copy constructor is needed
  RCTInfoFactory(const RCTInfoFactory&);

  // No equality operator is needed
  const RCTInfoFactory& operator=(const RCTInfoFactory&);

  // Helper functions
  bool verifyHammingCode(const unsigned char *data);

  bool verifyBXBytes(const unsigned int &evenFiber, const unsigned int &oddFiber);

  bool inAbortGap(const unsigned int &evenFiber, const unsigned int &oddFiber);

};

#endif
