#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include "GroundTruth.h"
#include "Strawman.h"
#include "SteadySketch.h"
#include "oo.h"
#define BeginTime   (0x5AAA6E50)
using namespace std;

Item ItemVector[0x4000000] = {};
int NumofPacket = 0;
int KEY_LEN = 0xd;

int main(){

    ifstream Data;
    Data.open("caida.dat", ios::binary);//open dataset file --- caida path
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


    /**********  G R O U N D  T R U T H      **********/
    StandardAlgorithm  GroundTruth = {};
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < NumofPacket; i ++){ 
        GroundTruth.Insert(&ItemVector[i]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli>tm = end - start;
    GroundTruth.Flush();
    GroundTruth.ReportValue(NumofPacket/(1.0 * tm.count()));
    
    cout<<NumofPacket<<" "<<GroundTruth.TopKReport.size()<<endl;

    /********** S T E A D Y   S K E T C H  ********************/
    {
        printf("sketch: steady\n");
        int _FilterSize = prop * memory_size * 1024 / _FilterArray;
        int _SketchSize = (_FilterSize * _FilterArray)/((_Period + 2) * _SketchArray * (prop/(1-prop)));

        SteadySketch Steady(_SketchArray, _SketchSize, _FilterSize,_FilterArray);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Steady.Insert(&ItemVector[i],KEY_LEN);
        Steady.Flush();
        auto  end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli>tm = end - start;
                
        Steady.ReportValue(NumofPacket / (1.0 * tm.count()));
        double ARE1 = 0, ARE2 = 0, tmp = 0, num = 0;
        string last_first = "";
        {   //Evaluate
            int SUMRANGE = 0;
            for (auto i = Steady.TopKReport.begin(); i != Steady.TopKReport.end(); i++) {
                auto Count = GroundTruth.TopKReport.count(i->first);
                auto it = GroundTruth.TopKReport.find(i->first);
                int Find = 0;
                while (Count) {
                    if (it->second == i->second){//
                        Find = 1;
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;

            }
            for (auto i = GroundTruth.TopKReport.begin(); ; i++) {
                if(tmp && (i == GroundTruth.TopKReport.end() || last_first != i->first)){
                    auto Count_ = Steady.TopKReport.count(last_first);
                    auto it_ = Steady.TopKReport.find(last_first);
                    double base = 0;
                    while (Count_--) {
                        base += it_->second.first;
                        it_++;
                    }
                    // printf("%lf %lf\n",tmp, base);
                    ARE2 += fabs(tmp-base)/tmp;
                    num += 1;
                    tmp = 0;
                }
                if(i == GroundTruth.TopKReport.end()) break;
                auto Count = Steady.TopKReport.count(i->first);
                auto it = Steady.TopKReport.find(i->first);
                int Find = 0;
                int Min = 1e9, Min_delta = i->second.first;
                last_first = i->first;
                tmp += i->second.first;
                while (Count) {
                    if(abs(it->second.second.first - i->second.second.first) < Min){
                        Min = abs(it->second.second.first - i->second.second.first);
                        Min_delta = abs(it->second.first - i->second.first);
                    }
                    it++;
                    Count--;
                }
                ARE1 += 1.0*Min_delta / i->second.first;

            }
            cout<<endl<<"ARE1 : "<<ARE1 / GroundTruth.TopKReport.size()<<endl;
            cout<<"ARE2 : "<<ARE2 / (num - 1)<<endl;
            cout<< "RR :"<<SUMRANGE * 1.0 / GroundTruth.TopKReport.size()<<endl<<"PR :"<<SUMRANGE * 1.0 / Steady.TopKReport.size()<<endl;

        
        }
                

    }
    /********** O N / O F F   S K E T C H  ********************/
    {
        printf("sketch: on_off\n");
        int mem = memory_size*1024 - _NumOfHeavyHitterBuckets * ElementPerBucket * sizeof(StableElement);
        int hash_num = 2;
        int hash_size = mem / ((_Period + 2) * (4.125) *  hash_num);
        // cerr<<hash_size<<endl;
        multimap<string, pair<int,int>> Instance_report = {};
        OO_PE<Item, uint32_t> OnOfSketch[7] = {OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size),OO_PE<Item, uint32_t>(hash_num, hash_size)};
        StableElement** HeavyHitter = new StableElement * [_NumOfHeavyHitterBuckets];
        int DX_pre = 0;
        int DX_cur = 0;
        int stable_len = 0;
        vector<pair<char*,  pair<int, int>>> ReportBuffer = {};		
        //To report the smooth item. ReportBuffer contains the smooth items.
        multimap<string, pair<int, pair<int, int>>> TopKReport = {};
        for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
            HeavyHitter[i] = new StableElement[ElementPerBucket]();
        }
        auto start = std::chrono::high_resolution_clock::now();
        for(int I = 0; I < NumofPacket; I++){
            bool returned = 0;
            Item* e = &ItemVector[I];
            int window = e->Window;
            int p = window%(_Period + 2);
            if(!I||window!=ItemVector[I-1].Window)
                OnOfSketch[p].New_LargeWindow();
            OnOfSketch[p].NewWindow(window);
            // printf("%d\n",i);
            bool isnew = OnOfSketch[p].Insert(e, KEY_LEN);
            int EX = 0, EX2 = 0;
            bool IgnoreThisItem = false;
            for(int j=2; j<_Period + 2;++j){
                uint32_t num = OnOfSketch[(window + j)%(_Period+2)].Query(e, KEY_LEN);
                if(!num) IgnoreThisItem = true;
                EX += num;
                EX2 += num*num;
            }
            if (!IgnoreThisItem && EX2 * _Period - EX * EX < StableThreshold * _Period * _Period){
                int DX_cur = (EX2 * _Period - EX * EX);
                //LXD

                unsigned HashBucket = MurmurHash64B((const void*)e->ItemID, KEY_LEN, 100) % _NumOfHeavyHitterBuckets;

                int LeastSmoothIndex = 0, LeastSmoothTime = INT_MAX;	//If the bucket is full, we should kick the least smooth item with certain probability.

                for (int i = 0; i < ElementPerBucket && !returned; i++) {

                    StableElement* e_ = &HeavyHitter[HashBucket][i];

                    if (e_->ItemID == NULL) {	//Case 1: If it is an empty slot. Insert the coming item here.
                        e_->StartStableTime = e->Window;
                        e_->RecentStableTime = e->Window + 1;
                        e_->ItemID = e->ItemID;
                        //LXD
                        returned = true;
                    }
                    if (!returned && e_->RecentStableTime < e->Window) {		//Case 2: If the stable element is interrupted, we can safely replace it.


                        if (e_->RecentStableTime - e_->StartStableTime > SmoothThreshold){
                            //LXD
                            stable_len = e_->RecentStableTime - e_->StartStableTime;
                            //e_->DX_sum = e_->DX_sum/stable_len;
                            ReportBuffer.push_back(make_pair(e_->ItemID, make_pair(e_->StartStableTime, e_->RecentStableTime))); 	//If the item is stable for more than SmoothThreshold
                        }																							//times continuously, we will report this value.

                        e_->StartStableTime = e->Window;
                        e_->RecentStableTime = e->Window + 1;

                        e_->ItemID = e->ItemID;//Replace.

                        returned = true;
                    }

                    if (!returned && memcmp(e_->ItemID, e->ItemID, KEY_LEN) == 0) {		//Case 3: If it is an existing item, we increase the corresponding RecentStableTime.

                        e_->RecentStableTime = e->Window + 1;

                        returned = true;
                    }

                    if (!returned && e_->RecentStableTime - e_->StartStableTime < LeastSmoothTime) {	//Case 4: If Case 1 to 3 are not satisfied. We should find the least smooth item.
                                                                                        //and replace it with certaion probability.

                        LeastSmoothTime = e_->RecentStableTime - e_->StartStableTime;

                        LeastSmoothIndex = i;
                    }
                }

                if(!returned){
                    HeavyHitter[HashBucket][LeastSmoothIndex].RecentStableTime = e->Window + 1;		//No matter the original is kicked or not,
                                                                                                    //we increase the RecentStableTime by 1.

                    int Random = rand();

                    if ((LeastSmoothTime + 1) * Random <= RAND_MAX)HeavyHitter[HashBucket][LeastSmoothIndex].ItemID = e->ItemID;
                }
            }
        }
        {
            for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
                for (int j = 0; j < ElementPerBucket; j++) {
                    if (HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime > SmoothThreshold)
                    {

                        stable_len = HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime;
                        ReportBuffer.push_back(make_pair(HeavyHitter[i][j].ItemID, 
                            make_pair(HeavyHitter[i][j].StartStableTime, HeavyHitter[i][j].RecentStableTime)));
                    }
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        {		//Report the smooth items.
            sort(ReportBuffer.begin(), ReportBuffer.end(), compare);
            int k = 0;
            int stablelength = 0;
            for (auto i = ReportBuffer.begin(); i != ReportBuffer.end(); i++) {
                string ItemID_2 = changeID(i->first);
                TopKReport.insert(make_pair(ItemID_2, make_pair(i->second.second - i->second.first,
                    make_pair(i->second.first, i->second.second))));//
                if (k >= TopK - 1 && stablelength != i->second.second - i->second.first)break;//
                k++, stablelength = i->second.second - i->second.first;//

            }

            printf("throughput: %lf Kibs.", NumofPacket / (1.0 * tm.count()));
        }
        
        {   //Evaluate
            int SUMRANGE = 0;
            double ARE1 = 0, ARE2 = 0, tmp = 0, num = 0;
            string last_first = "";
            for (auto i = TopKReport.begin(); i != TopKReport.end(); i++) {
                auto Count = GroundTruth.TopKReport.count(i->first);
                auto it = GroundTruth.TopKReport.find(i->first);
                int Find = 0;
                while (Count) {
                    if (it->second == i->second){//
                        Find = 1;
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;

            }
            for (auto i = GroundTruth.TopKReport.begin(); ; i++) {
                if(tmp && (i == GroundTruth.TopKReport.end() || last_first != i->first)){
                    auto Count_ = TopKReport.count(last_first);
                    auto it_ = TopKReport.find(last_first);
                    double base = 0;
                    while (Count_--) {
                        base += it_->second.first;
                        it_++;
                    }
                    ARE2 += fabs(tmp-base)/tmp;
                    num += 1;
                    tmp = 0;
                }
                if(i == GroundTruth.TopKReport.end()) break;
                auto Count = TopKReport.count(i->first);
                auto it = TopKReport.find(i->first);
                int Find = 0;
                int Min = 1e9, Min_delta = i->second.first;
                last_first = i->first;
                tmp += i->second.first;
                while (Count) {
                    if(abs(it->second.second.first - i->second.second.first) < Min){
                        Min = abs(it->second.second.first - i->second.second.first);
                        Min_delta = abs(it->second.first - i->second.first);
                    }
                    it++;
                    Count--;
                }
                ARE1 += 1.0*Min_delta / i->second.first;

            }
            cout<<endl<<"ARE1 : "<<ARE1 / GroundTruth.TopKReport.size()<<endl;
            cout<<"ARE2 : "<<ARE2 / num<<endl;
            cout<< "RR :"<<SUMRANGE * 1.0 / GroundTruth.TopKReport.size()<<endl<<"PR :"<<SUMRANGE * 1.0 / TopKReport.size()<<endl;

        
        }


    }

    /**********  S T R A W M A N   S O L U T I O N      **********/
    {
        printf("sketch: strawman\n");
        int _SketchSize = (1024 * memory_size)/(sizeof(int) * _SketchArray * (_Period + 2));
        CompareSketch Strawman(_SketchSize);
        //start_t = clock();
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Strawman.Insert(&ItemVector[i],KEY_LEN);
        //end_t = clock();
        Strawman.Flush();
        auto  end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli>tm = end - start;                    
        Strawman.ReportValue(NumofPacket / (1.0 * tm.count()));
    
        {   //Evaluate
            int SUMRANGE = 0;
            double ARE1 = 0, ARE2 = 0, tmp = 0, num = 0;
            string last_first = "";
            int  sameid_count_CM = 0;
            int  sameid_count_Ground = 0;
            for (auto i = Strawman.TopKReport.begin(); i != Strawman.TopKReport.end(); i++) {
                auto Count = GroundTruth.TopKReport.count(i->first);
                auto it = GroundTruth.TopKReport.find(i->first);
                int Find = 0;
                while (Count) {
                    if (it->second == i->second){//
                        Find = 1;
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;

            }
            for (auto i = GroundTruth.TopKReport.begin(); ; i++) {
                if(tmp && (i == GroundTruth.TopKReport.end() || last_first != i->first)){
                    auto Count_ = Strawman.TopKReport.count(last_first);
                    auto it_ = Strawman.TopKReport.find(last_first);
                    double base = 0;
                    while (Count_--) {
                        base += it_->second.first;
                        it_++;
                    }
                    ARE2 += fabs(tmp-base)/tmp;
                    num += 1;
                    tmp = 0;
                }
                if(i == GroundTruth.TopKReport.end()) break;
                auto Count = Strawman.TopKReport.count(i->first);
                auto it = Strawman.TopKReport.find(i->first);
                int Find = 0;
                int Min = 1e9, Min_delta = i->second.first;
                last_first = i->first;
                tmp += i->second.first;
                while (Count) {
                    if(abs(it->second.second.first - i->second.second.first) < Min){
                        Min = abs(it->second.second.first - i->second.second.first);
                        Min_delta = abs(it->second.first - i->second.first);
                    }
                    it++;
                    Count--;
                }
                ARE1 += 1.0*Min_delta / i->second.first;

            }
            cout<<endl<<"ARE1 : "<<ARE1 / GroundTruth.TopKReport.size()<<endl;
            cout<<"ARE2 : "<<ARE2 / (num - 1)<<endl;
            cout<<"RR : "<<SUMRANGE * 1.0 / GroundTruth.TopKReport.size()<<endl<<"PR : "<<SUMRANGE * 1.0 / Strawman.TopKReport.size()<<endl;
        }
    }
}