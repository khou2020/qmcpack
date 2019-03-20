//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Mark Dewing, mdewing@anl.gov, Argonne National Laboratory
//                    Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//
// File created by: Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//////////////////////////////////////////////////////////////////////////////////////


#include "QMCWaveFunctions/lcao/CuspCorrectionConstruction.h"

namespace qmcplusplus
{
// Modifies orbital set lcwc
void applyCuspCorrection(const Matrix<CuspCorrectionParameters>& info,
                         int num_centers,
                         int orbital_set_size,
                         ParticleSet& targetPtcl,
                         ParticleSet& sourcePtcl,
                         LCAOrbitalSetWithCorrection& lcwc,
                         const std::string& id)
{
  typedef QMCTraits::RealType RealType;

  LCAOrbitalSet phi = LCAOrbitalSet(lcwc.myBasisSet);
  phi.setOrbitalSetSize(lcwc.OrbitalSetSize);
  phi.BasisSetSize = lcwc.BasisSetSize;
  phi.setIdentity(false);

  LCAOrbitalSet eta = LCAOrbitalSet(lcwc.myBasisSet);
  eta.setOrbitalSetSize(lcwc.OrbitalSetSize);
  eta.BasisSetSize = lcwc.BasisSetSize;
  eta.setIdentity(false);


  std::vector<bool> corrCenter(num_centers, "true");

  //What's this grid's lifespan?  Why on the heap?
  LogGrid<RealType>* radial_grid = new LogGrid<RealType>;
  radial_grid->set(0.000001, 100.0, 1001);


  Vector<RealType> xgrid;
  Vector<RealType> rad_orb;
  xgrid.resize(radial_grid->size());
  rad_orb.resize(radial_grid->size());
  for (int ig = 0; ig < radial_grid->size(); ig++)
  {
    xgrid[ig] = radial_grid->r(ig);
  }

  for (int ic = 0; ic < num_centers; ic++)
  {
    *(eta.C) = *(lcwc.C);
    *(phi.C) = *(lcwc.C);

    splitPhiEta(ic, corrCenter, phi, eta);

    // loop over MO index - cot must be an array (of len MO size)
    //   the loop is inside cot - in the multiqunitic
    SoaCuspCorrection::COT* cot = new CuspCorrectionAtomicBasis<RealType>();
    cot->initializeRadialSet(*radial_grid, orbital_set_size);
    //How is this useful?
    // cot->ID.resize(orbital_set_size);
    // for (int mo_idx = 0; mo_idx < orbital_set_size; mo_idx++) {
    //   cot->ID[mo_idx] = mo_idx;
    // }

    for (int mo_idx = 0; mo_idx < orbital_set_size; mo_idx++)
    {
      computeRadialPhiBar(&targetPtcl, &sourcePtcl, mo_idx, ic, &phi, xgrid, rad_orb, info(ic, mo_idx));
      OneDimQuinticSpline<RealType> radial_spline(radial_grid, rad_orb);
      RealType yprime_i = (rad_orb[1] - rad_orb[0]) / (radial_grid->r(1) - radial_grid->r(0));
      radial_spline.spline(0, yprime_i, rad_orb.size() - 1, 0.0);
      cot->addSpline(mo_idx, radial_spline);

      if (outputManager.isDebugActive())
      {
        // For testing against AoS output
        // Output phiBar to soaOrbs.downdet.C0.MO0
        int nElms   = 500;
        RealType dx = info(ic, mo_idx).Rc * 1.2 / nElms;
        Vector<RealType> pos;
        Vector<RealType> output_orb;
        pos.resize(nElms);
        output_orb.resize(nElms);
        for (int i = 0; i < nElms; i++)
        {
          pos[i] = (i + 1.0) * dx;
        }
        computeRadialPhiBar(&targetPtcl, &sourcePtcl, mo_idx, ic, &phi, pos, output_orb, info(ic, mo_idx));
        std::string filename = "soaOrbs." + id + ".C" + std::to_string(ic) + ".MO" + std::to_string(mo_idx);
        std::cout << "Writing to " << filename << std::endl;
        std::ofstream out(filename.c_str());
        out << "# r phiBar(r)" << std::endl;
        for (int i = 0; i < nElms; i++)
        {
          out << pos[i] << "  " << output_orb[i] << std::endl;
        }
        out.close();
      }
    }
    lcwc.cusp.add(ic, cot);
  }
  removeSTypeOrbitals(corrCenter, lcwc);
}

void saveCusp(int orbital_set_size, int num_centers, Matrix<CuspCorrectionParameters>& info, std::string id)
{
  xmlDocPtr doc       = xmlNewDoc((const xmlChar*)"1.0");
  xmlNodePtr cuspRoot = xmlNewNode(NULL, BAD_CAST "qmcsystem");
  xmlNodePtr spo      = xmlNewNode(NULL, (const xmlChar*)"sposet");
  xmlNewProp(spo, (const xmlChar*)"name", (const xmlChar*)id.c_str());
  xmlAddChild(cuspRoot, spo);
  xmlDocSetRootElement(doc, cuspRoot);

  for (int center_idx = 0; center_idx < num_centers; center_idx++)
  {
    xmlNodePtr ctr = xmlNewNode(NULL, (const xmlChar*)"center");
    std::ostringstream num;
    num << center_idx;
    xmlNewProp(ctr, (const xmlChar*)"num", (const xmlChar*)num.str().c_str());

    for (int mo_idx = 0; mo_idx < orbital_set_size; mo_idx++)
    {
      std::ostringstream num0, C, sg, rc, a1, a2, a3, a4, a5;
      xmlNodePtr orb = xmlNewNode(NULL, (const xmlChar*)"orbital");
      num0 << mo_idx;
      xmlNewProp(orb, (const xmlChar*)"num", (const xmlChar*)num0.str().c_str());


      C.setf(std::ios::scientific, std::ios::floatfield);
      C.precision(14);
      C << info(center_idx, mo_idx).C;
      sg.setf(std::ios::scientific, std::ios::floatfield);
      sg.precision(14);
      sg << info(center_idx, mo_idx).sg;
      rc.setf(std::ios::scientific, std::ios::floatfield);
      rc.precision(14);
      rc << info(center_idx, mo_idx).Rc;
      a1.setf(std::ios::scientific, std::ios::floatfield);
      a1.precision(14);
      a1 << info(center_idx, mo_idx).alpha[0];
      a2.setf(std::ios::scientific, std::ios::floatfield);
      a2.precision(14);
      a2 << info(center_idx, mo_idx).alpha[1];
      a3.setf(std::ios::scientific, std::ios::floatfield);
      a3.precision(14);
      a3 << info(center_idx, mo_idx).alpha[2];
      a4.setf(std::ios::scientific, std::ios::floatfield);
      a4.precision(14);
      a4 << info(center_idx, mo_idx).alpha[3];
      a5.setf(std::ios::scientific, std::ios::floatfield);
      a5.precision(14);
      a5 << info(center_idx, mo_idx).alpha[4];
      xmlNewProp(orb, (const xmlChar*)"C", (const xmlChar*)C.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"sg", (const xmlChar*)sg.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"rc", (const xmlChar*)rc.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"a1", (const xmlChar*)a1.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"a2", (const xmlChar*)a2.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"a3", (const xmlChar*)a3.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"a4", (const xmlChar*)a4.str().c_str());
      xmlNewProp(orb, (const xmlChar*)"a5", (const xmlChar*)a5.str().c_str());
      xmlAddChild(ctr, orb);
    }
    xmlAddChild(spo, ctr);
  }

  std::string fname = id + ".cuspInfo.xml";
  app_log() << "Saving resulting cusp Info xml block to: " << fname << std::endl;
  xmlSaveFormatFile(fname.c_str(), doc, 1);
  xmlFreeDoc(doc);
}

void generateCuspInfo(int orbital_set_size,
                      int num_centers,
                      Matrix<CuspCorrectionParameters>& info,
                      ParticleSet& targetPtcl,
                      ParticleSet& sourcePtcl,
                      LCAOrbitalSetWithCorrection& lcwc,
                      std::string id)
{
  typedef QMCTraits::RealType RealType;

  LCAOrbitalSet phi = LCAOrbitalSet(lcwc.myBasisSet);
  phi.setOrbitalSetSize(lcwc.OrbitalSetSize);
  phi.BasisSetSize = lcwc.BasisSetSize;
  phi.setIdentity(false);

  LCAOrbitalSet eta = LCAOrbitalSet(lcwc.myBasisSet);
  eta.setOrbitalSetSize(lcwc.OrbitalSetSize);
  eta.BasisSetSize = lcwc.BasisSetSize;
  eta.setIdentity(false);


  std::vector<bool> corrCenter(num_centers, "true");

  typedef OneDimGridBase<RealType> GridType;
  int npts = 500;


  for (int center_idx = 0; center_idx < num_centers; center_idx++)
  {
    std::cout << "Working on Center " << center_idx << std::endl;
    *(eta.C) = *(lcwc.C);
    *(phi.C) = *(lcwc.C);

    splitPhiEta(center_idx, corrCenter, phi, eta);
    for (int mo_idx = 0; mo_idx < orbital_set_size; mo_idx++)
    {
      bool corrO = false;
      auto& cref(*(phi.C));
      for (int ip = 0; ip < cref.cols(); ip++)
      {
        if (std::abs(cref(mo_idx, ip)) > 0)
        {
          corrO = true;
          break;
        }
      }

      if (corrO)
      {
        std::cout << "Working on Mo " << mo_idx << std::endl;
        OneMolecularOrbital etaMO(&targetPtcl, &sourcePtcl, &eta);
        etaMO.changeOrbital(center_idx, mo_idx);

        OneMolecularOrbital phiMO(&targetPtcl, &sourcePtcl, &phi);
        phiMO.changeOrbital(center_idx, mo_idx);

        SpeciesSet& tspecies(sourcePtcl.getSpeciesSet());
        int iz     = tspecies.addAttribute("charge");
        RealType Z = tspecies(iz, sourcePtcl.GroupID[center_idx]);

        RealType Rc_max = 0.2;
        RealType rc     = 0.1;

        RealType dx = rc * 1.2 / npts;
        ValueVector_t pos(npts);
        ValueVector_t ELideal(npts);
        ValueVector_t ELcurr(npts);
        for (int i = 0; i < npts; i++)
        {
          pos[i] = (i + 1.0) * dx;
        }

        RealType eta0 = etaMO.phi(0.0);
        ValueVector_t ELorig(npts);
        CuspCorrection cusp(info(center_idx, mo_idx));
        minimizeForRc(cusp, phiMO, Z, rc, Rc_max, eta0, pos, ELcurr, ELideal);
        info(center_idx, mo_idx) = cusp.cparam;
      }
    }
  }
  saveCusp(orbital_set_size, num_centers, info, id);
}

} // namespace qmcplusplus
