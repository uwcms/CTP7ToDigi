import FWCore.ParameterSet.Config as cms

process = cms.Process("RCTToDigiTester")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1) )

process.source = cms.Source("EmptySource")

process.load("DQMServices.Core.DQM_cfg")
process.load("DQMServices.Components.DQMEnvironment_cfi")
process.dqmEnv.subSystemFolder = 'L1T'

process.load("DQM.L1TMonitorClient.L1TMonitorClient_cff")   

#process.load("DQM/L1TMonitor.environment_file_cff")
process.rctToDigi = cms.EDProducer('RCTToDigi', 
#                                    ctp7Host = cms.untracked.string("144.92.181.245"),
                                    ctp7Host = cms.untracked.string("127.0.0.1"),
                                    ctp7Port = cms.untracked.string("5554"),
                                    test = cms.untracked.bool(False),
                                    createLinkFile = cms.untracked.bool(True),
                                    testFile = cms.untracked.string("daqBuffers/daqBuffer-L1A-3359-nBCs-5.txt")
                                    )

process.p = cms.Path(process.rctToDigi)

process.o1 = cms.OutputModule("PoolOutputModule",
                              outputCommands = cms.untracked.vstring('keep *'),
                              fileName = cms.untracked.string('RCTToDigi.root'))
process.outpath = cms.EndPath(process.o1)
