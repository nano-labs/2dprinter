#include <Stepper.h>
#include <Servo.h>

#define STEPS_PER_MOTOR_REVOLUTION 32

Stepper stepper_y(STEPS_PER_MOTOR_REVOLUTION, 8, 10, 9, 11);
Stepper stepper_x(STEPS_PER_MOTOR_REVOLUTION, 4, 5, 3, 6);
Servo servo_z;

float mm_per_step = 0.0194;
float steps_per_mm = 1.0 / mm_per_step;
float pos_x = 0.0;
float pos_y = 0.0;
float pix_size = 1.3;
float noise_x = 0;
float noise_y = 0;

int z = 180;
void setup() {
  // put your setup code here, to run once:
  stepper_y.setSpeed(1000);
  stepper_x.setSpeed(1000);
  servo_z.attach(12);

  Serial.begin(57600);
  while (!Serial) {}
  Serial.println("started");
  servo_z.write(180);
}

void start_drawing() {
  servo_z.write(70);
  delay(300);
}
void stop_drawing() {
  servo_z.write(180);
//  delay(300);
}
void goto_xy(int pix_x, int pix_y) {
  float new_x = pix_x * pix_size;
  float new_y = pix_y * pix_size;
  stepper_x.step(((pos_x - new_x) * steps_per_mm));
  pos_x = new_x;
  stepper_y.step(((pos_y - new_y) * steps_per_mm));
  pos_y = new_y;

}
void dot_xy(int pix_x, int pix_y) {
  float new_x = pix_x * pix_size;
  float new_y = pix_y * pix_size;
  float new_noise_x = random(pix_size * steps_per_mm * -0.5, pix_size * steps_per_mm * 0.5);
  float new_noise_y = random(pix_size * steps_per_mm * -0.8, pix_size * steps_per_mm * 0.8);

  stepper_x.step(((pos_x - new_x) * steps_per_mm) - noise_x + new_noise_x);
  pos_x = new_x;
  stepper_y.step(((pos_y - new_y) * steps_per_mm) - noise_y + new_noise_y);
  pos_y = new_y;
  delay(100);
  start_drawing();
  stop_drawing();

  noise_x = new_noise_x;
  noise_y = new_noise_y;
}
void loop() {
  String payload = Serial.readStringUntil("\n");
  int previous_index = 0;
  int next_index = payload.indexOf(";", 0);
  while (next_index != -1) {
    String line = payload.substring(previous_index + 1, next_index);
    previous_index = next_index;
    next_index = payload.indexOf(";", previous_index + 1);
  
    String command = line.substring(0, 1);
    Serial.println("line "+ line);
    Serial.println("comando "+ command);
  
    if (command == "g") {
      int new_x = line.substring(1, line.indexOf(",")).toInt();
      int new_y = line.substring(line.indexOf(",") + 1).toInt();
      goto_xy(new_x, new_y);
    }
    if (command == "p") {
      int new_x = line.substring(1, line.indexOf(",")).toInt();
      int new_y = line.substring(line.indexOf(",") + 1).toInt();
      dot_xy(new_x, new_y);
    }
    if (command == "d") {
      start_drawing();
      delay(200);
    }
    if (command == "u") {
      stop_drawing();
      delay(200);
    }
    if (command == "t") {
      Serial.println("ready");
    }
  }

//  int value = command.substring(1).toInt();
//  Serial.println(axis);
//  Serial.println(value);
//  if (axis == "x") {
//    stepper_x.step(value);
//  };
//  if (axis == "y") {
//    stepper_y.step(value);
//  }
//  stop_drawing();
//  goto_xy(1, 1);
//  make_pixel();
//
//  goto_xy(2, 1);
//  make_pixel();
//
//  goto_xy(3, 1);
//  make_pixel();
//
//  goto_xy(4, 1);
//  make_pixel();
//
//  goto_xy(5, 1);
//  make_pixel();
//
//  goto_xy(1, 2);
//  make_pixel();
//
//  goto_xy(2, 2);
//  make_pixel();
//
//  goto_xy(3, 2);
//  make_pixel();
//
//  goto_xy(4, 2);
//  make_pixel();
//
//  goto_xy(5, 2);
//  make_pixel();
//
//  goto_xy(1, 3);
//  make_pixel();
//
//  goto_xy(2, 3);
//  make_pixel();
//
//  goto_xy(3, 3);
//  make_pixel();
//
//  goto_xy(4, 3);
//  make_pixel();
//
//  goto_xy(5, 3);
//  make_pixel();
//
//  goto_xy(1, 4);
//  make_pixel();
//
//  goto_xy(2, 4);
//  make_pixel();
//
//  goto_xy(3, 4);
//  make_pixel();
//
//  goto_xy(4, 4);
//  make_pixel();
//
//  goto_xy(5, 4);
//  make_pixel();
//
//  goto_xy(1, 5);
//  make_pixel();
//
//  goto_xy(2, 5);
//  make_pixel();
//
//  goto_xy(3, 5);
//  make_pixel();
//
//  goto_xy(4, 5);
//  make_pixel();
//
//  goto_xy(5, 5);
//  make_pixel();

//  if (z == 180) {
//    z = 70;
//  } else {
//    z = 180;
//  }
//  servo_z.write(70);
//  delay(200);
//  stepper_y.step(1000);
//  stepper_x.step(1000);
//  stepper_y.step(-1000);
//  stepper_x.step(-1000);
//  delay(100);
//  servo_z.write(180);
//  delay(200);
//  stepper_y.step(-100);
//  stepper_x.step(100);

//  stepper_y.step(random(-1000, 1000));
//  stepper_x.step(random(-1000, 1000));
//  delay(1000);
//  stepper_y.step(-500);
//  stepper_x.step(-500);
//  delay(1000);

}
