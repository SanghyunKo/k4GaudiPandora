#ifndef PfoCreationAlgorithmIdea_h
#define PfoCreationAlgorithmIdea_h 1

#include "Pandora/Algorithm.h"
#include "Api/PandoraContentApi.h"
#include "Helpers/XmlHelper.h"

namespace lc_content { // TODO make this part of LCContent

class PfoCreationAlgorithmIdea : public pandora::Algorithm {
public:
  PfoCreationAlgorithmIdea();
  ~PfoCreationAlgorithmIdea()=default;

  // class Factory : public pandora::AlgorithmFactory {
  // public:
  //   pandora::Algorithm* CreateAlgorithm() const;
  // };

private:
  pandora::StatusCode Run() override;
  pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

  pandora::StatusCode CreateElectronCandidates() const;

  std::string m_outputPfoListName;
}; // class

// inline pandora::Algorithm* PfoCreationAlgorithmIdea::Factory::CreateAlgorithm() const {
//   return new PfoCreationAlgorithmIdea();
// }

} // namespace

#endif
