#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include "GroundTruth.h"
#include "strawman.h"
#include "SteadySketch.h"
#include "oo.h"

using namespace std;

Item ItemVector[0x4000000] = {};
int NumofPacket = 0;
int KEY_LEN = 0xd;

int main(){

    //data file open
    ifstream Data;
    Data.open("caida.dat", ios::binary);//open dataset file --- caida 
    if(!Data.is_open()) return -1;

    //Read dataset file 
    int ts_window = 0;
    do{
        Data.read(ItemVector[NumofPacket].ItemID, KEY_LEN);
        char date[9];
        Data.read(date, 8);
        ts_window = NumofPacket/10000;
        ItemVector[NumofPacket].Window = ts_window;
        NumofPacket ++;
    }while (!Data.eof());
    // while (ItemVector[NumofPacket].Window<10);
    

    /**********  G R O U N D  T R U T H      **********/
    StandardAlgorithm  GroundTruth = {};
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < NumofPacket; i ++) GroundTruth.Insert(&ItemVector[i]);
    auto end = std::chrono::high_resolution_clock::now();
    
    cout<<NumofPacket<<" "<<GroundTruth.Instance_report.size()<<endl;

    /**********  1 B I T  R A T E    **********/
    {
        double bitrate = 0, num = 0;
        int _FilterSize = prop *  memory_size * 1024/_FilterArray;
        int _SketchSize = (_FilterSize * _FilterArray)/((_Period + 2) * _SketchArray * (prop/(1 - prop)));
        SteadySketch SteadySketch(_SketchArray,_SketchSize,_FilterSize,_FilterArray);
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NumofPacket; i++){ 
            SteadySketch.Insert(&ItemVector[i],KEY_LEN);
            if(i != NumofPacket - 1&&ItemVector[i].Window != ItemVector[i+1].Window){
                bitrate += SteadySketch.bitrate();
                num += 1;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        SteadySketch.flush();
        cout<<bitrate / num<<endl;

    }

    /**********  S T E A D Y   S K E T C H  w / o  F I L T E R  **********/
    {
        printf("sketch: steady w/o\n");
        int _FilterSize = 0;
        int _SketchSize = memory_size * 1024 / (_SketchArray * (_Period + 2));
        SteadySketch SteadySketch(_SketchArray,_SketchSize,0,0);
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NumofPacket; i++) SteadySketch.Insert(&ItemVector[i],KEY_LEN);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        SteadySketch.flush();
        cout<<SteadySketch.Instance_report.size()<<endl;

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;

            for (auto i = SteadySketch.Instance_report.begin(); i != SteadySketch.Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(make_pair(i->first, i->second.first));
                auto it = GroundTruth.Instance_report.find(make_pair(i->first, i->second.first));
                int Find = 0;
                float DX_per = 0;
                
                if (Count) {
                    Find = 1;
                    DX_per = pow((it->second - i->second.second), 2);
                }
                SUMRANGE += Find;
                DX_sum += DX_per;
            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }

            cout<<"throughput: "<<NumofPacket / (1.0 * ts_dif.count())<<endl<<"MSE: "<<DX_error_AVE<<endl;

            cout<<"group: "<<SteadySketch.Instance_report.size()<<endl<<"RR: "<< SUMRANGE * 1.0 / GroundTruth.Instance_report.size() <<endl<< "PR: "<< SUMRANGE * 1.0 / SteadySketch.Instance_report.size() <<endl;

        }

    }

    /**********  S T E A D Y   S K E T C H  w  F I L T E R  **********/
    {
        printf("sketch: steady w\n");
        int _FilterSize = prop *  memory_size * 1024/_FilterArray;
        int _SketchSize = (_FilterSize * _FilterArray)/((_Period + 2) * _SketchArray * (prop/(1 - prop)));
        SteadySketch SteadySketch(_SketchArray,_SketchSize,_FilterSize,_FilterArray);
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NumofPacket; i++) SteadySketch.Insert(&ItemVector[i],KEY_LEN);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        SteadySketch.flush();
        cout<<SteadySketch.Instance_report.size()<<endl;

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;

            for (auto i = SteadySketch.Instance_report.begin(); i != SteadySketch.Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(make_pair(i->first, i->second.first));
                auto it = GroundTruth.Instance_report.find(make_pair(i->first, i->second.first));
                int Find = 0;
                float DX_per = 0;
                
                if (Count) {
                    Find = 1;
                    DX_per = pow((it->second - i->second.second), 2);
                }
                SUMRANGE += Find;
                DX_sum += DX_per;

            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }

            cout<<"throughput: "<<NumofPacket / (1.0 * ts_dif.count())<<endl<<"MSE: "<<DX_error_AVE<<endl;

            cout<<"group: "<<SteadySketch.Instance_report.size()<<endl<<"RR: "<< SUMRANGE * 1.0 / GroundTruth.Instance_report.size() <<endl<< "PR: "<< SUMRANGE * 1.0 / SteadySketch.Instance_report.size() <<endl;

        }
    }

    /**********  O N / O F  S K E T C H  **********/
    {
        printf("sketch: on/off\n");
        int hash_num = _SketchArray;
        int hash_size = memory_size * 1024 / ((_Period + 2) * (4.125) *  hash_num);
        multimap<string, pair<int,int>> Instance_report = {};
        OO_PE<Item, uint32_t> OnOfSketch[7] = {OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size)};
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NumofPacket; i++){
            int window = ItemVector[i].Window;
            int p = window%(_Period + 2);
            if(!i||window!=ItemVector[i-1].Window)
                OnOfSketch[p].New_LargeWindow();
            OnOfSketch[p].NewWindow(window);
            // printf("%d\n",i);
            bool isnew = OnOfSketch[p].Insert(&ItemVector[i], KEY_LEN);
            int EX = 0, EX2 = 0;
            bool IgnoreThisItem = false;
            for(int j=2; j<_Period + 2;++j){
                uint32_t num = OnOfSketch[(window + j)%(_Period+2)].Query(&ItemVector[i], KEY_LEN);
                if(!num) IgnoreThisItem = true;
                EX += num;
                EX2 += num*num;
            }
            if (!IgnoreThisItem && EX2 * _Period - EX * EX < StableThreshold * _Period * _Period){
                int DX_cur = (EX2 * _Period - EX * EX);
                //LXD
                string ItemID_1 = changeID(ItemVector[i].ItemID);
                // cout<<ItemID_1<<" "<<window<<endl;
                Instance_report.insert(make_pair(ItemID_1,make_pair(window,DX_cur)));
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        // cout<<Instance_report.size()<<endl;

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;
            int sum = 0;
            map<pair<string, int>,bool>mp;
            for (auto i = Instance_report.begin(); i != Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(make_pair(i->first, i->second.first));
                auto it = GroundTruth.Instance_report.find(make_pair(i->first, i->second.first));
                int Find = 0;
                float DX_per = 0;
                if(mp[make_pair(i->first, i->second.first)]) continue;
                mp[make_pair(i->first, i->second.first)] = 1;
                sum++;
                if (Count) {
                    Find = 1;
                    DX_per = pow((it->second - i->second.second), 2);
                }
                SUMRANGE += Find;
                DX_sum += DX_per;

            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }

            cout<<"throughput: "<<NumofPacket / (1.0 * ts_dif.count())<<endl<<"MSE: "<<DX_error_AVE<<endl;

            cout<<"group: "<<sum<<endl<<"RR: "<< SUMRANGE * 1.0 / GroundTruth.Instance_report.size() <<endl<< "PR:"<< SUMRANGE * 1.0 / sum <<endl;

        }

    }

    /**********  S T R A W M A N   S O L U T I O N   **********/
    {
        printf("sketch: strawman\n");
        int Array_size = (memory_size * 1024)/(sizeof(int) * _SketchArray * (_Period + 2));
        CompareSketch Strawman(Array_size);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Strawman.Insert(&ItemVector[i], KEY_LEN, _HashSeed);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        Strawman.Duplicate();

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;
    
            for (auto i = Strawman.Instance_report.begin(); i != Strawman.Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(make_pair(i->first, i->second.first));
                auto it = GroundTruth.Instance_report.find(make_pair(i->first, i->second.first));
                int Find = 0;
                float DX_per = 0;
                
                if (Count) {
                    Find = 1;
                    DX_per = pow((it->second - i->second.second), 2);
                }
                SUMRANGE += Find;
                DX_sum += DX_per;

            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }
            
            cout<<"throughput: "<< NumofPacket / (1.0 * ts_dif.count())<<endl<<"MSE: "<<DX_error_AVE<<endl;
            cout<<"group: "<<Strawman.Instance_report.size()<<endl<<"RR: "<<SUMRANGE * 1.0 / GroundTruth.Instance_report.size()<<endl<<"PR: "<<SUMRANGE * 1.0 / Strawman.Instance_report.size()<<endl;

        }
                
    }
}