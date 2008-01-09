//////////////////////////////////////////////////////////////////
// (c) Copyright 2006-  by Jeongnum Kim
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
/** @file LRJastrowSingleton.h
 * @brief Define a LRHandler with two template parameters
 */
#ifndef QMCPLUSPLUS_LONGRANGEJASTROW_BREAKUPUTILITY_H
#define QMCPLUSPLUS_LONGRANGEJASTROW_BREAKUPUTILITY_H

#include "Numerics/OptimizableFunctorBase.h"

namespace qmcplusplus {
  /** RPABreakUp
   * 
   * A Func for LRHandlerTemp.  Four member functions have to be provided 
   * 
   * - reset(T volume) : reset the normalization factor
   * - operator() (T r, T rinv) : return a value of the original function e.g., 1.0/r
   * - Fk(T k, T rc)
   * - Xk(T k, T rc)
   * 
   */     
  template<class T=double>
  struct RPABreakup {
    T Rs;
    T SqrtRs;
    T OneOverSqrtRs;
    T NormFactor;
    inline RPABreakup(){}
    
    void reset(ParticleSet& ref) {
      NormFactor=4.0*M_PI/ref.Lattice.Volume;
      T Density=ref.getTotalNum()/ref.Lattice.Volume;
      Rs = std::pow(3.0/(4.0*M_PI*Density), 1.0/3.0);
      SqrtRs=std::sqrt(Rs);
      OneOverSqrtRs = 1.0 / SqrtRs;
    }
     
    void reset(ParticleSet& ref, T rs) {
       NormFactor=4.0*M_PI/ref.Lattice.Volume;
       Rs = rs;
       SqrtRs=std::sqrt(Rs);
       OneOverSqrtRs = 1.0 / SqrtRs;
    }
     
    inline T operator()(T r, T rinv) { 
      if(r< numeric_limits<T>::epsilon())
        return SqrtRs-0.5*r;
      else
        return Rs*rinv*(1.0-std::exp(-r*OneOverSqrtRs));
      //if (r > 1e-10) return Rs*rinv*(1.0 - std::exp(-r*OneOverSqrtRs));
      //return 1.0 / OneOverSqrtRs - 0.5 * r;
    }
    
    inline T df(T r) {
      if(r< numeric_limits<T>::epsilon())
        return -0.5+r*OneOverSqrtRs/3.0;
      else
      {
        T rinv=1.0/r;
        T exponential = std::exp(-r*OneOverSqrtRs);
        return -Rs*rinv*rinv*(1.0 - exponential) + exponential*rinv*SqrtRs;
      }
    }
    
    inline T Fk(T k, T rc) {
       return -Xk(k,rc);
    }
    
    inline T Xk(T k, T rc) {
       T coskr = std::cos(k*rc);
       T sinkr = std::sin(k*rc);
       T oneOverK = 1.0/k;
       return -NormFactor * Rs * 
         (coskr*oneOverK*oneOverK 
         - std::exp(-rc*OneOverSqrtRs)*(coskr - OneOverSqrtRs * sinkr * oneOverK)/(k*k+1.0/Rs));
    }

    /** return RPA value at |k|
     * @param kk |k|^2
     */
    inline T Uk(T kk)
    {
      return NormFactor*Rs/kk;
    }

    /** return d u(k)/d rs
     *
     * Implement a correct one
     */
    inline T derivUk(T kk)
    {
      return NormFactor/kk;
    }
  };

  template<class T=double>
  struct DerivRPABreakup {
    T Rs;
    T SqrtRs;
    T OneOverSqrtRs;
    T NormFactor;
    inline DerivRPABreakup(){}
    
    void reset(ParticleSet& ref) {
      NormFactor=4.0*M_PI/ref.Lattice.Volume;
      T Density=ref.getTotalNum()/ref.Lattice.Volume;
      Rs = std::pow(3.0/(4.0*M_PI*Density), 1.0/3.0);
      SqrtRs=std::sqrt(Rs);
      OneOverSqrtRs = 1.0 / SqrtRs;
    }
     
    void reset(ParticleSet& ref, T rs) {
       NormFactor=4.0*M_PI/ref.Lattice.Volume;
       Rs = rs;
       OneOverSqrtRs = 1.0 / std::sqrt(Rs);
    }
     
    /** need the df(r)/d(rs) */
    inline T operator()(T r, T rinv) { 
      if(r< numeric_limits<T>::epsilon())
        return SqrtRs-0.5*r;
      else
        return Rs*rinv*(1.0-std::exp(-r*OneOverSqrtRs));
      //if (r > 1e-10) return Rs*rinv*(1.0 - std::exp(-r*OneOverSqrtRs));
      //return 1.0 / OneOverSqrtRs - 0.5 * r;
    }
    
    /** need d(df(r)/d(rs))/dr */
    inline T df(T r, T rinv) {
      if(r< numeric_limits<T>::epsilon())
        return -0.5+r*OneOverSqrtRs/3.0;
      else
      {
        T exponential = std::exp(-r*OneOverSqrtRs);
        return -Rs*rinv*rinv*(1.0 - exponential) + exponential*rinv*SqrtRs;
      }
    }
    
    inline T Fk(T k, T rc) {
       return -Xk(k,rc);
    }
    
    /** integral from kc to infinity */
    inline T Xk(T k, T rc) {
       T coskr = std::cos(k*rc);
       T sinkr = std::sin(k*rc);
       T oneOverK = 1.0/k;
       return -NormFactor * Rs * 
         (coskr*oneOverK*oneOverK 
         - std::exp(-rc*OneOverSqrtRs)*(coskr - OneOverSqrtRs * sinkr * oneOverK)/(k*k+1.0/Rs));
    }
  };

  template<typename T>
  struct ShortRangePartAdapter : OptimizableFunctorBase<T> {
  public:
    typedef LRHandlerBase HandlerType;
    typedef typename OptimizableFunctorBase<T>::real_type real_type;  
    typedef typename OptimizableFunctorBase<T>::OptimizableSetType OptimizableSetType;  
    
    explicit ShortRangePartAdapter(HandlerType* inhandler): Uconst(0) {
      myHandler = inhandler;
    }
    inline void setRmax(real_type rm) { Uconst=myHandler->evaluate(rm,1.0/rm);}
    inline real_type evaluate(real_type r) { return f(r); }
    inline real_type f(real_type r) 
    { 
      return myHandler->evaluate(r, 1.0/r)-Uconst; 
    }
    inline real_type df(real_type r) 
    {
      return myHandler->srDf(r, 1.0/r);
    }
    void resetParameters(OptimizableSetType& optVariables) { }
    bool put(xmlNodePtr cur) {return true;}
    void addOptimizables(OptimizableSetType& vlist){}
  private:
    real_type Uconst;
    HandlerType* myHandler;
  };


}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2027 $   $Date: 2007-05-31 15:37:59 -0500 (Thu, 31 May 2007) $
 * $Id: LRJastrowSingleton.h 2027 2007-05-31 20:37:59Z jnkim $
 ***************************************************************************/
