#include "world.h"
#include "graphics.h"
#include "plan.h"
#include "search.h"
#include "constants.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace NavSim;
extern const bool IS_2D { true };

int main()
{
  World w;
  point_t goal;
  goal << 9., 6., 1.;
  setGoal(goal);
  w.addDefaultObstacles();
  w.addLandmark(20, 3);
  w.startSimulation();

  bool diag = false;
  bool truth = false;
  bool autonomous = false;

  std::cout << "Type 'q' to quit, 'wasd' to move around, 't' to view ground truth, 'r' to start autonomous mode.\n";
  char c = -2;
  action_t action;
  transform_t odom_viz_tf = toTransform({WINDOW_CENTER_X,WINDOW_CENTER_Y,M_PI/2});
  do {
    if (((int) c) != -1 || autonomous)
    {
      transform_t viz_tf = truth ? w.truth().back() : odom_viz_tf;

      truth ? w.renderTruth() : w.renderRobotView(viz_tf);
      display();
      truth ? w.renderTruth() : w.renderRobotView(viz_tf);

      action = act(w, viz_tf);
      if (action == action_t::Zero())
      {
        std::cout << "Disabling autonomous mode.\n";
        autonomous = false;
      }

      display();
    }
    c=pollWindowEvent();

    double x_dist = diag ? 1.414*PLAN_RESOLUTION : PLAN_RESOLUTION;
    switch(c) {
    case 22:
    case 73:
      w.moveRobot(0.0, x_dist);
      break;
    case 18:
    case 74:
      w.moveRobot(0.0, -x_dist);
      break;
    case 0:
    case 71:
      w.moveRobot(M_PI/4, 0.0);
      diag = !diag;
      break;
    case 3:
    case 72:
      w.moveRobot(-M_PI/4, 0.0);
      diag = !diag;
      break;
    case 17:
      autonomous = !autonomous;
      break;
    case 19:
      truth = !truth;
      break;
    default:
      break;
    }
    if (autonomous) {
      w.moveRobot(action(0), action(1));
    }
    usleep(10*1000);
  } while(c!=16);

  return 0;
}
