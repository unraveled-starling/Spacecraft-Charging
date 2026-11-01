#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cmath>

using namespace std;

void energy()
{
    TTree *t = (TTree*)_file0->Get("Hit Data");

    if(!t)
    {
        cout<<"Hit Data tree not found!"<<endl;
        return;
    }

    //==================================================
    // Branches
    //==================================================

    Int_t eventID;

    Char_t volume[11];

    Double_t edep;
    Double_t niel;

    t->SetBranchAddress("Event_Number",&eventID);
    t->SetBranchAddress("Volume_Name_Post",volume);
    t->SetBranchAddress("Energy_Deposit_keV",&edep);
    t->SetBranchAddress("NonIonizing_Energy_Deposit_keV",&niel);

    //==================================================
    // Containers
    //==================================================

    double totalEdep = 0.0;
    double totalNIEL = 0.0;

    map<int,double> eventEdep;
    map<string,double> layerEdep;

    vector<string> layerOrder =
    {
        "SiO2",
        "Adhesive1",
        "Silicon",
        "Adhesive2",
        "Kapton",
        "Aluminium"
    };

    Long64_t N = t->GetEntries();

    for(Long64_t i=0;i<N;i++)
    {
        t->GetEntry(i);

        totalEdep += edep;
        totalNIEL += niel;

        eventEdep[eventID] += edep;

        string layer = volume;

        if(layer!="World" && layer!="OutOfWorld")
            layerEdep[layer] += edep;
    }

    //==================================================
    // Event statistics
    //==================================================

    vector<double> values;

    for(auto &x : eventEdep)
        values.push_back(x.second);

    double mean = 0.0;

    for(double x : values)
        mean += x;

    if(!values.empty())
        mean /= values.size();

    double sigma = 0.0;

    for(double x : values)
        sigma += (x-mean)*(x-mean);

    if(!values.empty())
        sigma = sqrt(sigma/values.size());

    double minE = values.empty() ? 0 : *min_element(values.begin(),values.end());
    double maxE = values.empty() ? 0 : *max_element(values.begin(),values.end());

    //==================================================
    // Print
    //==================================================

    cout<<endl;
    cout<<"=========================================="<<endl;
    cout<<"ENERGY DEPOSITION"<<endl;
    cout<<"=========================================="<<endl;

    cout<<"Total Deposited Energy : "<<totalEdep<<" keV"<<endl;
    cout<<"Average / Primary      : "<<totalEdep/1000.0<<" keV"<<endl;

    cout<<endl;

    cout<<"Total NIEL             : "<<totalNIEL<<" keV"<<endl;
    cout<<"Average NIEL / Primary : "<<totalNIEL/1000.0<<" keV"<<endl;

    cout<<endl;
    cout<<"=========================================="<<endl;
    cout<<"EVENT STATISTICS"<<endl;
    cout<<"=========================================="<<endl;

    cout<<"Primaries              : "<<values.size()<<endl;
    cout<<"Mean                   : "<<mean<<" keV"<<endl;
    cout<<"Std Dev                : "<<sigma<<" keV"<<endl;
    cout<<"Minimum                : "<<minE<<" keV"<<endl;
    cout<<"Maximum                : "<<maxE<<" keV"<<endl;

    cout<<endl;
    cout<<"=========================================="<<endl;
    cout<<"LAYER-WISE ENERGY"<<endl;
    cout<<"=========================================="<<endl;

    for(auto &layer : layerOrder)
    {
        double percent = 0.0;

        if(totalEdep>0)
            percent = 100.0*layerEdep[layer]/totalEdep;

        cout<<left
            <<setw(15)
            <<layer
            <<" : "
            <<setw(15)
            <<right
            <<layerEdep[layer]
            <<" keV   ("
            <<fixed<<setprecision(2)
            <<percent
            <<" %)"
            <<endl;
    }

    //==================================================
    // CSV
    //==================================================

    ofstream fout("energy.csv");

    fout<<"Layer,Edep_keV\n";

    for(auto &layer : layerOrder)
    {
        fout
        <<layer
        <<","
        <<layerEdep[layer]
        <<"\n";
    }

    fout.close();

    cout<<endl;
    cout<<"Saved energy.csv"<<endl;
}
