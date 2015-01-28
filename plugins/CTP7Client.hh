#ifndef CTP7Client_hh
#define CTP7Client_hh

#include "CTP7.hh"

#define MSGLEN 64

class CTP7Client : public CTP7 {

public:
  
  CTP7Client(const char *server = "127.0.0.1", const char *port = "5555", bool verbose = false);
  virtual ~CTP7Client();

  void setVerbose(bool v) {verbose = v;}

  bool checkConnection();

  bool getConfiguration(std::string output);
  bool setConfiguration(std::string input);

  virtual bool hardReset();
  virtual bool softReset();
  virtual bool counterReset();

  bool getCaptureStatus(CaptureStatus *c);
  bool capture();

  bool setPattern(BufferType bufferType,
		  uint32_t linkNumber, 
		  uint32_t nInts,
		  std::vector<uint32_t> values);

  bool setConstantPattern(BufferType bufferType,
			  uint32_t linkNumber, 
			  uint32_t value);

  bool setIncreasingPattern(BufferType bufferType,
			    uint32_t linkNumber, 
			    uint32_t startValue = 0, 
			    uint32_t increment = 1);
  bool setDecreasingPattern(BufferType bufferType,
			    uint32_t linkNumber, 
			    uint32_t startValue = (NIntsPerLink - 1), 
			    uint32_t increment = 1);
  bool setRandomPattern(BufferType bufferType,
			uint32_t linkNumber, 
			uint32_t randomSeed);

  uint32_t getValue(BufferType bufferType, uint32_t addressOffset);

  bool getValues(BufferType bufferType, uint32_t startAddressOffset, uint32_t numberOfValues, uint32_t *buffer);

  bool setValue(BufferType bufferType, uint32_t addressOffset, uint32_t value);

  bool setValues(BufferType bufferType, uint32_t startAddressOffset, uint32_t numberOfValues, uint32_t *buffer);

  // Access to various types of registers for monitoring

  virtual bool getInputLinkRegisters(std::vector<InputLinkRegisters> &o) {
    o.clear();
    for(uint32_t i = 0; i < NILinks; i++) {
      InputLinkRegisters r;
      o.push_back(r);
    }
    bool status = getValues(inputLinkRegisters, 0, sizeof(InputLinkRegisters) * NILinks / sizeof(uint32_t), (uint32_t *) o.data());
    if(status && o.size() == NILinks) {
      return true;
    }
    return false;
  }
  virtual bool getLinkAlignmentRegisters(LinkAlignmentRegisters *o) {
    return getValues(linkAlignmentRegisters, 0, sizeof(LinkAlignmentRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getInputCaptureRegisters(InputCaptureRegisters *o) {
    return getValues(inputCaptureRegisters, 0, sizeof(InputCaptureRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getDAQSpyCaptureRegisters(DAQSpyCaptureRegisters *o) {
    return getValues(daqSpyCaptureRegisters, 0, sizeof(DAQSpyCaptureRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getDAQRegisters(DAQRegisters *o) {
    return getValues(daqRegisters, 0, sizeof(DAQRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getAMC13Registers(AMC13Registers *o) {
    return getValues(amc13Registers, 0, sizeof(AMC13Registers) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getTCDSRegisters(TCDSRegisters *o) {
    return getValues(tcdsRegisters, 0, sizeof(TCDSRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getTCDSMonitorRegisters(TCDSMonitorRegisters *o) {
    return getValues(tcdsMonitorRegisters, 0, sizeof(TCDSMonitorRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }
  virtual bool getGTHRegisters(std::vector<GTHRegisters> &o) {
    o.clear();
    for(uint32_t i = 0; i < NILinks; i++) {
      GTHRegisters r;
      o.push_back(r);
    }
    return getValues(gthRegisters, 0, sizeof(GTHRegisters) * NILinks / sizeof(uint32_t), (uint32_t *) o.data());
  }
  virtual bool getQPLLRegisters(std::vector<QPLLRegisters> &o) {
    o.clear();
    for(uint32_t i = 0; i < (NILinks + NOLinks) / 4; i++) {
      QPLLRegisters r;
      o.push_back(r);
    }
    return getValues(qpllRegisters, 0, sizeof(QPLLRegisters) * NILinks / sizeof(uint32_t), (uint32_t *) o.data());
  }
  virtual bool getMiscRegisters(MiscRegisters *o) {
    return getValues(miscRegisters, 0, sizeof(MiscRegisters) / sizeof(uint32_t), (uint32_t *) o);
  }

  uint32_t getAddress(BufferType bufferType, 
		      uint32_t addressOffset) {
    return getValue(bufferType, addressOffset);
  }

  uint32_t getAddress(BufferType bufferType, 
		      uint32_t linkNumber,
		      uint32_t addressOffset) {
    if((bufferType == inputBuffer && linkNumber < NILinks) ||
       (bufferType == outputBuffer && linkNumber < 36)) {
      return getValue(bufferType, (linkNumber * NIntsPerLink + addressOffset));
    }
    return 0xDEADBEEF;
  }
  

  uint32_t getRegister(BufferType bufferType, uint32_t addressOffset) {
    return getValue(bufferType, addressOffset);
  }

  bool dumpContiguousBuffer(BufferType bufferType, 
			    uint32_t linkNumber,
			    uint32_t startAddressOffset, 
			    uint32_t numberOfValues, 
			    uint32_t *buffer = 0) {
    return getValues(bufferType, (linkNumber * NIntsPerLink + startAddressOffset), numberOfValues, buffer);
  }

  bool setAddress(BufferType bufferType, 
		  uint32_t linkNumber,
		  uint32_t addressOffset, 
		  uint32_t value) {
    return setValue(bufferType, (linkNumber * NIntsPerLink + addressOffset), value);
  }

  bool setAddress(BufferType bufferType, 
		  uint32_t addressOffset, 
		  uint32_t value) {
    return setValue(bufferType, addressOffset, value);
  }

  bool setRegister(BufferType bufferType, uint32_t addressOffset, uint32_t value) {
    return setAddress(bufferType, addressOffset, value);
  }

  ssize_t getResult(void* iData, void* oData, 
		    ssize_t iSize, ssize_t oSize,
		    bool wait = false);

  bool dumpStatus(std::vector<uint32_t> &addressValues );
  bool dumpDecoderErrors(std::vector<uint32_t> &addressValues);
  bool dumpCRCErrors(std::vector<uint32_t> &addressValues);
  bool dumpAllLinkIDs(std::vector<uint32_t> &addressValues);

private:

  // Unnecessary methods are made private
  CTP7Client(const CTP7Client&);
  const CTP7Client& operator=(const CTP7Client&);
  
  // For most small messages use the local buffer (overwriting as needed)
  ssize_t getResult() {return getResult(msg, msg, MSGLEN, MSGLEN, false);}
  
  // Check arguments 

  uint32_t getMaxOffset(BufferType bufferType);

  bool checkArgs(BufferType bufferType, uint32_t addressOffset = 0, uint32_t numberOfValues = 1);

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
