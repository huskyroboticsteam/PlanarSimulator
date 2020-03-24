
#include "graphics.h"
#include "plan.h"
#include <Eigen/Core>
#include <queue>
#include <iostream>
#include <set>

constexpr double RESOLUTION = 0.3;
constexpr double TURN_COST = 0.5;
constexpr double SAFE_RADIUS = 0.4;

using action_t = Eigen::Vector2d;

// TODO implement goal orientations?
double heuristic(int x, int y, const landmark_t &goal)
{
  double dx = goal(0) - x*RESOLUTION;
  double dy = goal(1) - y*RESOLUTION;
  return sqrt(dx*dx + dy*dy);
}

struct Node
{
  int x;
  int y;
  int theta; // allows values 0..7; gives angle in increments of pi/4
  double acc_cost; // accumulated cost
  double heuristic_to_goal;
  int acc_steps; // for knowing how long (in discrete steps) the plan will be
  struct Node *parent;
  // action: the action taken to reach this node from the parent node
  // Tracking this makes it easier to construct the plan later.
  action_t action;

  // Note: In the robot frame, starting pose is always (0,0,0).
  Node(Node *p, action_t &a, landmark_t &goal) : x(0), y(0), theta(0),
        acc_cost(0.), heuristic_to_goal(0.), acc_steps(0),
        parent(p), action(a)
  {
    if (p != nullptr)
    {
      acc_steps = p->acc_steps + 1;
      bool moving_forward = action(1) > 0.0;
      double step_cost = moving_forward ? action(1) : TURN_COST;
      acc_cost = p->acc_cost + step_cost;
      if (moving_forward)
      {
        theta = p->theta;
        int dx(0), dy(0);
        if (theta >= 1 && theta <= 3) dy = 1;
        if (theta >= 3 && theta <= 5) dx = -1;
        if (theta >= 5 && theta <= 7) dy = -1;
        if (theta == 7 || theta <= 1) dx = 1;
        x = p->x + dx;
        y = p->y + dy;
      }
      else
      {
        x = p->x;
        y = p->y;
        theta = (p->theta + ((action(0) > 0.0) ? 1 : 7)) % 8;
      }
    }
    heuristic_to_goal = heuristic(x, y, goal);
  }
};

std::ostream& operator<< (std::ostream &out, const Node &n)
{
  out <<"("<< n.x << " " << n.y << " " << n.theta <<") ";
  return out;
}

class NodeEqualityCompare
{
public:
  bool operator() (const Node *lhs, const Node *rhs) const
  {
    bool res = lhs->x <  rhs->x ||
               (lhs->x == rhs->x && lhs->y <  rhs->y) ||
               (lhs->x == rhs->x && lhs->y == rhs->y && lhs->theta < rhs->theta);
    return res;
  }
};

class NodeCompare
{
public:
  bool operator() (const Node *lhs, const Node *rhs)
  {
    return (lhs->acc_cost + lhs->heuristic_to_goal) > (rhs->acc_cost + rhs->heuristic_to_goal);
  }
};

using pqueue_t = std::priority_queue<Node*, std::vector<Node*>, NodeCompare>;
using set_t = std::set<Node*, NodeEqualityCompare>;

bool is_valid(const Node *n, World &w)
{
  // To get away from obstacles, turning is always allowed.
  if (n->action(1) == 0.0) return true;
  pose_t p(n->x * RESOLUTION, n->y * RESOLUTION, n->theta * M_PI/4);
  transform_t tf = toTransform(p);
  landmark_readings_t lms = w.bag().back();
  return !collides(tf, lms, SAFE_RADIUS);
}

plan_t getPlan(World &w, const landmark_t &world_goal, double goal_radius)
{
  std::cout << "Planning... " << std::flush;
  landmark_t goal = w.gps().back() * world_goal; // in robot frame
  action_t action = action_t::Zero();
  std::vector<Node*> allocated_nodes;
  Node *start = new Node(nullptr, action, goal);
  allocated_nodes.push_back(start);
  pqueue_t fringe; // If you haven't guessed, we'll be using A*
  set_t visited_set;
  fringe.push(start);
  visited_set.insert(start);
  Node *n = start;
  plan_t valid_actions(3,2);
  valid_actions << 0., 0., M_PI/4, 0., -M_PI/4, 0.;
  int counter = 0;
  while (fringe.size() > 0)
  {
    counter++;
    n = fringe.top();
    fringe.pop();
    if (n->heuristic_to_goal < goal_radius)
    {
      break;
    }
    else
    {
      double forward_dist = (n->theta % 2 == 1) ? RESOLUTION*1.414 : RESOLUTION;
      valid_actions(0,1) = forward_dist;
      for (int i = 0; i < valid_actions.rows(); i++)
      {
        action = valid_actions.row(i);
        Node *next = new Node(n, action, goal);
        allocated_nodes.push_back(next);
        if (is_valid(next, w) && visited_set.count(next) == 0)
        {
          visited_set.insert(next);
          fringe.push(next);
        }
      }
    }
  }

  // TODO what should we return if planning fails?
  plan_t plan(n->acc_steps,2);
  for (int i = n->acc_steps-1; i >= 0; i--)
  {
    plan.row(i) = n->action;
    n = n->parent;
  }

  std::cout << "finished in " << counter << " iterations (";
  std::cout << allocated_nodes.size() << " visited nodes)\n";
  for (Node *p : allocated_nodes)
  {
    free(p);
  }

  return plan;
}

void drawPlan(const plan_t &p, const transform_t &initial_transform)
{
  trajectory_t traj({});
  transform_t trans = initial_transform;
  traj.push_back(trans);
  for (int i = 0; i < p.rows(); i++)
  {
    double theta = p(i,0);
    double x = p(i,1);
    // Doesn't matter if we rotate first because our plans never rotate
    // and move forward at the same time.
    trans = toTransformRotateFirst(x, 0, theta) * trans;
    traj.push_back(trans);
  }
  landmarks_t lms({}); // Just to conform with drawTraj interface
  drawTraj(lms, traj, sf::Color::Red);
}
