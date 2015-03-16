#ifndef RCTToDigi_h
#define RCTToDigi_h

// system include files

#include <vector>
#include <stdint.h>

// RCT data formats
#include "DataFormats/L1CaloTrigger/interface/L1CaloEmCand.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegion.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegionDetId.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloCollections.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"


typedef BXVector<L1CaloEmCand> L1CaloEmCandBxCollection;
typedef BXVector<L1CaloRegion> L1CaloRegionBxCollection;
#endif 
