#include "motor.h"
#include "LittleFS.h"
#include "pins.h"
#include "thirdparty/ULN2003.h"
#include "utils.h"

static const char* TAG = "motor";
static int old_step_pos = 0;
static CheapStepper stepper(STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);

int Motor::setup(void) {
  if (int err = load_from_fs(); err == -2) {
    assert(LittleFS.format());
    esp_restart();
  } else if (err < -2) {
    ESP_LOGE(TAG, "err is %d", err);
    assert(false);
  };

  stepper.setRpm(16);
  old_step_pos = current_step;
  return 0;
}

void Motor::loop(void) {
  if (!locked) {
    stepper.run();
  }
  current_step = (old_step_pos + stepper.getStep()) % MOTOR_STEPS;
}

int Motor::load_from_fs(void) {
  // TODO extract these into functions
  fs_mutex.lock();
  File file = LittleFS.open(MOTOR_PATH, FILE_READ);
  if (!file) {
    file.close();
    fs_mutex.unlock();
    ESP_LOGW(TAG, "no saved data at %s, ignoring", MOTOR_PATH);
    return -1;
  }
  assert(!file.isDirectory());

  size_t res = file.readBytes((char*)this, sizeof(Motor));
  if (res != sizeof(Motor)) {
    file.close();
    fs_mutex.unlock();
    ESP_LOGE(TAG, "reading from %s, res %d, size %d", MOTOR_PATH, res,
             sizeof(Motor));
    return -2;
  };

  file.close();
  fs_mutex.unlock();

  // TODO DEBUG
  char dump[256 * 3 + 1];
  int dump_res = hexdump(dump, this, sizeof(Motor));
  ESP_LOGD(TAG, "first %d bytes dump of %s: %s", 256, MOTOR_PATH, dump);

  if (version != MOTOR_VERSION) {
    ESP_LOGE(TAG, "unsupported version: %02X", version);
    return -3;
  }

  return 0;
}

int Motor::save_into_fs(void) {
  fs_mutex.lock();
  File file = LittleFS.open(MOTOR_PATH, FILE_WRITE);

  assert(file && !file.isDirectory());

  assert(file.write((const uint8_t*)this, sizeof(Motor)) == sizeof(Motor));
  // assert(file.write(version) == 1);
  //
  // for (int i = 0; i < MAX_ALARMS; i++) {
  //   assert(file.write((uint8_t *)&list[i], sizeof(Alarm)) == sizeof(Alarm));
  // }
  //
  // ESP_LOGD(TAG, "write err is %d", file.getWriteError());

  file.close();
  fs_mutex.unlock();

  return 0;
}

int Motor::spin_to(int compartment) {
  if (compartment < 0 || compartment >= COMPARTMENTS) {
    return -1;
  }

  if (locked) {
    return -2;
  }

  // TODO kinda inefficent movements
  double displacement = compartment * STEPS_PER_COMPARTMENT - current_step;
  int steps = abs((int)lround(displacement));

  ESP_LOGD(TAG, "%lf , %d, %d", displacement, steps, current_step);

  if (displacement > 0.0) {
    stepper.newMove(true, steps);
  } else {
    stepper.newMove(false, steps);
  }

  return 0;
}

int Motor::steps(void) {
  return current_step;
}

int Motor::calibrate(int steps) {
  current_step = 0;
  old_step_pos = 0;
  assert(save_into_fs() == 0);

  stepper = CheapStepper(STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);
  // TODO hardcoded
  stepper.setRpm(15);

  if (steps > 0) {
    stepper.newMove(true, steps);
  } else {
    stepper.newMove(false, abs(steps));
  }

  return 0;
}
