#ifndef CTP7Client_hh
#define CTP7Client_hh

#include "CTP7.hh"

#define MSGLEN 64

class CTP7Client : public CTP7 {

public:
  
  CTP7Client(const char *server = "127.0.0.1", const char *port = "5555", bool verbose = false);
  virtual ~CTP7Client();

  void setVerbose(bool v) {verbose = v;}

  unsigned int getAddress(BufferType bufferType, 
			  unsigned int linkNumber,
			  unsigned int addressOffset);

  unsigned int getAddress(unsigned int addressOffset) {
    return getAddress(registerBuffer, 0, addressOffset);
  }

  unsigned int getLinkID(unsigned int linkNumber);

  unsigned int getRegister(unsigned int addressOffset) {
    return getAddress(addressOffset);
  }

  bool dumpContiguousBuffer(BufferType bufferType, 
			    unsigned int linkNumber,
			    unsigned int startAddressOffset, 
			    unsigned int numberOfValues, 
			    unsigned int *buffer = 0);

  bool setAddress(BufferType bufferType, 
		  unsigned int linkNumber,
		  unsigned int addressOffset, 
		  unsigned int value);

  bool setAddress(unsigned int addressOffset, 
		  unsigned int value) {
    return setAddress(registerBuffer, 0, addressOffset, value);
  }

  bool setRegister(unsigned int addressOffset, unsigned int value) {
    return setAddress(registerBuffer, 0, addressOffset, value);
  }

  bool setPattern(BufferType bufferType,
		  unsigned int linkNumber, 
		  unsigned int nInts,
		  std::vector<unsigned int> values);

  bool setConstantPattern(BufferType bufferType,
			  unsigned int linkNumber, 
			  unsigned int value);

  bool setIncreasingPattern(BufferType bufferType,
			    unsigned int linkNumber, 
			    unsigned int startValue = 0, 
			    unsigned int increment = 1);
  bool setDecreasingPattern(BufferType bufferType,
			    unsigned int linkNumber, 
			    unsigned int startValue = (NIntsPerLink - 1), 
			    unsigned int increment = 1);
  bool setRandomPattern(BufferType bufferType,
			unsigned int linkNumber, 
			unsigned int randomSeed);

  ssize_t getResult(void* iData, void* oData, 
		    ssize_t iSize, ssize_t oSize,
		    bool wait = false);

  bool dumpStatus(std::vector<unsigned int> &addressValues );
  bool dumpDecoderErrors(std::vector<unsigned int> &addressValues);
  bool dumpCRCErrors(std::vector<unsigned int> &addressValues);
  bool dumpAllLinkIDs(std::vector<unsigned int> &addressValues);

  //bool getStatusRegisters();
  //bool counterReset();
  bool softReset();
  bool capture();
  //bool decoderLocked();
  bool checkConnection();
  bool hardReset();
  bool setAddressNames(std::vector<std::string> & addressNames, unsigned int & version);

private:

  // Unnecessary methods are made private
  CTP7Client(const CTP7Client&);
  const CTP7Client& operator=(const CTP7Client&);
  
  // For most small messages use the local buffer (overwriting as needed)
  ssize_t getResult() {return getResult(msg, msg, MSGLEN, MSGLEN, false);}
  
  // Check arguments 
  bool checkArgs(BufferType bufferType,
		 unsigned int linkNumber,
		 unsigned int addressOffset = 0,
		 unsigned int numberOfValues = 1);

  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
  int socketfd; // Socket file descriptor

  //print error message
  void printConnectionError(){
    std::cout<<"Error! MSG Received is NULL"<<std::endl;
    std::cout<<"There is most likely something wrong with your connection"<<std::endl;
    std::cout<<"Please check the server is running and the IP/Port is correct"<<std::endl;
  }
  
  bool verbose;

  char msg[MSGLEN];

};

#endif
