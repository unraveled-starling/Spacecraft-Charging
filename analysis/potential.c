#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

//#include "setup.h"

using namespace std;

void potential()
{
    const double BIN_WIDTH = 0.005;          // mm
    const double dx = BIN_WIDTH*1e-3;        // m

    ifstream in("field.csv");

    if(!in)
    {
        cout<<"Cannot open field.csv"<<endl;
        return;
    }

    string line;
    getline(in,line);

    vector<double> depth;
    vector<double> Efield;
    vector<double> potential;

    double V = 0.0;

    while(getline(in,line))
    {
        double d,rho,er,E;

        sscanf(line.c_str(),
               "%lf,%lf,%lf,%lf",
               &d,&rho,&er,&E);

        depth.push_back(d);
        Efield.push_back(E);

        potential.push_back(V);

        V -= E*dx;
    }

    in.close();

    //----------------------------------------------------

    ofstream out("potential.csv");

    out<<"Depth_mm,"
          "ElectricField_V_per_m,"
          "Potential_V\n";

    for(size_t i=0;i<depth.size();i++)
    {
        out
        <<depth[i]<<","
        <<Efield[i]<<","
        <<potential[i]<<"\n";
    }

    out.close();

    //----------------------------------------------------

    cout<<endl;
    cout<<"=========================================================="<<endl;
    cout<<"POTENTIAL DIFFERENCE ACROSS SOLAR PANEL"<<endl;
    cout<<"=========================================================="<<endl;
    cout<<endl;

    cout<<left<<setw(35)<<"Front Surface (SiO2)"
        <<"0.000 mm"<<endl;

    cout<<left<<setw(35)<<"Back Surface (Aluminium)"
        <<"0.880 mm"<<endl;

    cout<<left<<setw(35)<<"Reference Potential"
        <<"0.0 V"<<endl;

    cout<<left<<setw(35)<<"Potential at Back Surface"
        <<scientific<<setprecision(6)<<V<<" V"<<endl;

    cout<<left<<setw(35)<<"Potential Difference"
        <<scientific<<setprecision(6)<<fabs(V)<<" V"<<endl;

    cout<<endl;
    cout<<"Saved potential.csv"<<endl;
}
