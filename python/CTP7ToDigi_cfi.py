import FWCore.ParameterSet.Config as cms

demo = cms.EDProducer('CTP7ToDigi',
                      ctp7Host = cms.untracked.string("localhost"),
                      ctp7Port = cms.untracked.string("5555"))
