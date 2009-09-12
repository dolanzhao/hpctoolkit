// -*-Mode: C++;-*-

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// -----------------------------------
// Part of HPCToolkit (hpctoolkit.org)
// -----------------------------------
// 
// Copyright ((c)) 2002-2009, Rice University 
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

//************************ System Include Files ******************************

#include <iostream> 
using std::endl;

#include <string>
using std::string;

#include <cstring>

//************************* User Include Files *******************************

#include <include/uint.h>

#include "Metric-OLD.hpp"
#include "Struct-Tree.hpp"

#include <lib/support/diagnostics.h>
#include <lib/support/Trace.hpp>
#include <lib/support/StrUtil.hpp>

//************************ Forward Declarations ******************************

//****************************************************************************


// ************************************************************************* //
// DataDisplayInfo
// ************************************************************************* //

DataDisplayInfo::~DataDisplayInfo() 
{
}


string 
DataDisplayInfo::toString() const 
{
  string str = string("DisplayInfo ") + 
    "name=" + name + " " + 
    "color=" + color + " " + 
    "width=" + StrUtil::toStr(width) + " " + 
    "asInt=" + ((formatAsInt) ? "true" : "false"); 
  return str;
}


// ************************************************************************* //
// PerfMetric
// ************************************************************************* //

PerfMetric::PerfMetric(const char *nm, const char *nativeNm, 
		       const char* displayNm, 
		       bool disp, bool dispPercent, bool isPercent, bool sort)
 : name(nm), nativeName(nativeNm), dispInfo(NULL), 
   display(disp), m_dispPercent(dispPercent), m_isPercent(isPercent), 
   sortBy(sort) 
{
  Ctor(nm, displayNm);
}


PerfMetric::PerfMetric(const std::string& nm, const std::string& nativeNm, 
		       const std::string& displayNm, 
		       bool disp, bool dispPercent, bool isPercent, bool sort)
 : name(nm), nativeName(nativeNm), dispInfo(NULL), 
   display(disp), m_dispPercent(dispPercent), m_isPercent(isPercent), 
   sortBy(sort) 
{
  Ctor(nm.c_str(), displayNm.c_str());
}


void
PerfMetric::Ctor(const char *nm, const char* displayNm)
{
  // trace = 1; 
  m_id = 0;
  
  DIAG_Assert((displayNm != NULL) && (strlen(displayNm) > 0), ""); 
  dispInfo = new DataDisplayInfo(displayNm, NULL, 9, false); 
}


PerfMetric::~PerfMetric() 
{
  delete dispInfo; 
}


string
PerfMetric::toString(int flags) const 
{
  const string& dnm = dispInfo->Name();

  string str(name);
  
  if (dnm != name) {
    " (" + dispInfo->Name() + ")";
  }
  
  if (flags) {
    str += string("PerfMetric: ") + 
      "name=" + name + " " + 
      "display=" + ((display) ? "true " : "false ") + 
      "perfInfoIndex=" + StrUtil::toStr(m_id) + " " + 
      "dispInfo=" + dispInfo->toString(); 
  }
  return str;
}
  
// *************************************************************************
// FilePerfMetric
// *************************************************************************

FilePerfMetric::FilePerfMetric(const char* nm, 
			       const char* nativeNm,
			       const char* displayNm, 
			       bool doDisp, bool dispPercent, bool doSort, 
			       const char* file,
			       const char* type,
			       bool isunit_ev) 
  : PerfMetric(nm, nativeNm, displayNm, doDisp, dispPercent, false, doSort),
    m_file(file), m_type(type), m_isunit_event(isunit_ev)
{ 
  // trace = 1;
}


FilePerfMetric::FilePerfMetric(const std::string& nm, 
			       const std::string& nativeNm, 
			       const std::string& displayNm,
			       bool doDisp, bool dispPercent, bool doSort, 
			       const std::string& file, 
			       const std::string& type,
			       bool isunit_ev)
  : PerfMetric(nm, nativeNm, displayNm, doDisp, dispPercent, false, doSort),
    m_file(file), m_type(type), m_isunit_event(isunit_ev)
{ 
  // trace = 1;
}


FilePerfMetric::~FilePerfMetric() 
{
  IFTRACE << "~FilePerfMetric " << toString() << endl; 
}


string
FilePerfMetric::toString(int flags) const 
{
  string unitstr = isunit_event() ? " [events]" : " [samples]";
  string rawstr = " {" + m_rawdesc.description() + ":" + StrUtil::toStr(m_rawdesc.period()) + " ev/smpl}";
  string str = PerfMetric::toString() + unitstr + rawstr;

  if (flags) {
    str += "file='" + m_file + "' " + "type='" + m_type + "'";
  }
  return str;
} 


// **************************************************************************
// 
// **************************************************************************

ComputedPerfMetric::ComputedPerfMetric(const char* nm, const char* displayNm,
				       bool doDisp, bool dispPercent, 
				       bool isPercent,  bool doSort,
      				       Prof::Metric::AExpr* expr)
  : PerfMetric(nm, "", displayNm, doDisp, dispPercent, isPercent, doSort)
{
  Ctor(nm, expr);
}


ComputedPerfMetric::ComputedPerfMetric(const std::string& nm,
				       const std::string& displayNm,
				       bool doDisp, bool dispPercent, 
				       bool isPercent,  bool doSort,
				       Prof::Metric::AExpr* expr)
  : PerfMetric(nm, "", displayNm, doDisp, dispPercent, isPercent, doSort)
{
  Ctor(nm.c_str(), expr);
}


void
ComputedPerfMetric::Ctor(const char* nm, Prof::Metric::AExpr* expr)
{
  m_exprTree = expr;
}


ComputedPerfMetric::~ComputedPerfMetric() 
{
  IFTRACE << "~ComputedPerfMetric " << toString() << endl; 
  delete m_exprTree;
}


string
ComputedPerfMetric::toString(int flags) const 
{
  string str = PerfMetric::toString() + " {" + m_exprTree->toString() + "}";
  return str;
} 

// **************************************************************************