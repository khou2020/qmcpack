/////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/**@file HamiltonianFactory.cpp
 *@brief Manage instantiation of Coulomb-related potentials
 */
#include "QMCHamiltonians/HamiltonianFactory.h"
#include "QMCHamiltonians/QMCHamiltonian.h"
#include "QMCHamiltonians/CoulombPotential.h"
#include "QMCHamiltonians/CoulombPBCAA.h"
#include "QMCHamiltonians/CoulombPBCAB.h"
#if OHMMS_DIM == 3
#include "QMCHamiltonians/LocalCorePolPotential.h"
#include "QMCHamiltonians/ECPotentialBuilder.h"
#include "QMCHamiltonians/ForceBase.h"
#include "QMCHamiltonians/ForceCeperley.h"
#include "QMCHamiltonians/PulayForce.h"
#include "QMCHamiltonians/ZeroVarianceForce.h"
#if defined(HAVE_LIBFFTW)
#include "QMCHamiltonians/MPC.h"
#include "QMCHamiltonians/VHXC.h"
#endif
#endif
#include "OhmmsData/AttributeSet.h"

#ifdef QMC_CUDA
#include "QMCHamiltonians/CoulombPBCAA_CUDA.h"
#include "QMCHamiltonians/CoulombPBCAB_CUDA.h"
#include "QMCHamiltonians/CoulombPotential_CUDA.h"
#include "QMCHamiltonians/MPC_CUDA.h"
#endif

//#include <iostream>
namespace qmcplusplus
{
void
HamiltonianFactory::addMPCPotential(xmlNodePtr cur, bool isphysical)
{
#if OHMMS_DIM ==3 && defined(HAVE_LIBFFTW)
  string a("e"), title("MPC"), physical("no");
  OhmmsAttributeSet hAttrib;
  double cutoff = 30.0;
  hAttrib.add(title,"id");
  hAttrib.add(title,"name");
  hAttrib.add(cutoff,"cutoff");
  hAttrib.add(physical,"physical");
  hAttrib.put(cur);
  renameProperty(a);
  isphysical = (physical=="yes" || physical == "true");
#ifdef QMC_CUDA
  MPC_CUDA *mpc = new MPC_CUDA (*targetPtcl, cutoff);
#else
  MPC *mpc = new MPC (*targetPtcl, cutoff);
#endif
  targetH->addOperator(mpc, "MPC", isphysical);
#else
  APP_ABORT("HamiltonianFactory::addMPCPotential MPC is disabled because FFTW3 was not found during the build process.");
#endif // defined(HAVE_LIBFFTW)
}

void
HamiltonianFactory::addVHXCPotential(xmlNodePtr cur)
{
#if OHMMS_DIM==3 && defined(HAVE_LIBFFTW)
  string a("e"), title("VHXC");
  OhmmsAttributeSet hAttrib;
  bool physical = true;
  hAttrib.add(title,"id");
  hAttrib.add(title,"name");
  hAttrib.add(physical,"physical");
  hAttrib.put(cur);
  renameProperty(a);
  VHXC *vhxc = new VHXC (*targetPtcl);
  app_log() << "physical = " << physical << endl;
  targetH->addOperator(vhxc, "VHXC", physical);
#else
  APP_ABORT("HamiltonianFactory::addVHXCPotential VHXC is disabled because FFTW3 was not found during the build process.");
#endif // defined(HAVE_LIBFFTW)
}



void
HamiltonianFactory::addCoulombPotential(xmlNodePtr cur)
{
  string targetInp(targetPtcl->getName());
  string sourceInp(targetPtcl->getName());
  string title("ElecElec"),pbc("yes");
  string forces("no");
  bool physical = true;
  bool doForce = false;
  OhmmsAttributeSet hAttrib;
  hAttrib.add(title,"id");
  hAttrib.add(title,"name");
  hAttrib.add(targetInp,"target");
  hAttrib.add(sourceInp,"source");
  hAttrib.add(pbc,"pbc");
  hAttrib.add(physical,"physical");
  hAttrib.add(forces,"forces");
  hAttrib.put(cur);
  bool applyPBC= (PBCType && pbc=="yes");
  bool doForces = (forces == "yes") || (forces == "true");
  ParticleSet *ptclA=targetPtcl;
  if(sourceInp != targetPtcl->getName())
  {
    //renameProperty(sourceInp);
    PtclPoolType::iterator pit(ptclPool.find(sourceInp));
    if(pit == ptclPool.end())
    {
      ERRORMSG("Missing source ParticleSet" << sourceInp);
      APP_ABORT("HamiltonianFactory::addCoulombPotential");
      return;
    }
    ptclA = (*pit).second;
  }
  if(sourceInp == targetInp) // AA type
  {
    if(!applyPBC && ptclA->getTotalNum() == 1)
    {
      app_log() << "  CoulombAA for " << sourceInp << " is not created.  Number of particles == 1 and nonPeriodic" << endl;
      return;
    }
    bool quantum = (sourceInp==targetPtcl->getName());
#ifdef QMC_CUDA
    if(applyPBC)
      targetH->addOperator(new CoulombPBCAA_CUDA(*ptclA,quantum,doForces),title,physical);
    else
    {
      if(quantum)
        targetH->addOperator(new CoulombPotentialAA_CUDA(ptclA,true), title, physical);
      else
        targetH->addOperator(new CoulombPotential<double>(ptclA,0,quantum), title, physical);
    }
#else
    if(applyPBC)
      targetH->addOperator(new CoulombPBCAA(*ptclA,quantum,doForces),title,physical);
    else
    {
      targetH->addOperator(new CoulombPotential<double>(ptclA,0,quantum), title, physical);
    }
#endif
  }
  else //X-e type, for X=some other source
  {
#ifdef QMC_CUDA
    if(applyPBC)
      targetH->addOperator(new CoulombPBCAB_CUDA(*ptclA,*targetPtcl),title);
    else
      targetH->addOperator(new CoulombPotentialAB_CUDA(ptclA,targetPtcl),title);
#else
    if(applyPBC)
      targetH->addOperator(new CoulombPBCAB(*ptclA,*targetPtcl),title);
    else
      targetH->addOperator(new CoulombPotential<double>(ptclA,targetPtcl,true),title);
#endif
  }
}

// void
// HamiltonianFactory::addPulayForce (xmlNodePtr cur) {
//   string a("ion0"),targetName("e"),title("Pulay");
//   OhmmsAttributeSet hAttrib;
//   hAttrib.add(a,"source");
//   hAttrib.add(targetName,"target");

//   PtclPoolType::iterator pit(ptclPool.find(a));
//   if(pit == ptclPool.end()) {
//     ERRORMSG("Missing source ParticleSet" << a)
//     return;
//   }

//   ParticleSet* source = (*pit).second;
//   pit = ptclPool.find(targetName);
//   if(pit == ptclPool.end()) {
//     ERRORMSG("Missing target ParticleSet" << targetName)
//     return;
//   }
//   ParticleSet* target = (*pit).second;

//   targetH->addOperator(new PulayForce(*source, *target), title, false);

// }

void
HamiltonianFactory::addForceHam(xmlNodePtr cur)
{
#if OHMMS_DIM==3
  string a("ion0"),targetName("e"),title("ForceBase"),pbc("yes"),
         PsiName="psi0";
  OhmmsAttributeSet hAttrib;
  string mode("bare");
  //hAttrib.add(title,"id");
  hAttrib.add(title,"name");
  hAttrib.add(a,"source");
  hAttrib.add(targetName,"target");
  hAttrib.add(pbc,"pbc");
  hAttrib.add(mode,"mode");
  hAttrib.add(PsiName, "psi");
  hAttrib.put(cur);
  app_log() << "HamFac forceBase mode " << mode << endl;
  renameProperty(a);
  PtclPoolType::iterator pit(ptclPool.find(a));
  if(pit == ptclPool.end())
  {
    ERRORMSG("Missing source ParticleSet" << a)
    return;
  }
  ParticleSet* source = (*pit).second;
  pit = ptclPool.find(targetName);
  if(pit == ptclPool.end())
  {
    ERRORMSG("Missing target ParticleSet" << targetName)
    return;
  }
  ParticleSet* target = (*pit).second;
  //bool applyPBC= (PBCType && pbc=="yes");
  if(mode=="bare")
  {
    BareForce* bareforce = new BareForce(*source, *target);
    bareforce->put(cur);
    targetH->addOperator(bareforce, title, false);
  }
  else if(mode=="cep")
  {
    ForceCeperley* force_cep = new ForceCeperley(*source, *target);
    force_cep->put(cur);
    targetH->addOperator(force_cep, title, false);
  }
  else if(mode=="pulay")
  {
    OrbitalPoolType::iterator psi_it(psiPool.find(PsiName));
    if(psi_it == psiPool.end())
    {
      APP_ABORT("Unknown psi \""+PsiName+"\" for Pulay force.");
    }
    TrialWaveFunction &psi = *psi_it->second->targetPsi;
    targetH->addOperator(new PulayForce(*source, *target, psi),
                         "PulayForce", false);
  }
  else if(mode=="zero_variance")
  {
    app_log() << "Adding zero-variance force term.\n";
    OrbitalPoolType::iterator psi_it(psiPool.find(PsiName));
    if(psi_it == psiPool.end())
    {
      APP_ABORT("Unknown psi \""+PsiName+"\" for zero-variance force.");
    }
    TrialWaveFunction &psi = *psi_it->second->targetPsi;
    targetH->addOperator
    (new ZeroVarianceForce(*source, *target, psi), "ZVForce", false);
  }
  else
  {
    ERRORMSG("Failed to recognize Force mode " << mode);
    //} else if(mode=="FD") {
    //  targetH->addOperator(new ForceFiniteDiff(*source, *target), title, false);
  }
#endif
}

void
HamiltonianFactory::addPseudoPotential(xmlNodePtr cur)
{
#if OHMMS_DIM == 3
  string src("i"),title("PseudoPot"),wfname("invalid"),format("xml");
  OhmmsAttributeSet pAttrib;
  pAttrib.add(title,"name");
  pAttrib.add(src,"source");
  pAttrib.add(wfname,"wavefunction");
  pAttrib.add(format,"format"); //temperary tag to switch between format
  pAttrib.put(cur);
  if(format == "old")
  {
    APP_ABORT("pseudopotential Table format is not supported.");
  }
  renameProperty(src);
  renameProperty(wfname);
  PtclPoolType::iterator pit(ptclPool.find(src));
  if(pit == ptclPool.end())
  {
    ERRORMSG("Missing source ParticleSet" << src)
    return;
  }
  ParticleSet* ion=(*pit).second;
  OrbitalPoolType::iterator oit(psiPool.find(wfname));
  TrialWaveFunction* psi=0;
  if(oit == psiPool.end())
  {
    if(psiPool.empty())
      return;
    app_error() << "  Cannot find " << wfname << " in the Wavefunction pool. Using the first wavefunction."<< endl;
    psi=(*(psiPool.begin())).second->targetPsi;
  }
  else
  {
    psi=(*oit).second->targetPsi;
  }
  //remember the TrialWaveFunction used by this pseudopotential
  psiName=wfname;
  app_log() << endl << "  ECPotential builder for pseudopotential "<< endl;
  ECPotentialBuilder ecp(*targetH,*ion,*targetPtcl,*psi,myComm);
  ecp.put(cur);
#else
  APP_ABORT("HamiltonianFactory::addPseudoPotential\n pairpot@type=\"pseudo\" is invalid if DIM != 3");
#endif
}

void
HamiltonianFactory::addCorePolPotential(xmlNodePtr cur)
{
#if OHMMS_DIM == 3
  string src("i"),title("CorePol");
  OhmmsAttributeSet pAttrib;
  pAttrib.add(title,"name");
  pAttrib.add(src,"source");
  pAttrib.put(cur);
  PtclPoolType::iterator pit(ptclPool.find(src));
  if(pit == ptclPool.end())
  {
    ERRORMSG("Missing source ParticleSet" << src)
    return;
  }
  ParticleSet* ion=(*pit).second;
  QMCHamiltonianBase* cpp=(new LocalCorePolPotential(*ion,*targetPtcl));
  cpp->put(cur);
  targetH->addOperator(cpp, title);
#else
  APP_ABORT("HamiltonianFactory::addCorePolPotential\n pairpot@type=\"cpp\" is invalid if DIM != 3");
#endif
}

//  void
//  HamiltonianFactory::addConstCoulombPotential(xmlNodePtr cur, string& nuclei)
//  {
//    OhmmsAttributeSet hAttrib;
//    string hname("IonIon");
//    string forces("no");
//    hAttrib.add(forces,"forces");
//    hAttrib.add(hname,"name");
//    hAttrib.put(cur);
//    bool doForces = (forces == "yes") || (forces == "true");
//
//    app_log() << "  Creating Coulomb potential " << nuclei << "-" << nuclei << endl;
//    renameProperty(nuclei);
//    PtclPoolType::iterator pit(ptclPool.find(nuclei));
//    if(pit != ptclPool.end()) {
//      ParticleSet* ion=(*pit).second;
//      if(PBCType)
//      {
//#ifdef QMC_CUDA
//	targetH->addOperator(new CoulombPBCAA_CUDA(*ion,false,doForces),hname);
//#else
//	targetH->addOperator(new CoulombPBCAATemp(*ion,false,doForces),hname);
//#endif
//      } else {
//        if(ion->getTotalNum()>1)
//          targetH->addOperator(new CoulombPotential<double>(ion),hname);
//          //targetH->addOperator(new IonIonPotential(*ion),hname);
//      }
//    }
//  }

}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 5831 $   $Date: 2013-05-10 08:38:08 -0400 (Fri, 10 May 2013) $
 * $Id: HamiltonianFactory.cpp 5831 2013-05-10 12:38:08Z jnkim $
 ***************************************************************************/
