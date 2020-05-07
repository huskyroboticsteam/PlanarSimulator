#include "world_interface.h"
#include "world.h"
#include <unistd.h>

World world;

void world_interface_init() {
  world.addDefaultObstacles();
  world.addLandmark(20, 3);
  world.readSensors();
  world.spawnWindow();
  // Sleep to avoid situation where we're trying to launch two SFML windows
  // simultaneously (which seems to sometimes cause deadlock??)
  usleep(100 * 1000);
}

// TODO clean this up. Right now we measure sensor data
// iff we set the cmd_vel.
void setCmdVel(double dtheta, double dx) {
  world.readSensors();
  world.setCmdVel(dtheta, dx);
}

points_t getLidarScan() {
  return world.lidar().back();
}

points_t getLandmarks() {
  return world.landmarks().back();
}

transform_t getGPS() {
  return world.gps().back();
}
