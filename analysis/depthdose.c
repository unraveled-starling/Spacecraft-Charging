#include <map>
#include <iostream>
#include <fstream>
#include "setup.h"

using namespace std;

void depthdose()
{
    TTree *t = (TTree*)_file0->Get("Hit Data");

    if(!t)
    {
        cout<<"Hit Data tree not found!"<<endl;
        return;
    }

    vector<double> *pos = nullptr;
    double edep;

    t->SetBranchAddress("Post_Step_Position_mm",&pos);
    t->SetBranchAddress("Energy_Deposit_keV",&edep);

    // Bin width = 0.01 mm = 10 um
    const double binWidth = 0.005;

    map<int,double> doseProfile;

    Long64_t N = t->GetEntries();

    for(Long64_t i=0;i<N;i++)
    {
        t->GetEntry(i);

        if(edep<=0) continue;

        double x = (*pos)[0];

        int bin = (int)floor(x/binWidth);

        doseProfile[bin] += edep;
    }

    cout << endl;
    cout << "==========================================" << endl;
    cout << "DEPTH DOSE PROFILE" << endl;
    cout << "==========================================" << endl;
    cout << "Depth (mm)\tDeposited Energy (keV)" << endl;

    for(auto &b : doseProfile)
    {
        double xcentre = (b.first+0.2)*binWidth;

        cout
        << xcentre
        << "\t"
        << b.second
        << endl;
    }

    cout << "==========================================" << endl;
    
    
    
    
ofstream fout("depthdose.csv");

fout << "Depth_mm,Deposited_Energy_keV\n";

for(auto &b : doseProfile)
{
    double xcentre = (b.first+0.2)*binWidth;

    fout << xcentre
         << ","
         << b.second
         << "\n";
}

fout.close();

cout << "\nDepth-dose data saved to depthdose.csv\n";
    
    
    
    
    TGraph *g = new TGraph();
    
    int p = 0;
    
    for(auto &b : doseProfile)
    {
       double xcentre = (b.first+0.2)*binWidth;
       g->SetPoint(p++, xcentre, b.second);
    }
    
    g->SetTitle("Depth Dose Profile;Depth (mm);Deposited Energy (keV)");
    g->GetXaxis()->SetLimits(-0.4, 1.5);
    
    TCanvas *c = new TCanvas("c","Depth Dose",900,600);
    
    g->SetLineWidth(2);
    g->Draw("AL");
    
    c->SaveAs((PREFIX+"_depthdose.png").c_str());
}
