#include "DataFormats/L1CaloTrigger/interface/L1CaloEmCand.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegion.h"
#include "DataFormats/L1Trigger/interface/L1EmParticle.h"
#include "Rtypes.h"
#include "Math/Cartesian3D.h"
#include "Math/Polar3D.h"
#include "Math/CylindricalEta3D.h"
#include "Math/PxPyPzE4D.h"
#include <boost/cstdint.hpp>
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "L1CaloBXCollections.hh"
#include "DataFormats/Common/interface/Wrapper.h"
#include  <cstddef>

namespace { struct dictionary {
  L1CaloEmCandBxCollection dummy0;
  edm::Wrapper<L1CaloEmCandBxCollection> dummy1;

  L1CaloRegionBxCollection dummy2;
  edm::Wrapper<L1CaloRegionBxCollection> dummy3;
};
}
