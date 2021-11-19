//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2021 QMCPACK developers.
//
// File developed by: Peter Doak, doakpw@ornl.gov, Oak Ridge National Laboratory
//
// Some code refactored from: DensityMatrices1b.cpp
//////////////////////////////////////////////////////////////////////////////////////

#include "string_utils.h"
#include "EstimatorInput.h"
#include "OneBodyDensityMatricesInput.h"

namespace qmcplusplus
{

OneBodyDensityMatricesInput::OneBodyDensityMatricesInput(xmlNodePtr cur)
{
  // This results in checkParticularValidity being called on OneBodyDensityMatrixInputSection
  input_section_.readXML(cur);
  auto setIfInInput = [&](auto& var, const std::string& tag) -> bool { return input_section_.setIfInInput(var, tag); };
  setIfInInput(energy_matrix_, "energy_matrix");
  setIfInInput(use_drift_, "use_drift");
  setIfInInput(normalized_, "normalized");
  setIfInInput(volume_normalized_, "volume_normalized");
  setIfInInput(check_overlap_, "check_overlap");
  setIfInInput(check_derivatives_, "check_derivatives");
  setIfInInput(rstats_, "rstats");
  setIfInInput(write_acceptance_ratio_, "acceptance_ratio");
  setIfInInput(integrator_, "integrator");
  setIfInInput(evaluator_, "evaluator");
  setIfInInput(scale_, "scale");
  center_defined_ = setIfInInput(center_, "center");
  corner_defined_ = setIfInInput(corner_, "corner");
  setIfInInput(timestep_, "timestep");
  setIfInInput(points_, "points");
  setIfInInput(samples_, "samples");
  setIfInInput(warmup_samples_, "warmup_samples");
  setIfInInput(basis_sets_, "basis");
}

void OneBodyDensityMatricesInput::OneBodyDensityMatrixInputSection::checkParticularValidity()
{
  using namespace estimatorinput;
  const std::string error_tag{"OneBodyDensityMatrices input: "};
  checkCenterCorner(input_section, error_tag);
  if (has("scale"))
  {
    Real scale = get<Real>("scale");
    std::cout << "SCALE is :" << scale << '\n';
    if (scale > 1.0 + 1e-10)
      throw UniformCommunicateError(error_tag + "scale must be less than one");
    else if (scale < 0.0 - 1e-10)
      throw UniformCommunicateError(error_tag + "scale must be greater than zero");
  }
  std::vector<std::string> basis_sets = get<std::vector<std::string>>("basis");
  if (basis_sets.size() == 0 || basis_sets[0].size() == 0)
    throw UniformCommunicateError(error_tag + "basis must have at least one sposet");
  // An example of how to do specific validation of input.
  if (get<std::string>("integrator") != "density")
  {
    if (has("acceptance_ratio") && get<bool>("acceptance_ratio") == true)
      throw UniformCommunicateError(error_tag + "acceptance_ratio can only be true for density integrator");
  }
  if (get<std::string>("integrator") == "uniform_grid")
  {
    if (has("samples"))
      throw UniformCommunicateError(error_tag +
                                    "samples are set from points for uniform_grid integrator and are invalid input");
  }
}

std::any OneBodyDensityMatricesInput::OneBodyDensityMatrixInputSection::assignAnyEnum(const std::string& name) const
{
  std::string enum_value_str(name + "-" + get<std::string>(name));
  tolower(enum_value_str);
  try
  {
    return lookup_input_enum_value.at(enum_value_str);
  }
  catch (std::out_of_range& oor_exc)
  {
    std::throw_with_nested(std::logic_error("bad_enum_tag_value: " + enum_value_str));
  }
}

} // namespace qmcplusplus
