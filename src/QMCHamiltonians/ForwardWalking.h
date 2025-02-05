//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeremy McMinnis, jmcminis@gmail.com, University of Illinois at Urbana-Champaign
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//////////////////////////////////////////////////////////////////////////////////////


/** @file ForwardWalking.h
 * @brief Declarations of ForwardWalking
 */
#ifndef QMCPLUSPLUS_FORWARDWALKING_H
#define QMCPLUSPLUS_FORWARDWALKING_H
#include "QMCHamiltonians/OperatorBase.h"
namespace qmcplusplus
{
class QMCHamiltonian;

struct ForwardWalking : public OperatorBase
{
  std::vector<int> Hindices;
  std::vector<int> Pindices;
  std::vector<std::vector<int>> walkerLengths;
  std::vector<RealType> Values;
  std::vector<std::string> Names;
  int blockT, nObservables, nValues, FirstHamiltonian;
  double count;

  /** constructor
   */
  ForwardWalking() { update_mode_.set(OPTIMIZABLE, 1); }

  ///destructor
  ~ForwardWalking() override {}

  void resetTargetParticleSet(ParticleSet& P) override {}

  inline Return_t rejectedMove(ParticleSet& P) override
  {
    for (int i = 0; i < nObservables; i++)
    {
      int lastindex = t_walker_->PHindex[Pindices[i]] - 1;
      if (lastindex < 0)
        lastindex += walkerLengths[i][2];
      t_walker_->addPropertyHistoryPoint(Pindices[i], t_walker_->PropertyHistory[Pindices[i]][lastindex]);
    }
    calculate(P);
    return 0.0;
  }

  inline Return_t calculate(ParticleSet& P)
  {
    std::vector<RealType>::iterator Vit = Values.begin();
    for (int i = 0; i < nObservables; i++)
    {
      int j       = 0;
      int FWindex = t_walker_->PHindex[Pindices[i]] - 1;
      while (j < walkerLengths[i][1])
      {
        FWindex -= walkerLengths[i][0];
        if (FWindex < 0)
          FWindex += walkerLengths[i][2];
        (*Vit) = t_walker_->PropertyHistory[Pindices[i]][FWindex];
        j++;
        Vit++;
      }
    }
    copy(Values.begin(), Values.end(), t_walker_->getPropertyBase() + FirstHamiltonian + my_index_);
    return 0.0;
  }


  inline Return_t evaluate(ParticleSet& P) override
  {
    for (int i = 0; i < nObservables; i++)
      t_walker_->addPropertyHistoryPoint(Pindices[i], P.PropertyList[Hindices[i]]);
    calculate(P);
    return 0.0;
  }

  bool put(xmlNodePtr cur) override { return true; }

  ///rename it to avoid conflicts with put
  bool putSpecial(xmlNodePtr cur, QMCHamiltonian& h, ParticleSet& P);

  bool get(std::ostream& os) const override
  {
    os << "ForwardWalking";
    return true;
  }

  std::unique_ptr<OperatorBase> makeClone(ParticleSet& qp, TrialWaveFunction& psi) final;

  void addObservables(PropertySetType& plist);

  void addObservables(PropertySetType& plist, BufferType& collectables) override;

  void setObservables(PropertySetType& plist) override
  {
    copy(Values.begin(), Values.end(), plist.begin() + my_index_);
  }

  void setParticlePropertyList(PropertySetType& plist, int offset) override
  {
    copy(Values.begin(), Values.end(), plist.begin() + my_index_ + offset);
  }
};
} // namespace qmcplusplus
#endif
