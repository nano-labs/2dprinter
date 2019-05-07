//#include <SPI.h>
//#include <SD.h>
#include <Stepper.h>
#include <Servo.h>
//#include <math.h>

#define STEPS_PER_MOTOR_REVOLUTION 32

Stepper stepper_y(STEPS_PER_MOTOR_REVOLUTION, 8, 2, 9, 7);
//Stepper stepper_y(STEPS_PER_MOTOR_REVOLUTION, 8, 10, 9, 11);
Stepper stepper_x(STEPS_PER_MOTOR_REVOLUTION, 4, 5, 3, 6);
Servo servo_z;

const int chipSelect = 10;
const int switch_x = A2;
const int switch_y = A1;

float mm_per_step = 0.0194;
//float steps_per_mm = 1.0 / mm_per_step;
float steps_per_mm = 1.0;
float pos_x = 0.0;
float pos_y = 0.0;
float start_x = -1;
float start_y = -1;
float x0 = 0;
float y0 = 0;
float x1 = 0;
float y1 = 0;
float x2 = 0;
float y2 = 0;
float x3 = 0;
float y3 = 0;
float dx = 0;
float dy = 0;
float mx = 0;
float my = 0;
int previous_index = 0;
int next_index = 0;

void setup() {
  stepper_y.setSpeed(1000);
  stepper_x.setSpeed(1000);
  servo_z.attach(14);

  Serial.begin(57600);
  while (!Serial) {}
  Serial.println("started");
  servo_z.write(180);
  delay(5000);
//  Serial.print("Initializing SD card...");

//  // see if the card is present and can be initialized:
//  if (!SD.begin(chipSelect)) {
//    Serial.println("Card failed, or not present");
//    // don't do anything more:
//    while (1);
//  }
//  Serial.println("card initialized.");
//  File dataFile = SD.open("plot.cmd");
  go_home();
}

void go_home() {
  while (analogRead(switch_x) > 500) {
    goto_xy(pos_x -1, pos_y);
  }
  goto_xy(pos_x + 150, pos_y);
  pos_x = 0.0;
  while (analogRead(switch_y) > 500) {
    goto_xy(pos_x, pos_y - 1);
  }
  goto_xy(pos_x, pos_y + 150);
  pos_y = 0.0;
}

void reset() {
  pos_x = 0.0;
  pos_y = 0.0;
  start_x = -1;
  start_y = -1;
  x0 = 0;
  y0 = 0;
  x1 = 0;
  y1 = 0;
  x2 = 0;
  y2 = 0;
  x3 = 0;
  y3 = 0;
  dx = 0;
  dy = 0;
  mx = 0;
  my = 0;
  stop_drawing();
}

void start_drawing() {
  servo_z.write(70);
  delay(300);
}
void stop_drawing() {
  servo_z.write(180);
  delay(300);
}
int goto_xy(int pix_x, int pix_y) {
  float new_x = pix_x;
  float new_y = pix_y;
  float mov_x = ((pos_x - new_x) * steps_per_mm);
  stepper_x.step(mov_x * -1);
  pos_x = new_x;
  float mov_y = ((pos_y - new_y) * steps_per_mm);
  stepper_y.step(mov_y);
  pos_y = new_y;
}

void m(float x1, float y1) {
  M(pos_x + x1, pos_y + y1);
}
void M(float x0, float y0) {
  stop_drawing();
//  if (start_x == -1 && start_y == -1) {
//    start_x = x0;
//    start_y = y0;
//  }
  goto_xy(x0, y0);  
  start_drawing();
}
void h(float x1){
  H(pos_x + x1);  
}
void H(float x1) {
//  x0 = pos_x;
  goto_xy(x1, pos_y);
//  dx = ceil(x1 - x0);
//  if (dx > 0) {
//    for (float x = 0; x <= dx; x++){
//      goto_xy(x0 + x, pos_y);
//    }
//  } else {
//    for (float x = 0; x <= dx; x++) {
//      goto_xy(x0 - x, pos_y);  
//    }
//  }
}
void v(float y1) {
  return V(pos_y + y1);  
}
void V(float y1) {
  goto_xy(pos_x, y1);
//  y0 = pos_y;
//  dy = ceil(y1 - y0);
//  if (dy > 0) {
//    for (float y = 0; y <= dy; y++) {
//      goto_xy(pos_x, y0 + y);
//    }
//  } else {
//    for (float y = 0; y <= dy; y++) {
//      goto_xy(pos_x, y0 - y);
//    }
//  }
}
void c(float x1, float y1, float x2, float y2, float x3, float y3) {
  C(pos_x + x1, pos_y + y1, pos_x + x2, pos_y + y2, pos_x + x3, pos_y + y3);
}
void C(float x1, float y1, float x2, float y2, float x3, float y3) {
  cubic_bezier(pos_x, pos_y, x1, y1, x2, y2, x3, y3);
}
void l(float x1, float y1) {
  return L(pos_x + x1, pos_y + y1);  
}
void L(float x1, float y1) {
  x0 = pos_x;
  y0 = pos_y;
  dx = x1 - x0;
  dy = y1 - y0;
  if (dx == 0.0) {
    V(y1);
  } else if (dy == 0.0) {
    H(x1);
  } else {
    if (x1 > x0) {
      for (float mx = x0; mx <= x1; mx++) {
        my = y0 + dy * (mx - x0) / dx;
        goto_xy(mx, my);
      }
    } else {
      for (float mx = x0; mx >= x1; mx--) {
        my = y0 + dy * (mx - x0) / dx;
        goto_xy(mx, my);
      }
    }
    goto_xy(x1, y1);
  }
}
void Z() {
//  L(start_x, start_y);
//  start_x = -1;
//  start_y = -1;  
}

// void M(float x1, float y1) {
//   stop_drawing();
//   goto_xy(x1 * zoom, y1 * zoom);
//   start_drawing();
// }
// void l(float x0, float y0, float x1, float y1) {
//   M(x0, y0);
//   L(x1, y1);
// }
// void L(float X1, float Y1) {
//   float x0 = pos_x;
//   float y0 = pos_y;
//   float x1 = X1 * zoom;
//   float y1 = Y1 * zoom;
//   float tx = (y1 - y0) / (x1 - x0);
//   float dx = x1 - x0;
//   float dy = y1 - y0;
//   if (dx == 0 || dy == 0) {
//     goto_xy(x1, y1);
//   } else {
//     if (x1 > x0) {
//       for (float mx = x0; mx <= x1; mx++) {
//         float my = y0 + dy * (mx - x0) / dx;
//         goto_xy(mx, my);      
//       }
//     } else {
//       for (float mx = x0; mx >= x1; mx--) {
//         float my = y0 + dy * (mx - x0) / dx;
//         goto_xy(mx, my);
//       }
//     }
//   }
//   goto_xy(x1, y1);
// }
// void C(float X1, float Y1, float X2, float Y2, float X3, float Y3) {
//   float x1 = float(X1) * zoom;
//   float y1 = float(Y1) * zoom;
//   float x2 = float(X2) * zoom;
//   float y2 = float(Y2) * zoom;
//   float x3 = float(X3) * zoom;
//   float y3 = float(Y3) * zoom;
//   cubic_bezier(pos_x, pos_y, x1, y1, x2, y2, x3, y3);
// }

void cubic_bezier(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
    int delta = max(abs(x3 - x0), abs(y3 - y0)) * 2;
    for (float dt = 0; dt <= delta; dt++) {
        float t = dt / delta;
        float x = pow((1-t), 3) * x0 + 3*pow((1-t), 2) * t * x1 + 3*(1-t) * pow(t, 2) * x2 + pow(t, 3) * x3;
        float y = pow((1-t), 3) * y0 + 3*pow((1-t), 2) * t * y1 + 3*(1-t) * pow(t, 2) * y2 + pow(t, 3) * y3;
        goto_xy((int) x, (int) y);
    }
}

void loop() {  
//  String payload = Serial.readStringUntil("\n");
//  previous_index = -1;
//  next_index = payload.indexOf(";", 0);
//  while (next_index != -1) {
//    String line = payload.substring(previous_index + 1, next_index);
//    previous_index = next_index;
//    next_index = payload.indexOf(";", previous_index + 1);
//  
//    //  String payload = Serial.readStringUntil("\n");
//    String command = line.substring(0, 1);
//    String args = line.substring(1);
//    if (command == "C") {
//      int index1 = args.indexOf(",");
//      x1 = args.substring(0, index1).toFloat();
//      int index2 = args.indexOf(",", index1 + 1);
//      y1 = args.substring(index1 + 1, index2).toFloat();
//      int index3 = args.indexOf(",", index2 + 1);
//      x2 = args.substring(index2 + 1, index3).toFloat();
//      int index4 = args.indexOf(",", index3 + 1);
//      y2 = args.substring(index3 + 1, index4).toFloat();
//      int index5 = args.indexOf(",", index4 + 1);
//      x3 = args.substring(index4 + 1, index5).toFloat();
//      int index6 = args.indexOf("\r");
//      y3 = args.substring(index5 + 1, index6).toFloat();
//  //    Serial.println("ack " + command + " " + x1 + " " + y1 + " " + x2 + " " + y2 + " " + x3 + " " + y3);
//      C(x1, y1, x2, y2, x3, y3);
////      Serial.println("ack");
//    } else if (command == "M") {
//      int index1 = args.indexOf(",");
//      x1 = args.substring(0, index1).toFloat();
//      int index2 = args.indexOf(",", index1 + 1);
//      y1 = args.substring(index1 + 1, index2).toFloat();
//      M(x1, y1);
////      Serial.println("ack");
//    } else if (command == "L") {
//      int index1 = args.indexOf(",");
//      x1 = args.substring(0, index1).toFloat();
//      int index2 = args.indexOf(",", index1 + 1);
//      y1 = args.substring(index1 + 1, index2).toFloat();
//      L(x1, y1);
////      Serial.println("ack");
//    } else if (command == "H") {
//      int index1 = args.indexOf(",");
//      x1 = args.substring(0, index1).toFloat();
//      H(x1);
////      Serial.println("ack");
//    } else if (command == "V") {
//      int index1 = args.indexOf(",");
//      y1 = args.substring(0, index1).toFloat();
//      V(y1);
////      Serial.println("ack");
//    } else if (command == "Z") {
//      Z();
////      Serial.println("ack");
//    } else if (command == "R") {
//      reset();
////      Serial.println("ack");
//    }
//  }
//  Serial.println("ack");
}
//void bla() {
//  String payload = Serial.readStringUntil("\n");
//  int previous_index = 0;
//  int next_index = payload.indexOf(";", 0);
//  while (next_index != -1) {
//    String line = payload.substring(previous_index + 1, next_index);
//    previous_index = next_index;
//    next_index = payload.indexOf(";", previous_index + 1);
//  
//    String command = line.substring(0, 1);
////    Serial.println("line "+ line);
////    Serial.println("comando "+ command);
//  
//    if (command == "g") {
//      int new_x = line.substring(1, line.indexOf(",")).toInt();
//      int new_y = line.substring(line.indexOf(",") + 1).toInt();
//      int this_direction = goto_xy(new_x, new_y);
//      if (this_direction != last_direction) {
//        delay(100);      
//      }
//      delay(100);      
//      last_direction = this_direction;
//    }
//    if (command == "p") {
//      int new_x = line.substring(1, line.indexOf(",")).toInt();
//      int new_y = line.substring(line.indexOf(",") + 1).toInt();
//      dot_xy(new_x, new_y);
//    }
//    if (command == "s") {
//      pix_size = line.substring(1).toFloat();
//    }
//    if (command == "d") {
//      start_drawing();
//      delay(300);
//    }
//    if (command == "u") {
//      stop_drawing();
//      delay(300);
//    }
//    if (command == "t") {
//      Serial.println("ready");
//    }
//  }
////  int value = command.substring(1).toInt();
////  Serial.println(axis);
////  Serial.println(value);
////  if (axis == "x") {
////    stepper_x.step(value);
////  };
////  if (axis == "y") {
////    stepper_y.step(value);
////  }
////  stop_drawing();
////  goto_xy(1, 1);
////  make_pixel();
////
////  goto_xy(2, 1);
////  make_pixel();
////
////  goto_xy(3, 1);
////  make_pixel();
////
////  goto_xy(4, 1);
////  make_pixel();
////
////  goto_xy(5, 1);
////  make_pixel();
////
////  goto_xy(1, 2);
////  make_pixel();
////
////  goto_xy(2, 2);
////  make_pixel();
////
////  goto_xy(3, 2);
////  make_pixel();
////
////  goto_xy(4, 2);
////  make_pixel();
////
////  goto_xy(5, 2);
////  make_pixel();
////
////  goto_xy(1, 3);
////  make_pixel();
////
////  goto_xy(2, 3);
////  make_pixel();
////
////  goto_xy(3, 3);
////  make_pixel();
////
////  goto_xy(4, 3);
////  make_pixel();
////
////  goto_xy(5, 3);
////  make_pixel();
////
////  goto_xy(1, 4);
////  make_pixel();
////
////  goto_xy(2, 4);
////  make_pixel();
////
////  goto_xy(3, 4);
////  make_pixel();
////
////  goto_xy(4, 4);
////  make_pixel();
////
////  goto_xy(5, 4);
////  make_pixel();
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

//}
