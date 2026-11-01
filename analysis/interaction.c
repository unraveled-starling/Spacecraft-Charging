#include <set> 
#include <iostream> 
//#include "setup.h"

void interaction() 
{
  TTree *t = (TTree*)_file0->Get("Hit Data"); 
  
  if(!t){ 
      std::cout << "Hit Data tree not found!" << std::endl; 
      return; 
  } 
  
  
// ========================= 
// TRANSMITTED 
// ========================= 
t->Draw(
"Event_Number",
"Track_ID==1 && Parent_ID==0 && Volume_Name_Post==\"OutOfWorld\" && Post_Step_Position_mm[0]>0",
"goff");

std::set<double> transmitted;

for(int i=0;i<t->GetSelectedRows();i++)
    transmitted.insert(t->GetV1()[i]);
  
  
// ========================= 
// REFLECTED 
// ========================= 
t->Draw(
"Event_Number",
"Track_ID==1 && Parent_ID==0 && Volume_Name_Post==\"OutOfWorld\" && Post_Step_Position_mm[0]<0",
"goff");

std::set<double> reflected;

for(int i=0;i<t->GetSelectedRows();i++)
    reflected.insert(t->GetV1()[i]);
 
 
// ========================= 
// ABSORBED (stopped inside) 
// ========================= 
t->Draw(
"Event_Number",
"Track_ID==1 && Parent_ID==0 && Kinetic_Energy_Post_keV<=0",
"goff");

std::set<double> absorbed;

for(int i=0;i<t->GetSelectedRows();i++)
    absorbed.insert(t->GetV1()[i]);
 
 
 // ========================= 
 // PRINT 
 // ========================= 
 std::cout << "\n=====================================\n"; 
 std::cout <<"INTERACTION"<<endl;
 std::cout << "-------------------------------------\n"; 
 std::cout << "Transmitted : " << transmitted.size() << "\n"; 
 std::cout << "Reflected : " << reflected.size() << "\n"; 
 std::cout << "Absorbed : " << absorbed.size() << "\n"; 
 std::cout << "Total : " << transmitted.size()+reflected.size()+absorbed.size() << "\n"; 
 std::cout << "=====================================\n"; 
}
