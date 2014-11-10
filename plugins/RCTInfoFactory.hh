#ifndef RCTInfoFactory_hh
#define RCTInfoFactory_hh

#include "RCTInfo.hh"
#include <vector>

class RCTInfo;

class RCTInfoFactory {

public:

  RCTInfoFactory() : verbose(false) {;}
  ~RCTInfoFactory() {;}

  bool compare(RCTInfo rctInfoA, 
	       RCTInfo rctInfoB, 
	       RCTInfo rctInfoDiff, 
	       char ErrorMsg[100]);

  bool produce(const std::vector <unsigned int> evenFiberData, 
	       const std::vector <unsigned int> oddFiberData,
	       std::vector <RCTInfo> &rctInfo);

  bool produce(const std::vector < std::vector <unsigned int> > cableData,
	       std::vector <RCTInfo> &rctInfo);

  bool printRCTInfo(const std::vector<RCTInfo> &rctInfo);

  //Two Helper Functions
  unsigned int GetRegTenBits(RCTInfo rctInfo, unsigned int j, unsigned int k);
  unsigned int GetElectronTenBits(unsigned int Card, unsigned int Region, unsigned int Rank);

  /*
  bool compare(RCTInfo rctInfoA, 
	       RCTInfo rctInfoB, 
	       RCTInfo rctInfoDiff, 
	       char *ErrorMsg);
  */
  void setVerbose() {verbose = true;}
  void setQuiet() {verbose = false;}

private:

  // No copy constructor is needed
  RCTInfoFactory(const RCTInfoFactory&);

  // No equality operator is needed
  const RCTInfoFactory& operator=(const RCTInfoFactory&);

  // Helper functions

  bool verifyHammingCode(const unsigned char *data);

  bool verbose;

};

#endif
