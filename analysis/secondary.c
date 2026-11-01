#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

void secondary()
{
    TTree *t = (TTree*)_file0->Get("Hit Data");

    if(!t)
    {
        cout<<"Hit Data tree not found!"<<endl;
        return;
    }

    //==========================
    // Branches
    //==========================

    Int_t eventID;

    Char_t particle[6];
    Char_t volume[11];

    Double_t parentID;
    Double_t trackID;

    t->SetBranchAddress("Event_Number",&eventID);
    t->SetBranchAddress("Particle_Type",particle);
    t->SetBranchAddress("Volume_Name_Post",volume);
    t->SetBranchAddress("Parent_ID",&parentID);
    t->SetBranchAddress("Track_ID",&trackID);

    //==========================
    // Counters
    //==========================

    set<long long> totalSecondaries;
    set<long long> electronSecondaries;
    set<long long> gammaSecondaries;
    set<long long> directSecondaries;
    set<long long> tertiarySecondaries;

    map<string,set<long long>> layerSecondaries;

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

        if(parentID<=0)
            continue;

        long long id =
        (long long)eventID*1000000LL +
        (long long)trackID;

        totalSecondaries.insert(id);

        string p = particle;

        if(p=="e-")
            electronSecondaries.insert(id);

        else if(p=="gamma")
            gammaSecondaries.insert(id);

        if(parentID==1)
            directSecondaries.insert(id);

        if(parentID>1)
            tertiarySecondaries.insert(id);

        string layer = volume;

        if(layer!="World" && layer!="OutOfWorld")
            layerSecondaries[layer].insert(id);
    }

    //==========================
    // Summary
    //==========================

    cout<<endl;
    cout<<"=========================================="<<endl;
    cout<<"SECONDARY PARTICLE STATISTICS"<<endl;
    cout<<"=========================================="<<endl;

    cout<<"Total Secondaries      : "<<totalSecondaries.size()<<endl;
    cout<<"Electron Secondaries   : "<<electronSecondaries.size()<<endl;
    cout<<"Gamma Secondaries      : "<<gammaSecondaries.size()<<endl;
    cout<<"Direct Secondaries     : "<<directSecondaries.size()<<endl;
    cout<<"Tertiaries             : "<<tertiarySecondaries.size()<<endl;

    cout<<endl;
    cout<<"=========================================="<<endl;
    cout<<"SECONDARIES BY LAYER"<<endl;
    cout<<"=========================================="<<endl;

    for(auto &layer : layerOrder)
    {
        cout<<left
            <<setw(15)
            <<layer
            <<" : "
            <<layerSecondaries[layer].size()
            <<endl;
    }

    //==========================
    // CSV
    //==========================

    ofstream fout("secondary.csv");

    fout<<"Quantity,Count\n";

    fout<<"Total Secondaries,"
        <<totalSecondaries.size()<<"\n";

    fout<<"Electron Secondaries,"
        <<electronSecondaries.size()<<"\n";

    fout<<"Gamma Secondaries,"
        <<gammaSecondaries.size()<<"\n";

    fout<<"Direct Secondaries,"
        <<directSecondaries.size()<<"\n";

    fout<<"Tertiaries,"
        <<tertiarySecondaries.size()<<"\n";

    fout<<"\nLayer,Secondaries\n";

    for(auto &layer : layerOrder)
    {
        fout
        <<layer
        <<","
        <<layerSecondaries[layer].size()
        <<"\n";
    }

    fout.close();

    cout<<endl;
    cout<<"Saved secondary.csv"<<endl;
}
