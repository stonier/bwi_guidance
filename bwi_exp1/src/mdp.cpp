#include <bwi_exp1/mdp.h>

namespace bwi_exp1 {

  // class WeightedVertex {
  //  public:
  //   size_t vertex;
  //   int8_t weight;
  //   WeightedVertex(size_t v, int8_t w) : vertex(v), weight(w) {}
  //   bool operator==(const WeightedVertex& l, const WeightedVertex& r) {
  //     return l.vertex == r.vertex;
  //   }
  // };

  // struct weightedVertexComparator : 
  //     binary_function <WeightedVertex, WeightedVertex, bool> {
  //   inline bool operator() (const WeightedVertex& x, const WeightedVertex& y) const {
  //     return x.weight < y.weight;
  //   }
  // };

  // typedef WeightedVertexHeap boost::heap::fibonacci_heap<
  //   WeightedVertex, 
  //   boost::heap::compare<weightedVertexComparator>
  // >;
  // typedef topological_mapper::Graph Graph;

  // void populateStateSpace(const Graph &graph, 
  //     std::vector<State>& state_space) {

  //   size_t num_vertices = boost::num_vertices(graph);
  //   boost::property_map<Graph, boost::vertex_index_t>::type 
  //       indexmap = boost::get(boost::vertex_index, graph);

  //   for (size_t i = 0; i < num_vertices; ++i) { //graph_id

  //     Graph::vertex_descriptor source_v = boost::vertex(i, graph);
  //     topological_mapper::Point2f source_loc = graph[source_v].location;

  //     // Compute frontier at this vertex. 
  //     State state;
  //     WeightedVertexHeap heap;
  //     heap.push(WeightedVertex(i, FRONTIER_LOOKAHEAD));

  //     // Get a visted vector
  //     std::vector<bool> visited(num_vertices, false); 
  //     visited[i] = true;

  //     while(!heap.empty()) {
  //       const WeightedVertex &wv = heap.top();
  //       
  //       if (wv.weight == 0) {
  //         if (wv.vertex != i) // Don't push the source vertex into the 
  //           state.frontier.push_back(wv.vertex);
  //       } else {

  //         // Check if any of the adjacent vertices are not already on the heap,
  //         // and they are still line-of-sight
  //         bool current_vertex_in_frontier = true;
  //         Graph::vertex_descriptor v = boost::vertex(wv.vertex, graph);
  //         Graph::adjacency_iterator ai, aend;
  //         for (boost::tie(ai, aend) = boost::adjacent_vertices(v, graph); 
  //             ai != aend; ++ai) {

  //           // Check if adjacent location has already been visited
  //           if (visited[*ai]) {
  //             continue;
  //           }

  //           // Compute location to see if a given place 
  //           Point2f loc = graph[*ai].location;
  //         }


  //       }

  //       heap.pop();
  //     }

  //   }
  // }
  
  void populateStateSpace(const Graph &graph, 
      std::vector<State>& state_space, size_t goal_idx) {

    size_t num_vertices = boost::num_vertices(graph);
    state_space.resize(getStateSpaceSize(num_vertices));

    boost::property_map<Graph, boost::vertex_index_t>::type 
        indexmap = boost::get(boost::vertex_index, graph);

    state_space.clear();

    for (size_t i = 0; i < num_vertices; ++i) {
      Graph::vertex_descriptor v = boost::vertex(i, graph);

      // Compute the set of possible actions 
      std::vector<size_t> adjacent_idxs;
      Graph::adjacency_iterator ai, aend;
      for (boost::tie(ai, aend) = boost::adjacent_vertices(v, graph); 
          ai != aend; ++ai) {
        adjacent_idxs.push_back(indexmap[*ai]);
      }

      std::vector<Action> all_actions, no_robot_actions;
      all_actions.push_back(Action(DO_NOTHING,0));
      no_robot_actions.push_back(Action(DO_NOTHING,0));

      // For each adjacent idx, create an action
      for (size_t a = 0; a < adjacent_idxs.size(); ++a) {
        all_actions.push_back(Action(PLACE_ROBOT, adjacent_idxs[a]));
      }

      // Add the relevant states
      for (size_t dir = 0; dir < NUM_DIRECTIONS; ++dir) {

        std::vector<size_t> possible_next_directions(adjacent_idxs.size(), 0);
        for (size_t a = 0; a < adjacent_idxs.size(); ++a) {
          size_t next_dir = computeNextDirection(dir, i, adjacent_idxs[a], graph);
          possible_next_directions.push_back(next_dir);
        }

        for (size_t robots_remaining = 0; robots_remaining <= NUM_ROBOTS; 
            ++robots) {

          size_t state_idx = constructStateIndex(i, dir, robots_remaining);
          state_space[state_idx].graph_id = i;
          
          
        }
      }

    }
  }
}
