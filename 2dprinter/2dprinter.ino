//#include <SPI.h>
//#include <SD.h>
#include <Stepper.h>
#include <Servo.h>
//#include <math.h>


// Stepper Motor X
const int stepXPin = 2;
const int stepXDir = 5;

// Stepper Motor Y
const int stepYPin = 3;
const int stepYDir = 6;

// Servo motor Z
const int servoZPin = 7;
Servo servo_z;

const int switch_x = 9;
const int switch_y = 10;

// Table max dimensions
const int maxX = 10840;  // mm / 10
const int maxY = 7320;  // mm / 10

const float steps_per_mm = 1.0;
const int maxSpeed = 1000; // mm/s
const int stepperDelay = 500000 / (steps_per_mm * maxSpeed); // [(1s * 1000000micro s) / 2] / (max speed * steps per milimiter)

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
int index1;
int index2;
int index3;
int index4;
int index5;
int index6;

void setup() {

  servo_z.attach(servoZPin);
  pinMode(switch_x, INPUT_PULLUP);
  pinMode(switch_y, INPUT_PULLUP);

  pinMode(stepXPin, OUTPUT); 
  pinMode(stepXDir, OUTPUT);
  pinMode(stepYPin, OUTPUT); 
  pinMode(stepYDir, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("started");
  servo_z.write(180);
  delay(5000);
  go_home();
}

void stepper_x_step(int steps) {
  if (steps > 0) {
    digitalWrite(stepXDir, LOW);
    for(int x = 0; x < steps; x++) {
      digitalWrite(stepXPin,HIGH); 
      delayMicroseconds(stepperDelay); 
      digitalWrite(stepXPin,LOW); 
      delayMicroseconds(stepperDelay);
    }
  } else {
    digitalWrite(stepXDir, HIGH);
    for(int x = 0; x > steps; x--) {
      digitalWrite(stepXPin,HIGH); 
      delayMicroseconds(stepperDelay); 
      digitalWrite(stepXPin,LOW); 
      delayMicroseconds(stepperDelay); 
    }
  }
}
void stepper_y_step(int steps) {
  if (steps > 0) {
    digitalWrite(stepYDir, LOW);
    for(int y = 0; y < steps; y++) {
      digitalWrite(stepYPin,HIGH); 
      delayMicroseconds(stepperDelay); 
      digitalWrite(stepYPin,LOW); 
      delayMicroseconds(stepperDelay); 
    }
  } else {
    digitalWrite(stepYDir, HIGH);
    for(int y = 0; y > steps; y--) {
      digitalWrite(stepYPin,HIGH); 
      delayMicroseconds(stepperDelay); 
      digitalWrite(stepYPin,LOW); 
      delayMicroseconds(stepperDelay); 
    }
  }
}


void go_home() {
  while (digitalRead(switch_x) == HIGH) {
    goto_xy(pos_x -1, pos_y);
  }
  goto_xy(pos_x + 200, pos_y);
  pos_x = 0.0;
  while (digitalRead(switch_y) == HIGH) {
    goto_xy(pos_x, pos_y + 1);
  }
  goto_xy(pos_x, pos_y - 200);
  pos_y = maxY;
}

void reset() {
  pos_x = 0.0;
  pos_y = maxY;
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
  servo_z.write(0);
  delay(500);
}
void stop_drawing() {
  servo_z.write(180);
  delay(500);
}
int goto_xy(int pix_x, int pix_y) {
  float new_x = pix_x;
  float new_y = pix_y;
  float mov_x = ((pos_x - new_x) * steps_per_mm);
  if (new_x <= maxX) {
    stepper_x_step(mov_x);
  }
  pos_x = new_x;
  float mov_y = ((pos_y - new_y) * steps_per_mm);
  if (new_y <= maxY) {
    stepper_y_step(mov_y);
  }
  pos_y = new_y;
}

void m(float x1, float y1) {
  M(pos_x + x1, pos_y + y1);
}
void M(float x0, float y0) {
  stop_drawing();
  goto_xy(x0, y0);  
  start_drawing();
}
void h(float x1){
  H(pos_x + x1);  
}
void H(float x1) {
  goto_xy(x1, pos_y);
}
void v(float y1) {
  return V(pos_y + y1);  
}
void V(float y1) {
  goto_xy(pos_x, y1);
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

}


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
 String payload = Serial.readStringUntil("\n");
 previous_index = -1;
 next_index = payload.indexOf(";", 0);
 while (next_index != -1) {
   String line = payload.substring(previous_index + 1, next_index);
   previous_index = next_index;
   next_index = payload.indexOf(";", previous_index + 1);
 
   String command = line.substring(0, 1);
   String args = line.substring(1);
   if (command == "C") {
     index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     index3 = args.indexOf(",", index2 + 1);
     x2 = args.substring(index2 + 1, index3).toFloat();
     index4 = args.indexOf(",", index3 + 1);
     y2 = args.substring(index3 + 1, index4).toFloat();
     index5 = args.indexOf(",", index4 + 1);
     x3 = args.substring(index4 + 1, index5).toFloat();
     index6 = args.indexOf("\r");
     y3 = args.substring(index5 + 1, index6).toFloat();
     C(x1, y1, x2, y2, x3, y3);
   } else if (command == "M") {
     index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     M(x1, y1);
   } else if (command == "L") {
     index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     L(x1, y1);
   } else if (command == "H") {
     index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     H(x1);
   } else if (command == "V") {
     index1 = args.indexOf(",");
     y1 = args.substring(0, index1).toFloat();
     V(y1);
   } else if (command == "Z") {
     Z();
   } else if (command == "R") {
     reset();
   }
 }
 Serial.println("ack");
};
