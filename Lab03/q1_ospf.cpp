// g++ q1_ospf.cpp -o q1_ospf && ./q1_ospf
#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <map>

#define NORMAL "\033[0m"
#define INVERT "\033[30;47m"
#define HEADING "\033[1;100m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define UNDERLINE "\033[4m"

class Router {
private:
    int router_id;
    std::map<int, std::pair<Router*, int>> neighbours;
    std::map<int, Router*> routing_table;
public:
    Router(int rid) : router_id(rid) {}
    void add_neighbour(Router* neighbour, int link_cost);
    Router* print_routing_table(int dest_rid);
    void update_routing_table();
    void traceroute(Router* dest);
};

void Router::add_neighbour(Router* neighbour, int link_cost){
    this->neighbours[neighbour->router_id] = {neighbour, link_cost};
    neighbour->neighbours[this->router_id] = {this, link_cost};
}

Router* Router::print_routing_table(int dest_rid = (int)NULL){
    Router* next_hop_router = nullptr;
    std::cout << HEADING << " Routing Table - Router " << std::left << std::setw(4) << router_id << NORMAL << "\n";
    std::cout << "  Destination  |  NextHop  \n";
    for(auto entry : routing_table){
        int destination_rid = entry.first;
        int next_hop_rid = entry.second->router_id;
        if(destination_rid == dest_rid){
            next_hop_router = entry.second;
            std::cout << INVERT;
        }
        std::cout << "  " << std::setw(11) << destination_rid << "  |  " << std::setw(7) << next_hop_rid << "   " << NORMAL << "\n";
    }
    std::cout << "\n";
    return next_hop_router;
}

void Router::update_routing_table(){
    std::map<int, int> path_cost;
    std::map<int, Router*> next_hop;

    path_cost[this->router_id] = 0;
    next_hop[this->router_id] = this;

    std::priority_queue<std::pair<int, Router*>, std::vector<std::pair<int, Router*>>, std::greater<std::pair<int, Router*>> > pq;

    for (auto neighbour_info : this->neighbours) {
        int neighbour_rid = neighbour_info.first;
        Router* neighbour = neighbour_info.second.first;
        int link_cost = neighbour_info.second.second;
        next_hop[neighbour_rid] = neighbour;
        path_cost[neighbour_rid] = link_cost;
        pq.push({path_cost[neighbour_rid], neighbour});
    }

    while (!pq.empty()) {
        int curr_cost = pq.top().first;
        Router* router = pq.top().second;
        int curr_rid = router->router_id;
        pq.pop();

        for (auto neighbour_info : router->neighbours) {
            int neighbour_rid = neighbour_info.first;
            Router* neighbour = neighbour_info.second.first;
            int link_cost = neighbour_info.second.second;

            if ((path_cost.find(neighbour_rid) == path_cost.end()) || (path_cost[neighbour_rid] > curr_cost + link_cost)) {
                path_cost[neighbour_rid] = curr_cost + link_cost;
                next_hop[neighbour_rid] = next_hop[curr_rid];
                pq.push({path_cost[neighbour_rid], neighbour});
            }
        }
    }

    routing_table.clear();
    for(auto entry : next_hop){
        routing_table.insert(entry);
    }
}

void Router::traceroute(Router* dest){
    Router* curr_router = this;
    int curr_rid = this->router_id;
    int dest_rid = dest->router_id;
    std::cout << "Calculating Route from Router " << curr_rid << " to Router " << dest_rid << "...  ";
    if(curr_router->routing_table.find(dest_rid) == curr_router->routing_table.end()){
        std::cout << RED << "Not Reachable\n\n" << NORMAL;
        return;
    }
    if(curr_rid == dest_rid){
        std::cout << GREEN << "Localhost\n\n" << NORMAL;
        return;
    }
    std::cout << "\n\n";
    std::vector<int> path;
    while(curr_rid != dest_rid){
        path.push_back(curr_rid);
        curr_router = curr_router->print_routing_table(dest_rid);
        curr_rid = curr_router->router_id;
    }
    std::cout << "OSPF Route: ";
    for(int rid : path){
        std::cout << rid << " -> ";
    }
    std::cout << dest_rid << "\n\n";
}


int main() {
    Router r1(1);
    Router r2(2);
    Router r3(3);
    Router r4(4);
    Router r5(5);
    Router r6(6);

    r1.add_neighbour(&r2, 3);
    r1.add_neighbour(&r3, 2);
    r2.add_neighbour(&r5, 1);
    r2.add_neighbour(&r4, 5);
    r5.add_neighbour(&r3, 3);

    r1.update_routing_table();
    r2.update_routing_table();
    r3.update_routing_table();
    r4.update_routing_table();
    r5.update_routing_table();
    r6.update_routing_table();

    r3.traceroute(&r4);
    r1.traceroute(&r1);
    r1.traceroute(&r6);

    return 0;
}