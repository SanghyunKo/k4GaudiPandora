#include "DualReadoutCaloHitCreator.h"

#include "Pandora/PandoraEnumeratedTypes.h"
#include "Pandora/PandoraInputTypes.h"

DualReadoutCaloHitCreator::DualReadoutCaloHitCreator(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm)
    : m_settings(settings), m_pandora(pandora), /*m_volumeManager(),*/ m_algorithm(*algorithm) {
  // load volume manager
  // dd4hep::Detector& theDetector = dd4hep::Detector::getInstance();
  // m_volumeManager = theDetector.volumeManager();
  //
  // if (!m_volumeManager.isValid()) {
  //   theDetector.apply("DD4hepVolumeManager", 0, nullptr);
  //   m_volumeManager = theDetector.volumeManager();
  // }
}

pandora::StatusCode DualReadoutCaloHitCreator::createCaloHits(const std::vector<edm4hep::CalorimeterHit>& inputCaloHits) const {
  if (inputCaloHits.empty())
    return pandora::STATUS_CODE_SUCCESS;

  try {
    for (const auto& hit : inputCaloHits) {
      // create Pandora calo hit
      PandoraApi::CaloHit::Parameters caloHitParameters;
      // see PandoraSDK/include/Pandora/ObjectCreation.h for the full list
      auto pos = hit.getPosition();
      caloHitParameters.m_positionVector = pandora::InputCartesianVector(pandora::CartesianVector(pos.x, pos.y, pos.z));
      caloHitParameters.m_cellGeometry = pandora::InputCellGeometry(pandora::CellGeometry::POINTING);
      caloHitParameters.m_cellSize0 = 1.5; // mm (edm4hep unit) FIXME make configurable
      caloHitParameters.m_cellSize1 = 1.5; // mm (edm4hep unit) FIXME make configurable
      caloHitParameters.m_time = pandora::InputFloat(hit.getTime());
      caloHitParameters.m_inputEnergy = pandora::InputFloat(hit.getEnergy());
      caloHitParameters.m_hitType = pandora::HitType::ECAL; // TODO push scint & Cheren calo hit types to PandoraSDK
      caloHitParameters.m_hitRegion = pandora::HitRegion::SINGLE_REGION; // TODO identify barrel or endcap

      // dummy parameters
      caloHitParameters.m_isDigital = pandora::InputBool(false);
      caloHitParameters.m_layer = 0; // always zero for DRC
      caloHitParameters.m_expectedDirection = caloHitParameters.m_positionVector.Get().GetUnitVector();
      caloHitParameters.m_cellNormalVector = caloHitParameters.m_positionVector.Get().GetUnitVector();
      caloHitParameters.m_cellThickness = 2000.; // mm (edm4hep unit) FIXME make configurable
      caloHitParameters.m_nCellRadiationLengths = 100.; // FIXME make configurable
      caloHitParameters.m_nCellInteractionLengths = 8; // FIXME make configurable
      caloHitParameters.m_mipEquivalentEnergy = 0.1; // GeV (edm4hep unit) FIXME make configurable
      caloHitParameters.m_electromagneticEnergy = caloHitParameters.m_inputEnergy; // TODO this parameter doesn't make sense at all for DRC
      caloHitParameters.m_hadronicEnergy = 0.; // TODO this parameter doesn't make sense at all for DRC (for now assume pure EM)
      caloHitParameters.m_isInOuterSamplingLayer = false;

      // address to edm4hep calo hit
      caloHitParameters.m_pParentAddress = pandora::InputAddress(&hit);

      PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                              PandoraApi::CaloHit::Create(m_pandora, caloHitParameters));
    } // hits
  } catch (const std::exception& e) {
    m_algorithm.error() << "Exception processing dual-readout hit: " << e.what() << endmsg;
  }

  return pandora::STATUS_CODE_SUCCESS;
}
