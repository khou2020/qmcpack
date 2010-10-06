//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/QMCLinearOptimize.h"
#include "Particle/HDFWalkerIO.h"
#include "Particle/DistanceTable.h"
#include "OhmmsData/AttributeSet.h"
#include "Message/CommOperators.h"
#if defined(ENABLE_OPENMP)
#include "QMCDrivers/VMC/VMCSingleOMP.h"
#include "QMCDrivers/QMCCostFunctionOMP.h"
#endif
#include "QMCDrivers/VMC/VMCSingle.h"
#include "QMCDrivers/QMCCostFunctionSingle.h"
#include "QMCApp/HamiltonianPool.h"
#include "Numerics/Blasf.h"
#include <cassert>
#if defined(QMC_CUDA)
#include "QMCDrivers/VMC/VMC_CUDA.h"
#include "QMCDrivers/QMCCostFunctionCUDA.h"
#endif

#include <iostream>
#include <fstream>

/*#include "Message/Communicate.h"*/

namespace qmcplusplus
{

QMCLinearOptimize::QMCLinearOptimize(MCWalkerConfiguration& w,
                                     TrialWaveFunction& psi, QMCHamiltonian& h, HamiltonianPool& hpool): QMCDriver(w,psi,h),
        PartID(0), NumParts(1), WarmupBlocks(10),
        SkipSampleGeneration("no"), hamPool(hpool),
        optTarget(0), vmcEngine(0), Max_iterations(1),
        wfNode(NULL), optNode(NULL), allowedCostDifference(2.0e-6), exp0(-16),
        nstabilizers(3), stabilizerScale(4.0), bigChange(1), eigCG(1), w_beta(1),
        UseQuarticMin("yes")
{
    //set the optimization flag
    QMCDriverMode.set(QMC_OPTIMIZE,1);
    //read to use vmc output (just in case)
    RootName = "pot";
    QMCType ="QMCLinearOptimize";
    optmethod = "Linear";
    m_param.add(WarmupBlocks,"warmupBlocks","int");
    m_param.add(SkipSampleGeneration,"skipVMC","string");
    m_param.add(Max_iterations,"max_its","int");
    m_param.add(nstabilizers,"nstabilizers","int");
    m_param.add(stabilizerScale,"stabilizerscale","double");
    m_param.add(allowedCostDifference,"alloweddifference","double");
    m_param.add(bigChange,"bigchange","double");
    m_param.add(eigCG,"eigcg","int");
    m_param.add(w_beta,"beta","double");
    quadstep=3.0;
    m_param.add(quadstep,"stepsize","double");
    m_param.add(exp0,"exp0","double");
    m_param.add(UseQuarticMin,"UseQuarticMin","string");
    m_param.add(LambdaMax,"LambdaMax","double");
    //Set parameters for line minimization:
    this->add_timers(myTimers);

}

/** Clean up the vector */
QMCLinearOptimize::~QMCLinearOptimize()
{
    delete vmcEngine;
    delete optTarget;
}

void QMCLinearOptimize::add_timers(vector<NewTimer*>& timers)
{
  timers.push_back(new NewTimer("QMCLinearOptimize::GenerateSamples")); 
  timers.push_back(new NewTimer("QMCLinearOptimize::Initialize")); 
  timers.push_back(new NewTimer("QMCLinearOptimize::Eigenvalue")); 
  timers.push_back(new NewTimer("QMCLinearOptimize::Line_Minimization"));
  timers.push_back(new NewTimer("QMCLinearOptimize::GradCost"));
  for (int i=0; i<timers.size(); ++i) TimerManager.addTimer(timers[i]);
}

QMCLinearOptimize::RealType QMCLinearOptimize::Func(RealType dl)
{
    for (int i=0; i<optparm.size(); i++) optTarget->Params(i) = optparm[i] + dl*optdir[i];
    QMCLinearOptimize::RealType c = optTarget->Cost(false);
    validFuncVal= optTarget->IsValid;
    return c;
}

/** Add configuration files for the optimization
* @param a root of a hdf5 configuration file
*/
void QMCLinearOptimize::addConfiguration(const string& a)
{
    if (a.size()) ConfigFile.push_back(a);
}

bool QMCLinearOptimize::run()
{
    optTarget->initCommunicator(myComm);
    //close files automatically generated by QMCDriver
    //branchEngine->finalize();


    //generate samples
    myTimers[0]->start();
    generateSamples();
    myTimers[0]->stop();

    app_log() << "<opt stage=\"setup\">" << endl;
    app_log() << "  <log>"<<endl;

    //reset the rootname
    optTarget->setRootName(RootName);
    optTarget->setWaveFunctionNode(wfNode);

    app_log() << "   Reading configurations from h5FileRoot " << endl;
    //get configuration from the previous run
    Timer t1;
    

    myTimers[1]->start();
    optTarget->getConfigurations(h5FileRoot);
    optTarget->checkConfigurations();
    myTimers[1]->stop();

    app_log() << "  Execution time = " << t1.elapsed() << endl;
    app_log() << "  </log>"<<endl;
    app_log() << "</opt>" << endl;

    app_log() << "<opt stage=\"main\" walkers=\""<< optTarget->getNumSamples() << "\">" << endl;
    app_log() << "  <log>" << endl;

//       optTarget->setTargetEnergy(branchEngine->getEref());

    t1.restart();

    ///Here is our optimization routine
    bool Valid(true);
    int Total_iterations(0);

    TOL = allowedCostDifference;

//size of matrix
    int N=optTarget->NumParams() + 1;
    int numParams = optTarget->NumParams();
    optdir.resize(numParams,0);

    vector<RealType> currentParameterDirections(N,0);
    vector<RealType> currentParameters(numParams,0);
    for (int i=0; i<numParams; i++) currentParameters[i] = optTarget->Params(i);
    while (Max_iterations>Total_iterations)
    {
        Total_iterations+=1;
        app_log()<<"Iteration: "<<Total_iterations<<"/"<<Max_iterations<<endl;

// mmorales
        if (!Valid)
        {
            app_log() <<"Aborting current opt cycle due to small wfn overlap during correlated sampling. If this happens too frequently, try reducing the step size of the line minimization or reduce the number of cycles. " <<endl;
            continue;
        }
//         store this for use in later tries
        int bestStability(0);
//this is the small amount added to the diagonal to stabilize the eigenvalue equation. 10^stabilityBase
        RealType stabilityBase(exp0);

        vector<vector<RealType> > LastDirections;
        RealType deltaPrms(-1.0);
        RealType storedQuadStep = quadstep;
        for (int tries=0; tries<eigCG; tries++)
        {
            Matrix<RealType> Ham(N,N);
            Matrix<RealType> Ham2(N,N);
            Matrix<RealType> S(N,N);
            vector<RealType> BestDirection(N,0);



            for (int i=0; i<numParams; i++) optTarget->Params(i) = currentParameters[i];

//           checkConfigurations should be rewritten to include the necessary functions of Cost.
//           optTarget->checkConfigurations();
            myTimers[4]->start();
            RealType lastCost(optTarget->Cost(true));
            myTimers[4]->start();
            RealType newCost(lastCost);
            optTarget->fillOverlapHamiltonianMatrices(Ham2, Ham, S);

// mmorales
            if (!optTarget->IsValid)
            {
                app_log()<<"Invalid Cost Function!"<<endl;
                Valid=false;
                continue;
            }


            vector<RealType> bestParameters(currentParameters);
            bool acceptedOneMove(false);
            int tooManyTries(10);
            for (int stability=0; stability<nstabilizers; stability++)
            {
                Matrix<RealType> HamT(N,N), ST(N,N), HamT2(N,N);
                for (int i=0; i<N; i++)
                    for (int j=0; j<N; j++)
                    {
                        HamT(i,j)= (Ham)(j,i);
                        ST(i,j)= (S)(j,i);
                        HamT2(i,j)= (Ham2)(j,i);
                    }

                RealType Xs = std::pow(10.0,stabilityBase + stabilizerScale*stability);
                for (int i=1; i<N; i++) HamT(i,i) += Xs;

                Matrix<RealType> ST2(N,N);
                if (w_beta>=0.0)
                {
                    RealType H2rescale=1.0/HamT2(0,0);
                    for (int i=0; i<N; i++)  for (int j=0; j<N; j++) HamT2(i,j) *= H2rescale;
                    for (int i=0; i<N; i++)  for (int j=0; j<N; j++) ST2(i,j) = (1.0-w_beta)*ST(i,j) + w_beta*HamT2(i,j);
                }
                else
                {
                    Matrix<RealType> VarT(HamT2);
                    //~ true variance minimization
                    //~ RealType ENoffset = std::sqrt(HamT2(0,0));
                    RealType ENoffset = HamT(0,0);
                    for (int i=0; i<N; i++)  for (int j=0; j<N; j++) VarT(i,j) -= ENoffset*HamT(i,j);
                    for (int i=1; i<N; i++) VarT(i,i) -= ENoffset*Xs;
                    HamT=VarT;
                    ST2=ST;
                }
                
                myTimers[2]->start();
                getLowestEigenvector(HamT,ST2,currentParameterDirections);
                myTimers[2]->stop();

                RealType lowestEV = getLowestEigenvector(HamT,ST2,currentParameterDirections);
                if((w_beta>=0.0) && (abs(lowestEV/Ham(0,0))>1.5) && (lowestEV+10.0<Ham(0,0)))
                {
                  app_log()<<"Probably will not converge: E_lin="<<lowestEV<<" H(0,0)="<<Ham(0,0)<<endl;
                  //try a larger stability base and repeat
                  stabilityBase+=stabilizerScale;
                  //maintain same number of "good" stability tries
                  stability-=1;
                  continue;
                }
                
                //eigenCG part
                for (int ldi=0; ldi<LastDirections.size(); ldi++)
                {
                    RealType nrmold(0), ovlpold(0);
                    for (int i=1; i<N; i++) nrmold += LastDirections[ldi][i]*LastDirections[ldi][i];
                    for (int i=1; i<N; i++) ovlpold += LastDirections[ldi][i]*currentParameterDirections[i];
                    ovlpold*=1.0/nrmold;
                    for (int i=1; i<N; i++) currentParameterDirections[i] -= ovlpold * LastDirections[ldi][i];
                }

                optparm= currentParameters;
                for (int i=0; i<numParams; i++) optdir[i] = currentParameterDirections[i+1];

                RealType bigVec(0);
                for (int i=0; i<numParams; i++) bigVec = std::max(bigVec,std::abs(optdir[i]));
                TOL = allowedCostDifference/bigVec;

                largeQuarticStep=bigChange/bigVec;
                if (deltaPrms>0) quadstep=deltaPrms/bigVec;
                else quadstep = storedQuadStep/bigVec;

                myTimers[3]->start();
                if (UseQuarticMin=="yes")  Valid=lineoptimization();
                else  Valid=lineoptimization2();
                myTimers[3]->stop();

                if (!Valid)
                {
                    app_log()<<"Invalid Cost Function!"<<endl;
                    continue;
                }

                RealType biggestParameterChange = bigVec*std::abs(Lambda);

                if ((abs(Lambda)<1e100)&&(biggestParameterChange<bigChange))
                {
//                 app_log() <<endl <<"lambda: " <<Lambda <<endl;
//                 for (int i=0;i<numParams; i++) {
//                   app_log() <<"param: i, currValue, optdir: "
//                             <<i <<"  " <<optparm[i] <<"  " <<optdir[i] <<endl;
//                 }


                    for (int i=0; i<numParams; i++) optTarget->Params(i) = optparm[i] + Lambda * optdir[i];
                    //Might need to recompute this if the bracketing is too wide
//                 newCost = optTarget->computedCost();
                    newCost = optTarget->Cost(false);

// mmorales
                    if (!optTarget->IsValid)
                    {
                        app_log()<<"Invalid Cost Function!"<<endl;
                        Valid=false;
                        continue;
                    }
                    app_log()<<" OldCost: "<<lastCost<<" NewCost: "<<newCost<<"  Largest Parameter Change: "<<biggestParameterChange<<" Lambda: "<<Lambda<<endl;
                    optTarget->printEstimates();
//                 quit if newcost is greater than lastcost. E(Xs) looks quadratic (between steepest descent and parabolic)

                    if ((newCost < lastCost)&&(newCost==newCost))
                    {
                        //Move was acceptable
                        for (int i=0; i<numParams; i++) bestParameters[i] = optTarget->Params(i);
                        bestStability=stability;
                        lastCost=newCost;
                        BestDirection=currentParameterDirections;
                        acceptedOneMove=true;

                        deltaPrms=std::abs(Lambda*bigVec);
                    }
//                else if (newCost>lastCost+0.001) stability = nstabilizers;
                }
                else
                {
                    app_log()<<"  Failed Step. Largest parameter change:"<<biggestParameterChange<<endl;
                    tooManyTries--;
                    if ((tooManyTries>0) && (w_beta<0.0))
                    {
                        stabilityBase+=stabilizerScale;
                        stability-=1;
                        app_log()<<" Re-run Pure Variance with stabilizer:"<<stabilityBase<<endl;

                        continue;
                    }
                }

            }

            if (acceptedOneMove)
            {
                for (int i=0; i<numParams; i++) optTarget->Params(i) = bestParameters[i];
                currentParameters=bestParameters;
                LastDirections.push_back(BestDirection);
//             app_log()<< " Wave Function Parameters updated."<<endl;
//             optTarget->reportParameters();
            }
            else
            {
                for (int i=0; i<numParams; i++) optTarget->Params(i) = currentParameters[i];
                tries=eigCG;
            }

        }
    }


    MyCounter++;
    app_log() << "  Execution time = " << t1.elapsed() << endl;
    TimerManager.print(myComm);
    TimerManager.reset();
    app_log() << "  </log>" << endl;
    optTarget->reportParameters();
    app_log() << "</opt>" << endl;
    app_log() << "</optimization-report>" << endl;
    return (optTarget->getReportCounter() > 0);
}

void QMCLinearOptimize::generateSamples()
{
    Timer t1;
    app_log() << "<optimization-report>" << endl;
    //if(WarmupBlocks)
    //{
    //  app_log() << "<vmc stage=\"warm-up\" blocks=\"" << WarmupBlocks << "\">" << endl;
    //  //turn off QMC_OPTIMIZE
    //  vmcEngine->setValue("blocks",WarmupBlocks);
    //  vmcEngine->QMCDriverMode.set(QMC_WARMUP,1);
    //  vmcEngine->run();
    //  vmcEngine->setValue("blocks",nBlocks);
    //  app_log() << "  Execution time = " << t1.elapsed() << endl;
    //  app_log() << "</vmc>" << endl;
    //}

    if (W.getActiveWalkers()>NumOfVMCWalkers)
    {
        W.destroyWalkers(W.getActiveWalkers()-NumOfVMCWalkers);
        app_log() << "  QMCLinearOptimize::generateSamples removed walkers." << endl;
        app_log() << "  Number of Walkers per node " << W.getActiveWalkers() << endl;
    }

    vmcEngine->QMCDriverMode.set(QMC_OPTIMIZE,1);
    vmcEngine->QMCDriverMode.set(QMC_WARMUP,0);

    //vmcEngine->setValue("recordWalkers",1);//set record
    vmcEngine->setValue("current",0);//reset CurrentStep
    app_log() << "<vmc stage=\"main\" blocks=\"" << nBlocks << "\">" << endl;
    t1.restart();
    //     W.reset();
    branchEngine->flush(0);
    branchEngine->reset();
    vmcEngine->run();
    app_log() << "  Execution time = " << t1.elapsed() << endl;
    app_log() << "</vmc>" << endl;

    //write parameter history and energies to the parameter file in the trial wave function through opttarget
    RealType e,w,var;
    vmcEngine->Estimators->getEnergyAndWeight(e,w,var);
    optTarget->recordParametersToPsi(e,var);

    //branchEngine->Eref=vmcEngine->getBranchEngine()->Eref;
//         branchEngine->setTrialEnergy(vmcEngine->getBranchEngine()->getEref());
    //set the h5File to the current RootName
    h5FileRoot=RootName;
}

QMCLinearOptimize::RealType QMCLinearOptimize::getLowestEigenvector(Matrix<RealType>& A, Matrix<RealType>& B, vector<RealType>& ev)
{
  int N(ev.size());
  //Tested the single eigenvalue speed and It was no faster.
  //segfault issues with single eigenvalue problem for some machines
  bool singleEV(false);
  if (singleEV)
  {
    Matrix<double> TAU(N,N);
    int INFO;
    int LWORK(-1);
    vector<RealType> WORK(1);
    //optimal work size
    dgeqrf( &N, &N, B.data(), &N, TAU.data(), &WORK[0], &LWORK, &INFO);
    LWORK=int(WORK[0]);
    WORK.resize(LWORK);
    //QR factorization of S, or H2 matrix. to be applied to H before solve.
    dgeqrf( &N, &N, B.data(), &N, TAU.data(), &WORK[0], &LWORK, &INFO);

    char SIDE('L');
    char TRANS('T');
    LWORK=-1;
    //optimal work size
    dormqr(&SIDE, &TRANS, &N, &N, &N, B.data(), &N, TAU.data(), A.data(), &N, &WORK[0], &LWORK, &INFO);
    LWORK=int(WORK[0]);
    WORK.resize(LWORK);
    //Apply Q^T to H
    dormqr(&SIDE, &TRANS, &N, &N, &N, B.data(), &N, TAU.data(), A.data(), &N, &WORK[0], &LWORK, &INFO);

    //now we have a pair (A,B)=(Q^T*H,Q^T*S) where B is upper triangular and A is general matrix.
    //reduce the matrix pair to generalized upper Hesenberg form
    char COMPQ('N'), COMPZ('I');
    int ILO(1);
    int LDQ(N);
    Matrix<double> Z(N,N), Q(N,LDQ); //starts as unit matrix
    for (int zi=0; zi<N; zi++) Z(zi,zi)=1;
    dgghrd(&COMPQ, &COMPZ, &N, &ILO, &N, A.data(), &N, B.data(), &N, Q.data(), &LDQ, Z.data(), &N, &INFO);

    //Take the pair and reduce to shur form and get eigenvalues
    vector<RealType> alphar(N),alphai(N),beta(N);
    char JOB('S');
    COMPQ='N';
    COMPZ='V';
    LWORK=-1;
    //get optimal work size
    dhgeqz(&JOB, &COMPQ, &COMPZ, &N, &ILO, &N, A.data(), &N, B.data(), &N, &alphar[0], &alphai[0], &beta[0], Q.data(), &LDQ, Z.data(), &N, &WORK[0], &LWORK, &INFO);
    LWORK=int(WORK[0]);
    WORK.resize(LWORK);
    dhgeqz(&JOB, &COMPQ, &COMPZ, &N, &ILO, &N, A.data(), &N, B.data(), &N, &alphar[0], &alphai[0], &beta[0], Q.data(), &LDQ, Z.data(), &N, &WORK[0], &LWORK, &INFO);
    //find the best eigenvalue
    vector<std::pair<RealType,int> > mappedEigenvalues(N);
    for (int i=0; i<N; i++)
    {
        RealType evi(alphar[i]/beta[i]);
        if (abs(evi)<1e10)
        {
            mappedEigenvalues[i].first=evi;
            mappedEigenvalues[i].second=i;
        }
        else
        {
            mappedEigenvalues[i].first=1e100;
            mappedEigenvalues[i].second=i;
        }
    }
    std::sort(mappedEigenvalues.begin(),mappedEigenvalues.end());
    int BestEV(mappedEigenvalues[0].second);

//                   now we rearrange the  the matrices
    if (BestEV!=0)
    {
        bool WANTQ(false);
        bool WANTZ(true);
        int ILST(1);
        int IFST(BestEV+1);
        LWORK=-1;

        dtgexc(&WANTQ, &WANTZ, &N, A.data(), &N, B.data(), &N, Q.data(), &N, Z.data(), &N, &IFST, &ILST, &WORK[0], &LWORK, &INFO);
        LWORK=int(WORK[0]);
        WORK.resize(LWORK);
        dtgexc(&WANTQ, &WANTZ, &N, A.data(), &N, B.data(), &N, Q.data(), &N, Z.data(), &N, &IFST, &ILST, &WORK[0], &LWORK, &INFO);
    }
    //now we compute the eigenvector
    SIDE='R';
    char HOWMNY('S');
    int M(0);
    Matrix<double> Z_I(N,N);
    bool SELECT[N];
    for (int zi=0; zi<N; zi++) SELECT[zi]=false;
    SELECT[0]=true;

    WORK.resize(6*N);
    dtgevc(&SIDE, &HOWMNY, &SELECT[0], &N, A.data(), &N, B.data(), &N, Q.data(), &LDQ, Z_I.data(), &N, &N, &M, &WORK[0], &INFO);

    std::vector<RealType> evec(N,0);
    for (int i=0; i<N; i++) for (int j=0; j<N; j++) evec[i] += Z(j,i)*Z_I(0,j);
    for (int i=0; i<N; i++) ev[i] = evec[i]/evec[0];
//     for (int i=0; i<N; i++) app_log()<<ev[i]<<" ";
//     app_log()<<endl;
    return mappedEigenvalues[0].first;
  }
  else
  {
// OLD ROUTINE. CALCULATES ALL EIGENVECTORS
//   Getting the optimal worksize
    char jl('N');
    char jr('V');
    vector<RealType> alphar(N),alphai(N),beta(N);
    Matrix<RealType> eigenT(N,N);
    int info;
    int lwork(-1);
    vector<RealType> work(1);

    RealType tt(0);
    int t(1);
    dggev(&jl, &jr, &N, A.data(), &N, B.data(), &N, &alphar[0], &alphai[0], &beta[0],&tt,&t, eigenT.data(), &N, &work[0], &lwork, &info);
    lwork=work[0];
    work.resize(lwork);

    //~ //Get an estimate of E_lin
    //~ Matrix<RealType> H_tmp(HamT);
    //~ Matrix<RealType> S_tmp(ST);
    //~ dggev(&jl, &jr, &N, H_tmp.data(), &N, S_tmp.data(), &N, &alphar[0], &alphai[0], &beta[0],&tt,&t, eigenT.data(), &N, &work[0], &lwork, &info);
    //~ RealType E_lin(alphar[0]/beta[0]);
    //~ int e_min_indx(0);
    //~ for (int i=1; i<N; i++)
      //~ if (E_lin>(alphar[i]/beta[i]))
        //~ {
          //~ E_lin=alphar[i]/beta[i];
          //~ e_min_indx=i;
        //~ }
    dggev(&jl, &jr, &N, A.data(), &N, B.data(), &N, &alphar[0], &alphai[0], &beta[0],&tt,&t, eigenT.data(), &N, &work[0], &lwork, &info);
    if (info!=0)
    {
      APP_ABORT("Invalid Matrix Diagonalization Function!");
    }

    vector<std::pair<RealType,int> > mappedEigenvalues(N);
    for (int i=0; i<N; i++)
      {
        RealType evi(alphar[i]/beta[i]);
        if(abs(evi)<1e10)
        {
          mappedEigenvalues[i].first=evi;
          mappedEigenvalues[i].second=i;
        }
        else
        {
          mappedEigenvalues[i].first=1e100;
          mappedEigenvalues[i].second=i;
        }
      }
    std::sort(mappedEigenvalues.begin(),mappedEigenvalues.end());

    for (int i=0; i<N; i++) ev[i] = eigenT(mappedEigenvalues[0].second,i)/eigenT(mappedEigenvalues[0].second,0);
    return mappedEigenvalues[0].first;
  }
}

/** Parses the xml input file for parameter definitions for the wavefunction optimization.
* @param q current xmlNode
* @return true if successful
*/
bool
QMCLinearOptimize::put(xmlNodePtr q)
{
    string useGPU("no");
    string vmcMove("pbyp");
    OhmmsAttributeSet oAttrib;
    oAttrib.add(useGPU,"gpu");
    oAttrib.add(vmcMove,"move");
    oAttrib.put(q);

    xmlNodePtr qsave=q;
    xmlNodePtr cur=qsave->children;


    int pid=OHMMS::Controller->rank();
    while (cur != NULL)
    {
        string cname((const char*)(cur->name));
        if (cname == "mcwalkerset")
        {
            mcwalkerNodePtr.push_back(cur);
        }
        else if (cname == "optimizer")
        {
            xmlChar* att= xmlGetProp(cur,(const xmlChar*)"method");
            if (att)
            {
                optmethod = (const char*)att;
            }
            optNode=cur;
        }
        else if (cname == "optimize")
        {
            xmlChar* att= xmlGetProp(cur,(const xmlChar*)"method");
            if (att)
            {
                optmethod = (const char*)att;
            }
        }
        cur=cur->next;
    }
    //no walkers exist, add 10
    if (W.getActiveWalkers() == 0) addWalkers(omp_get_max_threads());

    NumOfVMCWalkers=W.getActiveWalkers();

    //create VMC engine
    if (vmcEngine ==0)
    {
#if defined (QMC_CUDA)
        if (useGPU == "yes")
            vmcEngine = new VMCcuda(W,Psi,H);
        else
#endif
//#if defined(ENABLE_OPENMP)
//        if(omp_get_max_threads()>1)
//          vmcEngine = new VMCSingleOMP(W,Psi,H,hamPool);
//        else
//#endif
//          vmcEngine = new VMCSingle(W,Psi,H);
            vmcEngine = new VMCSingleOMP(W,Psi,H,hamPool);
        vmcEngine->setUpdateMode(vmcMove[0] == 'p');
        vmcEngine->initCommunicator(myComm);
    }
    vmcEngine->setStatus(RootName,h5FileRoot,AppendRun);
    vmcEngine->process(qsave);

    bool success=true;

    if (optTarget == 0)
    {
#if defined (QMC_CUDA)
        if (useGPU == "yes")
            optTarget = new QMCCostFunctionCUDA(W,Psi,H,hamPool);
        else
#endif
#if defined(ENABLE_OPENMP)
            if (omp_get_max_threads()>1)
            {
                optTarget = new QMCCostFunctionOMP(W,Psi,H,hamPool);
            }
            else
#endif
                optTarget = new QMCCostFunctionSingle(W,Psi,H);

        optTarget->setStream(&app_log());
        success=optTarget->put(q);
    }
    return success;
}

void QMCLinearOptimize::resetComponents(xmlNodePtr cur)
{
    exp0=-16;
    m_param.put(cur);
    optTarget->put(cur);
    vmcEngine->resetComponents(cur);
}
}
/***************************************************************************
* $RCSfile$   $Author: jnkim $
* $Revision: 1286 $   $Date: 2006-08-17 12:33:18 -0500 (Thu, 17 Aug 2006) $
* $Id: QMCLinearOptimize.cpp 1286 2006-08-17 17:33:18Z jnkim $
***************************************************************************/
