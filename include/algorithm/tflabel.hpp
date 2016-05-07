#include "debug.hpp"
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
                int* fa;
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
                    fa = new int[maxn];
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
                        LOGGER(info, "label_in[{}] = ", i);
                        for (const auto& item : label_in[i])
                            LOGGER(info, "item={}", item);
                    }
                    for (size_t i=0; i<8; ++i)
                    {
                        LOGGER(info, "label_out[{}] = ", i);
                        for (const auto& item : label_out[i])
                            LOGGER(info, "item={}", item);
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
    //æœ€å¤§ç¼–å·ï¼š7102531 æœ€å°ç¼–å·ï¼š15 æ€»è¾¹æ•°ï¼š10600587
    //æ€»ç»“ç‚¹æ•°ï¼š22309488
    //int fa[maxn*4];
    //***************************************tarjanç®—æ³•æ±‚å¼ºè¿žé€šåˆ†é‡********************************//
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
    //***************************************tarjanç®—æ³•æ±‚å¼ºè¿žé€šåˆ†é‡********************************//

    template<typename GraphT>
        void TFLabel<GraphT>::read(){
            //freopen("data.txt","r",stdin);
            ifstream is("data.txt");
            LOGGER(debug, "start");
            int x,y = 0;
            vector<pair<int,int> >E;
            vector<int>v;
            int same = 0; 
            while (is >> x >> y){
                //if (x == y && x < 1000)cout<<x<<endl;
                LOGGER(debug, "Read {} {}", x, y);
                if (x == y)same++;
                E.push_back(make_pair(x,y));
                v.push_back(x);
                v.push_back(y);
            }
            is.close();
            LOGGER(debug, "same:{}", same);
            sort(v.begin(),v.end());
            v.erase(unique(v.begin(), v.end()),v.end());
            tot = v.size();

            LOGGER(debug, "first node {}, last node {}", v[0], v[tot-1]);
            for (int i = 0; i < tot; ++i)
                pre_to_new_NUM[v[i]] = i;
            for (size_t i = 0; i < E.size(); ++i)
            {
                int xx = pre_to_new_NUM[E[i].first];
                int yy = pre_to_new_NUM[E[i].second];
                GG[xx].push_back(yy);
            }
            LOGGER(debug, "Start Tarjan");
            find_scc(tot);//tarjan
            LOGGER(debug, "End Tarjan");
            vector<Edge> edgeVec;
            vector<Node> nodeVec;
            for (int i = 0; i < tot; ++i){
                Node x;
                x.set_id(to_string(sccno[i]-1));
                x.set_level(1);
                nodeVec.push_back(x);
                LOGGER(debug, "#####{}", i);
            }
            graph.setNodesBundle(nodeVec);
            int cnt = 0;
            for (auto it = graph.query("select (a) return a"_graphsql);
                        it!=graph.end();
                        ++it)
            {
                ++cnt;
                LOGGER(debug, "{}.  {} is in graph", cnt, it->getNode("a").id());
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
            for (int i = 0; i < tot; ++i){
                fa[i] = i;
            }
            LOGGER(debug, "Tot={}", tot);
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
            LOGGER(debug, "All edges: {}", cnt);
            cnt = 0;
            for (int i = 0; i < tot; ++i)
            {
                if (deg[i] != 0)cnt++;
            }
            LOGGER(debug, "CNT = {}", cnt);
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
            LOGGER(debug, "Count={}", cnt);
            LOGGER(debug, "Average={}", sum*1.0/ tot);
            LOGGER(debug, "MaxX={}", maxx);
            cnt = 0;
            for (int i = 0; i < tot; ++i)
            {
                if (deg[i] > 0)cnt++;
            }
            LOGGER(debug, "Cnt = {}", cnt);
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
                    a.set_level(tln[atoi(a.id().c_str())]);
                    LOGGER(debug, "{}. Fuck {} tln={}", fuck++, a.id(), tln[atoi(a.id().c_str())]);
                    graph.setNode(a);
                }
            }

            LOGGER(debug, "The outedges of #7");
            for (const auto &item : graph.getOutEdge("7"))
                LOGGER(debug, "    {}", item);
            LOGGER(debug, "TF= {}", TF);
            for (int i = 0; i < tot; ++i){
                label_in[i].insert(i);
                label_out[i].insert(i);
            }
            vector<int>order;
            for (int ii = 1; ii <= TF; ++ii){
                //a***********************************************************a
                {
                    auto start = graph.query("select (a) return a"_graphsql); // return an iterator
                    for (auto it=start; it!=graph.end(); ++it) //iterates over result set
                    {
                        LOGGER(debug, "Start iterating");
                        Node a = it->getNode("a");
                        int x = findfa(a.id());
                        LOGGER(debug, "Id= {}", a.id());
                        int curl = a.level();
                        if (curl % 2 == 0)continue;
                        set<string> outEdges = graph.getOutEdge(a.id());
                        vector<string> edge_remove;
                        bool flag = 0;
                        Node n;
                        for (auto &e: outEdges)
                        {
                            LOGGER(debug, "The Edge id is {}", e);
                            Node m = graph.getNode(graph.getEdge(e).to());
                            if (m.level() > curl + 1)
                            {
                                if (!flag){
                                    flag = 1;
                                    n = a;
                                    fa[tot] = fa[x];
                                    string name = to_string(tot++);
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
                        int x = findfa(a.id());
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
                                    fa[tot] = fa[x];
                                    string name = to_string(tot++);
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
                        LOGGER(debug, "Curl = {}   a.id={}", curl, a.id());
                        if (curl % 2 == 0){ a.set_level(curl / 2); graph.setNode(a); ++it; continue; }
                        set<string> inEdges = graph.getInEdge(a.id());			
                        set<string> outEdges = graph.getOutEdge(a.id());
                        set<string> inNode;
                        set<string> outNode;
                        int x = findfa(a.id());
                        LOGGER(debug, "x={}", x);
                       // label_in[x].insert(x);
                       // label_out[x].insert(x);
                        for (auto &e: outEdges)
                        {
                            string to = graph.getEdge(e).to();
                            outNode.insert(to);
                            int y = findfa(to);
                            for (auto &ele: label_in[y])
                            label_in[x].insert(ele);		
                        }
                        for (auto &e: inEdges)
                        {
                            string from = graph.getEdge(e).from();
                            inNode.insert(from);	
                            int y = findfa(from);
                            for (auto &ele: label_out[y])
                            label_out[x].insert(ele);
                        }
                        //¼Ólabel 
                        vector<Edge>edgeVec;
                        for (auto &to: outNode){
                            for (auto &from: inNode){
                                Edge nm;
                                nm.set_id(from+"-"+to);
                                nm.set_from(from);
                                nm.set_to(to);
                                LOGGER(debug, "Add edge from {} to {}", from, to);
                                edgeVec.push_back(nm);					
                            }
                        }
                        graph.setEdgesBundle(edgeVec);
                        ++it;
                        order.push_back(findfa(a.id()));
                        graph.removeNode(a.id());						
                        LOGGER(debug, "ii= {}", ii);
                        LOGGER(debug, "{} was removed", a.id());
                    }    
                }
                {
                    for (int i = order.size() - 1; i >= 0; --i)
                    {
                        set<int>tmp = label_in[i];
                        for (auto &ele: tmp)
                        {
                            for (auto &e: label_in[ele])
                            label_in[i].insert(e);
                        }
                        tmp = label_out[i];
                        for (auto &ele: tmp)
                        {
                            for (auto &e: label_out[ele])
                            label_out[i].insert(e);
                        }
                    }
                }
                {
                    for (int i = 0; i < scc_cnt; ++i)
                    {
                        set<int>tmp;
                        swap(tmp,label_in[i]);
                        for (auto &ele:tmp){
                            label_in[i].insert(fa[ele]);
                        }
                        set<int>tmp2;
                        swap(tmp2,label_out[i]);
                        for (auto &ele:tmp2){
                            label_out[i].insert(fa[ele]);
                        }
                    }
                    
                }
                
                {
                    for (auto ite = graph.query("select (a) return a"_graphsql);
                                ite != graph.end();
                                ++ite)
                    {
                        for (const auto & item : graph.getOutEdge(
                                        ite->getNode("a").id()
                                        ))
                        {
                            LOGGER(debug, " all edge include: {}",
                                        item);
                        }
                    }
                }
            }	
        }
}

