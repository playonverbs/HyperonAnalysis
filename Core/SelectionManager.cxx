#ifndef _SelectionManager_cxx_
#define _SelectionManager_cxx_

#include "SelectionManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////

SelectionManager::SelectionManager() : 
   a_FluxWeightCalc(0) ,
   a_FiducialVolume(1,0.0) ,
   a_MuonID(-0.35,18.0,0.0) ,
   a_TrackLengthCutManager(1000.0,1000.0) , 
   a_SelectorBDTManager("Test") ,
   a_AnalysisBDTManager("Test") 
{
   DeclareCuts();
}

///////////////////////////////////////////////////////////////////////////////////////////////

SelectionManager::~SelectionManager(){


}

///////////////////////////////////////////////////////////////////////////////////////////////

SelectionManager::SelectionManager(SelectionParameters p) :
   a_FluxWeightCalc(p.p_RunPeriod) ,
   a_FiducialVolume(p.p_FV,p.p_Padding) ,
   a_MuonID(p.p_PID_Cut,p.p_Minimum_MIP_Length,p.p_Max_Displacement) ,
   a_TrackLengthCutManager(p.p_SecondaryTrackLengthCut,p.p_SecondaryTrackLengthCut) ,
   a_SelectorBDTManager("Test") , 
   a_AnalysisBDTManager("Test")
{
   // Set the selection parameters
   TheParams = p;
   DeclareCuts();

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::SetPOT(double POT){

   fPOT = POT;

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::AddSample(std::string Name,std::string Type,double SamplePOT){

   std::cout << "Processing Sample " << Name << " of type " << Type << " and POT " << SamplePOT <<  std::endl;

   if(Type != "Data") thisSampleWeight = fPOT/SamplePOT;
   else thisSampleWeight = 1.0;

   thisSampleName = Name;
   thisSampleType = Type;

  if(Type == "Data") fHasData = true;

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::AddEvent(Event &e){

   // Set flux weight if setup
   if(thisSampleType != "Data" && thisSampleType != "EXT"){

      double nu_e = e.Neutrino.at(0).E;
      double nu_angle = GetNuMIAngle(e.Neutrino.at(0).Px,e.Neutrino.at(0).Py,e.Neutrino.at(0).Pz);
      int nu_pdg = e.Neutrino.at(0).PDG;

      e.Weight *= a_FluxWeightCalc.GetFluxWeight(nu_e,nu_angle,nu_pdg);

   }

   if(thisSampleType != "Data") e.Weight *= thisSampleWeight;

   for(size_t i_c=0;i_c<Cuts.size();i_c++){

      Cuts[i_c].fTotalEvents += e.Weight;    
      if(e.IsSignal) Cuts[i_c].fSignalEvents += e.Weight;
      if(e.GoodReco) Cuts[i_c].fGoodRecoEvents += e.Weight;

   }

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::SetSignal(Event &e){

   e.IsSignal = false;
   e.GoodReco = false;

   e.InActiveTPC = a_FiducialVolume.InFiducialVolume(e.TruePrimaryVertex);

   if( e.Mode == "HYP" && e.IsLambdaCharged && e.InActiveTPC && e.Neutrino.size() == 1 && e.Neutrino.at(0).PDG == -14 ){
      e.IsSignal = true;

      // Search the list of reco'd tracks for the proton and pion
      bool found_proton=false,found_pion=false;

      for(size_t i_tr=0;i_tr<e.TracklikePrimaryDaughters.size();i_tr++){

         if(e.TracklikePrimaryDaughters.at(i_tr).HasTruth && e.TracklikePrimaryDaughters.at(i_tr).TrackTruePDG == 2212 && e.TracklikePrimaryDaughters.at(i_tr).TrackTrueOrigin == 2) found_proton = true;
         if(e.TracklikePrimaryDaughters.at(i_tr).HasTruth && e.TracklikePrimaryDaughters.at(i_tr).TrackTruePDG == -211 && e.TracklikePrimaryDaughters.at(i_tr).TrackTrueOrigin == 2) found_pion = true;

      }

      if(found_proton && found_pion) e.GoodReco = true;

   }
   else return;


   //get Lambda decay products
   for(size_t i_d=0;i_d<e.Decay.size();i_d++){

      //cut on proton momentum
      if( e.Decay.at(i_d).PDG == 2212 && e.Decay.at(i_d).ModMomentum < 0.3 ) { e.IsSignal = false; e.GoodReco = false; return; }

      if( e.Decay.at(i_d).PDG == -211 && e.Decay.at(i_d).ModMomentum < 0.1 ) { e.IsSignal = false; e.GoodReco = false; return; }

   }



}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::DeclareCuts(){

   for(size_t i_c=0;i_c<CutTypes.size();i_c++){
      Cut c;
      c.fName = CutTypes.at(i_c);
      Cuts.push_back(c);
   }


}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::UpdateCut(Event e,bool Passed,std::string CutName){

   for(size_t i_c=0;i_c<Cuts.size();i_c++){

      if(Cuts.at(i_c).fName == CutName) {

         Cuts[i_c].fEventsIn += e.Weight;
         if(e.IsSignal) Cuts[i_c].fSignalEventsIn += e.Weight;
         if(e.GoodReco) Cuts[i_c].fGoodRecoEventsIn += e.Weight;

         if(Passed){

            Cuts[i_c].fEventsOut += e.Weight;
            if(e.IsSignal) Cuts[i_c].fSignalEventsOut += e.Weight;
            if(e.GoodReco) Cuts[i_c].fGoodRecoEventsOut += e.Weight;

         }       

      }
   }

}

///////////////////////////////////////////////////////////////////////////////////////////////

Cut SelectionManager::GetCut(std::string CutName){

   for(size_t i_c=0;i_c<Cuts.size();i_c++){

      if(Cuts.at(i_c).fName == CutName) return Cuts.at(i_c);

   }

   std::cout << "Cut " << CutName << " not found" << std::endl;

   Cut c;
   return c;

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::ImportSelectorBDTWeights(std::string WeightDir){

a_SelectorBDTManager.SetupSelectorBDT(WeightDir);

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::ImportAnalysisBDTWeights(std::string WeightDir){

a_AnalysisBDTManager.SetupAnalysisBDT(WeightDir);

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::FiducialVolumeCut(Event e){

bool passed = a_FiducialVolume.InFiducialVolume(e.RecoPrimaryVertex);

UpdateCut(e,passed,"FV");

return passed;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::TrackCut(Event e){

bool passed = e.NPrimaryTrackDaughters > 2; 

UpdateCut(e,passed,"Tracks");

return passed;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::ShowerCut(Event e){

bool passed = e.NPrimaryShowerDaughters < 1; 

UpdateCut(e,passed,"Showers");

return passed;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::ChooseMuonCandidate(Event &e){

int i_muon = a_MuonID.SelectCandidate(e.TracklikePrimaryDaughters);

if(i_muon == -1){
 UpdateCut(e,false,"MuonID");
 return false;
}

RecoParticle theMuonCandidate = e.TracklikePrimaryDaughters.at(i_muon);
e.MuonCandidate = theMuonCandidate;
e.TracklikePrimaryDaughters.erase(e.TracklikePrimaryDaughters.begin()+i_muon);
 
UpdateCut(e,true,"MuonID");

return true;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::TrackLengthCut(Event e){

bool passed = a_TrackLengthCutManager.ApplyCut(e.TracklikePrimaryDaughters);

UpdateCut(e,passed,"SubleadingTracks");

return passed;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::ChooseProtonPionCandidates(Event &e){

   std::pair<int,int> candidates = a_SelectorBDTManager.NominateTracks(e);

   bool passed = candidates.first != -1 && candidates.second != -1;

   if(!passed){
      UpdateCut(e,passed,"DecaySelector");
      return false;
   }

   e.DecayProtonCandidate = e.TracklikePrimaryDaughters.at(candidates.first);
   e.DecayPionCandidate = e.TracklikePrimaryDaughters.at(candidates.second);

   // Erase from track vector
   std::vector<RecoParticle> TracklikePrimaryDaughters_tmp;

   for(size_t i=0;i<e.TracklikePrimaryDaughters.size();i++)
      if(i != candidates.first && i != candidates.second) TracklikePrimaryDaughters_tmp.push_back(e.TracklikePrimaryDaughters.at(i));

   e.TracklikePrimaryDaughters = TracklikePrimaryDaughters_tmp;

   
   UpdateCut(e,passed,"DecaySelector");
   return true;

}

///////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionManager::AnalysisBDTCut(Event &e){

bool passed = a_AnalysisBDTManager.CalculateScore(e) > TheParams.p_AnalysisBDT_Cut; 

UpdateCut(e,passed,"DecayAnalysis");

return passed;

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::SetupHistograms(int n,double low,double high,std::string title){

   fTitle = title;

   for(size_t i_proc=0;i_proc<Procs.size();i_proc++){

      std::string histname = "h_" + Procs.at(i_proc);

      Hists_ByProc[Procs.at(i_proc)] = new TH1D(histname.c_str(),fTitle.c_str(),n,low,high);

   }

   for(size_t i_type=0;i_type<Types.size();i_type++){

      std::string histname = "h_ByType_" + Types.at(i_type);

      Hists_ByType[Types.at(i_type)] = new TH1D(histname.c_str(),fTitle.c_str(),n,low,high);

   }

}

///////////////////////////////////////////////////////////////////////////////////////////////

void SelectionManager::FillHistograms(Event e,double variable,double weight){

   std::string mode;

   bool isNuBackground = false;

   if( thisSampleType == "Data" ) mode = "Data";
   else  if( e.IsSignal ) mode = "Signal";
   else if( e.Mode == "HYP") mode = "OtherHYP";
   else if( thisSampleType == "EXT" ) mode = "EXT";
   else if( thisSampleType == "Dirt" ) mode = "Dirt";
   else { mode = e.Mode; isNuBackground = true; }

   if(!isNuBackground){
      Hists_ByType[ mode ]->Fill(variable,weight*e.Weight);
      Hists_ByProc[ mode ]->Fill(variable,weight*e.Weight);
   }
   else {

      Hists_ByType[ "OtherNu" ]->Fill(variable,weight*e.Weight);


      if(mode != "ElectronScattering" && mode != "Diffractive" && mode != "Other"){
         if(e.CCNC == "CC"){
            Hists_ByProc[ "CC"+mode ]->Fill(variable,weight*e.Weight);
         }
         else Hists_ByProc[ "NC" ]->Fill(variable,weight*e.Weight);
      }
      else if( mode == "ElectronScattering") Hists_ByProc[ "ElectronScattering" ]->Fill(variable,weight*e.Weight);
      else if( mode == "Diffractive") Hists_ByProc[ "Diffractive" ]->Fill(variable,weight*e.Weight);
      else Hists_ByProc[ "Other" ]->Fill(variable,weight*e.Weight);

   }

}

///////////////////////////////////////////////////////////////////////////////////////////////


void SelectionManager::DrawHistograms(std::string label,double Scale){

   double y_limit = -1;

   system("mkdir -p Plots");
   system("mkdir -p rootfiles");

   // Create weight sums

   for(size_t i_proc=0;i_proc<Procs.size();i_proc++){
      Hists_ByProc[Procs.at(i_proc)]->Scale(Scale);
      Hists_ByProc[Procs.at(i_proc)]->Sumw2();

   }


   for(size_t i_type=0;i_type<Types.size();i_type++){

      Hists_ByType[Types.at(i_type)]->Scale(Scale);
      Hists_ByType[Types.at(i_type)]->Sumw2();

   }



//   if(y_limit != -1) gStyle->SetHistTopMargin(0);

/*
   //write histograms to file
   TFile *f_Hists = TFile::Open("rootfiles/Hists.root","RECREATE");

   for(size_t i_proc=0;i_proc<Procs.size();i_proc++){

      //scale according to POT
      if(Procs.at(i_proc) == "Signal" || Procs.at(i_proc) == "OtherHYP") Hists_ByProc[Procs.at(i_proc)]->Scale(HyperonScale);
      Hists_ByProc[Procs.at(i_proc)]->Write();

   }


   f_Hists->Close();
*/


   TCanvas *c = new TCanvas("c","c",800,600);

   TPad *p_plot = new TPad("pad1","pad1",0,0,1,0.85);
   TPad *p_legend = new TPad("pad2","pad2",0,0.85,1,1);

   p_legend->SetBottomMargin(0);
   p_legend->SetTopMargin(0.1);
   p_plot->SetTopMargin(0.01);

   THStack *hs_Type = new THStack("hs_Type",fTitle.c_str());

   TLegend *l = new TLegend(0.1,0.0,0.9,1.0);
   l->SetBorderSize(0);

   TLegend *l_POT = new TLegend(0.55,0.905,0.89,0.985);
   l_POT->SetBorderSize(0);
   l_POT->SetMargin(0.005);


   if(BeamMode == "FHC")  l_POT->SetHeader(("NuMI FHC, " + to_string_with_precision(fPOT/1e21,1) + " #times 10^{21} POT").c_str());
   if(BeamMode == "RHC")  l_POT->SetHeader(("NuMI RHC, " + to_string_with_precision(fPOT/1e21,1) + " #times 10^{21} POT").c_str());

   l->SetNColumns(3);

   TH1D *h_errors = MakeErrorBand(Hists_ByType);
   h_errors->SetTitle(hs_Type->GetTitle());
   h_errors->SetFillStyle(3253);
   h_errors->SetFillColor(1);

   // Draw histograms by event category

   // Signal first
   Hists_ByType["Signal"]->SetFillColor(8);
   //Hists_ByType["Signal"]->SetLineWidth(0.5);
   //Hists_ByType["Signal"]->SetLineColor(1);
   hs_Type->Add(Hists_ByType["Signal"],"HIST");
   if(fHasData) l->AddEntry(Hists_ByType["Signal"],("Signal = "+ to_string_with_precision(Hists_ByType["Signal"]->Integral(),1)).c_str(),"F");
   else l->AddEntry(Hists_ByType["Signal"],"Signal","F");

   Hists_ByType["OtherHYP"]->SetFillColor(46);
   //Hists_ByType["OtherHYP"]->SetLineWidth(0.5);
   //Hists_ByType["OtherHYP"]->SetLineColor(1);
   hs_Type->Add(Hists_ByType["OtherHYP"],"HIST");
   if(fHasData) l->AddEntry(Hists_ByType["OtherHYP"],("Other HYP = "+ to_string_with_precision(Hists_ByType["OtherHYP"]->Integral(),1)).c_str(),"F");
   else l->AddEntry(Hists_ByType["OtherHYP"],"Other Hyperon","F");

   Hists_ByType["OtherNu"]->SetFillColor(38);
   //Hists_ByType["OtherNu"]->SetLineWidth(0.5);
   //Hists_ByType["OtherNu"]->SetLineColor(1);
   hs_Type->Add(Hists_ByType["OtherNu"],"HIST");
   if(fHasData) l->AddEntry(Hists_ByType["OtherNu"],("Other #nu = "+ to_string_with_precision(Hists_ByType["OtherNu"]->Integral(),1)).c_str(),"F");
   else l->AddEntry(Hists_ByType["OtherNu"],"Other #nu","F");

   Hists_ByType["Dirt"]->SetFillColor(30);
   hs_Type->Add(Hists_ByType["Dirt"],"HIST");
   if(fHasData) l->AddEntry(Hists_ByType["Dirt"],("Dirt = "+ to_string_with_precision(Hists_ByType["Dirt"]->Integral(),1)).c_str(),"F");
   else l->AddEntry(Hists_ByType["Dirt"],"Dirt","F");

   Hists_ByType["EXT"]->SetFillColor(15);
   //Hists_ByType["Signal"]->SetLineWidth(0.5);
   //Hists_ByType["Signal"]->SetLineColor(1);
   hs_Type->Add(Hists_ByType["EXT"],"HIST");
   if(fHasData) l->AddEntry(Hists_ByType["EXT"],("EXT = "+ to_string_with_precision(Hists_ByType["EXT"]->Integral(),1)).c_str(),"F");
   else l->AddEntry(Hists_ByType["EXT"],"EXT","F");

if(fHasData){

   Hists_ByType["Data"]->SetLineWidth(1);
   Hists_ByType["Data"]->SetLineColor(1);
   Hists_ByType["Data"]->SetMarkerStyle(20);
   Hists_ByType["Data"]->SetMarkerColor(1);
   l->AddEntry(Hists_ByType["Data"],("Data = "+ to_string_with_precision(Hists_ByType["Data"]->Integral(),1)).c_str(),"P");

}

   TLegend *l_DataMCRatio = new TLegend(0.12,0.905,0.46,0.985);
   l_DataMCRatio->SetBorderSize(0);
   l_DataMCRatio->SetMargin(0.005);

   double Data = Hists_ByType["Data"]->Integral();
   double MC = Hists_ByType["Signal"]->Integral() + Hists_ByType["OtherHYP"]->Integral() + Hists_ByType["OtherNu"]->Integral() + Hists_ByType["Dirt"]->Integral() + Hists_ByType["EXT"]->Integral();
 

   l_DataMCRatio->SetHeader(("Data/MC = " + to_string_with_precision(Data/MC,2)).c_str());

   // Find the highest bin off the error hist
  
   p_legend->Draw();
   p_legend->cd();
   l->Draw();
   c->cd();
   p_plot->Draw();
   p_plot->cd();

   hs_Type->Draw();


   h_errors->Draw("E2");
   hs_Type->Draw("HIST same");
   h_errors->Draw("E2 same");
   h_errors->GetYaxis()->SetRangeUser(0.0,GetHistMax(h_errors)*1.2);
   h_errors->SetStats(0);
   if(fHasData)   Hists_ByType["Data"]->Draw("E0 P0 same");

   l_POT->Draw();
   if(fHasData)   l_DataMCRatio->Draw();
   if(y_limit != -1){ hs_Type->SetMaximum(y_limit); gPad->Update(); }

   c->cd();

   c->Print(("Plots/" + label + "_By_Type.png").c_str());
   c->Print(("Plots/" + label + "_By_Type.pdf").c_str());
   c->Print(("Plots/" + label + "_By_Type.C").c_str());

  c->Clear();

   if(fHasData) {

        std::cout << std::endl << "Performing Chi2 test" << std::endl;
        double p = Hists_ByType["Data"]->Chi2Test(h_errors,"UWP");
        std::cout << "Chi2 test p value = " << p << std::endl << std::endl;

      TH1D * DataMCRatioPlot = MakeRatioPlot(Hists_ByType,h_errors);
        
      DataMCRatioPlot->Draw("E0 P0");
      //c->Print("Plots/Hists_By_Type_Ratio.pdf");

      c->Clear();
      c->SetCanvasSize(800,750);

       // Setup split canvas
      TPad *p_plot2 = new TPad("p_plot2","p_plot2",0,0.25,1,0.9);
      TPad *p_legend2 = new TPad("p_plot2","p_plot2",0,0.9,1,1);
      TPad *p_ratio = new TPad("p_ratio","p_ratio",0,0.0,1,0.25);
      p_legend2->SetBottomMargin(0);
      p_legend2->SetTopMargin(0.1);
      p_ratio->SetTopMargin(0.01);
      p_plot2->SetTopMargin(0.01);
   // p_plot2->SetBottomMargin(0.01);

        p_ratio->SetGrid(0,1);
        
        p_plot2->Draw();
        p_plot2->cd();
        h_errors->Draw("E2");
        hs_Type->Draw("HIST same");
        h_errors->Draw("E2 same");
        h_errors->GetYaxis()->SetRangeUser(0.0,std::max(GetHistMax(h_errors),GetHistMax(Hists_ByType["Data"]))*1.20);
        h_errors->SetStats(0);
        Hists_ByType["Data"]->Draw("E0 P0 same");

             l_POT->Draw();
        l_DataMCRatio->Draw();
        if(y_limit != -1){ hs_Type->SetMaximum(y_limit); gPad->Update(); }
     
        c->cd();
        p_legend2->Draw();
        p_legend2->cd();
                
        l->Draw(); 

        c->cd();
        
        p_ratio->Draw();
        p_ratio->cd();
        DataMCRatioPlot->Draw("E0 P0");
        DataMCRatioPlot->SetStats(0);

        DataMCRatioPlot->GetYaxis()->SetTitle("Data/MC");
        DataMCRatioPlot->GetYaxis()->SetTitleSize(0.08);
        DataMCRatioPlot->GetYaxis()->SetTitleOffset(0.45);
        DataMCRatioPlot->GetXaxis()->SetLabelSize(0.09);
        DataMCRatioPlot->GetYaxis()->SetLabelSize(0.09);

        c->cd();
        c->Print(("Plots/" + label + "_By_Type_Combined.png").c_str());
        c->Print(("Plots/" + label + "_By_Type_Combined.pdf").c_str());
        c->Print(("Plots/" + label + "_By_Type_Combined.C").c_str());
        c->Clear();    
    
   }


  // Split neutrino backgrounds by channel

  std::cout << "Drawing Proc Hist" << std::endl;

  c->SetCanvasSize(800,600);
   TPad *p_plot3 = new TPad("p_plot3","p_plot3",0,0,1,0.85);
   TPad *p_legend3 = new TPad("p_legend3","p_legend3",0,0.85,1,1);

   p_legend3->SetBottomMargin(0);
   p_legend3->SetTopMargin(0.1);
   p_plot3->SetTopMargin(0.01);

  THStack *hs_Proc = new THStack("hs_Proc",fTitle.c_str());

  l->Clear();
  l->SetNColumns(7);

  // reuse the error hist (errors on predicton should be the same) 

  // Draw histograms by event category

  // Signal first
  Hists_ByProc["Signal"]->SetFillColor(8);
  //Hists_ByProc["Signal"]->SetLineWidth(0.5);
  //Hists_ByProc["Signal"]->SetLineColor(1);
  hs_Proc->Add(Hists_ByProc["Signal"],"HIST");
  if(fHasData) l->AddEntry(Hists_ByProc["Signal"],("Signal = "+ to_string_with_precision(Hists_ByProc["Signal"]->Integral(),1)).c_str(),"F");
  else l->AddEntry(Hists_ByProc["Signal"],"Signal","F");

  Hists_ByProc["OtherHYP"]->SetFillColor(46);
  //Hists_ByProc["OtherHYP"]->SetLineWidth(0.5);
  //Hists_ByProc["OtherHYP"]->SetLineColor(1);
  hs_Proc->Add(Hists_ByProc["OtherHYP"],"HIST");
 if(fHasData) l->AddEntry(Hists_ByProc["OtherHYP"],("Other HYP = "+ to_string_with_precision(Hists_ByProc["OtherHYP"]->Integral(),1)).c_str(),"F");
 else l->AddEntry(Hists_ByProc["OtherHYP"],"Other Hyperon","F");

  // Add the remaining channels

  int i_color=2;

  for(size_t i_proc=0;i_proc<Procs.size();i_proc++){

     std::string thisProc = Procs.at(i_proc);

     if(thisProc == "Signal" || thisProc == "OtherHYP" || thisProc == "Dirt" || thisProc == "EXT" || thisProc == "Data") continue;

     Hists_ByProc[thisProc]->SetFillColor(i_color);
     Hists_ByProc[thisProc]->SetFillStyle(3104);
     
     hs_Proc->Add(Hists_ByProc[thisProc],"HIST");
     if(fHasData) l->AddEntry(Hists_ByProc[thisProc],(thisProc + " = " + to_string_with_precision(Hists_ByProc[thisProc]->Integral(),1)).c_str(),"F");
     else  l->AddEntry(Hists_ByProc[thisProc],thisProc.c_str(),"F");   

     i_color++;

     if(i_color == 10) i_color++;

  }


Hists_ByProc["Dirt"]->SetFillColor(30);
hs_Proc->Add(Hists_ByProc["Dirt"],"HIST");
if(fHasData) l->AddEntry(Hists_ByProc["Dirt"],("Dirt = "+ to_string_with_precision(Hists_ByProc["Dirt"]->Integral(),1)).c_str(),"F");
else l->AddEntry(Hists_ByProc["Dirt"],"Dirt","F");


Hists_ByProc["EXT"]->SetFillColor(15);
//Hists_ByProc["Signal"]->SetLineWidth(0.5);
//Hists_ByProc["Signal"]->SetLineColor(1);
hs_Proc->Add(Hists_ByProc["EXT"],"HIST");
if(fHasData) l->AddEntry(Hists_ByProc["EXT"],("EXT = "+ to_string_with_precision(Hists_ByProc["EXT"]->Integral(),1)).c_str(),"F");
else l->AddEntry(Hists_ByProc["EXT"],"EXT","F");


if(fHasData){

   Hists_ByProc["Data"]->SetLineWidth(1);
   Hists_ByProc["Data"]->SetLineColor(1);
   Hists_ByProc["Data"]->SetMarkerStyle(20);
   Hists_ByProc["Data"]->SetMarkerColor(1);
   l->AddEntry(Hists_ByProc["Data"],("Data = "+ to_string_with_precision(Hists_ByProc["Data"]->Integral(),1)).c_str(),"P");

}


std::cout << "Hists set up" << std::endl;

   p_legend3->Draw();
   p_legend3->cd();
   l->Draw();
   c->cd();
   p_plot3->Draw();
   p_plot3->cd();

   hs_Proc->Draw();


   h_errors->Draw("E2");
   hs_Proc->Draw("HIST same");
   h_errors->Draw("E2 same");
   h_errors->GetYaxis()->SetRangeUser(0.0,GetHistMax(h_errors)*1.2);
   h_errors->SetStats(0);
   if(fHasData)   Hists_ByProc["Data"]->Draw("E0 P0 same");

   l_POT->Draw();
   if(fHasData)   l_DataMCRatio->Draw();
   if(y_limit != -1){ hs_Proc->SetMaximum(y_limit); gPad->Update(); }

   c->cd();

   c->Print(("Plots/" + label + "_By_Proc.png").c_str());
   c->Print(("Plots/" + label + "_By_Proc.pdf").c_str());
   c->Print(("Plots/" + label + "_By_Proc.C").c_str());

  c->Clear();

   if(fHasData) {

        std::cout << std::endl << "Performing Chi2 test" << std::endl;

        double p = Hists_ByProc["Data"]->Chi2Test(h_errors,"UWP");
   
        std::cout << "Chi2 test p value = " << p << std::endl << std::endl;

      TH1D * DataMCRatioPlot2 = MakeRatioPlot(Hists_ByType,h_errors);

      c->Clear();
      c->SetCanvasSize(800,750);

       // Setup split canvas
      TPad *p_plot4 = new TPad("p_plot4","p_plot4",0,0.25,1,0.9);
      TPad *p_legend4 = new TPad("p_plot4","p_plot4",0,0.9,1,1);
      TPad *p_ratio2 = new TPad("p_ratio2","p_ratio2",0,0.0,1,0.25);

      p_legend4->SetBottomMargin(0);
      p_legend4->SetTopMargin(0.1);
      p_ratio2->SetTopMargin(0.01);
      p_plot4->SetTopMargin(0.01);
      p_ratio2->SetGrid(0,1);
        
        p_plot4->Draw();
        p_plot4->cd();
        h_errors->Draw("E2");
        hs_Proc->Draw("HIST same");
        h_errors->Draw("E2 same");
        h_errors->GetYaxis()->SetRangeUser(0.0,std::max(GetHistMax(h_errors),GetHistMax(Hists_ByProc["Data"]))*1.20);
        h_errors->SetStats(0);
        Hists_ByProc["Data"]->Draw("E0 P0 same");

        l_POT->Draw();
        l_DataMCRatio->Draw();
        if(y_limit != -1){ hs_Proc->SetMaximum(y_limit); gPad->Update(); }
     
        c->cd();
        p_legend4->Draw();
        p_legend4->cd();
                
        l->Draw(); 

        c->cd();
        
        p_ratio2->Draw();
        p_ratio2->cd();
        DataMCRatioPlot2->Draw("E0 P0");
        DataMCRatioPlot2->SetStats(0);

        DataMCRatioPlot2->GetYaxis()->SetTitle("Data/MC");
        DataMCRatioPlot2->GetYaxis()->SetTitleSize(0.08);
        DataMCRatioPlot2->GetYaxis()->SetTitleOffset(0.45);
        DataMCRatioPlot2->GetXaxis()->SetLabelSize(0.09);
        DataMCRatioPlot2->GetYaxis()->SetLabelSize(0.09);

        c->cd();
        c->Print(("Plots/" + label + "_By_Proc_Combined.png").c_str());
        c->Print(("Plots/" + label + "_By_Proc_Combined.pdf").c_str());
        c->Print(("Plots/" + label + "_By_Proc_Combined.C").c_str());
        c->Clear();    
    
   }








      c->Close();

}

#endif
