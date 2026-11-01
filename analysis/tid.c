#include <iostream>
#include <fstream>
#include <map>
#include <iomanip>
//#include "setup.h"

using namespace std;

void tid()
{
ifstream fin("energy.csv");

if(!fin)
{
    cout << "Cannot open energy.csv" << endl;
    return;
}

string line;
getline(fin, line);          // Skip header

map<string,double> energy;

while(getline(fin, line))
{
    size_t comma = line.find(',');

    if(comma == string::npos)
        continue;

    string layer = line.substr(0, comma);

    double edep = stod(line.substr(comma + 1));

    energy[layer] = edep;
}

fin.close();

cout << "\nCSV READ:\n";

for(auto &x : energy)
    cout << x.first << "  " << x.second << endl;

    //--------------------------------------------------
    // Geometry
    //--------------------------------------------------

    const double width  = 2.0;      // mm
    const double height = 2.0;      // mm

    map<string,double> thickness;
    map<string,double> density;

    thickness["SiO2"]       = 0.10;
    thickness["Adhesive1"]  = 0.025;
    thickness["Silicon"]    = 0.18;
    thickness["Adhesive2"]  = 0.025;
    thickness["G4_KAPTON"]  = 0.05;
    thickness["Aluminium"]  = 0.50;

    density["SiO2"]       = 2.201;
    density["Adhesive1"]  = 1.03;
    density["Silicon"]    = 2.33;
    density["Adhesive2"]  = 1.03;
    density["G4_KAPTON"]  = 1.42;
    density["Aluminium"]  = 2.70;

    //--------------------------------------------------
    // Constants
    //--------------------------------------------------

    const double keV_to_J = 1.602176634e-16;

    double totalEnergyJ = 0.0;
    double totalMasskg  = 0.0;

    ofstream fout("tid.csv");

    fout << "Layer\tDose_Gy\n";

    cout<<endl;
    cout<<"=============================================="<<endl;
    cout<<"LAYER-WISE TID"<<endl;
    cout<<"=============================================="<<endl;

    for(auto &x : energy)
    {
        string mat = x.first;

        double volume_cm3 =
            (width/10.0) *
            (height/10.0) *
            (thickness[mat]/10.0);

        double mass_kg =
            volume_cm3 *
            density[mat] /
            1000.0;

        double dose =
            x.second *
            keV_to_J /
            mass_kg;

        cout
        <<left<<setw(15)
        <<mat
        <<" : "
        <<dose
        <<" Gy"<<endl;

        fout
        <<mat
        <<"\t"
        <<dose
        <<"\n";

        totalEnergyJ += x.second * keV_to_J;
        totalMasskg  += mass_kg;
    }

    double totalDose =
        totalEnergyJ / totalMasskg;

    cout<<"----------------------------------------------"<<endl;
    cout<<"WHOLE STACK TID : "<<totalDose<<" Gy"<<endl;
    cout<<"=============================================="<<endl;

    fout<<"\nWholeStack\t"<<totalDose<<"\n";

    fout.close();
}
