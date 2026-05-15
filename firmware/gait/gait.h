#ifndef GAIT_H
#define GAIT_H

#define FRONT_LEFT  0
#define FRONT_RIGHT 1
#define REAR_LEFT   2
#define REAR_RIGHT  3

#define HIP  0
#define KNEE 1

void standStill();
void walkForward();
void walkBackward();
void turnLeft();
void turnRight();
void sitDown();

#endif
