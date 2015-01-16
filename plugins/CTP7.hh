#ifndef CTP7_hh
#define CTP7_hh

#include <stdint.h>

#include <vector>
#include <string>

// This class defines the interface to CTP7
// The idea is to keep this pure virtual and enable client/server communication
// Syntax for externally accessible functions to get/set on-board buffers 
// and register groups are defined

// CTP7 has up to 67 input and 48 output gigabit capable links
// Not all links are enabled in firmware and the number of links
// is different for STAGE1 and STAGE2 firmware images.

#ifdef STAGE2
#define NILinks 64
#define NOLinks 48
#else
#define NILinks 36
#define NOLinks 0
#endif

// Each link has a data buffer associated with it
// These buffers are accessible on the ZYNQ via AXI bus
// Data may be captured or played back from there 

#define NIntsPerLink 1024
#define LinkBufSize NIntsPerLink * sizeof(int)

// There are RAM buffers to capture DAQ data and TCDS data

#define NIntsInDAQBuffer  2048
#define DAQBufferSize     NIntsInDAQBuffer * sizeof(int)
#define NIntsInTCDSBuffer 2048
#define TCDSBufferSize    NIntsInTCDSBuffer * sizeof(int)

class CTP7 {

public:

  virtual ~CTP7() {;}

  // These data structures provide access to various classes of CTP7 registers

  typedef struct InputLinkRegisters {
    uint32_t LINK_STATUS_REG;
    uint32_t BC0_LATENCY_REG;
    uint32_t LINK_ALIGN_MASK_REG;
    uint32_t CRC_ERR_CNT_REG;
    uint32_t BC0_ERR_CNT_REG;
    uint32_t LINK_ID_REG;
    uint32_t CAPTURE_STATE_REG;
    uint32_t CAPTURE_START_CTP7_BCID_REG;
    uint32_t CAPTURE_START_oRSC_BCID_REG;
    uint32_t SpareRegisters[7];
  } InputLinkRegisters;

  typedef struct LinkAlignmentRegisters {
    uint32_t LINK_ALIGN_REQ_REG;
    uint32_t LINK_ALIGN_FIFO_LATENCY_REG;
    uint32_t LINK_ALIGN_ERR_REG;
  } LinkAlignmentRegisters;

  typedef struct InputCaptureRegisters {
    uint32_t CAPTURE_REQ_REG;
    uint32_t CAPTURE_MODE_REG;
    uint32_t CAPTURE_START_BCID_REG;
    uint32_t CAPTURE_START_TCDS_CMD_REG;
  } InputCaptureRegisters;

  typedef struct DAQSpyCaptureRegisters {
    uint32_t DAQ_SPY_CAPTURE_REQ_REG;
    uint32_t DAQ_SPY_CAPTURE_DONE_REG;
    uint32_t DAQ_SPY_CAPTURE_STATE_REG;
    uint32_t DAQ_SPY_CAPTURE_EVENT_SIZE_REG;
  } DAQSpyCaptureRegisters;

  typedef struct DAQRegisters {
    uint32_t DAQ_L1A_DELAY_LINE_VALUE_REG;
    uint32_t DAQ_L1A_DELAY_LINE_RST_REG;
    uint32_t DAQ_READOUT_SIZE_REG;
    uint32_t DAQ_USER_DATA_REG;
  } DAQRegisters;

  typedef struct AMC13Registers {
    uint32_t AMC13_LINK_READY_REG;
  } AMC13Registers;

  typedef struct TCDSRegisters {
    uint32_t BC_CLOCK_RST_REG;
    uint32_t TCDS_STATUS_REG;
    uint32_t spare1[2];
    uint32_t TCDS_DECODER_RST_REG;
    uint32_t spare2[7];
    uint32_t TCDS_DECODER_ERR_CNT_RST_REG;
    uint32_t TCDS_DECODER_SNGL_ERR_CNT_REG;
    uint32_t TCDS_DECODER_DBL_ERR_CNT_REG;
  } TCDSRegisters;

  typedef struct TCDSMonitorRegisters {
    uint32_t TCDS_MON_CAPTURE_MASK_REG;
    uint32_t TCDS_MON_CAPTURE_REQ_REG;
    uint32_t TCDS_MON_CAPTURE_RUNNING_REG;
    uint32_t TCDS_MON_CAPTURE_DEPTH_REG;
    uint32_t TCDS_MON_CAPTURE_FULL_REG;
  } TCDSMonitorRegisters;

  typedef struct GTHRegisters {
    uint32_t GTH_STAT_REG;
    uint32_t GTH_RST_REG;
    uint32_t GTH_CTRL_REG;
    uint32_t spare[61];
  } GTHRegisters;

  typedef struct QPLLRegisters {
    uint32_t QPLL_STAT_REG;
    uint32_t QPLL_RST_REG;
    uint32_t spare[62];
  } QPLLRegisters;

  typedef struct MiscRegisters {
    uint32_t C_DATE_CODE_REG;
    uint32_t GITHASH_CODE_REG;
    uint32_t GITHASH_DIRTY_REG;
  } MiscRegisters;

  // Type of memory buffers or register groups available on CTP7

  enum BufferType {
    inputBuffer = 0,
    outputBuffer = 1,
    daqBuffer = 2,
    tcdsBuffer = 3,
    inputLinkRegisters = 4,
    linkAlignmentRegisters = 5,
    inputCaptureRegisters = 6,
    daqSpyCaptureRegisters = 7,
    daqRegisters = 8,
    amc13Registers = 9,
    tcdsRegisters = 10,
    tcdsMonitorRegisters = 11,
    gthRegisters = 12,
    qpllRegisters = 13,
    miscRegisters = 14,
    unnamed = 15
  };

  // Startup

  virtual bool checkConnection() = 0;

  // Configuration string for conveniently storing user information
  // This is just an arbitrary string that can be set and retrieved
  // and is left to the user to maintain -- it has no bearing on the
  // CTP7 or its functionality

  virtual bool getConfiguration(std::string output) = 0;
  virtual bool setConfiguration(std::string input) = 0;

  // Resets
  // These functions reset the CTP7 firmware to a known status

  virtual bool hardReset() = 0;
  virtual bool softReset() = 0;
  virtual bool counterReset() = 0;

  // Special functions for control

  enum CaptureStatus{
    Idle = 0,
    Armed = 1, 
    Capturing = 2,
    Done = 3
  };
  virtual bool getCaptureStatus(CaptureStatus *c) = 0;
  virtual bool capture() = 0;

  // Special test patterns for link input/output buffers
  // In principle setPattern() is generic and sufficient
  // However, the remaining functions were found to be 
  // convenient for debugging and are retained

  virtual bool setPattern(BufferType bufferType,
			  uint32_t linkNumber, 
			  uint32_t numberOfValues,
			  std::vector<uint32_t> values) = 0;

  virtual bool setConstantPattern(BufferType bufferType,
				  uint32_t linkNumber, 
				  uint32_t value) = 0;
  virtual bool setIncreasingPattern(BufferType bufferType,
				    uint32_t linkNumber, 
				    uint32_t startValue, 
				    uint32_t increment) = 0;
  virtual bool setDecreasingPattern(BufferType bufferType,
				    uint32_t linkNumber, 
				    uint32_t startValue, 
				    uint32_t increment) = 0;
  virtual bool setRandomPattern(BufferType bufferType,
				uint32_t linkNumber, 
				uint32_t randomSeed) = 0;

  // Generic functions for getting data 
  // One should avoid using this function in favor of getting 
  // structured data using access functions declared above
  // These are exposed temporarily to enable development hacks

  virtual uint32_t getValue(BufferType bufferType,
			    uint32_t addressOffset) = 0;
  
  virtual bool getValues(BufferType bufferType,
			 uint32_t startAddressOffset,
			 uint32_t numberOfValues, 
			 uint32_t *buffer) = 0;

  // Generic functions for setting data
  // One should avoid using these functions in favor of specific control 
  // related functions declared above
  // These are exposed temporarily to enable development hacks

  virtual bool setValue(BufferType bufferType,
			uint32_t addressOffset, 
			uint32_t value) = 0;

  virtual bool setValues(BufferType bufferType,
			 uint32_t startAddressOffset,
			 uint32_t numberOfValues, 
			 uint32_t *buffer) = 0;

};

#endif
