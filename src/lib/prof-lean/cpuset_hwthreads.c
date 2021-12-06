// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2021, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *


//******************************************************************************
// system include files
//******************************************************************************

#define _GNU_SOURCE
#include <pthread.h>



//******************************************************************************
// local include files
//******************************************************************************

#include "cpuset_hwthreads.h"



//******************************************************************************
// public operations
//******************************************************************************

//------------------------------------------------------------------------------
//   Function cpuset_hwthreads
//   Purpose:
//     return the number of hardware threads available to this process
//     return 1 if no other value can be computed
//------------------------------------------------------------------------------
unsigned int 
cpuset_hwthreads
(
  void
)
{  
  int processors = 1;
  pthread_t thread = pthread_self();

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  int err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

  if (err == 0) {
    int j;
    for (j = 0; j < CPU_SETSIZE; j++)
      if (CPU_ISSET(j, &cpuset)) processors++;
  }

  return processors;
}