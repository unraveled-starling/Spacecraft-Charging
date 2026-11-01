#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

//#include "setup.h"

using namespace std;

struct ElectronData
{
    int trackID;
    string layer;
    double x;
    double y;
    double z;
};

void charge()
{
    TTree *t = (TTree*)_file0->Get("Hit Data");

    if(!t)
    {
        cout<<"Hit Data tree not found!"<<endl;
        return;
    }

    char particle[100];
    char volume[100];

    double ke;
    double trackID;

    vector<double> *postPos = nullptr;

    t->SetBranchAddress("Particle_Type",particle);
    t->SetBranchAddress("Volume_Name_Post",volume);
    t->SetBranchAddress("Kinetic_Energy_Post_keV",&ke);
    t->SetBranchAddress("Track_ID",&trackID);
    t->SetBranchAddress("Post_Step_Position_mm",&postPos);

    map<string,set<int>> trappedTracks;

    trappedTracks["SiO2"];
    trappedTracks["Adhesive1"];
    trappedTracks["Silicon"];
    trappedTracks["Adhesive2"];
    trappedTracks["Kapton"];
    trappedTracks["Aluminium"];

    vector<ElectronData> electrons;

    Long64_t N=t->GetEntries();

    for(Long64_t i=0;i<N;i++)
    {
        t->GetEntry(i);

        if(string(particle)!="e-")
            continue;

        if(ke>1e-6)
            continue;

        string layer(volume);

        if(trappedTracks.find(layer)==trappedTracks.end())
            continue;

        int id=(int)trackID;

        // Already counted this electron
        if(trappedTracks[layer].count(id))
            continue;

        trappedTracks[layer].insert(id);

        ElectronData e;

        e.trackID=id;
        e.layer=layer;
        e.x=(*postPos)[0];
        e.y=(*postPos)[1];
        e.z=(*postPos)[2];

        electrons.push_back(e);
    }

    const double q=-1.602176634e-19;
    
    
ofstream summary("charge.csv");

summary<<"Layer,Unique_Trapped_Electrons,Charge_C\n";


// Calculate total trapped charge
int totalElectrons = 0;

for (auto &x : trappedTracks)
{
    totalElectrons += x.second.size();
}

double totalCharge = totalElectrons * q;

cout<<endl;
cout<<"=============================================================="<<endl;
cout<<"CHARGE ACCUMULATION"<<endl;
cout<<"=============================================================="<<endl;

cout<<"Total Trapped Charge = "
    << scientific << setprecision(6)
    << totalCharge << " C ("
    << totalElectrons << " electrons)" << endl;
cout<<"                                                             "<<endl;

cout<<"--------------------------------------------------------------"<<endl;
cout<<left
    <<setw(15)<<"Layer"
    <<right<<setw(15)<<"Electrons"
    <<setw(20)<<"Charge (C)"
    <<endl;

cout<<"--------------------------------------------------------------"<<endl;

for(auto &x:trappedTracks)
{
    int n = x.second.size();
    double charge = n*q;

    cout<<left
        <<setw(15)<<x.first
        <<right<<setw(15)<<n
        <<setw(20)<<scientific<<setprecision(6)<<charge
        <<endl;

    summary
        <<x.first<<","
        <<n<<","
        <<charge<<"\n";
}

    summary.close();

    ofstream detail("charge_positions.csv");

    detail<<"Track_ID,"
          <<"Layer,"
          <<"X_mm,"
          <<"Y_mm,"
          <<"Z_mm,"
          <<"Charge_C\n";

    for(auto &e:electrons)
    {
        detail
        <<e.trackID<<","
        <<e.layer<<","
        <<e.x<<","
        <<e.y<<","
        <<e.z<<","
        <<q<<"\n";
    }

    detail.close();

    cout<<endl;
    cout<<"Saved charge.csv"<<endl;
    cout<<"Saved charge_positions.csv"<<endl;
}
