// HomeWork2 FFT Iterative and Parallel
// Name: Chaitanya Attarde SUID: 660307939
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <ctime>
#include <math.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

using namespace std;
using namespace std::chrono_literals;

const int MaxTimePart{ 30000 }, MaxTimeProduct{ 28000 }, numberOfIterations{ 5 };
const vector<int> part_Time{500,500,600,600,700},move_Time{200,200,300,300,400},assembly_time{600,600,700,700,800},BUFFER_MAX{7,6,5,5,4};

int seed = INT_MIN + int(time(nullptr)) ;
atomic_int productsDelivered;
mutex m1, m2;
condition_variable cv1, cv2;
vector<int> buffer(5,0), partIterations(100,1), productIteration(100,1);

bool operator<(vector<int> &buff1, vector<int> &buff2);
bool operator>(vector<int> &buff1, vector<int> &buff2);

void partWorker(int id, vector<int> parts);
void productWorker(int id);

vector<int> makePartOrder(vector<int> & prev_order);
vector<int> makeProductOrder( vector<int> cart);

bool load_Order(vector<int> &parts,int id);
bool pickup_Order(vector<int> &pickups, vector<int> &cart, vector<int> &local ,int id);

bool canPlace(vector<int> parts);
bool canPickup(vector<int> pickups);

int main() {
    const int m = 20, n = 16;
    vector<thread> PartW, ProductW;
    vector<int> parts = {0,0,0,0,0};

    for (int i = 0; i < m; ++i) {
        PartW.emplace_back(partWorker, i + 1,parts);
    }
    for (int i = 0; i < n; ++i) {
        ProductW.emplace_back(productWorker, i + 1);
    }
    for (auto& i : PartW) i.join();
    for (auto& i : ProductW) i.join();
    cout << endl;
    for(auto i: buffer)cout << i << " ";
    cout<<" Total products delivered: "<< productsDelivered << endl;
    cout << "Finish!" << endl;
    return 0;
}

void partWorker(int id, vector<int> parts) {
    chrono::time_point<chrono::system_clock> start = std::chrono::system_clock::now() + chrono::microseconds(MaxTimePart);
    while (partIterations[id] <= numberOfIterations) {
        if (parts == vector<int>(5, 0)) {
            makePartOrder(parts);
            start = std::chrono::system_clock::now() + chrono::microseconds(MaxTimePart);
        }
        unique_lock<mutex> UG1(m1);
        if (cv1.wait_until(UG1, start, [&parts]() { return canPlace(parts); })) {
            cout << "Iteration " << partIterations[id] << endl;
            cout << "Part Worker ID: " << id << endl;
            cout << "Buffer State : (";
            for (auto i: buffer)cout << i << ",";
            cout << ")" << endl;
            cout << "Load Order : (";
            for (auto i: parts)cout << i << ",";
            cout << ")" << endl;

            if (load_Order(parts, id)) {
                partIterations[id]++;
                cout << "Updated Buffer State : (";
                for (auto i: buffer)cout << i << ",";
                cout << ")" << endl;
                cout << "Updated Load Order : (";
                for (auto i: parts)cout << i << ",";
                cout << ")" << endl;
                cout << endl;
                cv1.notify_all();
                cv2.notify_all();
                continue;
            }
            else {
                cout << "Updated Buffer State : (";
                for (auto i: buffer)cout << i << ",";
                cout << ")" << endl;
                cout << "Updated Load Order : (";
                for (auto i: parts)cout << i << ",";
                cout << ")" << endl;
                cout << endl;
                cout << " Time for : " << id << " is " << chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start).count()<< endl;
            }
        }
        else {
            partIterations[id]++;
            makePartOrder(parts);
            cout << " Time for : " << id << " is " << chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start).count() << endl;
            //cv1.notify_all();
            //cv2.notify_all();
            //continue;
        }
        cv1.notify_all();
        cv2.notify_all();
    }

}

void productWorker(int id) {
    vector<int> local(5,0),cart(5,0),pickups(5,0);
    chrono::time_point<chrono::system_clock> start;
    while (productIteration[id] <= numberOfIterations) {
        if ((cart == vector<int>(5, 0)) && (local == vector<int>(5, 0))) {
            pickups = makeProductOrder(cart);
            start = std::chrono::system_clock::now() + chrono::microseconds(MaxTimeProduct);
        }
        unique_lock<mutex> UG1(m1);
        if (cv2.wait_until(UG1, start, [&pickups] { return canPickup(pickups); })) {
            cout << "Iteration " << productIteration[id] << endl;
            cout << "Product Worker ID: " << id << endl;
            cout << "Buffer State : (";
            for (auto i: buffer)cout << i << ",";
            cout << ")" << endl;
            cout << "Pickup Order : (";
            for (auto i: pickups)cout << i << ",";
            cout << ")" << endl;

            if (pickup_Order(pickups, cart, local , id)) {
                int total_time = 0;
                for(int i=0;i<5;i++){
                    total_time += (assembly_time[i]*cart[i]);
                    cart[i] = 0;local[i] =0;
                }
                this_thread::sleep_for(chrono::microseconds (total_time));
                productIteration[id]++;
                productsDelivered++;
                for (auto i: cart)i = 0;
                cout << "Updated Buffer State : (";
                for (auto i: buffer)cout << i << ",";
                cout << ")" << endl;
                cout << "Updated Pickup Order : (";
                for (auto i: pickups)cout << i << ",";
                cout << ")" << endl;
                cout << "Total Completed Products: " << productsDelivered << endl;
                cout << endl;
                cv1.notify_all();
                cv2.notify_all();
                continue;
            }
            else {
                cout << "Updated Buffer State : (";
                for (auto i: buffer)cout << i << ",";
                cout << ")" << endl;
                cout << "Updated Pickup Order : (";
                for (auto i: pickups)cout << i << ",";
                cout << ")" << endl;
                cout << endl;
            }
        }
        else {
            productIteration[id]++;
            for(int i=0; i<5;i++){
                if(local[i]<cart[i]){local[i]++;cart[i]--;}
            }
            pickups = makeProductOrder(local);
            cout << "Timeout Occured for product worker : " << id << endl;
            cout << "productIteration for: " << id << " is " << productIteration[id] << endl;
            //cv1.notify_all();
            //cv2.notify_all();
            //continue;
        }
        cv1.notify_all();
        cv2.notify_all();
    }

}
// Part Worker Helper functions
bool load_Order(vector<int> &parts,int id){
    int wait_time = 0;
    for(int i=0;i<5;i++){
        while(buffer[i] < BUFFER_MAX[i] && parts[i] > 0){
            buffer[i]++;
            parts[i]--;
            wait_time += move_Time[i];
        }
    }
    this_thread::sleep_for(chrono::microseconds(wait_time));
    return (parts == vector<int>(5,0));
}

// Product Worker Helper functions

bool pickup_Order(vector<int> &pickups, vector<int> &cart, vector<int> &local, int id){
    int wait_time =0;
    for(int i=0; i<5; i++){
        while(buffer[i] > 0 && pickups[i]){
            buffer[i]--;
            cart[i]++;
            pickups[i]--;
            wait_time += move_Time[i];
        }
    }
    this_thread::sleep_for(chrono::microseconds(wait_time));
    int total_parts = 0;
    for(int i=0; i<5; i++){
        total_parts += (cart[i]+local[i]);
    }
    return (total_parts == 5);
}
vector<int> makePartOrder(vector<int> & prev_order){
    int sum = 0;
    for(auto i:prev_order){sum+= i;}
    int time_already_generated_parts = prev_order[0]*part_Time[0] + prev_order[1]*part_Time[1] + prev_order[2]*part_Time[2] + prev_order[3]*part_Time[3] + prev_order[4]*part_Time[4];
    srand(seed++);
    while (sum != 6) {
        int idx = rand() % 5;
        if (sum < 6 && prev_order[idx] < 5) {
            prev_order[idx]++;
            sum++;
        }
    }
    this_thread::sleep_for(chrono::microseconds((prev_order[0]*part_Time[0] + prev_order[1]*part_Time[1] + prev_order[2]*part_Time[2] + prev_order[3]*part_Time[3] + prev_order[4]*part_Time[4]) - time_already_generated_parts));
    return prev_order;
}

vector<int> makeProductOrder(vector<int> cart){
    vector<int> pickup(5,0);
    auto hasFive = [](const std::vector<int>& v) {
        return std::find(v.begin(), v.end(), 5) != v.end();
    };
    int sum =0;
    set<int> check_id;
    if(cart != vector<int>(5,0)){
        for(int i=0; i<5;i++){
            if(cart[i] > 0){
                sum += cart[i];
                check_id.insert(i);
            }
        }
    }
    srand(seed++);
    int set_size = rand()%2 + 2;
    while(true){
        while (sum != 5) {
            int idx = rand() % 5;
            if (sum < 5 && ((pickup[idx] + cart[idx]) < 5) ) {
                if(check_id.size() < set_size)check_id.insert(idx);
                if((check_id.find(idx) != check_id.end())){
                    pickup[idx]++;
                    sum++;
                }
            }
        }
        if(!hasFive(pickup))break;
    }
    return pickup;
}
bool canPlace(vector<int> parts){
    for(int i=0 ; i< 5; i++){
        if(BUFFER_MAX[i] - buffer[i] >= parts[i] && parts[i] != 0)return true;
    }
    return false;
}
bool canPickup(vector<int> pickups){
    for(int i=0; i<5; i++){
        if(buffer[i] > 0 && pickups[i] > 0)return true;
    }
    return false;
}
bool operator<(vector<int> &buff1, vector<int> &buff2){
    for(int i=0;i<buff1.size();i++){
        if(buff1[i] < buff2[i]){
            return true;
        }
    }
    return false;
}
bool operator>(vector<int> &buff1, vector<int> &buff2){
    for(int i=0;i<buff1.size();i++){
        if(buff1[i] > buff2[i]){
            return true;
        }
    }
    return false;
}
