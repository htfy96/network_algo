#include "graphdsl.hpp"
#include "backend/leveldbgraph.hpp"
#include<cstdio>
#include<iostream>
#include <fstream>
#include<cstdlib>
#include<algorithm>
#include<vector>
#include<queue>
#include<cstring>
#include<string>
#include<map>
#include<cmath>
#include<stack>
#include<set>

#include "tflabel.pb.h"

namespace netalgo
{


    using namespace std;

    template<typename GraphT>
        class TFLabel
        {
            public:
                typedef TFLabelNode Node;
                typedef TFLabelEdge Edge;
            private:
                static const size_t maxn = 3e6;
                static const size_t maxN = 8e6;
                static const size_t maxm = 10700000;
                set<int> *label_in, *label_out;
                int* pre_to_new_NUM;
                int* tln;
                int tot;
                int TF;
                LevelDbGraph<Node, Edge> graph;
                GraphT& originGraph;
                vector<int> *G;
                vector<int> *GG;
                int *pre, *lowlink, *sccno, dfs_clock, scc_cnt;
            protected:
                void dfs(int u);
                void find_scc(int n);
                int findfa(const string &s);
                void toposort();
                void construct();
            public:
                TFLabel(GraphT &g): originGraph(g), graph("tflabel.db")
                {
                    graph.destroy();
                    pre_to_new_NUM = new int [maxN];
                    tln = new int[maxn];
                    pre = new int[maxn];
                    lowlink = new int[maxn];
                    sccno = new int[maxn];
                    label_in = new set<int>[maxn * 4];
                    label_out = new set<int> [maxn * 4];
                    G = new vector<int>[maxn];
                    GG = new vector<int>[maxn];
                    memset(tln, 0, sizeof(int)* maxn);
                    memset(pre, 0, sizeof(int) * maxn);
                }
                TFLabel(const TFLabel&) = default;
                TFLabel& operator=(const TFLabel&) = default;
                TFLabel(TFLabel&&) = default;
                TFLabel& operator=(TFLabel&&) = default;
                ~TFLabel()
                {
                    delete [] pre_to_new_NUM;
                    delete[] tln;
                    delete[] pre;
                    delete[] lowlink;
                    delete[] sccno;
                    delete[] label_in;
                    delete[] label_out;
                    delete[] G;
                    delete[] GG;
                }

                void read();
                void preprocess();
                //TODO bool query(int id1, int id2);
                void printDebugMessage()
                {
                    for (size_t i=0; i<8; ++i)
                    {
                        cout << "label_in"<<i<<endl;
                        for (const auto& item : label_in[i])
                            cout << item <<" ";
                        cout << endl;
                    }
                    for (size_t i=0; i<8; ++i)
                    {
                        cout << "label_out"<<i<<endl;
                        for (const auto& item : label_out[i])
                            cout << item <<" ";
                        cout << endl;
                    }
                }
        };

    template<typename GraphT>
        void TFLabel<GraphT>::preprocess()
        {
            toposort();
            construct();
        }
    //ieeepaper
    //最大编号：7102531 最小编号：15 总边数：10600587
    //总结点数：22309488
    //int fa[maxn*4];
    //***************************************tarjan算法求强连通分量********************************//
    template<typename GraphT>
        void TFLabel<GraphT>::dfs(int u)  
        {  
            static stack<int> s;
            pre[u] = lowlink[u] = ++dfs_clock;  
            s.push(u);  
            for(size_t i=0; i<GG[u].size(); i++)  
            {  
                int v=GG[u][i];  
                if(!pre[v])  
                {  
                    dfs(v);  
                    lowlink[u]=min(lowlink[u],lowlink[v]);  
                }  
                else if(!sccno[v])  
                {  
                    lowlink[u]=min(lowlink[u],pre[v]);  
                }  
            }  
            if(lowlink[u]==pre[u])  
            {  
                scc_cnt++;  
                for(;;)  
                {  
                    int x=s.top();  
                    s.pop();  
                    sccno[x]=scc_cnt;  
                    if(x==u)  
                        break;  
                }  
            } 
        }  

    template<typename GraphT>
        void TFLabel<GraphT>::find_scc(int n)  
        {  
            dfs_clock=scc_cnt=0;  
            memset(sccno,0, sizeof(int)*maxn);   
            for(int i=0; i<n; i++)  
                if(!pre[i]) dfs(i);  
        } 
    //***************************************tarjan算法求强连通分量********************************//

    template<typename GraphT>
        void TFLabel<GraphT>::read(){
            //freopen("data.txt","r",stdin);
            ifstream is("data.txt");
            cout<<"start"<<endl; 
            int x,y = 0;
            vector<pair<int,int> >E;
            vector<int>v;
            int same = 0; 
            while (is >> x >> y){
                //if (x == y && x < 1000)cout<<x<<endl;
                cout << "Read "<<x <<" "<<y << endl;
                if (x == y)same++;
                E.push_back(make_pair(x,y));
                v.push_back(x);
                v.push_back(y);
            }
            is.close();
            cout<<"same:"<<same<<endl;
            sort(v.begin(),v.end());
            v.erase(unique(v.begin(), v.end()),v.end());
            tot = v.size();

            cout<<v[0]<<' '<<v[tot-1]<<endl;
            for (int i = 0; i < tot; ++i)
                pre_to_new_NUM[v[i]] = i;
            for (size_t i = 0; i < E.size(); ++i)
            {
                int xx = pre_to_new_NUM[E[i].first];
                int yy = pre_to_new_NUM[E[i].second];
                GG[xx].push_back(yy);
            }
            cout<<"start tarjan"<<endl;
            find_scc(tot);//tarjan
            cout<<"end tarjan"<<endl;	
            vector<Edge> edgeVec;
            vector<Node> nodeVec;
            for (int i = 0; i < tot; ++i){
                Node x;
                x.set_id(to_string(i));
                x.set_level(1);
                nodeVec.push_back(x);
                cout<<"#####"<<i<<endl;

            }
            graph.setNodesBundle(nodeVec);
            int cnt = 0;
            for (auto it = graph.query("select (a) return a"_graphsql);
                        it!=graph.end();
                        ++it)
            {
                ++cnt;
                cout <<cnt << it->getNode("a").id() << endl;
            }


            for (int i = 0; i < tot; ++i)
                for (size_t j = 0; j < GG[i].size(); ++j){
                    int x = sccno[i] - 1;
                    int y = sccno[GG[i][j]] - 1;
                    if (x != y){
                        G[x].push_back(y);
                        Edge nm;
                        nm.set_id(to_string(x)+"-"+to_string(y));
                        nm.set_from(to_string(x));
                        nm.set_to(to_string(y));
                        edgeVec.push_back(nm);
                    }
                }
            graph.setEdgesBundle(edgeVec);
            tot = scc_cnt;
            cout<<"tot="<<tot<<endl;


        }

    template<typename GraphT>
        void TFLabel<GraphT>::toposort(){
            static int deg[maxn]={0};
            int cnt = 0;
            for (int i = 0; i < tot; ++i)
                for (size_t j = 0; j < G[i].size(); ++j){
                    deg[G[i][j]]++;
                    ++cnt;
                } 
            cout<<cnt<<endl;
            cnt = 0;
            for (int i = 0; i < tot; ++i)
            {
                if (deg[i] != 0)cnt++;
            }
            printf("%d\n",cnt);
            queue<int>q;
            int sum = 0;
            for (int i = 0; i < tot; ++i)
            {
                if (deg[i] == 0){
                    q.push(i);
                    tln[i] = 1;
                    ++sum;
                }
            }
            int maxx = 0;
            cnt = 0;
            while (!q.empty()){
                ++cnt;
                int x = q.front();
                q.pop();
                for (size_t i = 0; i < G[x].size(); ++i)
                {
                    int y = G[x][i];
                    --deg[y];
                    tln[y] = max(tln[y], tln[x] + 1);
                    maxx = max(tln[y], maxx);
                    if (deg[y] == 0){
                        q.push(y);
                        sum += tln[y];
                    }
                }
            }
            TF = (int)(log(maxx)/log(2)) + 1;
            cout<<cnt<<endl;
            cout<<"average:"<<sum*1.0/tot<<endl;
            cout<<"maxx:"<<maxx<<endl;
            cnt = 0;
            for (int i = 0; i < tot; ++i)
            {
                if (deg[i] > 0)cnt++;
            }
            printf("%d\n",cnt);
        }


    template<typename GraphT>
        int TFLabel<GraphT>::findfa(const string &s){
            int x = 0;
            //cout<<"##"<<endl;
            int i = 0;
            int len = s.length();
            while (i < len){
                if (s[i] >= '0' && s[i] <= '9')
                {
                    x *= 10;
                    x += s[i] - '0';
                }else break;
                ++i;
            }
            //cout<<x<<endl;
            return x;	
        } 

    template<typename GraphT>
        void TFLabel<GraphT>::construct(){	
            {
                auto start = graph.query("select (a) return a"_graphsql); // return an iterator
                int fuck = 0;
                for (auto it=start; it!=graph.end(); ++it) //iterates over result set
                {
                    Node a = it->getNode("a");
                    cout<<fuck++<<a.id()<<endl;
                    a.set_level(tln[atoi(a.id().c_str())]);
                    graph.setNode(a);
                }
            }
            cout<<TF<<endl;
            for (int ii = 1; ii <= TF; ++ii){
                //a***********************************************************a
                {
                    auto start = graph.query("select (a) return a"_graphsql); // return an iterator
                    for (auto it=start; it!=graph.end(); ++it) //iterates over result set
                    {
                        Node a = it->getNode("a");
                        cout<<a.id()<<endl;
                        int curl = a.level();
                        if (curl % 2 == 0)continue;
                        set<string> outEdges = graph.getOutEdge(a.id());
                        vector<string> edge_remove;
                        bool flag = 0;
                        Node n;
                        for (auto &e: outEdges)
                        {
                            Node m = graph.getNode(graph.getEdge(e).to());
                            if (m.level() > curl + 1)
                            {
                                if (!flag){
                                    flag = 1;
                                    n = a;
                                    string name = a.id() + "#";
                                    n.set_id(name);
                                    n.set_level(a.level() + 1);
                                    graph.setNode(n);		        		
                                }
                                edge_remove.push_back(e);
                                Edge nm;
                                nm.set_id(a.id()+"-"+n.id());
                                nm.set_from(a.id());
                                nm.set_to(n.id());
                                graph.setEdge(nm);

                                nm.set_id(n.id()+"-"+m.id());
                                nm.set_from(n.id());
                                nm.set_to(m.id());
                                graph.setEdge(nm);
                            }
                        }
                        for (size_t i = 0; i < edge_remove.size(); ++i)
                            graph.removeEdge(edge_remove[i]);
                    }
                }
                //b**********************************************************************b
                {
                    auto start = graph.query("select (a) return a"_graphsql); // return an iterator
                    for (auto it=start; it!=graph.end(); ++it) //iterates over result set
                    {
                        Node a = it->getNode("a");
                        int curl = a.level();
                        if (curl % 2 == 0)continue;
                        set<string> inEdges = graph.getInEdge(a.id());
                        vector<string> edge_remove;
                        bool flag = 0;
                        Node n;
                        for (auto &e: inEdges)
                        {
                            Node m = graph.getNode(graph.getEdge(e).from());
                            int tmp = m.level();
                            if (tmp < curl - 1 && tmp % 2 == 0)
                            {
                                if (!flag){
                                    flag = 1;
                                    n = a;
                                    string name = a.id() + "#";
                                    n.set_id(name);
                                    n.set_level(a.level() - 1);
                                    graph.setNode(n);		        		
                                }
                                edge_remove.push_back(e);
                                Edge nm;
                                nm.set_id(n.id() + "-" + a.id());
                                nm.set_from(n.id());
                                nm.set_to(a.id());
                                graph.setEdge(nm);

                                nm.set_id(m.id() + "-" + n.id());
                                nm.set_from(m.id());
                                nm.set_to(n.id());
                                graph.setEdge(nm);
                            }
                        }
                        for (size_t i = 0; i < edge_remove.size(); ++i)
                            graph.removeEdge(edge_remove[i]);
                    }
                }
                //c*****************************************************************c
                {
                    auto start = graph.query("select (a) return a"_graphsql); // return an iterator
                    for (auto it=start; it!=graph.end();) //iterates over result set
                    {
                        Node a = it->getNode("a");
                        int curl = a.level();
                        if (curl % 2 == 0){ ++it; continue; }
                        set<string> inEdges = graph.getInEdge(a.id());			
                        set<string> outEdges = graph.getOutEdge(a.id());
                        set<string> inNode;
                        set<string> outNode;
                        int x = findfa(a.id());
                        for (auto &e: outEdges)
                        {
                            string to = graph.getEdge(e).to();
                            outNode.insert(to);
                            label_in[x].insert(findfa(to));		
                        }
                        for (auto &e: inEdges)
                        {
                            string from = graph.getEdge(e).from();
                            inNode.insert(from);			
                            label_out[x].insert(findfa(from));
                        }
                        //��label 
                        vector<Edge>edgeVec;
                        for (auto &to: outNode){
                            for (auto &from: inNode){
                                Edge nm;
                                nm.set_id(from+"-"+to);
                                nm.set_from(from);
                                nm.set_to(to);
                                edgeVec.push_back(nm);					
                            }
                        }
                        graph.setEdgesBundle(edgeVec);
                        ++it;

                        graph.removeNode(a.id());						
                        std::cout << "removed" << a.id() << std::endl;
                    }    
                }
            }	
        }
}

