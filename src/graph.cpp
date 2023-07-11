#include "graph.h"

graph::graph(id_t max_node_id) : max_id_(max_node_id)
{
    for(id_t i=0;i<=max_node_id;i++)
    {
        graph_[i]=std::set<id_t>();
    }
}

void graph::add_free_node(id_t id)
{
    free_list_.insert(id);
}
void graph::remove_free_node(id_t id)
{
    free_list_.erase(id);
}
void graph::add_edge(id_t id_x,id_t id_y)
{
    graph_[id_x].insert(id_y);
    graph_[id_y].insert(id_x);

}

bool graph::shortest_path_from_src(id_t src_node_id,std::vector<id_t>& kick_path) const
{
    std::unordered_map<int,int> dist;          // 节点到源点的距离
    std::unordered_map<int,bool> visited;      // 记录节点是否被访问过
    std::unordered_map<id_t,id_t> path;        // 记录到一个节点的上一个节点
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> pq; // 优先队列，按距起点的距离从小到大排列
    for(auto &x:graph_)
    {
        dist[x.first]=INF;
        visited[x.first]=false;
    }
    
    dist[src_node_id]=0;
    visited[src_node_id]=true;
    pq.push({0,src_node_id});
    while(!pq.empty())
    {
        id_t u=pq.top().second;
        pq.pop();
        if(visited[u])
            continue;
        visited[u] = true;
        // 对于与u相邻的每个点v，更新v的距离
        auto now_node_neighbor=graph_.at(u);
        for(auto it=now_node_neighbor.cbegin();it!=now_node_neighbor.cend();it++)
        {
            id_t v=*it;  
            if(dist[v]>dist[u]+1)
            {
                dist[v]=dist[u]+1;
                pq.push(std::make_pair(dist[v],v));
                path[v]=u;
                if(free_list_.count(v))
                {
                    while(v!=src_node_id)
                    {
                        kick_path.push_back(v);
                        v=path[v];
                    }
                    kick_path.push_back(src_node_id);
                    return true;
                }
            }
        }
    }
    return false;
}

void graph::reconstruct()
{
    for(id_t i=0;i<=max_id_;i++)
    {
        add_free_node(i);
        graph_[i]=std::set<id_t>();
    }
}