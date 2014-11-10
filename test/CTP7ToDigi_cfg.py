import FWCore.ParameterSet.Config as cms

process = cms.Process("CTP7ToDigiTester")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(170) )

process.source = cms.Source("EmptySource")

process.ctp7ToDigi = cms.EDProducer('CTP7ToDigi', 
                                    ctp7Host = cms.untracked.string("144.92.181.245"),
                                    ctp7Port = cms.untracked.string("5555"))

process.p = cms.Path(process.ctp7ToDigi)

process.o1 = cms.OutputModule("PoolOutputModule",
                              outputCommands = cms.untracked.vstring('keep *'),
                              fileName = cms.untracked.string('CTP7ToDigi.root'))
process.outpath = cms.EndPath(process.o1)
