// $Id$
// -*-C++-*-
// * BeginRiceCopyright *****************************************************
// 
// Copyright ((c)) 2002, Rice University 
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

//***************************************************************************
//
// File:
//    main.C
//
// Purpose:
//    [The purpose of this file]
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;
#include <fstream>
#include <new>

#include <string>
using std::string;

//*************************** User Include Files ****************************

#include "Args.h"
#include "ProfileReader.h"
#include "ProfileWriter.h"
#include "PCProfile.h"
#include "DerivedProfile.h"

#include "DCPIProfile.h"

#include <lib/binutils/LoadModule.h>
#include <lib/binutils/PCToSrcLineMap.h>
#include <lib/binutils/LoadModuleInfo.h>

//*************************** Forward Declarations ***************************

typedef std::list<string>          StringList;
typedef StringList::iterator       StringListIt;
typedef StringList::const_iterator StringListCIt;

int
real_main(int argc, char* argv[]);

void
ListAvailPredefDCPIFilters(DCPIProfile* prof, bool longlist = false);

PCProfileFilterList*
GetDCPIFilters(DCPIProfile* prof, LoadModule* lm, 
	       const char* metricList, const char* excludeMList);

//****************************************************************************

int
main(int argc, char* argv[])
{
  try {
    return real_main(argc, argv);
  }
  catch (CmdLineParser::Exception& e) {
    e.Report(cerr); // fatal error
    exit(1);
  }
  catch (std::bad_alloc& x) {
    cerr << "Error: Memory alloc failed!\n";
    exit(1);
  }
  catch (...) {
    cerr << "Unknown exception caught\n";
    exit(2);
  }
}


int
real_main(int argc, char* argv[])
{
  Args args(argc, argv);
  
  // ------------------------------------------------------------
  // Read 'pcprof', the raw profiling data file
  // ------------------------------------------------------------
  PCProfile* pcprof = NULL;
  try {
    pcprof = ProfileReader::ReadProfileFile(args.profFile.c_str() /*type*/);
    if (!pcprof) { exit(1); }
  } 
  catch (std::bad_alloc& x) {
    cerr << "Error: Memory alloc failed while reading profile!\n";
    exit(1);
  } 
  
  if (args.listAvailableMetrics > 0) {
    // * Assume DCPI mode *
    DCPIProfile* dcpiprof = dynamic_cast<DCPIProfile*>(pcprof);
    ListAvailPredefDCPIFilters(dcpiprof, args.listAvailableMetrics == 2);
    return (0);
  }
  
  // ------------------------------------------------------------
  // Read executable
  // ------------------------------------------------------------
  LoadModule* exe = NULL; // Executable*: use LoadModule for now (Alpha)
  try {
    string exeNm = args.progFile;

    // Try to find exe from profile info if not given
    if (exeNm.empty()) {
      exeNm = pcprof->GetProfiledFile();
      
      std::ifstream ifile(exeNm.c_str(), std::ios::in);
      if ( !ifile.is_open() || ifile.fail() ) {
	cerr << "Error: Could not find associated binary '" << exeNm
	     << "'; please specify explicitly.\n";
	exit(1);
      }
    }
    
    exe = new LoadModule(); // Executable(): use LoadModule for now (Alpha)
    if (!exe->Open(exeNm.c_str())) { exit(1); } // Error already printed 
    if (!exe->Read()) { exit(1); }              // Error already printed 
  }
  catch (std::bad_alloc& x) {
    cerr << "Error: Memory alloc failed while reading binary!\n";
    exit(1);
  }
  
  exe->Relocate(pcprof->GetTxtStart());
  
  // ------------------------------------------------------------
  // Read 'PCToSrcLineXMap', if available
  // ------------------------------------------------------------
  PCToSrcLineXMap* map = NULL;
  if (!args.pcMapFile.empty()) {
    map = ReadPCToSrcLineMap(args.pcMapFile.c_str());
    if (!map) {
      cerr << "Error reading file `" << args.pcMapFile << "'\n";
      exit(1); 
    }
  }
  // N.B. No relocation for map at the moment
  
  LoadModuleInfo modInfo(exe, map);
    
  // ------------------------------------------------------------
  // Construct some metrics, derived in some way from the raw
  // profiling data, that we are interested in.
  // ------------------------------------------------------------
  // * Assume DCPI mode *
  DerivedProfile* drvdprof = NULL;
  PCProfileFilterList* filtList = NULL;
  if (!args.outputRawMetrics) {
    DCPIProfile* dcpiprof = dynamic_cast<DCPIProfile*>(pcprof);
    filtList = GetDCPIFilters(dcpiprof, modInfo.GetLM(), 
			      args.metricList.c_str(), 
			      args.excludeMList.c_str());
    if (!filtList) {
      exit(1); // Error already printed
    }
  }
  
  drvdprof = new DerivedProfile(pcprof, filtList);
  
  if (filtList) { filtList->destroyContents(); }
  delete filtList; // done with filters
  
  if (drvdprof->GetNumMetrics() == 0) {
    cerr << "Error: Could not find any metrics to convert to PROFILE!\n";
    exit(1);
  }
  
  // ------------------------------------------------------------
  // Translate the derived metrics to a PROFILE file
  // ------------------------------------------------------------
  // * Assume DCPI mode *
  ProfileWriter::WriteProfile(cout, drvdprof, &modInfo);
  
  delete exe;
  delete map;
  delete pcprof;
  delete drvdprof;
  return (0);
}

//****************************************************************************

PCProfileFilterList*
GetDCPIFilters(StringList* mlist, LoadModule* lm);

StringList*
GetAvailPredefDCPIFilterNms(DCPIProfile* prof, LoadModule* lm);

bool
IsPredefDCPIFilterAvail(DCPIProfile* prof, StringList* flist, 
			const char* emsg = NULL);

bool
IsPredefDCPIFilterAvail(DCPIProfile* prof, 
			PredefinedDCPIMetricTable::Entry* e);

StringList*
ConvertColonListToStringList(const char* str);

void
RemoveFromList(StringList* l1, StringList* l2);


// ListAvailPredefDCPIFilters: Iterate through the DCPI predefined
// metric table, listing any metrics (filters) that are available for
// this profile.
void
ListAvailPredefDCPIFilters(DCPIProfile* prof, bool longlist)
{
  // When longlist is false, a short listing is printed
  // When longlist is true, a long listing is print (name and description)

  string out;
  for (suint i = 0; i < PredefinedDCPIMetricTable::GetSize(); ++i) {
    PredefinedDCPIMetricTable::Entry* e = PredefinedDCPIMetricTable::Index(i);
    if (IsPredefDCPIFilterAvail(prof, e)) {
      
      if (longlist) {
	out += string("  ") + e->name + ": " + e->description + "\n";
      } else {
	if (!out.empty()) { out += ":"; }
	out += e->name;
      }
      
    }
  }
  
  if (out.empty()) {
    cout << "No derived metrics available for this DCPI profile.";
  } else {
    cout << "The following metrics are available for this DCPI profile:\n";
    if (longlist) {
      cout << out;
    } else {
      cout << "  -M " << out << endl;
      cout << "(This line may be edited and passed to xprof.)" << endl;
    }
  }
}


// GetDCPIFilters: Returns the list of DCPI filters determined by the
// colon-separated include list in 'metricList' and the colon-separated
// exclude list in 'excludeMList'.  If 'metricList' is empty, a list of
// available metrics will automatically computed.  If an error occurs
// during processing -- such as invalid metrics -- an error message is
// printed and NULL will be returned.  User is responsible for memory
// deallocation.
PCProfileFilterList*
GetDCPIFilters(DCPIProfile* prof, LoadModule* lm, 
	       const char* metricList, const char* excludeMList)
{
  bool noError = true;
  PCProfileFilterList* flist = NULL;

  // 1. Determine the initial list of metrics to compute.  All metrics
  // should be valid even if some will later be excluded.  (We do not
  // allow users to specify an unavailable metric here and then
  // exclude it.)
  StringList* mlist = NULL;
  if (metricList[0] != '\0') {
    mlist = ConvertColonListToStringList(metricList);
    noError &= IsPredefDCPIFilterAvail(prof, mlist, "Error in -M:");
  } else {
    mlist = GetAvailPredefDCPIFilterNms(prof, lm);
  }
  
  // 2. Remove excludes from this list.  (All metrics in 'mlist' are valid.)
  StringList* xlist = NULL;
  if (excludeMList[0] != '\0') {
    xlist = ConvertColonListToStringList(excludeMList);
    noError &= IsPredefDCPIFilterAvail(prof, xlist, "Error in -X:");
    RemoveFromList(mlist, xlist);
  }

  // 3. Create the list of filters for the final metric list. (All should
  // be avilable.)
  if (noError) {
    flist = GetDCPIFilters(mlist, lm);
  }
  delete mlist;
  delete xlist;
  return flist;
}


// GetDCPIFilters: Return a list of filters for the metrics in 'mlist'.
// User is responsible for memory deallocation of list and its contents. 
// Note: assumes that every filter name in 'mlist' is available.
PCProfileFilterList*
GetDCPIFilters(StringList* mlist, LoadModule* lm) 
{
  PCProfileFilterList* flist = new PCProfileFilterList;
  for (StringListIt it = mlist->begin(); it != mlist->end(); ++it) {
    const string& nm = *it;
    flist->push_back(GetPredefinedDCPIFilter(nm.c_str(), lm));
  }
  return flist;
}

// GetAvailPredefDCPIFilterNms: Return a list of all the filter names
// within the DCPI predefined metric table that are available for this
// profile.
StringList*
GetAvailPredefDCPIFilterNms(DCPIProfile* prof, LoadModule* lm)
{
  StringList* flist = new StringList;
  for (suint i = 0; i < PredefinedDCPIMetricTable::GetSize(); ++i) {
    PredefinedDCPIMetricTable::Entry* e = PredefinedDCPIMetricTable::Index(i);
    if (IsPredefDCPIFilterAvail(prof, e)) {
      flist->push_back(string(e->name));
    }
  }
  return flist;
}


// IsPredefDCPIFilterAvail: Given a list of filter names, determine if
// each one is available given this profile.  If 'emsg' is non-NULL,
// an error message will be printed for each invalid filter name, with
// 'emsg' printed first on each error line.
bool 
IsPredefDCPIFilterAvail(DCPIProfile* prof, StringList* flist, const char* emsg)
{
  bool ret = true;
  PredefinedDCPIMetricTable::Entry* e; 
  for (StringListIt it = flist->begin(); it != flist->end(); ++it) {
    const string& nm = *it;
    
    e = PredefinedDCPIMetricTable::FindEntry(nm.c_str());
    if ( !(e && IsPredefDCPIFilterAvail(prof, e)) ) {
      ret = false;
      if (emsg) {
	cerr << emsg << " The DCPI filter '" << nm << "' is invalid for this profile.\n";
      }
    } 
  }
  return ret;
}

// IsPredefDCPIFilterAvail: Given the DCPIProfile and an entry in the
// DCPI predefined metric table, determine if the corresponding filter
// is available (meaningful) for this profile.
bool 
IsPredefDCPIFilterAvail(DCPIProfile* prof, PredefinedDCPIMetricTable::Entry* e)
{
  DCPIProfile::PMMode pmmode = prof->GetPMMode();
  
  if (e->avail & PredefinedDCPIMetricTable::RM) {

    // The filter may or may not be available (whether in ProfileMe
    // mode or not): must check each metric
    for (PCProfileMetricSetIterator it(*prof); it.IsValid(); ++it) {
      PCProfileMetric* m = it.Current();
      if (strcmp(e->availStr, m->GetName()) == 0) {
	return true;
      }
    }
    return false;
    
  } else {

    // The filter may be available in any ProfileMe mode or in only
    // one mode.
    bool pmavail = false;
    if (e->avail & PredefinedDCPIMetricTable::PM0) {
      if (pmmode == DCPIProfile::PM0) { pmavail |= true; }
    } 
    if (e->avail & PredefinedDCPIMetricTable::PM1) {
      if (pmmode == DCPIProfile::PM1) { pmavail |= true; }
    }
    if (e->avail & PredefinedDCPIMetricTable::PM2) {
      if (pmmode == DCPIProfile::PM2) { pmavail |= true; }
    }
    if (e->avail & PredefinedDCPIMetricTable::PM3) {
      if (pmmode == DCPIProfile::PM3) { pmavail |= true; }
    }
    return pmavail;
  }
  
}


// ConvertColonListToStringList: convert the colon-separated list of
// names to a StringList.  Users are responsible for memory.
StringList*
ConvertColonListToStringList(const char* str)
{
  StringList* l = new StringList;
 
  if (!str) { return l; }
 
  char* tok = strtok(const_cast<char*>(str), ":");
  while (tok != NULL) {
    l->push_back(string(tok));
    tok = strtok((char*)NULL, ":");
  }
  
  return l;
}


// RemoveFromList: Removes all items in l2 from l1.
void
RemoveFromList(StringList* l1, StringList* l2) 
{
  for (StringListIt it2 = l2->begin(); it2 != l2->end(); ++it2) {
    const string& nm2 = *it2;

    // Remove nm2 every time it occurs in l1
    for (StringListIt it1 = l1->begin(); it1 != l1->end(); ) {
      const string& nm1 = *it1;
      if (nm1 == nm2) { 
	it1 = l1->erase(it1); // it1 now points at next element or end
      } else {
	++it1;
      }
    }
  }
}
