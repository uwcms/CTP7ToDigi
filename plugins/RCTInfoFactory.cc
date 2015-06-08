#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;
#include "RCTInfo.hh"

#include "RCTInfoFactory.hh"

/*
 * This class contains tools to take bit information and extract object information
 * from oRSC capture RAMs and oRSC output (to CTP7/MP7)
 * As well as tools to make bit comparison easier
 *
 * June 2014
 */

//captured Value is the captured link ID on the CTP7/MP7 and the remaining values
//are returned decoded values.
bool RCTInfoFactory::decodeCapturedLinkID(unsigned int capturedValue, unsigned int & crateNumber, unsigned int & linkNumber, bool & even)
{

  crateNumber = ( capturedValue >> 8 ) & 0xFF;
  //if crateNumber not valid set to 0xFF
  if(crateNumber > 17)
    crateNumber = 0xFF;

  linkNumber = ( capturedValue ) & 0xFF;
  //if linkNumber not valid set to 0xFF
  if(linkNumber > 12)
    linkNumber = 0xFF;

  //currently 0-5 are odd and 6-11 are even
  //std::cout<<"linkNumber "<<linkNumber<< " " << (linkNumber & 0x1) <<std::endl;
  if((linkNumber&0x1) == 0)
    even=true;
  else
    even=false;

  return true;
  };

bool RCTInfoFactory::setRCTInfoCrateID(std::vector<RCTInfo> &rctInfoVector, unsigned int crateID)
{
  for( unsigned int i = 0 ; i < rctInfoVector.size() ; i++ ){
    rctInfoVector.at(i).crateID = crateID;
  }

  return true;

}

/*
 * Extract RCT Object Info from oRSC Output Fibers
 * Primarily intended for use on CTP7/MP7
 */

bool RCTInfoFactory::produce(const std::vector <unsigned int> evenFiberData, 
			     const std::vector <unsigned int> oddFiberData,
			     std::vector <RCTInfo> &rctInfoData) {
  static int nPrintOuts = 0;
  // Ensure that there is data to process
  unsigned int nWordsToProcess = evenFiberData.size();
  unsigned int remainder = nWordsToProcess%6;
  if(nWordsToProcess != oddFiberData.size()) {
    std::cerr << "RCTInfoFactory::produce -- even and odd fiber sizes are different!" << std::endl;
    return false;
  }
  if(nWordsToProcess == 0|| nWordsToProcess/6 == 0) {
    std::cerr << "RCTInfoFactory::produce -- evenFiberData is null :(" << std::endl;
    return false;
  }
  else if((nWordsToProcess % 6) != 0) {
    nWordsToProcess=nWordsToProcess-remainder;
  }

  // Extract RCTInfo

  unsigned int nBXToProcess = nWordsToProcess / 6;

  for(unsigned int iBX = 0; iBX < nBXToProcess; iBX++) {
    // Check hamming codes for data -- nevertheless continue
    if(!verifyHammingCode((unsigned char *) &evenFiberData[iBX * 6])) {
      std::cerr << "Hamming code failed for even fiber for bunch crossing" << iBX << std::endl;
    }
    if(!verifyHammingCode((unsigned char *) &oddFiberData[iBX * 6])) {
      std::cerr << "Hamming code failed for odd fiber for bunch crossing" << iBX << std::endl;
    }

    if(inAbortGap( evenFiberData[iBX * 6 ], oddFiberData[iBX * 6 ])) {
      if(nPrintOuts<10)
	std::cout<<"First word is 0x505050BC. Appears we are in the Abort Gap. Skipping."<<std::endl;
      nPrintOuts++;
      return false;
    }

    if(!verifyBXBytes( evenFiberData[iBX * 6 ], oddFiberData[iBX * 6 ])) {
      std::cerr << "Error BX Byte is not 0x7C or 0x3C --- Discarding this capture!! " <<std::hex<< evenFiberData[iBX * 6 ] << " " << oddFiberData[iBX * 6 ] << std::endl;
      std::cerr << "Possibly this is due to a single dropped packet or something worse is wrong"<< std::endl;
      rctInfoData.clear();
      return false;
    }
    RCTInfo rctInfo;
    // We extract into rctInfo the data from RCT crate
    // Bit field description can be found in the spreadsheet:
    // https://twiki.cern.ch/twiki/pub/CMS/ORSCOperations/oRSCFiberDataSpecificationV5.xlsx
    // Even fiber bits contain 4x4 region information
    rctInfo.rgnEt[0][0]  = (evenFiberData[iBX * 6 + 0] & 0x0003FF00) >>  8;
    rctInfo.rgnEt[0][1]  = (evenFiberData[iBX * 6 + 0] & 0x0FFC0000) >> 18;
    rctInfo.rgnEt[1][0]  = (evenFiberData[iBX * 6 + 0] & 0xF0000000) >> 28;
    rctInfo.rgnEt[1][0] |= (evenFiberData[iBX * 6 + 1] & 0x0000003F) <<  4;
    rctInfo.rgnEt[1][1]  = (evenFiberData[iBX * 6 + 1] & 0x0000FFC0) >>  6;
    rctInfo.rgnEt[2][0]  = (evenFiberData[iBX * 6 + 1] & 0x03FF0000) >> 16;
    rctInfo.rgnEt[2][1]  = (evenFiberData[iBX * 6 + 1] & 0xFC000000) >> 26;
    rctInfo.rgnEt[2][1] |= (evenFiberData[iBX * 6 + 2] & 0x0000000F) <<  6;
    rctInfo.rgnEt[3][0]  = (evenFiberData[iBX * 6 + 2] & 0x00003FF0) >>  4;
    rctInfo.rgnEt[3][1]  = (evenFiberData[iBX * 6 + 2] & 0x00FFC000) >> 14;
    rctInfo.rgnEt[4][0]  = (evenFiberData[iBX * 6 + 2] & 0xFF000000) >> 24;
    rctInfo.rgnEt[4][0] |= (evenFiberData[iBX * 6 + 3] & 0x00000003) <<  8;
    rctInfo.rgnEt[4][1]  = (evenFiberData[iBX * 6 + 3] & 0x00000FFC) >>  2;
    rctInfo.rgnEt[5][0]  = (evenFiberData[iBX * 6 + 3] & 0x003FF000) >> 12;
    rctInfo.rgnEt[5][1]  = (evenFiberData[iBX * 6 + 3] & 0xFFC00000) >> 22;
    rctInfo.rgnEt[6][0]  = (evenFiberData[iBX * 6 + 4] & 0x000003FF) >>  0;
    rctInfo.rgnEt[6][1]  = (evenFiberData[iBX * 6 + 4] & 0x000FFC00) >> 10;
    rctInfo.tBits  = (evenFiberData[iBX * 6 + 4] & 0xFFF00000) >> 20;
    rctInfo.tBits |= (evenFiberData[iBX * 6 + 5] & 0x00000003) << 12; //bug? 4 to 5
    rctInfo.oBits  = (evenFiberData[iBX * 6 + 5] & 0x0000FFFC) >>  2;
    rctInfo.c4BC0  = (evenFiberData[iBX * 6 + 5] & 0x000C0000) >> 18;
    rctInfo.c5BC0  = (evenFiberData[iBX * 6 + 5] & 0x00300000) >> 20;
    rctInfo.c6BC0  = (evenFiberData[iBX * 6 + 5] & 0x00C00000) >> 22;
    // Odd fiber bits contain 2x1, HF and other miscellaneous information
    rctInfo.hfEt[0][0]  = (oddFiberData[iBX * 6 + 0] & 0x0000FF00) >>  8;
    rctInfo.hfEt[0][1]  = (oddFiberData[iBX * 6 + 0] & 0x00FF0000) >> 16;
    rctInfo.hfEt[1][0]  = (oddFiberData[iBX * 6 + 0] & 0xFF000000) >> 24;
    rctInfo.hfEt[1][1]  = (oddFiberData[iBX * 6 + 1] & 0x000000FF) >>  0;
    rctInfo.hfEt[0][2]  = (oddFiberData[iBX * 6 + 1] & 0x0000FF00) >>  8;
    rctInfo.hfEt[0][3]  = (oddFiberData[iBX * 6 + 1] & 0x00FF0000) >> 16;
    rctInfo.hfEt[1][2]  = (oddFiberData[iBX * 6 + 1] & 0xFF000000) >> 24;
    rctInfo.hfEt[1][3]  = (oddFiberData[iBX * 6 + 2] & 0x000000FF) >>  0;
    rctInfo.hfQBits     = (oddFiberData[iBX * 6 + 2] & 0x0000FF00) >>  8;
    rctInfo.ieRank[0]   = (oddFiberData[iBX * 6 + 2] & 0x003F0000) >> 16;
    rctInfo.ieRegn[0]   = (oddFiberData[iBX * 6 + 2] & 0x00400000) >> 22;
    rctInfo.ieCard[0]   = (oddFiberData[iBX * 6 + 2] & 0x03800000) >> 23; //bug? 25 to 23
    rctInfo.ieRank[1]   = (oddFiberData[iBX * 6 + 2] & 0xFC000000) >> 26;
    rctInfo.ieRegn[1]   = (oddFiberData[iBX * 6 + 3] & 0x00000001) >>  0;
    rctInfo.ieCard[1]   = (oddFiberData[iBX * 6 + 3] & 0x0000000E) >>  1;
    rctInfo.ieRank[2]   = (oddFiberData[iBX * 6 + 3] & 0x000003F0) >>  4;
    rctInfo.ieRegn[2]   = (oddFiberData[iBX * 6 + 3] & 0x00000400) >> 10;
    rctInfo.ieCard[2]   = (oddFiberData[iBX * 6 + 3] & 0x00003800) >> 11;
    rctInfo.ieRank[3]   = (oddFiberData[iBX * 6 + 3] & 0x000FC000) >> 14;
    rctInfo.ieRegn[3]   = (oddFiberData[iBX * 6 + 3] & 0x00100000) >> 20;
    rctInfo.ieCard[3]   = (oddFiberData[iBX * 6 + 3] & 0x00E00000) >> 21;
    rctInfo.neRank[0]   = (oddFiberData[iBX * 6 + 3] & 0x3F000000) >> 24; 
    rctInfo.neRegn[0]   = (oddFiberData[iBX * 6 + 3] & 0x40000000) >> 30;
    rctInfo.neCard[0]   = (oddFiberData[iBX * 6 + 3] & 0x80000000) >> 31; 
    rctInfo.neCard[0]  |= (oddFiberData[iBX * 6 + 4] & 0x00000003) <<  1; //bug? >> 0 to << 1
    rctInfo.neRank[1]   = (oddFiberData[iBX * 6 + 4] & 0x000000FC) >>  2;
    rctInfo.neRegn[1]   = (oddFiberData[iBX * 6 + 4] & 0x00000100) >>  8;
    rctInfo.neCard[1]   = (oddFiberData[iBX * 6 + 4] & 0x00000E00) >>  9;
    rctInfo.neRank[2]   = (oddFiberData[iBX * 6 + 4] & 0x0003F000) >> 12;
    rctInfo.neRegn[2]   = (oddFiberData[iBX * 6 + 4] & 0x00040000) >> 18;
    rctInfo.neCard[2]   = (oddFiberData[iBX * 6 + 4] & 0x00380000) >> 19;
    rctInfo.neRank[3]   = (oddFiberData[iBX * 6 + 4] & 0x0FC00000) >> 22;
    rctInfo.neRegn[3]   = (oddFiberData[iBX * 6 + 4] & 0x10000000) >> 28;
    rctInfo.neCard[3]   = (oddFiberData[iBX * 6 + 4] & 0xE0000000) >> 29;
    rctInfo.mBits       = (oddFiberData[iBX * 6 + 5] & 0x00003FFF) >>  0;
    rctInfo.c1BC0       = (oddFiberData[iBX * 6 + 5] & 0x00030000) >> 16;
    rctInfo.c2BC0       = (oddFiberData[iBX * 6 + 5] & 0x000C0000) >> 18;
    rctInfo.c3BC0       = (oddFiberData[iBX * 6 + 5] & 0x00300000) >> 20;
    unsigned int oddFiberc4BC0 = (oddFiberData[iBX * 6 + 5] & 0x00C00000) >> 22;
    if(oddFiberc4BC0 != rctInfo.c4BC0) {
      std::cerr << "Even and odd fibers do not agree on cable 4 BC0 mark :(" << std::endl;
    }

    //Adding in extra function to make comparison of the region tau and overflow bits easier
    for(int i = 0; i < 7; i++) 
      for(int j = 0; j < 2; j++) 
	rctInfo.rgnEtTenBit[i][j] = GetRegTenBits(rctInfo, i, j);

    for(int j = 0; j <4; j++){
      rctInfo.ieTenBit[j] = GetElectronTenBits( rctInfo.ieCard[j] , rctInfo.ieRegn[j] , rctInfo.ieRank[j] );
      rctInfo.neTenBit[j] = GetElectronTenBits( rctInfo.neCard[j] , rctInfo.neRegn[j] , rctInfo.neRank[j] );
    }

    rctInfoData.push_back(rctInfo);
  }
  return true;

}

unsigned int RCTInfoFactory::GetElectronTenBits(unsigned int Card, unsigned int Region, unsigned int Rank)
{
/* 
 * Function to return the 10 bits from electron ET
 * 3 bits Card   1 bit Region   6 bits Rank
 * Combine them and return an unsigned int
 */
  unsigned int ElectronBits;

  ElectronBits  = Card<<1;
  ElectronBits |= Region;
  ElectronBits = ElectronBits<<6;
  ElectronBits |= Rank;

  return ElectronBits;
}

unsigned int RCTInfoFactory::GetRegTenBits(RCTInfo rctInfo, unsigned int j, unsigned int k)
{

  /*
   * Shift bit to get correct tauBit/overflowBit 
   * RC       6     5     4     3     2     1     0
   * Region   1  0  1  0  1  0  1  0  1  0  1  0  1  0
   * Bit #   13 12 11 10  9  8  7  6  5  4  3  2  1  0
   * The tau bit and obit go in the 9th and 8th (respectively) place of the Region Bits
   */

  unsigned int bit=j*2+k; 
  unsigned int tbit=(((rctInfo.tBits>>bit)&0x1)<<1);
  unsigned int obit=((rctInfo.oBits>>bit)&0x1);
  unsigned int RegTenBits=(tbit|obit)<<10;

  RegTenBits |= rctInfo.rgnEt[j][k];

  return RegTenBits;
}

/*
 * Extract RCT Object Info from oRSC Capture RAMS
 * Takes as input 5 raw cable data in the form of a vector
 * Intended for use with oRSC Capture RAMs
 */

bool RCTInfoFactory::produce(const std::vector < std::vector < unsigned int > > rawCableData, std::vector< RCTInfo > &rctInfoData) {
  const unsigned int *ieArray = rawCableData[0].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  const unsigned int *neArray = rawCableData[1].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  const unsigned int *j3Array = rawCableData[2].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  const unsigned int *j4Array = rawCableData[3].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  const unsigned int *j5Array = rawCableData[4].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  const unsigned int *j6Array = rawCableData[5].data();  // [i] - cycle 0, [i+1] - cycle 1 of 80 MHz Clock
  for(unsigned int i = 0, j = 1, ii = 0; i < rawCableData[0].size(); i += 2, j += 2, ii += 6 ) {
    RCTInfo rct;
    // Data from Iso Cable 
    rct.ieRank[0] = 0x0000003F & ((ieArray[i]      )     ); // Bottom 6 bits are Rank of Electron0
    rct.ieRegn[0] = 0x00000001 & ((ieArray[i] >>  6)     ); // Next bit is Region of Electron0
    rct.ieCard[0] = 0x00000007 & ((ieArray[i] >>  7)     ); // Next 3 bits are Card of Electron0
    rct.ieRank[1] = 0x0000003F & ((ieArray[i] >> 10)     ); // Next 6 bits are Rank of Electron1
    rct.ieRegn[1] = 0x00000001 & ((ieArray[i] >> 16)     ); // Next bit is Region of Electron1
    rct.ieCard[1] = 0x00000007 & ((ieArray[i] >> 17)     ); // Next 3 bits are Card of Electron1
    rct.qBits     = 0x000000FF & ((ieArray[i] >> 20)     ); // Next 8 bits are MIP bits, but off by 0.5 BX per Pam. So, flip mip and quiet!
    rct.ieRank[2] = 0x0000003F & ((ieArray[j]      )     ); // Bottom 6 bits are Rank of Electron0
    rct.ieRegn[2] = 0x00000001 & ((ieArray[j] >>  6)     ); // Next bit is Region of Electron0
    rct.ieCard[2] = 0x00000007 & ((ieArray[j] >>  7)     ); // Next 3 bits are Card of Electron0
    rct.ieRank[3] = 0x0000003F & ((ieArray[j] >> 10)     ); // Next 6 bits are Rank of Electron1
    rct.ieRegn[3] = 0x00000001 & ((ieArray[j] >> 16)     ); // Next bit is Region of Electron1
    rct.ieCard[3] = 0x00000007 & ((ieArray[j] >> 17)     ); // Next 3 bits are Card of Electron1
    rct.mBits     = 0x000000FF & ((ieArray[j] >> 20)     ); // Next 8 bits are Q bits, but off by 0.5 BX per Pam. So, flip mip and quiet!
    rct.c1BC0     = 0x00000001 & ((ieArray[i] >> 31)     ); // Bits: Top bit
    rct.c1BC0    |= 0x00000002 & ((ieArray[j] >> 31) << 1); // Bits: Top bit -- set it as second bit of cable BC0
    // Data from NIso Cable 
    rct.neRank[0] = 0x0000003F & ((neArray[i]      )     ); // Bottom 6 bits are Rank of Electron0
    rct.neRegn[0] = 0x00000001 & ((neArray[i] >>  6)     ); // Next bit is Region of Electron0
    rct.neCard[0] = 0x00000007 & ((neArray[i] >>  7)     ); // Next 3 bits are Card of Electron0
    rct.neRank[1] = 0x0000003F & ((neArray[i] >> 10)     ); // Next 6 bits are Rank of Electron1
    rct.neRegn[1] = 0x00000001 & ((neArray[i] >> 16)     ); // Next bit is Region of Electron1
    rct.neCard[1] = 0x00000007 & ((neArray[i] >> 17)     ); // Next 3 bits are Card of Electron1
    rct.qBits    |= 0x00003F00 & ((neArray[i] >> 20) << 8); // Next 6 bits are MIP bits, but off by 0.5 BX per Pam. So, flip mip and quiet!
    rct.neRank[2] = 0x0000003F & ((neArray[j]      )     ); // Bottom 6 bits are Rank of Electron0
    rct.neRegn[2] = 0x00000001 & ((neArray[j] >>  6)     ); // Next bit is Region of Electron0
    rct.neCard[2] = 0x00000007 & ((neArray[j] >>  7)     ); // Next 3 bits are Card of Electron0
    rct.neRank[3] = 0x0000003F & ((neArray[j] >> 10)     ); // Next 6 bits are Rank of Electron1
    rct.neRegn[3] = 0x00000001 & ((neArray[j] >> 16)     ); // Next bit is Region of Electron1
    rct.neCard[3] = 0x00000007 & ((neArray[j] >> 17)     ); // Next 3 bits are Card of Electron1
    rct.mBits    |= 0x00003F00 & ((neArray[j] >> 20) << 8); // Next 6 bits are Q bits, but off by 0.5 BX per Pam. So, flip mip and quiet!
    rct.c2BC0     = 0x00000001 & ((neArray[i] >> 31)     ); // Bits: Top bit
    rct.c2BC0    |= 0x00000002 & ((neArray[j] >> 31) << 1); // Bits: Top bit -- set it as second bit of cable BC0
    // Data from Cable 3
    rct.hfEt[0][0]  |= 0x000000FE & ((j3Array[i]      ) <<  1); // Bits 1-7 -- Bit 0 is from Cable 4!!
    rct.hfEt[0][1]  |= 0x000000FE & ((j3Array[i] >>  7) <<  1); // Bits 1-7 -- Bit 0 is from Cable 4!! 
    rct.hfEt[1][0]   = 0x000000FF & ((j3Array[i] >> 14)      ); // Bits 0-7
    rct.hfEt[1][1]   = 0x000000FF & ((j3Array[i] >> 22)      ); // Bits 0-7
    rct.c3BC0        = 0x00000001 & ((j3Array[i] >> 31)      ); // Bits: Top bit
    rct.hfEt[0][2]  |= 0x000000FE & ((j3Array[j]      ) <<  1); // Bits 1-7 -- Bit 0 is from Cable 4!!
    rct.hfEt[0][3]  |= 0x000000FE & ((j3Array[j] >>  7) <<  1); // Bits 1-7 -- Bit 0 is from Cable 4!! 
    rct.hfEt[1][2]   = 0x000000FF & ((j3Array[j] >> 14)      ); // Bits 0-7
    rct.hfEt[1][3]   = 0x000000FF & ((j3Array[j] >> 22)      ); // Bits 0-7
    rct.c3BC0       |= 0x00000002 & ((j3Array[j] >> 31) <<  1); // Bits: Top bit -- set it as second bit of cable BC0
    // Data from Cable 4
    rct.rgnEt[5][0]  = 0x000003FF & ((j4Array[i]      )      ); // Bottom 10 bits of rgnET[5][0]
    rct.oBits       |= 0x00000400 & ((j4Array[i] >> 10) << 10); // Next up is overflow bit for [5][0] -- bit 10
    rct.tBits       |= 0x00000400 & ((j4Array[i] >> 11) << 10); // Next up is tau bit for [5][0] -- bit 10
    rct.rgnEt[6][0]  = 0x000003FF & ((j4Array[i] >> 12)      ); // Next 10 bits of rgnET[6][0]
    rct.oBits       |= 0x00001000 & ((j4Array[i] >> 22) << 12); // Next up is overflow bit for [6][0] -- bit 12
    rct.tBits       |= 0x00001000 & ((j4Array[i] >> 23) << 12); // Next up is tau bit for [6][0] -- bit 12
    rct.hfQBits     |= 0x0000000F & ((j4Array[i] >> 24)      ); // Bit 0-4
    rct.hfEt[0][0]  |= 0x00000001 & ((j4Array[i] >> 28)      ); // Bit 0
    rct.hfEt[0][1]  |= 0x00000001 & ((j4Array[i] >> 29)      ); // Bit 0
    rct.rgnEt[5][1]  = 0x000003FF & ((j4Array[j]      )      ); // Bottom 10 bits of rgnET[5][1]
    rct.oBits       |= 0x00000800 & ((j4Array[j] >> 10) << 11); // Next up is overflow bit for [5][1] -- bit 11
    rct.tBits       |= 0x00000800 & ((j4Array[j] >> 11) << 11); // Next up is tau bit for [5][1] -- bit 11
    rct.rgnEt[6][1]  = 0x000003FF & ((j4Array[j] >> 12)      ); // Next 10 bits of rgnET[6][1]
    rct.oBits       |= 0x00002000 & ((j4Array[j] >> 22) << 13); // Next up is overflow bit for [6][1] -- bit 13
    rct.tBits       |= 0x00002000 & ((j4Array[j] >> 23) << 13); // Next up is tau bit for [6][1] -- bit 13
    rct.hfQBits     |= 0x0000000F & ((j4Array[j] >> 24)      ); // Bit 0-4
    rct.hfEt[0][2]  |= 0x00000001 & ((j4Array[j] >> 28)      ); // Bit 0
    rct.hfEt[0][3]  |= 0x00000001 & ((j4Array[j] >> 29)      ); // Bit 0
    rct.c4BC0        = 0x00000001 & ((j4Array[i] >> 31)      ); // Bits: Top bit
    rct.c4BC0       |= 0x00000002 & ((j4Array[j] >> 31) <<  1); // Bits: Top bit -- set it as second bit of cable BC0
    // Data from Cable 5
    rct.rgnEt[0][0]  = 0x000003FF & ((j5Array[i]      )      ); // Bottom 10 bits of rgnET[0][0]
    rct.oBits       |= 0x00000001 & ((j5Array[i] >> 10)      ); // Next up is overflow bit for [0][0] -- bit 0
    rct.tBits       |= 0x00000001 & ((j5Array[i] >> 11)      ); // Next up is tau bit for [0][0] -- bit 0
    rct.rgnEt[1][0]  = 0x000003FF & ((j5Array[i] >> 12)      ); // Next 10 bits of rgnET[1][0]
    rct.oBits       |= 0x00000004 & ((j5Array[i] >> 22) <<  2); // Next up is overflow bit for [1][0] -- bit 2
    rct.tBits       |= 0x00000004 & ((j5Array[i] >> 23) <<  2); // Next up is tau bit for [1][0] -- bit 2
    rct.rgnEt[2][0]  = 0x0000003F & ((j5Array[i] >> 24)      ); // Next 6 bits of rgnET[2][0] (bottom six only -- rest on next cable)
    rct.rgnEt[0][1]  = 0x000003FF & ((j5Array[j]      )      ); // Bottom 10 bits of rgnET[0][1]
    rct.oBits       |= 0x00000002 & ((j5Array[j] >> 10) <<  1); // Next up is overflow bit for [0][1] -- bit 1
    rct.tBits       |= 0x00000002 & ((j5Array[j] >> 11) <<  1); // Next up is tau bit for [0][1] -- bit 1
    rct.rgnEt[1][1]  = 0x000003FF & ((j5Array[j] >> 12)      ); // Next 10 bits of rgnET[1][0]
    rct.oBits       |= 0x00000008 & ((j5Array[j] >> 22) <<  3); // Next up is overflow bit for [1][1] -- bit 3
    rct.tBits       |= 0x00000008 & ((j5Array[j] >> 23) <<  3); // Next up is tau bit for [1][1] -- bit 3
    rct.rgnEt[2][1]  = 0x0000003F & ((j5Array[j] >> 24)      ); // Next 6 bits of rgnET[2][1] (bottom six only -- rest on next cable)
    rct.c5BC0        = 0x00000001 & ((j5Array[i] >> 31)      ); // Bits: Top bit
    rct.c5BC0       |= 0x00000002 & ((j5Array[j] >> 31)  << 1); // Bits: Top bit -- set it as second bit of cable BC0
    // Data from Cable 6
    rct.rgnEt[2][0] |= 0x000003C0 & ((j6Array[i]      ) <<  6); // Top 4 bits of rgnET[2][0]
    rct.oBits       |= 0x00000010 & ((j6Array[i] >>  4) <<  4); // Next up is overflow bit for [2][0] -- bit 4
    rct.tBits       |= 0x00000010 & ((j6Array[i] >>  5) <<  4); // Next up is tau bit for [2][0] -- bit 4
    rct.rgnEt[3][0]  = 0x000003FF & ((j6Array[i] >>  6)      ); // Next 10 bits of rgnET[3][0]
    rct.oBits       |= 0x00000040 & ((j6Array[i] >> 16) <<  6); // Next up is overflow bit for [3][0] -- bit 6
    rct.tBits       |= 0x00000040 & ((j6Array[i] >> 17) <<  6); // Next up is tau bit for [3][0] -- bit 6
    rct.rgnEt[4][0]  = 0x000003FF & ((j6Array[i] >> 18)      ); // Next 10 bits of rgnET[4][0]
    rct.oBits       |= 0x00000100 & ((j6Array[i] >> 28) <<  8); // Next up is overflow bit for [4][0] -- bit 8
    rct.tBits       |= 0x00000100 & ((j6Array[i] >> 29) <<  8); // Next up is tau bit for [4][0] -- bit 8
    rct.rgnEt[2][1] |= 0x000003C0 & ((j6Array[j]      ) <<  6); // Top 4 bits of rgnET[2][1]
    rct.oBits       |= 0x00000020 & ((j6Array[j] >>  4) <<  5); // Next up is overflow bit for [2][1] -- bit 5
    rct.tBits       |= 0x00000020 & ((j6Array[j] >>  5) <<  5); // Next up is tau bit for [2][1] -- bit 5
    rct.rgnEt[3][1]  = 0x000003FF & ((j6Array[j] >>  6)      ); // Next 10 bits of rgnET[3][1]
    rct.oBits       |= 0x00000080 & ((j6Array[j] >> 16) <<  7); // Next up is overflow bit for [3][1] -- bit 7
    rct.tBits       |= 0x00000080 & ((j6Array[j] >> 17) <<  7); // Next up is tau bit for [3][1] -- bit 7
    rct.rgnEt[4][1]  = 0x000003FF & ((j6Array[j] >> 18)      ); // Next 10 bits of rgnET[4][1]
    rct.oBits       |= 0x00000200 & ((j6Array[j] >> 28) <<  9); // Next up is overflow bit for [4][1] -- bit 9
    rct.tBits       |= 0x00000200 & ((j6Array[j] >> 29) <<  9); // Next up is tau bit for [4][1] -- bit 9
    rct.c6BC0        = 0x00000001 & ((j6Array[i] >> 31)      ); // Bits: Top bit
    rct.c6BC0       |= 0x00000002 & ((j6Array[j] >> 31)  << 1); // Bits: Top bit -- set it as second bit of cable BC0
    rctInfoData.push_back(rct);
  }
  return true;
}

/*
 * Print RCT Info!
 */

bool RCTInfoFactory::printRCTInfo(const std::vector<RCTInfo> &rctInfo){

  for(unsigned int iBC = 0; iBC < rctInfo.size(); iBC++) {
    cout <<dec<< "===== BC Cycle: "<< iBC <<endl;
    cout << "BC0/1 for Cables 1-6:             ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c1BC0 << " ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c2BC0 << " ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c3BC0 << " ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c4BC0 << " ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c5BC0 << " ";
    cout << hex << setfill('0') << setw(1) << rctInfo[iBC].c6BC0 << endl;
    cout << "EISO/NISO (Card,Region,Rank) 1-4: ";
    for(int i = 0; i < 4; i++) {
      cout << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].ieCard[i] << "," << setw(1) << rctInfo[iBC].ieRegn[i] << "," << setw(2) << rctInfo[iBC].ieRank[i] << ") ";
    }
    for(int i = 0; i < 4; i++) {
      cout << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].neCard[i] << "," << setw(1) << rctInfo[iBC].neRegn[i] << "," << setw(2) << rctInfo[iBC].neRank[i] << ") ";
    }
    cout << endl;
    cout << "HFET[2][4]:                       ";
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < 4; j++) {
	cout << hex << setfill('0') << setw(3) << rctInfo[iBC].hfEt[i][j] << " ";
      }
    }
    cout << endl;
    cout << "RgnET[7][2]:                      ";
    for(int i = 0; i < 7; i++) {
      for(int j = 0; j < 2; j++) {
	cout << hex << setfill('0') << setw(4) << rctInfo[iBC].rgnEt[i][j] << " ";
      }
    }
    cout << endl;
    cout << "Q/MIP/Tau/OF/HF-Q Bits:           ";
    cout << hex << setfill('0') << setw(4) << rctInfo[iBC].qBits << " ";
    cout << hex << setfill('0') << setw(4) << rctInfo[iBC].mBits << " ";
    cout << hex << setfill('0') << setw(4) << rctInfo[iBC].tBits << " ";
    cout << hex << setfill('0') << setw(4) << rctInfo[iBC].oBits << " ";
    cout << hex << setfill('0') << setw(4) << rctInfo[iBC].hfQBits << endl;
  }
  cout << "Done" << endl;
  return true;
}

/*
 * Print RCT Info To File!
 */

bool RCTInfoFactory::printRCTInfoToFile(const std::vector<RCTInfo> &rctInfo, std::ofstream &myfile){

  if(!myfile.is_open())
     myfile.open("textfile");

  for(unsigned int iBC = 0; iBC < rctInfo.size(); iBC++) {
    myfile <<dec<< "===== BC Cycle: "<< iBC <<"\n";
    myfile << "BC0/1 for Cables 1-6:             ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c1BC0 << " ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c2BC0 << " ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c3BC0 << " ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c4BC0 << " ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c5BC0 << " ";
    myfile << hex << setfill('0') << setw(1) << rctInfo[iBC].c6BC0 << "\n";
    myfile << "EISO/NISO (Card,Region,Rank) 1-4: ";
    for(int i = 0; i < 2; i++) {
      myfile << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].ieCard[i] << "," << setw(1) << rctInfo[iBC].ieRegn[i] << "," << setw(2) << rctInfo[iBC].ieRank[i] << ") ";
    }
    for(int i = 0; i < 2; i++) {
      myfile << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].neCard[i] << "," << setw(1) << rctInfo[iBC].neRegn[i] << "," << setw(2) << rctInfo[iBC].neRank[i] << ") ";
    }
    for(int i = 2; i < 4; i++) {
      myfile << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].ieCard[i] << "," << setw(1) << rctInfo[iBC].ieRegn[i] << "," << setw(2) << rctInfo[iBC].ieRank[i] << ") ";
    }
    for(int i = 2; i < 4; i++) {
      myfile << hex << setfill('0') << "(" << setw(1) << rctInfo[iBC].neCard[i] << "," << setw(1) << rctInfo[iBC].neRegn[i] << "," << setw(2) << rctInfo[iBC].neRank[i] << ") ";
    }
    myfile << "\n";
    myfile << "HFET[2][4]:                       ";
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < 4; j++) {
	myfile << hex << setfill('0') << setw(3) << rctInfo[iBC].hfEt[i][j] << " ";
      }
    }
    myfile << "\n";
    myfile << "RgnET[7][2]:                      ";
    for(int i = 0; i < 7; i++) {
      for(int j = 0; j < 2; j++) {
	myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].rgnEt[i][j] << " ";
      }
    }
    myfile << "\n";
    myfile << "Q/MIP/Tau/OF/HF-Q Bits:           ";
    myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].qBits << " ";
    myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].mBits << " ";
    myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].tBits << " ";
    myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].oBits << " ";
    myfile << hex << setfill('0') << setw(4) << rctInfo[iBC].hfQBits << "\n";
  }
  myfile << "Done" << "\n";

  return true;
}

bool RCTInfoFactory::verifyHammingCode(const unsigned char *data) {return true;};

bool RCTInfoFactory::verifyBXBytes(const unsigned int &evenFiber, const unsigned int &oddFiber) {

  bool status = true;

  if(((evenFiber&0xFF) != 0x7C) && ((evenFiber&0xFF) != 0x3C)){
    std::cerr<<"Error BX Signal on Even Fiber is not 0x7C or 0x3C it is:"<< (evenFiber&0xFF) << std::endl;
    status = false;
  }

  if(((oddFiber&0xFF) != 0x7C) && ((oddFiber&0xFF) != 0x3C)){
    std::cerr<<"Error BX Signal on Odd Fiber is not 0x7C or 0x3C it is: "<< oddFiber <<" "<< (oddFiber&0xFF) << std::endl;
    status = false;
  }

  return status;

};

/*
 * Check if first word matches abort gap, if yes, presume in abort gap and skip BX
 */
bool RCTInfoFactory::inAbortGap(const unsigned int &evenFiber, const unsigned int &oddFiber) {
  if(evenFiber == 0x505050BC)
    return true;

  if(oddFiber == 0x505050BC)
    return true;

  else return false;
}


bool RCTInfoFactory::timeStampChar( char  timeStamp[80] )
{
  char *the_time = (char *) malloc(sizeof(char)*80);
  time_t rawtime;  struct tm * timeinfo;  
  time ( &rawtime ); timeinfo = localtime ( &rawtime );
  strftime (the_time,80,"%b%d_%Hhr%Mmn%Ss",timeinfo);
  strcpy(timeStamp, the_time);
  return true;
}

bool RCTInfoFactory::timeStampCharTime( char  timeStamp[80] )
{
  char *the_time = (char *) malloc(sizeof(char)*80);
  time_t rawtime;  struct tm * timeinfo;  
  time ( &rawtime ); timeinfo = localtime ( &rawtime );
  strftime (the_time,80,"%H%M%S",timeinfo);
  strcpy(timeStamp, the_time);
  return true;
}

bool RCTInfoFactory::timeStampCharDate( char  timeStamp[80] )
{
  char *the_time = (char *) malloc(sizeof(char)*80);
  time_t rawtime;  struct tm * timeinfo;  
  time ( &rawtime ); timeinfo = localtime ( &rawtime );
  strftime (the_time,80,"%d%m",timeinfo);
  strcpy(timeStamp, the_time);
  return true;
}
