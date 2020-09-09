////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2019 QMCPACK developers.
//
// File developed by:
// Peter Doak, doakpw@ornl.gov, Oak Ridge National Lab
//
// File created by:
// Peter Doak, doakpw@ornl.gov, Oak Ridge National Lab
////////////////////////////////////////////////////////////////////////////////

#ifndef QMCPLUSPLUS_THREAD_HPP
#define QMCPLUSPLUS_THREAD_HPP

#include "Concurrency/Info.hpp"

namespace qmcplusplus
{
/** Abstraction for running concurrent tasks in parallel by an executor
 *
 *  Construct with num_tasks to run
 *  then call operator(F, args...) 
 *  F is lambda or function with form
 *      void F(int task_id, args...)
 *      [](int task_id, args...){...}
 *
 *  This has not been tested for nested threading with openmp
 *  It is not intended for use below the top level of openmp threading.
 */
template<Executor TT = Executor::OPENMP>
class TasksOneToOne
{
public:
  /** Concurrently execute an arbitrary function/kernel with task id and arbitrary args
   *
   *  ie each task will run f(int task_id, Args... args)
   */
  template<typename F, typename... Args>
  void operator()(int num_tasks, F&& f, Args&&... args);
};

} // namespace qmcplusplus

// Implementation includes must follow functor declaration
#include "Concurrency/TasksOneToOneOPENMP.hpp"
#ifdef QMC_EXP_THREADING
#include "Concurrency/TasksOneToOneSTD.hpp"
#endif
// Additional implementations enabled by cmake options would go here
#endif
