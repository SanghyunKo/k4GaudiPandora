from Gaudi.Configuration import INFO
from k4FWCore import ApplicationMgr, IOSvc
from Configurables import EventDataSvc
from Configurables import DDPandoraPFAIdeaAlgorithm

import os

iosvc = IOSvc()
iosvc.Input = "output_REC.edm4hep.root"
iosvc.Output = "output_pandora.root"

# detector geometry
# if K4GEO is empty, this should use relative path to working directory
from Configurables import GeoSvc
import os
geoservice = GeoSvc("GeoSvc")
path_to_detector = os.environ.get("K4GEO", "")
detectors_to_use = [
    'FCCee/IDEA/compact/IDEA_o1_v03/IDEA_o1_v03.xml'
]

geoservice.detectors = [
    os.path.join(path_to_detector, _det) for _det in detectors_to_use
]


params = {
    "PandoraSettingsXmlFile": "PandoraSettingsIdea.xml",
    "inputCaloHitCollection" : "TopoClusterAllCells",
    "inputTrackCollection" : "TracksFromGenParticles",
    "inputClusterCollection" : "TopoClusterAll",
    "outputPfoCollection" : "PandoraPfaIdea"
}

pandoraIDEA = DDPandoraPFAIdeaAlgorithm("DDPandoraPFAIdeaAlgorithm", **params)

ApplicationMgr(
    TopAlg = [pandoraIDEA],
    EvtSel = "NONE",
    EvtMax = -1,
    ExtSvc = [EventDataSvc("EventDataSvc"),geoservice],
    OutputLevel = INFO,
)
