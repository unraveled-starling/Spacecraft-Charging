#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>

//#include "setup.h"

using namespace std;

void density()
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

    //----------------------------------------------------
    // Geometry
    //----------------------------------------------------

    const double PANEL_LENGTH = 0.880;      // mm
    const double PANEL_WIDTH  = 2.0;        // mm
    const double PANEL_HEIGHT = 2.0;        // mm

    const double BIN_WIDTH = 0.005;         // mm (5 um)

    const int NBINS = int(PANEL_LENGTH/BIN_WIDTH);

    vector<int> electrons(NBINS,0);

    vector< set<int> > counted(NBINS);

    //----------------------------------------------------

    Long64_t N = t->GetEntries();

    for(Long64_t i=0;i<N;i++)
    {
        t->GetEntry(i);

        if(string(particle)!="e-")
            continue;

        if(ke>1e-6)
            continue;

        double x = (*postPos)[0];

        if(x<0.0 || x>=PANEL_LENGTH)
            continue;

        int bin = int(x/BIN_WIDTH);

        int id = (int)trackID;

        if(counted[bin].count(id))
            continue;

        counted[bin].insert(id);

        electrons[bin]++;
    }

    //----------------------------------------------------

    const double eCharge = -1.602176634e-19;
     
    const double SiO2_end      = 0.100;
    const double Adhesive1_end = 0.125;
    const double Silicon_end   = 0.305;
    const double Adhesive2_end = 0.330;
    const double Kapton_end    = 0.380;
    const double Aluminium_end = 0.880;

double rhoSiO2=0,rhoAd1=0,rhoSi=0,rhoAd2=0,rhoKap=0,rhoAl=0;
double qSiO2=0,qAd1=0,qSi=0,qAd2=0,qKap=0,qAl=0;   

    // mm³ → m³
    const double binVolume =
        (BIN_WIDTH*1e-3)*
        (PANEL_WIDTH*1e-3)*
        (PANEL_HEIGHT*1e-3);

    ofstream out("chargedensity.csv");

    out<<"Depth_mm,"
          "Electrons,"
          "Charge_C,"
          "ChargeDensity_C_per_m3\n";

    for(int i=0;i<NBINS;i++)
    {
        double depth = (i+0.5)*BIN_WIDTH;

        double charge = electrons[i]*eCharge;

        double rho = charge/binVolume;
        
if(depth < SiO2_end)
{
    rhoSiO2 += rho;
    qSiO2 += charge;
}
else if(depth < Adhesive1_end)
{
    rhoAd1 += rho;
    qAd1 += charge;
}
else if(depth < Silicon_end)
{
    rhoSi += rho;
    qSi += charge;
}
else if(depth < Adhesive2_end)
{
    rhoAd2 += rho;
    qAd2 += charge;
}
else if(depth < Kapton_end)
{
    rhoKap += rho;
    qKap += charge;
}
else
{
    rhoAl += rho;
    qAl += charge;
}

        out
        <<depth<<","
        <<electrons[i]<<","
        <<charge<<","
        <<rho<<"\n";
    }

    out.close();
    

cout<<endl;
cout<<"==========================================================================="<<endl;
cout<<"LAYER-WISE CHARGE DENSITY"<<endl;
cout<<"==========================================================================="<<endl;
cout<<"                                                                           "<<endl;

cout<<left
    <<setw(15)<<"Layer"
    //<<right
    <<setw(12)<<"Thickness"
    <<setw(15)<<"Mat. Density"
    <<setw(18)<<"Charge (C)"
    <<setw(18)<<"Charge Density"
    <<endl;

cout<<left
    <<setw(15)<<""
    //<<right
    <<setw(12)<<"(um)"
    <<setw(15)<<"(g/cm^3)"
    <<setw(18)<<""
    <<setw(18)<<"(C/m^3)"
    <<endl;

cout<<"==========================================================================="<<endl;

cout<<"---------------------------------------------------------------------------"<<endl;

auto printLayer=[&](string name,
                    double thickness_um,
                    double material_density,
                    double Q,
                    double rhoSum,
                    double thickness_mm)
{
    int bins = thickness_mm/BIN_WIDTH;

    cout<<left
        <<setw(15)<<name
        //<<right
        <<setw(12)<<fixed<<setprecision(1)<<thickness_um
        <<setw(15)<<fixed<<setprecision(2)<<material_density
        <<setw(18)<<scientific<<setprecision(6)<<Q
        <<setw(18)<<scientific<<setprecision(6)<<rhoSum/bins
        <<endl;
};

printLayer("SiO2",      100.0, 2.20, qSiO2, rhoSiO2, 0.100);
printLayer("Adhesive1",  25.0, 1.03, qAd1 , rhoAd1 , 0.025);
printLayer("Silicon",   180.0, 2.33, qSi  , rhoSi  , 0.180);
printLayer("Adhesive2",  25.0, 1.03, qAd2 , rhoAd2 , 0.025);
printLayer("Kapton",     50.0, 1.42, qKap , rhoKap , 0.050);
printLayer("Aluminium", 500.0, 2.70, qAl  , rhoAl  , 0.500);


cout<<"\nSaved chargedensity.csv"<<endl;
}
