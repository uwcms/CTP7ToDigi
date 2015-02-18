import FWCore.ParameterSet.Config as cms

process = cms.Process("CTP7ToDigiTester")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(170) )

process.source = cms.Source("EmptySource")

process.load("DQMServices.Core.DQM_cfg")
process.load("DQMServices.Components.DQMEnvironment_cfi")
process.dqmEnv.subSystemFolder = 'L1T'

process.load("DQM.L1TMonitorClient.L1TMonitorClient_cff")   

#process.load("DQM/L1TMonitor.environment_file_cff")
process.ctp7ToDigi = cms.EDProducer('CTP7ToDigi', 
#                                    ctp7Host = cms.untracked.string("144.92.181.245"),
                                    ctp7Host = cms.untracked.string("127.0.0.1"),
                                    ctp7Port = cms.untracked.string("5554"),
                                    test = cms.untracked.bool(False),
                                    createLinkFile = cms.untracked.bool(True),
                                    #Note to switch to MP7 Mapping you MUST put mp7Mapping to true AND change the testFile.txt to the MP7 Pattern File Name
                                    testFile = cms.untracked.string("testFile.txt"),
                                    mp7Mapping = cms.untracked.bool(False)
                                    )

process.p = cms.Path(process.ctp7ToDigi)

process.o1 = cms.OutputModule("PoolOutputModule",
                              outputCommands = cms.untracked.vstring('keep *'),
                              fileName = cms.untracked.string('CTP7ToDigi.root'))
process.outpath = cms.EndPath(process.o1)
