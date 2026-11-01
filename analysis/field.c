#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

//#include "setup.h"

using namespace std;

void field()
{
    //----------------------------------------------------
    // Geometry
    //----------------------------------------------------

    const double PANEL_LENGTH = 0.880;      // mm
    const double BIN_WIDTH    = 0.005;      // mm

    const int NBINS = int(PANEL_LENGTH/BIN_WIDTH);

    //----------------------------------------------------
    // Layer boundaries (mm)
    //----------------------------------------------------

    const double SiO2_end      = 0.100;
    const double Adhesive1_end = 0.125;
    const double Silicon_end   = 0.305;
    const double Adhesive2_end = 0.330;
    const double Kapton_end    = 0.380;
    const double Aluminium_end = 0.880;

    //----------------------------------------------------
    // Constants
    //----------------------------------------------------

    const double epsilon0 = 8.854187817e-12;
    const double dx = BIN_WIDTH * 1e-3;     // m

    //----------------------------------------------------
    // Read charge density CSV
    //----------------------------------------------------

    ifstream in("chargedensity.csv");

    if(!in)
    {
        cout<<"Cannot open chargedensity.csv"<<endl;
        return;
    }

    string line;
    getline(in,line);      // skip header

    vector<double> depth;
    vector<double> rho;
    vector<double> Er;
    vector<double> eps_r;

    //----------------------------------------------------

    double avgESiO2=0, avgEAd1=0, avgESi=0, avgEAd2=0, avgEKap=0, avgEAl=0;

    double maxESiO2=0, maxEAd1=0, maxESi=0, maxEAd2=0, maxEKap=0, maxEAl=0;

    double E = 0.0;

    while(getline(in,line))
    {
        double d,electrons,charge,rhoValue;

        sscanf(line.c_str(),
               "%lf,%lf,%lf,%lf",
               &d,&electrons,&charge,&rhoValue);

        double er;

        if(d < SiO2_end)
            er = 3.9;
        else if(d < Adhesive1_end)
            er = 2.8;
        else if(d < Silicon_end)
            er = 11.7;
        else if(d < Adhesive2_end)
            er = 2.8;
        else if(d < Kapton_end)
            er = 3.4;
        else
            er = 1.0;          // Aluminium

        double epsilon = epsilon0 * er;

        E += (rhoValue/epsilon)*dx;
        
        double absE = fabs(E);

        depth.push_back(d);
        rho.push_back(rhoValue);
        Er.push_back(E);
        eps_r.push_back(er);

if(d < SiO2_end)
{
    avgESiO2 += E;
    if(absE > maxESiO2) maxESiO2 = absE;
}
else if(d < Adhesive1_end)
{
    avgEAd1 += E;
    if(absE > maxEAd1) maxEAd1 = absE;
}
else if(d < Silicon_end)
{
    avgESi += E;
    if(absE > maxESi) maxESi = absE;
}
else if(d < Adhesive2_end)
{
    avgEAd2 += E;
    if(absE > maxEAd2) maxEAd2 = absE;
}
else if(d < Kapton_end)
{
    avgEKap += E;
    if(absE > maxEKap) maxEKap = absE;
}
else
{
    avgEAl += E;
    if(absE > maxEAl) maxEAl = absE;
}
    
    }

    in.close();

    //----------------------------------------------------
    // Write CSV
    //----------------------------------------------------

    ofstream out("field.csv");

    out<<"Depth_mm,"
          "ChargeDensity_C_per_m3,"
          "RelativePermittivity,"
          "ElectricField_V_per_m\n";

    for(size_t i=0;i<depth.size();i++)
    {
        out
        <<depth[i]<<","
        <<rho[i]<<","
        <<eps_r[i]<<","
        <<Er[i]<<"\n";
    }

    out.close();

    //----------------------------------------------------
    // Print Summary
    //----------------------------------------------------

    cout<<endl;
    cout<<"==============================================================================="<<endl;
    cout<<"LAYER-WISE ELECTRIC FIELD"<<endl;
    cout<<"==============================================================================="<<endl;
    cout<<endl;

    cout<<left
        <<setw(15)<<"Layer"
        <<setw(12)<<"Thickness"
        <<setw(10)<<"eps_r"
        <<setw(22)<<"Avg Charge Density"
        <<setw(20)<<"Avg E-Field"
        <<setw(20)<<"Max E-Field"
        <<endl;

    cout<<left
        <<setw(15)<<""
        <<setw(12)<<"(um)"
        <<setw(10)<<""
        <<setw(22)<<"(C/m^3)"
        <<setw(20)<<"(V/m)"
        <<setw(20)<<"(V/m)"
        <<endl;

    cout<<"==============================================================================="<<endl;

    auto printLayer=[&](string name,
                        double thickness,
                        double er,
                        double avgE,
                        double maxE,
                        double avgRho,
                        double thickness_mm)
    {
        int bins = thickness_mm/BIN_WIDTH;

        cout<<left
            <<setw(15)<<name
            <<setw(12)<<fixed<<setprecision(1)<<thickness
            <<setw(10)<<fixed<<setprecision(1)<<er
            <<setw(22)<<scientific<<setprecision(6)<<avgRho
            <<setw(20)<<scientific<<setprecision(6)<<avgE
            <<setw(20)<<scientific<<setprecision(6)<<maxE
            <<endl;
    };

    //----------------------------------------------------
    // Average charge density from field CSV
    //----------------------------------------------------

    double rhoSiO2=0,rhoAd1=0,rhoSi=0,rhoAd2=0,rhoKap=0,rhoAl=0;

    for(size_t i=0;i<depth.size();i++)
    {
        if(depth[i] < SiO2_end)
            rhoSiO2 += rho[i];
        else if(depth[i] < Adhesive1_end)
            rhoAd1 += rho[i];
        else if(depth[i] < Silicon_end)
            rhoSi += rho[i];
        else if(depth[i] < Adhesive2_end)
            rhoAd2 += rho[i];
        else if(depth[i] < Kapton_end)
            rhoKap += rho[i];
        else
            rhoAl += rho[i];
    }

printLayer("SiO2",
           100.0,
           3.9,
           avgESiO2/20,
           maxESiO2,
           rhoSiO2/20,
           0.100);

printLayer("Adhesive1",
           25.0,
           2.8,
           avgEAd1/5,
           maxEAd1,
           rhoAd1/5,
           0.025);

printLayer("Silicon",
           180.0,
           11.7,
           avgESi/36,
           maxESi,
           rhoSi/36,
           0.180);

printLayer("Adhesive2",
           25.0,
           2.8,
           avgEAd2/5,
           maxEAd2,
           rhoAd2/5,
           0.025);

printLayer("Kapton",
           50.0,
           3.4,
           avgEKap/10,
           maxEKap,
           rhoKap/10,
           0.050);

printLayer("Aluminium",
           500.0,
           1.0,
           avgEAl/100,
           maxEAl,
           rhoAl/100,
           0.500);
        
    cout<<endl;
    cout<<"Saved field.csv"<<endl;
}
