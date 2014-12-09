#ifndef CTP7_hh
#define CTP7_hh

#include <vector>

// This class defines the interface to CTP7
// The idea is to keep this pure virtual and enable client/server communication

#define NIntsPerLink 1024
#define LinkBufSize NIntsPerLink * sizeof(int)
#define NRegisters 32
#define MaxValidAddress 0x63000000
#define RegBufSize NRegisters *sizeof(int)
#define NILinks 36
#define NOLinks 3
#define statusRegisterSize 21
//#define NILinks 67
//#define NOLinks 48

#define S2NILinks 48
class CTP7 {

public:

  virtual ~CTP7() {;}

  // Type of buffers available on CTP7

  enum BufferType {
    inputBuffer = 0,
    outputBuffer = 1,
    registerBuffer = 2,
    unnamed = 3,
    s2inputBuffer = 4
  };

  // Externally accessible functions to get/set on-board buffers

  virtual unsigned int getAddress(BufferType bufferType,
				  unsigned int linkNumber,
				  unsigned int addressOffset) = 0;

  virtual unsigned int getRegister(unsigned int addressOffset) = 0;

  virtual bool dumpContiguousBuffer(BufferType bufferType,
				    unsigned int linkNumber,
				    unsigned int startAddressOffset, 
				    unsigned int numberOfValues, 
				    unsigned int *buffer) = 0;
  
  virtual bool setAddress(BufferType bufferType, 
			  unsigned int linkNumber,
			  unsigned int addressOffset, 
			  unsigned int value) = 0;

  virtual bool setRegister(unsigned int addressOffset, unsigned int value) = 0;

  virtual bool setPattern(BufferType bufferType,
			  unsigned int linkNumber, 
			  unsigned int nInts,
			  std::vector<unsigned int> values) = 0;
  virtual bool setConstantPattern(BufferType bufferType,
				  unsigned int linkNumber, 
				  unsigned int value) = 0;
  virtual bool setIncreasingPattern(BufferType bufferType,
				    unsigned int linkNumber, 
				    unsigned int startValue, 
				    unsigned int increment) = 0;
  virtual bool setDecreasingPattern(BufferType bufferType,
				    unsigned int linkNumber, 
				    unsigned int startValue, 
				    unsigned int increment) = 0;
  virtual bool setRandomPattern(BufferType bufferType,
				unsigned int linkNumber, 
				unsigned int randomSeed) = 0;

  virtual bool capture() = 0;
  virtual bool softReset() = 0;
  //virtual bool counterReset() = 0;
  virtual bool checkConnection() = 0;
  virtual bool hardReset() = 0;

  //virtual bool decoderLocked() = 0;

};

#endif
