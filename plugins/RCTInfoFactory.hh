#ifndef RCTInfoFactory_hh
#define RCTInfoFactory_hh

#include "RCTInfo.hh"
#include <iostream>
#include <fstream>
#include <vector>

class RCTInfo;

class RCTInfoFactory {

public:

  RCTInfoFactory() : verbose(false) {;}
  ~RCTInfoFactory() {;}

  bool decodeCapturedLinkID(unsigned int capturedValue, unsigned int & crateNumber, unsigned int & linkNumber, bool & even);

  bool compare(RCTInfo rctInfoA, 
	       RCTInfo rctInfoB, 
	       RCTInfo rctInfoDiff, 
	       char ErrorMsg[100]);

  bool compareByWord(char * nameA, std::vector<unsigned int> &linkA, char * nameB, std::vector<unsigned int> &linkB, unsigned int BCoffset, bool log);

  bool produce(const std::vector <unsigned int> evenFiberData, 
	       const std::vector <unsigned int> oddFiberData,
	       std::vector <RCTInfo> &rctInfo);

  bool produce(const std::vector < std::vector <unsigned int> > cableData,
	       std::vector <RCTInfo> &rctInfo);

  bool setRCTInfoCrateID(std::vector<RCTInfo> &rctInfoVector, unsigned int crateID);

  bool printRCTInfo(const std::vector<RCTInfo> &rctInfo);

  bool printRCTInfoToFile(const std::vector<RCTInfo> &rctInfo, std::ofstream &fileName);
  //Compare Region and create error file output
  bool regionCompareFile(std::vector<RCTInfo> &rctInfoEmulator,
			 std::vector<RCTInfo> &rctInfoCapture,
			 unsigned int BCOffset, unsigned int crate);

  //Helper Functions
  unsigned int GetRegTenBits(RCTInfo rctInfo, unsigned int j, unsigned int k);
  unsigned int GetElectronTenBits(unsigned int Card, unsigned int Region, unsigned int Rank);

  void setVerbose() {verbose = true;}
  void setQuiet() {verbose = false;}

  //Readin in Pattern Test Files
  bool readPatternTestFile(char *textFile , std::vector<RCTInfo> &rctInfo, unsigned int inputCrateNumber = 99);
  bool scanInRegions(FILE *fptr, RCTInfo &rctInfo, int crateID);
  bool scanInHF(FILE *fptr, RCTInfo &rctInfo, int crateID);
  bool scanInEG(FILE *fptr, RCTInfo &rctInfo, int crateID);
  bool readFileSearch(char* textFile, int searchTerm , unsigned int nInts, unsigned int * Array);
  bool readInputTextFile(char* textFile,std::vector<RCTInfo> &rctInfo);
  bool timeStampChar( char timeStamp[80] );
  bool verbose;

private:

  // No copy constructor is needed
  RCTInfoFactory(const RCTInfoFactory&);

  // No equality operator is needed
  const RCTInfoFactory& operator=(const RCTInfoFactory&);

  // Helper functions
  bool verifyHammingCode(const unsigned char *data);



};

#endif
