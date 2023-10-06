#include <bits/stdc++.h>
using namespace std;
#define pb push_back
#define vint vector<int>
#define vvint vector<vector<int>>
template<typename T> ostream& operator<<(ostream& os, vector<T>& v) { for (auto& it : v) os<<it<<' '; return os; }
template<typename T> void print(T a) { cout<<a<<'\n'; }
template<typename T, typename... Args> void print(T a, Args... b) { cout<<a<<' '; print(b...); } 
vector<int> scan(int n){ vector<int> v; int p; while(n--){ cin>>p; v.push_back(p); } return v; }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void solution(){
}

int main(){
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int tc=1; cin>>tc;
    while(tc--) solution();
    return 0;
}