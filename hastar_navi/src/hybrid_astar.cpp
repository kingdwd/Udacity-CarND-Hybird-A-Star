#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <vector>
#include "hybrid_astar.h"
#include <boost/heap/binomial_heap.hpp>

using namespace std;


/**
 * Initializes HAS
 */
HAS::HAS() {

}

HAS::~HAS() {}


//heap optimization method
struct Compare_cost {

  bool operator()(const HAS::maze_s & lhs, const HAS::maze_s & rhs) const {
    return lhs.f < rhs.f;
  }

};
typedef boost::heap::binomial_heap< HAS::maze_s,
                                    boost::heap::compare<Compare_cost>> SortedQueue;

//sort method
bool HAS::compare_maze_s(const HAS::maze_s & lhs,
                         const HAS::maze_s & rhs) {

    return lhs.f < rhs.f;
}

double HAS::heuristic(double x, double y,
                      vector<int> goal,
                      string heuristic_method){

      double dx  = fabs(y - goal[0]);
      double dy  = fabs(x - goal[1]);
      double tie = (1.0 + 1.0/10);

      //http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html#speed-or-accuracy
      if (heuristic_method == "Manhattan")       return dx + dy;
      if (heuristic_method == "Chebyshev")       return (dx + dy) -  min(dx, dy);
      if (heuristic_method == "Octile")          return 2*(dx + dy) - (3-2*2)*min(dx, dy);
      if (heuristic_method == "Euclidean")       return sqrt(dx * dx + dy * dy);
      if (heuristic_method == "Octile_breaktie") return tie*(2*(dx + dy) - (3-2*2)*min(dx, dy));
      //

}

/*  HAS::turning_cost(HAS::maze_s current_state,
                      double next_angle)
    Input:  current state of car [x,y,theta]: current_state
            next state of car [theta]: next_angle when expanding
    Output: cost to turn
*/
int HAS::turning_cost(HAS::maze_s current_state,
                      double next_angle){

    double angle_diff_rad = fabs(next_angle -current_state.theta); // radian
    int    angle_diff_deg = angle_diff_rad * 180.0/ M_PI; // convert to degree
    int    cost           = angle_diff_deg % turning_res;

    return cost;

}

/* HAS::theta_to_stack_number(double theta)
Takes an angle (in radians) and returns which "stack" in the 3D configuration space
this angle corresponds to. Angles near 0 go in the lower stacks while angles near
2 * pi go in the higher stacks.
*/
int HAS::theta_to_stack_number(double theta){

  double new_theta = fmod((theta + 2 * M_PI),(2 * M_PI));
  int stack_number = (int)(round(new_theta * NUM_THETA_CELLS / (2*M_PI))) % NUM_THETA_CELLS;
  return stack_number;
}

/*
Returns the index into the grid for continuous position. So if x is 3.621, then this
would return 3 to indicate that 3.621 corresponds to array index 3.
*/
int HAS::idx(double float_num) {

  return int(floor(float_num));
}


vector<HAS::maze_s> HAS::expand(HAS::maze_s state,
                                vector<int> goal,
                                string heuristic_method) {

  int g        = state.g;
  double x     = state.x;
  double y     = state.y;
  double theta = state.theta;

  int g2 = g + 1;
  vector<HAS::maze_s> next_states;

  for(double delta_i = -max_turnable;
             delta_i < (max_turnable + turning_res);
             delta_i += turning_res) {

    // Update next state
    double delta  = M_PI / 180.0 * delta_i;
    double omega  = SPEED / LENGTH * tan(delta);

    double theta2 = theta + omega;
    if(theta2 < 0){ theta2 += 2*M_PI;}

    double x2 = x + SPEED * cos(theta);
    double y2 = y + SPEED * sin(theta);

    // Update next state cost
    //g2        = g2 + turning_cost(state, theta2);
    int f2    = g2 + heuristic(x2, y2, goal, heuristic_method);

    // Create a new State object with all of the "next" values.
    HAS::maze_s state2 {g2, f2, x2, y2, theta2,};
    next_states.push_back(state2);

  }
  return next_states;
}

vector<HAS::maze_s> HAS::reconstruct_path(vector< vector< vector<HAS::maze_s> > > came_from,
                                          vector<double> start,
                                          HAS::maze_s final){

	vector<maze_s> path = {final};

	int stack = theta_to_stack_number(final.theta);

	maze_s current = came_from[stack][idx(final.x)][idx(final.y)];

	stack = theta_to_stack_number(current.theta);

	double x = current.x;
	double y = current.y;

	while( x != start[0] || y != start[1] ){
		path.push_back(current);
		current = came_from[stack][idx(x)][idx(y)];
		x = current.x;
		y = current.y;
		stack = theta_to_stack_number(current.theta);
	}

	return path;

}

HAS::maze_path HAS::search(vector<vector<int>> grid,
                           vector<double> start,
                           vector<int> goal,
                           string heuristic_method) {

  // Initializes closed list as 0
  vector<vector<vector<int> > >    closed(NUM_THETA_CELLS,
                                          vector<vector<int>>(grid[0].size(),
                                                              vector<int>(grid.size()) ));
  vector<vector<vector<maze_s> > > came_from(NUM_THETA_CELLS,
                                             vector<vector<maze_s>>(grid[0].size(),
                                                                    vector<maze_s>(grid.size()) ));
  double theta = start[2];
  int stack    = theta_to_stack_number(theta);
  int g        = 0;
  int f        = g + heuristic(start[0], start[1], goal, heuristic_method);

  // Create new state object to start the search with.
  maze_s state {g, f, start[0], start[1], theta};

  closed[stack][idx(state.x)][idx(state.y)]    = 1;
  came_from[stack][idx(state.x)][idx(state.y)] = state;

  int total_closed = 1;

  // Sort Method
  vector<maze_s> opened = {state};

  bool finished = false;

  while(!opened.empty()) {

    // Sort Method
    sort(opened.begin(), opened.end(), compare_maze_s);
    maze_s current = opened[0];   //grab first elment
    opened.erase(opened.begin()); //pop first element


    int x = current.x;
    int y = current.y;

    // Check if reach the goal
    if(idx(x) == goal[0] && idx(y) == goal[1]){
      cout << " found path to goal in " << total_closed << " expansions" << endl;
      maze_path path {closed, came_from, current,};

      return path;
    }

    // Otherwise, expand the current state to get
    // a list of possible next states.
    vector<maze_s> next_state = expand(current, goal, heuristic_method);

    for(int i = 0; i < next_state.size(); i++) {
      int g2        = next_state[i].g;
      double x2     = next_state[i].x;
      double y2     = next_state[i].y;
      double theta2 = next_state[i].theta;


      // If we have expanded outside the grid, skip this next_state.
      if((x2 < 0 || x2 >= grid.size()) || (y2 < 0 || y2 >= grid[0].size())) {
        //invalid cell
        continue;
      }

      int stack2 = theta_to_stack_number(theta2);

      /*Otherwise, check that we haven't already visited this cell and
       *that there is not an obstacle in the grid there.
       */
      if(closed[stack2][idx(x2)][idx(y2)] == 0 && grid[idx(x2)][idx(y2)] == 0) {

        // The state can be added to the opened stack.
        opened.push_back(next_state[i]);

        /*The stack_number, idx(next_state.x), idx(next_state.y) tuple
         *has now been visited, so it can be closed.
         */
        closed[stack2][idx(x2)][idx(y2)] = 1;

        //The next_state came from the current state, and that is recorded.
        came_from[stack2][idx(x2)][idx(y2)] = current;

        total_closed += 1;
      }


    }

  }
  cout << "no valid path." << endl;
  HAS::maze_path path {closed, came_from, state,};

  return path;

}


HAS::maze_path HAS::search_heap(vector<vector<int>> grid,
                                vector<double> start,
                                vector<int> goal,
                                string heuristic_method) {

  vector<vector<vector<int> > >    closed(NUM_THETA_CELLS,
                                          vector<vector<int>>(grid[0].size(), vector<int>(grid.size())));
  vector<vector<vector<maze_s> > > came_from(NUM_THETA_CELLS,
                                             vector<vector<maze_s>>(grid[0].size(), vector<maze_s>(grid.size())));
  double theta = start[2];
  int stack    = theta_to_stack_number(theta);
  int g        = 0;
  int f        = g + heuristic(start[0], start[1], goal, heuristic_method);

  // Create new state object to start the search with.
  maze_s state {g, f, start[0], start[1], theta};

  closed[stack][idx(state.x)][idx(state.y)]    = 1;
  came_from[stack][idx(state.x)][idx(state.y)] = state;

  int total_closed = 1;

  // Heap Method
  SortedQueue opened_heap;
  opened_heap.push(state);

  bool finished = false;

  while(!opened_heap.empty()) {

    // Heap Method
    maze_s current = opened_heap.top();// get smallest value
    opened_heap.pop();// delete

    int x = current.x;
    int y = current.y;

    // Check if reach the goal
    if(idx(x) == goal[0] && idx(y) == goal[1]){
      cout << " found path to goal in " << total_closed << " expansions" << endl;
      maze_path path {closed, came_from, current,};

      return path;
    }

    // Otherwise, expand the current state to get
    // a list of possible next states.
    vector<maze_s> next_state = expand(current, goal, heuristic_method);

    for(int i = 0; i < next_state.size(); i++) {
      int g2        = next_state[i].g;
      double x2     = next_state[i].x;
      double y2     = next_state[i].y;
      double theta2 = next_state[i].theta;


      // If we have expanded outside the grid, skip this next_state.
      if((x2 < 0 || x2 >= grid.size()) || (y2 < 0 || y2 >= grid[0].size())) {
        //invalid cell
        continue;
      }

      int stack2 = theta_to_stack_number(theta2);

      //Otherwise, check that we haven't already visited this cell and
      //that there is not an obstacle in the grid there.
      if(closed[stack2][idx(x2)][idx(y2)] == 0 && grid[idx(x2)][idx(y2)] == 0) {

        // The state can be added to the opened stack.
        opened_heap.push(next_state[i]);

        //The stack_number, idx(next_state.x), idx(next_state.y) tuple
        //has now been visited, so it can be closed.
        closed[stack2][idx(x2)][idx(y2)] = 1;

        //The next_state came from the current state, and that is recorded.
        came_from[stack2][idx(x2)][idx(y2)] = current;

        total_closed += 1;
      }


    }

  }
  cout << "no valid path." << endl;
  HAS::maze_path path {closed, came_from, state,};

  return path;

}