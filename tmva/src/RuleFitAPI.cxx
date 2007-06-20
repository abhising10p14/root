// @(#)root/tmva $Id: RuleFitAPI.cxx,v 1.12 2007/06/01 15:52:00 andreas.hoecker Exp $
// Author: Andreas Hoecker, Joerg Stelzer, Fredrik Tegenfeldt, Helge Voss 

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : RuleFitAPI                                                            *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Implementation (see header file for description)                          *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Fredrik Tegenfeldt <Fredrik.Tegenfeldt@cern.ch>  - Iowa State U., USA     *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland                                                         * 
 *      Iowa State U.                                                             *
 *      MPI-KP Heidelberg, Germany                                                * 
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

//_______________________________________________________________________
//
// J Friedman's RuleFit method
//_______________________________________________________________________

#include "TROOT.h"
#include "TSystem.h"
#include "TMath.h"
#include "TMVA/MethodRuleFit.h"
#include "TMVA/RuleFitAPI.h"
#include "TMVA/Tools.h"
#include "TMVA/Timer.h"
#include "Riostream.h"
#include <algorithm>

ClassImp(TMVA::RuleFitAPI)

   TMVA::RuleFitAPI::RuleFitAPI( const MethodRuleFit *rfbase,
                                 RuleFit *rulefit,
                                 EMsgType minType = kINFO ) :
   fMethodRuleFit(rfbase),
   fRuleFit(rulefit),
   fLogger("RuleFitAPI",minType)
{
   // standard constructor
   if (rfbase) {
      SetRFWorkDir(rfbase->GetRFWorkDir());
   } else {
      SetRFWorkDir("./rulefit");
   }
   InitRuleFit();
}


//_______________________________________________________________________
TMVA::RuleFitAPI::~RuleFitAPI()
{
   // destructor
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::WelcomeMessage()
{
   // welcome message
   fLogger << kINFO
           << "\n"
           << "---------------------------------------------------------------------------\n"
           << "-   You are running the interface to Jerome Friedmans RuleFit(tm) code.   -\n"
           << "-   For a full manual see the following web page:                         -\n"
           << "-                                                                         -\n"
           << "-        http://www-stat.stanford.edu/~jhf/R-RuleFit.html                 -\n"
           << "-                                                                         -\n"
           << "---------------------------------------------------------------------------"
           << Endl;
}
//_______________________________________________________________________
void TMVA::RuleFitAPI::HowtoSetupRF()
{
   // howto message
   fLogger << kINFO
           << "\n"
           << "------------------------ RULEFIT-JF INTERFACE SETUP -----------------------\n"
           << "\n"
           << "1. Create a rulefit directory in your current work directory:\n"
           << "       mkdir " << fRFWorkDir << "\n\n"
           << "   the directory may be set using the option RuleFitDir\n"
           << "\n"
           << "2. Copy (or make a link) the file rf_go.exe into this directory\n"
           << "\n"
           << "The file can be obtained from Jerome Friedmans homepage (linux):\n"
           << "   wget http://www-stat.stanford.edu/~jhf/r-rulefit/linux/rf_go.exe\n"
           << "\n"
           << "Don't forget to do:\n"
           << "   chmod +x rf_go.exe\n"
           << "\n"
           << "For Windows download:\n"
           << "   http://www-stat.stanford.edu/~jhf/r-rulefit/windows/rf_go.exe\n"
           << "\n"
           << "NOTE: other platforms are not supported (see Friedmans homepage)\n"
           << "\n"
           << "---------------------------------------------------------------------------\n"
           << Endl;
}
//_______________________________________________________________________
void TMVA::RuleFitAPI::InitRuleFit()
{
   // default initialisation
   //   SetRFWorkDir("./rulefit");
   CheckRFWorkDir();
   FillIntParmsDef();
   FillRealParmsDef();
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::ImportSetup()
{
   // import setup from MethodRuleFit
   fRFIntParms.p            = fMethodRuleFit->Data().GetNVariables();
   fRFIntParms.max_rules    = fMethodRuleFit->GetRFNrules();
   fRFIntParms.tree_size    = fMethodRuleFit->GetRFNendnodes();
   fRFIntParms.path_steps   = fMethodRuleFit->GetGDNPathSteps();
   //
   fRFRealParms.path_inc    = fMethodRuleFit->GetGDPathStep();
   fRFRealParms.samp_fract  = fMethodRuleFit->GetTreeEveFrac();
   fRFRealParms.trim_qntl   = fMethodRuleFit->GetLinQuantile();
   fRFRealParms.conv_fac    = fMethodRuleFit->GetGDErrScale();
   //
   if      (fRuleFit->GetRuleEnsemblePtr()->DoOnlyLinear() )
      fRFIntParms.lmode = kRfLinear;
   else if (fRuleFit->GetRuleEnsemblePtr()->DoOnlyRules() )
      fRFIntParms.lmode = kRfRules;
   else
      fRFIntParms.lmode = kRfBoth;
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::SetRFWorkDir(const char * wdir)
{
   // set the directory containing rf_go.exe.
   fRFWorkDir = wdir;
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::CheckRFWorkDir()
{
   // check if the rulefit work dir is properly setup.
   // it aborts (kFATAL) if not.
   //
   // Check existance of directory
   TString oldDir = gSystem->pwd();
   if (!gSystem->cd(fRFWorkDir)) {
      fLogger << kWARNING << "Must create a rulefit directory named : " << fRFWorkDir << Endl;
      HowtoSetupRF();
      fLogger << kFATAL << "Setup failed - aborting!" << Endl;
   }
   // check rf_go.exe
   FILE *f = fopen("rf_go.exe","r");
   if (f==0) {
      fLogger << kWARNING << "No rf_go.exe file in directory : " << fRFWorkDir << Endl;
      HowtoSetupRF();
      fLogger << kFATAL << "Setup failed - aborting!" << Endl;
   }
   fclose(f);
   gSystem->cd(oldDir.Data());
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::SetTrainParms()
{
   // set the training parameters
   ImportSetup();
   //
   Int_t    n    = fMethodRuleFit->Data().GetNEvtTrain();
   //   Double_t neff = Double_t(n); // When weights are added: should be sum(wt)^2/sum(wt^2)
   fRFIntParms.n = n; // number of data points in tree
   fRFProgram    = kRfTrain;
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::SetTestParms()
{
   // set the test params
   ImportSetup();
   Int_t    n    = fMethodRuleFit->Data().GetNEvtTest();
   //   Double_t neff = Double_t(n); // When weights are added: should be sum(wt)^2/sum(wt^2)
   fRFIntParms.n = n; // number of data points in tree
   fRFProgram    = kRfPredict;
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::FillRealParmsDef()
{
   // set default real params
   fRFRealParms.xmiss       = 9.0e30;
   fRFRealParms.trim_qntl   = 0.025;
   fRFRealParms.huber       = 0.8;
   fRFRealParms.inter_supp  = 3.0;
   fRFRealParms.memory_par  = 0.01;
   fRFRealParms.samp_fract  = 0.5; // calculated later
   fRFRealParms.path_inc    = 0.01;
   fRFRealParms.conv_fac    = 1.1;
}

//_______________________________________________________________________
void TMVA::RuleFitAPI::FillIntParmsDef()
{
   // set default int params
   fRFIntParms.mode           = (int)kRfClass;
   fRFIntParms.lmode          = (int)kRfBoth;
   //   fRFIntParms.n;
   //   fRFIntParms.p;
   fRFIntParms.max_rules      = 2000;
   fRFIntParms.tree_size      = 4;
   fRFIntParms.path_speed     = 2;
   fRFIntParms.path_xval      = 3;
   fRFIntParms.path_steps     = 50000;
   fRFIntParms.path_testfreq  = 100;
   fRFIntParms.tree_store     = 10000000;
   fRFIntParms.cat_store      = 1000000;

}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteAll()
{
   // write all files read by rf_go.exe
   WriteIntParms();
   WriteRealParms();
   WriteLx();
   WriteProgram();
   WriteVarNames();
   if (fRFProgram==kRfTrain)   WriteTrain();
   if (fRFProgram==kRfPredict) WriteTest();
   if (fRFProgram==kRfVarimp)  WriteRealVarImp();
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteIntParms()
{
   // write int params file
   std::ofstream f;
   if (!OpenRFile("intparms",f)) return kFALSE;
   WriteInt(f,&fRFIntParms.mode,sizeof(fRFIntParms)/sizeof(Int_t));
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRealParms()
{
   // write int params file
   std::ofstream f;
   if (!OpenRFile("realparms",f)) return kFALSE;
   WriteFloat(f,&fRFRealParms.xmiss,sizeof(fRFRealParms)/sizeof(Float_t));
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteLx()
{
   // Save input variable mask
   //
   // If the lx vector size is not the same as inputVars,
   // resize it and fill it with 1
   // NOTE: Always set all to 1
   //  if (fRFLx.size() != m_inputVars->size()) {
   fRFLx.clear();
   fRFLx.resize(fMethodRuleFit->Data().GetNVariables(),1);
   //  }
   std::ofstream f;
   if (!OpenRFile("lx",f)) return kFALSE;
   WriteInt(f,&fRFLx[0],fRFLx.size());
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteProgram()
{
   // write command to rf_go.exe
   std::ofstream f;
   if (!OpenRFile("program",f)) return kFALSE;
   TString program;
   switch (fRFProgram) {
   case kRfTrain:
      program = "rulefit";
      break;
   case kRfPredict:
      program = "rulefit_pred";
      break;
      // calculate variable importance
   case kRfVarimp:
      program = "varimp";
      break;
   default:
      fRFProgram = kRfTrain;
      program="rulefit";
      break;
   }
   f << program;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRealVarImp()
{
   // write the minimum importance to be considered
   std::ofstream f;
   if (!OpenRFile("realvarimp",f)) return kFALSE;
   Float_t rvp[2];
   rvp[0] = 0.0; // Mode: see varimp() in rulefit.r
   rvp[1] = 0.0; // Minimum importance considered (1 is max)
   WriteFloat(f,&rvp[0],2);
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRfOut()
{
   // written by rf_go.exe; write rulefit output (rfout)
   fLogger << kWARNING << "WriteRfOut is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRfStatus()
{
   // written by rf_go.exe; write rulefit status
   fLogger << kWARNING << "WriteRfStatus is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRuleFitMod()
{
   // written by rf_go.exe (NOTE:Format unknown!)
   fLogger << kWARNING << "WriteRuleFitMod is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteRuleFitSum()
{
   // written by rf_go.exe (NOTE: format unknown!)
   fLogger << kWARNING << "WriteRuleFitSum is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteTrain()
{
   // write training data, columnwise
   std::ofstream fx;
   std::ofstream fy;
   std::ofstream fw;
   //
   if (!OpenRFile("train.x",fx)) return kFALSE;
   if (!OpenRFile("train.y",fy)) return kFALSE;
   if (!OpenRFile("train.w",fw)) return kFALSE;
   //
   Float_t x,y,w;
   //
   // The loop order cannot be changed.
   // The data is stored <var1(eve1), var1(eve2), ...var1(eveN), var2(eve1),....
   //
   for (UInt_t ivar=0; ivar<fMethodRuleFit->Data().GetNVariables(); ivar++) {
      for (Int_t ievt=0;ievt<fMethodRuleFit->Data().GetNEvtTrain(); ievt++) {
         fMethodRuleFit->ReadTrainingEvent(ievt);
         x = fMethodRuleFit->GetEventVal(ivar);
         WriteFloat(fx,&x,1);
         if (ivar==0) {
            w = fMethodRuleFit->GetEventWeight();
            y = fMethodRuleFit->IsSignalEvent() ? 1.0 : -1.0;
            WriteFloat(fy,&y,1);
            WriteFloat(fw,&w,1);
         }
      }
   }
   fLogger << kINFO << "Number of training data written: " << fMethodRuleFit->Data().GetNEvtTrain() << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteTest()
{
   // Write test data
   std::ofstream f;
   //
   if (!OpenRFile("test.x",f)) return kFALSE;
   //
   Float_t vf;
   Float_t neve;
   //
   neve = static_cast<Float_t>(fMethodRuleFit->Data().GetNEvtTest());
   WriteFloat(f,&neve,1);
   // Test data is saved as:
   // 0      : <N> num of events, type float, 4 bytes
   // 1-N    : First variable for all events
   // N+1-2N : Second variable...
   // ...
   for (UInt_t ivar=0; ivar<fMethodRuleFit->Data().GetNVariables(); ivar++) {
      for (Int_t ievt=0;ievt<fMethodRuleFit->Data().GetNEvtTest(); ievt++) {
         fMethodRuleFit->ReadTestEvent(ievt);
         vf =   fMethodRuleFit->GetEventVal(ivar);
         WriteFloat(f,&vf,1);
      }
   }
   fLogger << kINFO << "Number of test data written: " << fMethodRuleFit->Data().GetNEvtTest() << Endl;
   //
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteVarNames()
{
   // write variable names, ascii
   std::ofstream f;
   if (!OpenRFile("varnames",f)) return kFALSE;
   for (UInt_t ivar=0; ivar<fMethodRuleFit->Data().GetNVariables(); ivar++) {
      f << fMethodRuleFit->Data().GetExpression(ivar) << '\n';
   }
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteVarImp()
{
   // written by rf_go.exe
   fLogger << kWARNING << "WriteVarImp is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::WriteYhat()
{
   // written by rf_go.exe
   fLogger << kWARNING << "WriteYhat is not yet implemented" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::ReadYhat()
{
   // read the score
   fRFYhat.clear();
   //
   std::ifstream f;
   if (!OpenRFile("yhat",f)) return kFALSE;
   Int_t   neve;
   Float_t xval;
   ReadFloat(f,&xval,1);
   neve = static_cast<Int_t>(xval);
   if (neve!=fMethodRuleFit->Data().GetNEvtTest()) {
      fLogger << kWARNING << "Inconsistent size of yhat file and test tree!" << Endl;
      fLogger << kWARNING << "neve = " << neve << " , tree = " << fMethodRuleFit->Data().GetNEvtTest() << Endl;
      return kFALSE;
   }
   for (Int_t ievt=0; ievt<fMethodRuleFit->Data().GetNEvtTest(); ievt++) {
      ReadFloat(f,&xval,1);
      fRFYhat.push_back(xval);
   }
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::ReadVarImp()
{
   // read variable importance
   fRFVarImp.clear();
   //
   std::ifstream f;
   if (!OpenRFile("varimp",f)) return kFALSE;
   UInt_t   nvars;
   Float_t xval;
   Float_t xmax=1.0;
   nvars=fMethodRuleFit->Data().GetNVariables();
   //
   // First read all importances
   //
   for (UInt_t ivar=0; ivar<nvars; ivar++) {
      ReadFloat(f,&xval,1);
      if (ivar==0) {
         xmax=xval;
      } else {
         if (xval>xmax) xmax=xval;
      }
      fRFVarImp.push_back(xval);
   }
   //
   // Read the indices.
   // They are saved as float (!) by rf_go.exe.
   //
   for (UInt_t ivar=0; ivar<nvars; ivar++) {
      fRFVarImp[ivar] = fRFVarImp[ivar]/xmax;
      ReadFloat(f,&xval,1);
      fRFVarImpInd.push_back(Int_t(xval)-1);
   }
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::RuleFitAPI::ReadModelSum()
{
   // read model from rulefit.sum
   fRFVarImp.clear();
   //
   fLogger << kVERBOSE << "Reading RuleFit summary file" << Endl;
   std::ifstream f;
   if (!OpenRFile("rulefit.sum",f)) return kFALSE;
   Int_t    lines=0;
   Int_t    nrules=0;
   Int_t    nvars=0;
   Int_t    nvarsOpt=0;
   Int_t    dumI;
   Float_t  dumF;
   Float_t  offset;
   Double_t impref=-1.0;
   Double_t imp;

   fRuleFit->GetRuleEnsemblePtr()->SetAverageRuleSigma(0.4); // value used by Friedmans RuleFit
   //
   //--------------------------------------------
   //       first read rulefit.sum header
   //--------------------------------------------
   // line      type    val     descr
   //   0       <int>   86      N(rules)x2
   //   1       <int>   155     ???
   //   2       <int>   1       ???
   //   3       <int>   1916    ???
   //   4       <int>   2       N(vars) ?
   //   5       <int>   2       N(vars) ?
   //   6       <float> 9e+30   xmiss
   //   7       <float> 1.1e-1  a0 (model offset)
   //--------------------------------------------
   //
   // NOTE: a model without any rules, will look like
   // for the first four lines:
   //
   //   0        1
   //   1        1
   //   2        1
   //   3        0
   //
   // There will later be one block of dummy data for one rule.
   // In order to catch this situation, some special checks are made below.
   //
   Bool_t norules;
   lines += ReadInt(f,&nrules);
   norules = (nrules==1);
   lines += ReadInt(f,&dumI);
   norules = norules && (dumI==1);
   lines += ReadInt(f,&dumI);
   norules = norules && (dumI==1);
   lines += ReadInt(f,&dumI);
   norules = norules && (dumI==0);
   if (nrules==0) norules=kTRUE; // this ugly construction is needed:(
   if (norules) nrules = 0;
   //
   lines += ReadInt(f,&nvars);
   lines += ReadInt(f,&nvarsOpt);
   lines += ReadFloat(f,&dumF);
   lines += ReadFloat(f,&offset);
   fLogger << kDEBUG << "N(rules) = " << nrules   << Endl;
   fLogger << kDEBUG << "N(vars)  = " << nvars    << Endl;
   fLogger << kDEBUG << "N(varsO) = " << nvarsOpt << Endl;
   fLogger << kDEBUG << "xmiss    = " << dumF     << Endl;
   fLogger << kDEBUG << "offset   = " << offset   << Endl;
   if (nvars!=nvarsOpt) {
      fLogger << kWARNING << "Format of rulefit.sum is ... weird?? Continuing but who knows how it will end...?" << Endl;
   }
   std::vector<Double_t> rfSupp;
   std::vector<Double_t> rfCoef;
   std::vector<Int_t>    rfNcut;
   std::vector<Rule *>   rfRules;
   if (norules) {
      // if no rules, read 8 blocks of data
      // this corresponds to one dummy rule
      for (Int_t t=0; t<8; t++) {
         lines += ReadFloat(f,&dumF);
      }
   }
   //
   //--------------------------------------------
   //       read first part of rule info
   //--------------------------------------------
   // 
   //   8       <int>   10      ???
   //   9       <float> 0.185   support
   //   10      <float> 0.051   coefficient
   //   11      <float> 2       num of cuts in rule
   //   12      <float> 1       ??? not used by this interface
   //
   for (Int_t r=0; r<nrules; r++) {
      lines += ReadFloat(f,&dumF);
      lines += ReadFloat(f,&dumF);
      rfSupp.push_back(dumF);
      lines += ReadFloat(f,&dumF);
      rfCoef.push_back(dumF);
      lines += ReadFloat(f,&dumF);
      rfNcut.push_back(static_cast<int>(dumF+0.5));
      lines += ReadFloat(f,&dumF);
      //
   }
   //--------------------------------------------
   //       read second part of rule info
   //--------------------------------------------
   //
   // Per range (cut):
   //   0    <float> 1       varind
   //   1    <float> -1.0    low
   //   2    <float>  1.56   high
   //

   for (Int_t r=0; r<nrules; r++) {
      Int_t    varind;
      Double_t xmin;
      Double_t xmax;
      Rule *rule = new Rule(fRuleFit->GetRuleEnsemblePtr());
      rfRules.push_back( rule );
      RuleCut *rfcut = new RuleCut();
      rfcut->SetNvars(rfNcut[r]);
      rule->SetRuleCut( rfcut );
      // the below are set to default values since no info is
      // available in rulefit.sum
      rule->SetNorm(1.0);
      rule->SetSupport(0);
      rule->SetSSB(0.0);
      rule->SetSSBNeve(0.0);
      rule->SetImportanceRef(1.0);
      rule->SetSSB(0.0);
      rule->SetSSBNeve(0.0);
      // set support etc
      rule->SetSupport(rfSupp[r]);
      rule->SetCoefficient(rfCoef[r]);
      rule->CalcImportance();
      imp = rule->GetImportance();
      if (imp>impref) impref = imp; // find max importance
      //
      fLogger << kDEBUG << "Rule #" << r << " : " << nvars << Endl;
      fLogger << kDEBUG << "  support  = " << rfSupp[r] << Endl;
      fLogger << kDEBUG << "  sigma    = " << rule->GetSigma() << Endl;
      fLogger << kDEBUG << "  coeff    = " << rfCoef[r] << Endl;
      fLogger << kDEBUG << "  N(cut)   = " << rfNcut[r] << Endl;

      for (Int_t c=0; c<rfNcut[r]; c++) {
         lines += ReadFloat(f,&dumF);
         varind = static_cast<Int_t>(dumF+0.5)-1;
         lines += ReadFloat(f,&dumF);
         xmin   = static_cast<Double_t>(dumF);
         lines += ReadFloat(f,&dumF);
         xmax   = static_cast<Double_t>(dumF);
         // create Rule HERE!
         rfcut->SetSelector(c,varind);
         rfcut->SetCutMin(c,xmin);
         rfcut->SetCutMax(c,xmax);
         // the following is not nice - this is however defined
         // by the rulefit.sum format.
         rfcut->SetCutDoMin(c,(xmin<-8.99e35 ? kFALSE:kTRUE));
         rfcut->SetCutDoMax(c,(xmax> 8.99e35 ? kFALSE:kTRUE));
         //
      }
   }
   fRuleFit->GetRuleEnsemblePtr()->SetRules( rfRules );
   fRuleFit->GetRuleEnsemblePtr()->SetOffset( offset );
   //--------------------------------------------
   //       read second part of rule info
   //--------------------------------------------
   //
   // Per linear term:
   // 73      1               var index
   // 74      -1.99594        min
   // 75      1.99403         max
   // 76      -0.000741858    ??? average ???
   // 77      0.970935        std
   // 78      0               coeff
   //
   std::vector<Int_t>    varind;
   std::vector<Double_t> xmin;
   std::vector<Double_t> xmax;
   std::vector<Double_t> average;
   std::vector<Double_t> stdev;
   std::vector<Double_t> norm;
   std::vector<Double_t> coeff;
   //
   for (Int_t c=0; c<nvars; c++) {
      lines += ReadFloat(f,&dumF);
      varind.push_back(static_cast<Int_t>(dumF+0.5)-1);
      lines += ReadFloat(f,&dumF);
      xmin.push_back(static_cast<Double_t>(dumF));
      lines += ReadFloat(f,&dumF);
      xmax.push_back(static_cast<Double_t>(dumF));
      lines += ReadFloat(f,&dumF);
      average.push_back(static_cast<Double_t>(dumF));
      lines += ReadFloat(f,&dumF);
      stdev.push_back(static_cast<Double_t>(dumF));
      Double_t nv = fRuleFit->GetRuleEnsemblePtr()->CalcLinNorm(stdev.back());
      norm.push_back(nv);
      lines += ReadFloat(f,&dumF);
      coeff.push_back(dumF/nv); // save coefficient for normalised var
      //
      fLogger << kDEBUG << "Linear #" << c << Endl;
      fLogger << kDEBUG << "  varind   = " << varind.back()  << Endl;
      fLogger << kDEBUG << "  xmin     = " << xmin.back()    << Endl;
      fLogger << kDEBUG << "  xmax     = " << xmax.back()    << Endl;
      fLogger << kDEBUG << "  average  = " << average.back() << Endl;
      fLogger << kDEBUG << "  stdev    = " << stdev.back()  << Endl;
      fLogger << kDEBUG << "  coeff    = " << coeff.back()  << Endl;
   }
   if (xmin.size()>0) {      
      fRuleFit->GetRuleEnsemblePtr()->SetLinCoefficients(coeff);
      fRuleFit->GetRuleEnsemblePtr()->SetLinDM(xmin);
      fRuleFit->GetRuleEnsemblePtr()->SetLinDP(xmax);
      fRuleFit->GetRuleEnsemblePtr()->SetLinNorm(norm);
   }
   //   fRuleFit->GetRuleEnsemblePtr()->CalcImportance();
   imp = fRuleFit->GetRuleEnsemblePtr()->CalcLinImportance();
   if (imp>impref) impref=imp;
   fRuleFit->GetRuleEnsemblePtr()->SetImportanceRef(impref);
   fRuleFit->GetRuleEnsemblePtr()->CleanupLinear(); // to fill fLinTermOK vector

   fRuleFit->GetRuleEnsemblePtr()->CalcVarImportance();
   //   fRuleFit->GetRuleEnsemblePtr()->CalcRuleSupport();

   fLogger << kDEBUG << "Reading model done" << Endl;
   return kTRUE;
}

//_______________________________________________________________________
Int_t TMVA::RuleFitAPI::RunRuleFit()
{
   // execute rf_go.exe
   TString oldDir = gSystem->pwd();
   TString cmd = "./rf_go.exe"; 
   gSystem->cd(fRFWorkDir.Data());
   int rval = gSystem->Exec(cmd.Data());
   gSystem->cd(oldDir.Data());
   return rval;
}
