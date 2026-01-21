#ifndef SMC_MOTOR_H
#define SMC_MOTOR_H

static const char MOTOR_VERSION = 0x00;
static const int COMPARTMENTS = 8;
static const char* MOTOR_PATH = "/motor";
static const int MOTOR_STEPS = 4096;
static const double STEPS_PER_COMPARTMENT = MOTOR_STEPS / COMPARTMENTS;

class Motor {
 public:
  int setup(void);
  void loop(void);

  int load_from_fs(void);
  int save_into_fs(void);

  int steps(void);

  int spin_to(int compartment);
  int calibrate(int steps);

 private:
  char version = MOTOR_VERSION;
  int current_step;
  bool locked;
};

#endif
