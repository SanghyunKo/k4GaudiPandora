from Gaudi.Configuration import INFO
from k4FWCore import ApplicationMgr, IOSvc
from Configurables import EventDataSvc
from Configurables import PandoraPFAIdeaAlgorithm

import os

iosvc = IOSvc()
iosvc.Input = "input_reco.root"
iosvc.Output = "output_pandora.root"

# detector geometry
# if K4GEO is empty, this should use relative path to working directory
from Configurables import GeoSvc

geoservice = GeoSvc("GeoSvc")
path_to_detector = os.environ.get("K4GEO", "")
detectors_to_use = [
    'FCCee/IDEA/compact/IDEA_o2_v01/IDEA_o2_v01.xml'
]

geoservice.detectors = [
    os.path.join(path_to_detector, _det) for _det in detectors_to_use
]


params = {
    "PandoraSettingsXmlFile": "PandoraSettingsIdea.xml",
    "inputTrackCollection": "TracksFromGenParticles",
    "inputClusterCollections": ["TopoGrownClusters"],
    "outputPfoCollection": "PandoraPfaIdea",
    "outputClusterCollection": "PandoraClusters",
    "CherenkovFieldName": "cherenkov",
    "inputCaloHitCollections": [
        "SCEPCal_digi_cheren",
        "SCEPCal_digi_scint",
        "DRBTScin_digi",
        "DRBTCher_digi",
        "DRETScinLeft_digi",
        "DRETCherLeft_digi",
        "DRETScinRight_digi",
        "DRETCherRight_digi",
    ],
    # the five vectors below are indexed together, one entry per calorimeter
    "CaloSystemIDs": [4, 5, 28, 25],
    "CaloCollectionTypes": ["ECAL", "ECAL", "HCAL", "HCAL"],
    "CaloLayerFieldNames": ["depth", "depth", "", ""],
    "CaloEncodingStrings": [
        "system:5,phi:7,theta:11,gamma:4,epsilon:4,depth:1,cherenkov:1",
        "system:5,phi:7,theta:11,gamma:4,epsilon:4,depth:1,cherenkov:1",
        "system:5,stave:10,tower:-8,air:6,col:-16,row:16,clad:1,core:1,cherenkov:1",
        "system:5,stave:10,tower:6,air:1,col:16,row:16,clad:1,core:1,cherenkov:1",
    ],
    "CaloCellSizes": [10, 10, 2., 2.],
}

pandoraIdea = PandoraPFAIdeaAlgorithm("PandoraPFAIdeaAlgorithm", **params)

ApplicationMgr(
    TopAlg=[pandoraIdea],
    EvtSel="NONE",
    EvtMax=-1,
    ExtSvc=[EventDataSvc("EventDataSvc"), geoservice],
    OutputLevel=INFO,
)
