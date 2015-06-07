#include <iostream>
#include <iomanip>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "CTP7Client.hh"
#include <climits>

/*
 * Primary Client class for CTP7
 * Authors: Sridhara Dasu, Isobel Ojalvo
 * June 2014
 */

CTP7Client::CTP7Client(const char* serverHost, const char* serverPort, bool v) : verbose(v) {

  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not nessesary
  // empty. Therefor we use the memset function to make sure all fields are NULL.

  memset(&host_info, 0, sizeof host_info);

  if(verbose) std::cout << "Setting up the structs..."  << std::endl;

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  // Now fill up the linked list of host_info structs with google's address information.

  int status = getaddrinfo(serverHost, serverPort, &host_info, &host_info_list);

  // getaddrinfo returns 0 on succes, or some other value when an error occured.
  // (translated into human readable text by the gai_gai_strerror function).

  if (status != 0)  {
    std::cout << "getaddrinfo error" << gai_strerror(status) ;
    exit(1);
  }

  if(verbose) std::cout << "Creating a socket..."  << std::endl;

  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
		    host_info_list->ai_protocol);

  if (socketfd == -1) {
    if(verbose) std::cout << "socket error " ;
    exit(1);
  }

  int a = 0x4000; // Use large receive buffer

  if (setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &a, sizeof(int)) == -1) {
    std::cerr << "Error setting socket opts" << std::endl;
    exit(1);
  }

  if(verbose) std::cout << "Connect()ing..."  << std::endl;

  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);

  if(status == -1) {
    std::cout << "Error Connecting to CTP7, exiting. :(" <<std::endl;
    exit(1);
  }

}

ssize_t CTP7Client::getResult(void *iData, void *oData, 
			      ssize_t iSize, ssize_t oSize, 
			      bool wait) {

  ssize_t bytes_sent = send(socketfd, iData, iSize, 0);

  if(verbose) 
    std::cout << "bytes sent     : "<< bytes_sent << std::endl ;

  if(verbose) 
    std::cout << "Waiting to receive data..."  << std::endl;

  ssize_t bytes_received;

  // If no data arrives, the program will just wait here until some data arrives.

  if(!wait) 
    bytes_received = recv(socketfd, oData, oSize, 0);
  else 
    bytes_received = recv(socketfd, oData, oSize, MSG_WAITALL);

  if (bytes_received == 0) 
    if(verbose) 
      std::cout << "host shut down." << std::endl;

  if (bytes_received == -1) 
    if(verbose) 
      std::cout << "receive error!" << std::endl ;

  if(verbose) 
    std::cout << "bytes received : " << bytes_received << std::endl ;

  if(verbose){////
    uint32_t *temp = (uint32_t *)oData;////
    std::cout<<"oData "<<temp[0]<<" "<<temp[1]<<std::endl;    ////
  }


  return bytes_received;
}

CTP7Client::~CTP7Client() {

  if(verbose) std::cout << "Sending HANGUP" << std::endl;

  ssize_t bytes_sent = send(socketfd, "HANGUP", 7, 0);

  if(verbose) std::cout << "CTP7Client being destroyed. Closing socket... " 
			<< bytes_sent << std::endl;

  freeaddrinfo(host_info_list);

  close(socketfd);
}

uint32_t CTP7Client::getMaxOffset(BufferType bufferType) {
  switch(bufferType) {
  case(inputBuffer):
    return NILinks * NIntsPerLink;
  case(outputBuffer):
    return NOLinks * NIntsPerLink;
  case(daqBuffer):
    return NIntsInDAQBuffer;
  case(tcdsBuffer):
    return NIntsInTCDSBuffer;
  case(inputLinkRegisters):
    return NILinks * sizeof(InputLinkRegisters) / sizeof(uint32_t);
  case(linkAlignmentRegisters):
    return sizeof(LinkAlignmentRegisters) / sizeof(uint32_t);
  case(inputCaptureRegisters):
    return sizeof(InputCaptureRegisters) / sizeof(uint32_t);
  case(daqSpyCaptureRegisters):
    return sizeof(DAQSpyCaptureRegisters) / sizeof(uint32_t);
  case(daqRegisters):
    return sizeof(DAQRegisters) / sizeof(uint32_t);
  case(amc13Registers):
    return sizeof(AMC13Registers) / sizeof(uint32_t);
  case(tcdsRegisters):
    return sizeof(TCDSRegisters) / sizeof(uint32_t);
  case(tcdsMonitorRegisters):
    return sizeof(TCDSMonitorRegisters) / sizeof(uint32_t);
  case(gthRegisters):
    return NILinks * sizeof(GTHRegisters) / sizeof(uint32_t);
  case(qpllRegisters):
    return (NILinks / 4) * sizeof(QPLLRegisters) / sizeof(uint32_t);
  case(miscRegisters):
    return sizeof(MiscRegisters) / sizeof(uint32_t);
  case(unnamed):
    // This is super dangerous, but we use it for kludging
    // Server side should protect itself with appropriate length checks
    return 0xFFFFFFFF;
  }
  // Unknown BufferType returns 0 so that it will fail in the calling function
  return 0;
}

/*
 * Check that args do not exceed the maximum valid address
 * OR the index doesn't exceed the number of ints per link
 * OR the Link Number doesn't exceed the Numbr of Input Links or Output Links
 */

bool CTP7Client::checkArgs(BufferType bufferType,
			   uint32_t addressOffset,
			   uint32_t numberOfValues) {
  uint32_t maxOffset = (addressOffset/sizeof(uint32_t)) + numberOfValues;
  if(maxOffset > getMaxOffset(bufferType)) return false;
  return true;
}

uint32_t CTP7Client::getValue(BufferType bufferType, 
			      uint32_t addressOffset) {
  uint32_t value = 0xDEADBEEF;
  if(checkArgs(bufferType, addressOffset)) {
    sprintf(msg, "getValue(%x,%x)", bufferType, addressOffset);
    ssize_t bytes_received = getResult();
    if(msg == NULL){
      printConnectionError();
    }
    else {
      msg[bytes_received] = '\0';
      if(sscanf(msg, "%x", &value) != 1) std::cerr << msg << std::endl;
    }
  }    
  return true;
}

bool CTP7Client::getValues(BufferType bufferType,
			   uint32_t startAddressOffset, 
			   uint32_t numberOfValues, 
			   uint32_t *buffer) {

  if(!checkArgs(bufferType, startAddressOffset, numberOfValues)){
    std::cout<<"Failed Check Args Step "<<std::endl; 
    return false;
  }

  sprintf(msg, "getValues(%x,%x,%x)", bufferType, startAddressOffset, numberOfValues);

  ssize_t bytes_received = getResult(msg, buffer, MSGLEN, numberOfValues*4, true);

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((uint32_t)bytes_received == (numberOfValues*4)) {
    return true;
  }

  return false;

}

bool CTP7Client::setValue(BufferType bufferType, 
			  uint32_t addressOffset, 
			  uint32_t value) {
  if(!checkArgs(bufferType, addressOffset)) return false;
  sprintf(msg, "setValue(%x,%x,%x)", bufferType, addressOffset, value);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(msg == NULL){
    printConnectionError();
    return false;
  }
  return true;
}

bool CTP7Client::checkConnection(){
  sprintf(msg, "Hello");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(msg == NULL){
    printConnectionError();
    return false;
  }
  if(strcmp(msg, "HelloToYou!") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::getConfiguration(std::string o){
  sprintf(msg, "getConfiguration");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(msg == NULL){
    printConnectionError();
    return false;
  }
  o = msg;
  return true;
}

bool CTP7Client::setConfiguration(std::string i){
  std::string s = "setConfiguration(" + i + ")";
  ssize_t bytes_received = getResult((void *) s.c_str(), msg, s.size(), MSGLEN);
  msg[bytes_received] = '\0';
  if(msg == NULL){
    printConnectionError();
    return false;
  }
  if(strcmp(msg, "SUCCESS") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::hardReset(){
  sprintf(msg, "hardReset");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "SUCCESS") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::softReset(){
  sprintf(msg, "softReset");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(strcmp(msg, "SUCCESS") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::counterReset(){
  sprintf(msg, "counterReset");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(strcmp(msg, "SUCCESS") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::getCaptureStatus(CaptureStatus *c){
  sprintf(msg, "checkCaptureStatus");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(msg == NULL){
    printConnectionError();
    return false;
  }
  else {
    if(sscanf(msg, "%x", (uint32_t *) c) != 1) {
      std::cerr << "Error! MSG Received: " << msg << std::endl;
      return false;
    }
  }
  return true;
}

bool CTP7Client::capture(){
  sprintf(msg, "capture");
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(strcmp(msg, "SUCCESS") != 0){
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::setCapturePoint(uint32_t capture_point){

  //BufferType : inputCaptureRegisters = 6, reg offset = 8 (CAPTURE_START_BCID_REG)
  sprintf(msg, "setValue(%x,%x,%x)", 6, 8, capture_point);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';
  if(strcmp(msg, "SUCCESS") != 0){
            std::cout<<"Error! MSG Received: "<<msg<<std::endl;
            return false;
  }
  return true;
}
  
bool CTP7Client::setConstantPattern(BufferType bufferType, 
				    uint32_t linkNumber, 
				    uint32_t value) {
  if(!checkArgs(bufferType, linkNumber)){
    std::cout<<"Failed Check Args Step "<<std::endl;
    return false;
  }

  sprintf(msg, "setConstantPattern(%x,%x,%x)", bufferType, linkNumber, value);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "SUCCESS") != 0) {
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }

  return true;
}

bool CTP7Client::setIncreasingPattern(BufferType bufferType,
				      uint32_t linkNumber, 
				      uint32_t startValue, 
				      uint32_t increment) {

  if(!checkArgs(bufferType, linkNumber)){
    std::cout<<"Failed Check Args Step "<<std::endl;
    return false;
  }

  sprintf(msg, "setIncreasingPattern(%x,%x,%x,%x)", 
	  bufferType, linkNumber, startValue, increment);

  ssize_t bytes_received = getResult();

  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "SUCCESS") != 0) {
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }

  return true;
}

bool CTP7Client::setDecreasingPattern(BufferType bufferType,
				      uint32_t linkNumber, 
				      uint32_t startValue, 
				      uint32_t increment) {
  if(!checkArgs(bufferType, linkNumber)) return false;
  sprintf(msg, "setDecreasingPattern(%x,%x,%x,%x)", 
	  bufferType, linkNumber, startValue, increment);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "SUCCESS") != 0) {
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }
  return true;
}

bool CTP7Client::setRandomPattern(BufferType bufferType,
				  uint32_t linkNumber, 
				  uint32_t randomSeed) {
  if(!checkArgs(bufferType, linkNumber)){
    std::cout<<"Failed Check Args Step "<<std::endl;
    return false;
  }

  sprintf(msg, "setRandomPattern(%x,%x,%x)", bufferType, linkNumber, randomSeed);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "SUCCESS") != 0) {
    std::cout<<"Error! MSG Received: "<<msg<<std::endl;
    return false;
  }

  return true;
}

/*
 * Set a Pattern from a vector to a link
 * First tells the server we want to send a pattern
 * Then a pattern is sent
 * 
 * This requires 3 arguments not 2
 *
 * TODO: Check to see what controls are in place
 */

bool CTP7Client::setValues(BufferType bufferType, uint32_t startAddressOffset, uint32_t numberOfValues, uint32_t *buffer) {

  if(!checkArgs(bufferType, startAddressOffset, numberOfValues)){ 
    std::cout<<"Failed Check Args Step "<<std::endl;
    return false;
  }

  sprintf(msg, "setValues(%x,%x,%x)", bufferType, startAddressOffset, numberOfValues);

  ssize_t bytes_received = getResult();

  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "READY_FOR_PATTERN_DATA") != 0){
    std::cout<< "Failed to read back a green light from CTP7 Server, failed. :("<<std::endl;
    return false;
  }    
  else {

    bytes_received = getResult(buffer, msg, numberOfValues*4, MSGLEN);
    msg[bytes_received] = '\0';

    if(msg == NULL){
      printConnectionError();
      return false;
    }

    if(verbose) std::cout << msg << std::endl;

    if(strcmp(msg, "SUCCESS") != 0) {
      std::cout<<"Error! Failed to write pattern! MSG Received: "<<msg<<std::endl;
      return false;
    }

  }

  return true;

}

bool CTP7Client::setPattern(BufferType bufferType,
			    uint32_t linkNumber,
			    uint32_t nInts,
			    std::vector<uint32_t> pattern) {

  if(!checkArgs(bufferType, linkNumber * NIntsPerLink)){ 
    std::cout<<"Failed Check Args Step "<<std::endl;
    return false;
  }

  sprintf(msg, "setPattern(%x,%x,%x)", bufferType, linkNumber, nInts);

  ssize_t bytes_received = getResult();

  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(strcmp(msg, "READY_FOR_PATTERN_DATA") != 0){
    std::cout<< "Failed to read back a green light from CTP7 Server, failed. :("<<std::endl;
    return false;
  }    
  else {

    bytes_received = getResult(pattern.data(), msg, NIntsPerLink*4, MSGLEN);
    msg[bytes_received] = '\0';

    if(msg == NULL){
      printConnectionError();
      return false;
    }

    if(verbose) std::cout << msg << std::endl;

    if(strcmp(msg, "SUCCESS") != 0) {
      std::cout<<"Error! Failed to write pattern! MSG Received: "<<msg<<std::endl;
      return false;
    }

  }

  return true;
}

bool CTP7Client::dumpStatus(std::vector<uint32_t> &statusValues) {
  statusValues.clear();
  std::vector <CTP7::InputLinkRegisters> inputLinkRegisters;
  if(getInputLinkRegisters(inputLinkRegisters)) {
    for(uint32_t i = 0; i < NILinks; i++) {
      statusValues.push_back(inputLinkRegisters[i].LINK_STATUS_REG);
    }
  }
  else {
    return false;
  }
  return true;
}

bool CTP7Client::dumpDecoderErrors(std::vector<uint32_t> &bc0Errors) {
  bc0Errors.clear();
  std::vector <CTP7::InputLinkRegisters> inputLinkRegisters;
  if(getInputLinkRegisters(inputLinkRegisters)) {
    for(uint32_t i = 0; i < NILinks; i++) {
      bc0Errors.push_back(inputLinkRegisters[i].BC0_ERR_CNT_REG);
    }
  }
  else {
    return false;
  }
  return true;
}

bool CTP7Client::dumpCRCErrors(std::vector<uint32_t> &crcErrors) {
  crcErrors.clear();
  std::vector <CTP7::InputLinkRegisters> inputLinkRegisters;
  if(getInputLinkRegisters(inputLinkRegisters)) {
    for(uint32_t i = 0; i < NILinks; i++) {
      crcErrors.push_back(inputLinkRegisters[i].CRC_ERR_CNT_REG);
    }
  }
  else {
    return false;
  }
  return true;
}

bool CTP7Client::dumpAllLinkIDs(std::vector<uint32_t> &linkIDs) {
  linkIDs.clear();
  std::vector <CTP7::InputLinkRegisters> inputLinkRegisters;
  if(getInputLinkRegisters(inputLinkRegisters)) {
    for(uint32_t i = 0; i < NILinks; i++) {
      linkIDs.push_back(inputLinkRegisters[i].LINK_ID_REG);
    }
  }
  else {
    return false;
  }
  return true;
}
