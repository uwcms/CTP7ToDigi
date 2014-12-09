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
    unsigned int *temp = (unsigned int *)oData;////
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

/*
 * Check that args do not exceed the maximum valid address
 * OR the index doesn't exceed the number of ints per link
 * OR the Link Number doesn't exceed the Numbr of Input Links or Output Links
 */

bool CTP7Client::checkArgs(BufferType bufferType,
			   unsigned int linkNumber,
			   unsigned int addressOffset,
			   unsigned int numberOfValues) {

  unsigned int maxIndex = (addressOffset + numberOfValues * 4) / 4 ;

  switch(bufferType){

  case registerBuffer:
    if(maxIndex <= MaxValidAddress) 
      return true;
    
  case inputBuffer:
    if(maxIndex <= NIntsPerLink)
      if(linkNumber < NILinks) 
	return true;

  case s2inputBuffer:
    if(maxIndex <= NIntsPerLink)
      if(linkNumber < S2NILinks) 
	return true;

  case outputBuffer:
    if(maxIndex <= NIntsPerLink) 
      if(linkNumber < NOLinks) 
	return true;
    
  case unnamed:
    std::cout << "Warning: Using unnamed buffers may cause bus errors :(" << std::endl;
    return true;

  default:
    std::cout<<"Message Failed Checkargs Step!"<<std::endl;

  }

  return false;
}

unsigned int CTP7Client::getAddress(BufferType bufferType, 
				    unsigned int linkNumber,
				    unsigned int addressOffset) {

  if(!checkArgs(bufferType, linkNumber, addressOffset )) return false;

  //msg is defined with len 64, this needs to be longer
  sprintf(msg, "getAddress(%x,%x,%x)", bufferType, linkNumber, addressOffset);
  ssize_t bytes_received = getResult();

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  msg[bytes_received] = '\0';

  unsigned int value;
  if(sscanf(msg, "%x", &value) != 1) {
    std::cerr << msg << std::endl;
    return NULL;
  }

  return value;

}

unsigned int CTP7Client::getLinkID(unsigned int linkNumber){

  CTP7::BufferType type = CTP7::registerBuffer;
  sprintf(msg, "getLinkID(%x,%x,%x)", type, 0, linkNumber);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  unsigned int value;

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  if(sscanf(msg, "%x", &value) != 1) {
    std::cerr << msg << std::endl;
    return NULL;
  }
  return value;
}

bool CTP7Client::setAddress(BufferType bufferType, 
			    unsigned int linkNumber,
			    unsigned int addressOffset, 
			    unsigned int value) {

  if(!checkArgs(bufferType, linkNumber, addressOffset)) return false;

  sprintf(msg, "setAddress(%x,%x,%x,%x)", bufferType, linkNumber, addressOffset, value);
  ssize_t bytes_received = getResult();
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }

  return true;
}

//TODO:not a great implementation for statusRegisterSize,
bool CTP7Client::dumpStatus(std::vector<unsigned int> &addressValues){

  unsigned int buffer[30]={0};
  sprintf(msg, "dumpStatus");
  ssize_t bytes_received = getResult(msg, buffer, MSGLEN, statusRegisterSize*4, true);
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((unsigned int) bytes_received != (statusRegisterSize*4)){
    std::cout<<"Wrong number of bytes received, bytes_received: "<< bytes_received << " statusRegisterSize*4: " << statusRegisterSize*4 <<std::endl;
    //return false;
  }

  for(unsigned int i = 0 ; i < statusRegisterSize+1 ; i++ ){
    addressValues.push_back(buffer[i]);
  }
  
  return true;
}

bool CTP7Client::dumpCRCErrors(std::vector<unsigned int> &addressValues){

  unsigned int buffer[40]={0};
  sprintf(msg, "dumpCRCErrors");

  ssize_t bytes_received = getResult(msg, buffer, MSGLEN, (NILinks+1)*4, false);

  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((unsigned int) bytes_received != ((NILinks+1)*4)){
    std::cout<<"Wrong number of bytes received"<<std::endl;
    //return false;
  }

  for(unsigned int i = 0 ; i < NILinks ; i++ ){
    addressValues.push_back(buffer[i]);
  }
  
  return true;
}

bool CTP7Client::dumpAllLinkIDs(std::vector<unsigned int> &addressValues){

  unsigned int buffer[NILinks]={0};
  sprintf(msg, "dumpAllLinkIDs");

  ssize_t bytes_received = getResult(msg, buffer, MSGLEN, (NILinks)*4, false);

  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((unsigned int) bytes_received != ((NILinks)*4)){
    std::cout<<"Wrong number of bytes received"<<std::endl;
    //return false;
  }

  for(unsigned int i = 0 ; i < NILinks; i++ ){
    addressValues.push_back(buffer[i]);
  }
  
  return true;
}

bool CTP7Client::dumpDecoderErrors(std::vector<unsigned int> &addressValues){

  unsigned int buffer[40]={0};
  sprintf(msg, "dumpDecoderErrors");
  ssize_t bytes_received = getResult(msg, buffer, MSGLEN, (NILinks+1)*4, false);
  msg[bytes_received] = '\0';

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((unsigned int) bytes_received != ((NILinks+1)*4)){
    std::cout<<"Wrong number of bytes received"<<std::endl;
    //return false;
  }
  for(unsigned int i = 0 ; i < NILinks+1 ; i++ ){
    addressValues.push_back(buffer[i]);
  }
  
  return true;
}

//address ordering should only be changed at the same time they change on the server
bool CTP7Client::setAddressNames(std::vector<std::string> & addressNames, unsigned int &version)
{
    version = 0xc0000001;
    addressNames.push_back( "QPLL_RESET"          );
    addressNames.push_back( "QPLL_LOCK"	          );
    addressNames.push_back( "DECODER_LOCKED_CXP0" );
    addressNames.push_back( "DECODER_LOCKED_CXP1" );
    addressNames.push_back( "DECODER_LOCKED_CXP2" );
    addressNames.push_back( "CAPTURE_DONE_CXP0"   );
    addressNames.push_back( "CAPTURE_DONE_CXP1"   );
    addressNames.push_back( "CAPTURE_DONE_CXP2"   );
    addressNames.push_back( "GT_RX_RESET_EN_CXP0" );
    addressNames.push_back( "GT_RX_RESET_EN_CXP1" );
    addressNames.push_back( "GT_RX_RESET_EN_CXP2" );
    addressNames.push_back( "TX_PRBS_SEL"	  );
    addressNames.push_back( "RX_PRBS_SEL"	  );
    addressNames.push_back( "GT_LOOPBACK"	  );
    addressNames.push_back( "MMCM_RST_EN"	  );
    addressNames.push_back( "MMCM_LOCKED"	  );
    addressNames.push_back( "BC0_ERR"	          );
    addressNames.push_back( "BC0_LOCKED"	  );
    addressNames.push_back( "FW_DATE_CODE"	  );
    addressNames.push_back( "FW_GIT_HASH"	  );
    addressNames.push_back( "FW_GIT_HASH_DIRTY"   );

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

bool CTP7Client::capture(){

  sprintf(msg, "capture");
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

/*
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
*/
bool CTP7Client::setConstantPattern(BufferType bufferType, 
				    unsigned int linkNumber, 
				    unsigned int value) {
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
				      unsigned int linkNumber, 
				      unsigned int startValue, 
				      unsigned int increment) {

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
				      unsigned int linkNumber, 
				      unsigned int startValue, 
				      unsigned int increment) {
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
				  unsigned int linkNumber, 
				  unsigned int randomSeed) {
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

bool CTP7Client::setPattern(BufferType bufferType,
			    unsigned int linkNumber,
			    unsigned int nInts,
			    std::vector<unsigned int> pattern) {

  if(!checkArgs(bufferType, linkNumber)){ 
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

bool CTP7Client::dumpContiguousBuffer(BufferType bufferType,
				      unsigned int linkNumber, 
				      unsigned int startAddressOffset, 
				      unsigned int numberOfValues, 
				      unsigned int *buffer) {

  sprintf(msg, "dumpContiguousBuffer(%x,%x,%x,%x)", 
	  bufferType, linkNumber, startAddressOffset, numberOfValues);

  if(!checkArgs(bufferType, linkNumber, startAddressOffset, numberOfValues)){
    std::cout<<"Failed Check Args Step "<<std::endl; 
      return false;
    }

 ssize_t bytes_received = getResult(msg, buffer, MSGLEN, numberOfValues*4, true);

  if(msg == NULL){
    printConnectionError();
    return false;
  }
    
  if((unsigned int)bytes_received == (numberOfValues*4)) {
    return true;
  }

  //std::cout<<"bytes_received "<<bytes_received<< "numberOfValues*4 "<<numberOfValues*4 <<std::endl;

  return false;
}


