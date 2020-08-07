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
    digitalWrite(stepYDir, HIGH);
    for(int y = 0; y < steps; y++) {
      digitalWrite(stepYPin,HIGH); 
      delayMicroseconds(stepperDelay); 
      digitalWrite(stepYPin,LOW); 
      delayMicroseconds(stepperDelay); 
    }
  } else {
    digitalWrite(stepYDir, LOW);
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
    goto_xy(pos_x, pos_y - 1);
  }
  goto_xy(pos_x, pos_y + 200);
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
 String payload = Serial.readStringUntil("\n");
 previous_index = -1;
 next_index = payload.indexOf(";", 0);
 while (next_index != -1) {
   String line = payload.substring(previous_index + 1, next_index);
   previous_index = next_index;
   next_index = payload.indexOf(";", previous_index + 1);
 
   //  String payload = Serial.readStringUntil("\n");
   String command = line.substring(0, 1);
   String args = line.substring(1);
   if (command == "C") {
     int index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     int index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     int index3 = args.indexOf(",", index2 + 1);
     x2 = args.substring(index2 + 1, index3).toFloat();
     int index4 = args.indexOf(",", index3 + 1);
     y2 = args.substring(index3 + 1, index4).toFloat();
     int index5 = args.indexOf(",", index4 + 1);
     x3 = args.substring(index4 + 1, index5).toFloat();
     int index6 = args.indexOf("\r");
     y3 = args.substring(index5 + 1, index6).toFloat();
 //    Serial.println("ack " + command + " " + x1 + " " + y1 + " " + x2 + " " + y2 + " " + x3 + " " + y3);
     C(x1, y1, x2, y2, x3, y3);
//      Serial.println("ack");
   } else if (command == "M") {
     int index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     int index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     M(x1, y1);
//      Serial.println("ack");
   } else if (command == "L") {
     int index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     int index2 = args.indexOf(",", index1 + 1);
     y1 = args.substring(index1 + 1, index2).toFloat();
     L(x1, y1);
//      Serial.println("ack");
   } else if (command == "H") {
     int index1 = args.indexOf(",");
     x1 = args.substring(0, index1).toFloat();
     H(x1);
//      Serial.println("ack");
   } else if (command == "V") {
     int index1 = args.indexOf(",");
     y1 = args.substring(0, index1).toFloat();
     V(y1);
//      Serial.println("ack");
   } else if (command == "Z") {
     Z();
//      Serial.println("ack");
   } else if (command == "R") {
     reset();
//      Serial.println("ack");
   }
 }
 Serial.println("ack");
};
void _loop() {
//  Serial.println(digitalRead(switch_x));
  delay(15000);
//  go_home();
M(50.0,50.0);
//M(0.0,1000.0);
//M(1000.0,1000.0);
//M(0.0,1000.0);
//M(1000.0,1000.0);
//M(0.0,1000.0);
//M(1000.0,1000.0);
//M(0.0,1000.0);
//M(1000.0,1000.0);
//M(0.0,1000.0);
delay(10000);
//M(1879.8,3954.0);
//C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);
//M(1879.8,3954.0);
//C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);
//M(1879.8,3954.0);
//C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);
//M(1879.8,3954.0);
//C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);
//M(1879.8,3954.0);
//C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);

}
void bla() {
M(1879.8,3954.0);
C(1793.5,3944.9,1694.8,3930.1,1660.5,3921.1);
C(1557.3,3894.1,1460.7,3812.2,1426.5,3722.6);
C(1397.6,3646.8,1397.6,3639.9,1426.7,3575.3);
C(1443.3,3538.6,1483.6,3482.3,1516.0,3450.9);
C(1548.1,3419.8,1574.7,3388.7,1574.7,3382.2);
C(1574.7,3375.8,1547.7,3325.7,1514.8,3271.0);
C(1429.3,3129.0,1383.2,2979.7,1371.3,2805.6);
C(1365.7,2722.7,1354.2,2658.5,1345.9,2663.7);
C(1308.7,2686.6,1260.3,2820.3,1226.0,2994.9);
C(1180.2,3228.0,1141.6,3319.3,1048.1,3414.7);
C(953.0,3511.9,829.7,3558.0,664.8,3557.8);
C(325.3,3557.3,141.2,3292.7,256.5,2970.0);
C(280.5,2902.7,297.0,2900.9,374.0,2957.7);
C(445.0,3010.2,479.4,3011.7,522.7,2963.9);
C(551.0,2932.7,556.7,2891.2,564.3,2655.2);
C(572.3,2406.2,576.9,2375.4,620.3,2282.6);
C(680.8,2153.3,850.2,1976.7,980.6,1907.2);
C(1258.9,1758.9,1499.9,1773.7,1679.9,1950.5);
C(1751.5,2020.8,1761.8,2025.8,1783.0,2000.2);
C(1820.0,1955.6,1998.1,1872.5,2117.5,1844.4);
C(2301.1,1801.1,2598.5,1817.1,2759.3,1879.0);
C(2780.6,1887.3,2789.3,1875.4,2797.2,1826.5);
C(2802.6,1793.1,2828.0,1729.2,2853.4,1686.0);
C(2904.0,1599.5,3002.1,1542.3,3101.2,1541.6);
C(3148.3,1541.3,3175.7,1522.2,3266.1,1426.2);
C(3326.7,1361.9,3397.2,1263.2,3428.3,1199.1);
C(3458.2,1137.5,3502.3,1065.7,3526.0,1040.2);
C(3554.8,1009.3,3572.4,965.3,3579.3,906.9);
C(3593.8,785.3,3619.7,703.7,3665.0,637.1);
C(3744.7,520.2,3798.7,523.8,3860.7,650.7);
C(3879.4,689.1,3900.8,727.0,3907.7,733.9);
C(3914.6,740.7,3955.5,713.9,3998.6,674.2);
C(4041.0,635.1,4087.3,602.6,4100.8,602.6);
C(4128.4,602.6,4181.4,675.6,4181.4,714.0);
C(4181.4,763.7,4234.0,793.7,4322.0,793.7);
C(4401.5,793.7,4406.9,790.9,4406.9,749.3);
C(4406.9,725.6,4394.8,682.3,4380.1,653.7);
C(4325.4,548.0,4409.0,550.5,4500.7,657.7);
L(4557.4,723.8);
L(4790.9,476.7);
C(5018.5,235.9,5024.6,231.0,5029.8,283.9);
C(5032.7,313.4,5019.5,368.5,5000.2,406.2);
C(4972.2,461.1,4891.8,656.9,4708.6,1116.0);
C(4704.9,1125.1,4744.5,1151.2,4797.0,1172.9);
C(4916.3,1222.3,5045.8,1335.3,5087.2,1426.7);
C(5148.2,1560.9,5113.6,1796.1,5014.5,1918.4);
C(4904.2,2054.6,4714.2,2132.1,4489.7,2132.1);
H(4354.1);
V(2234.3);
C(4355.4,2359.1,4314.8,2603.7,4267.0,2767.7);
C(4247.5,2834.3,4227.5,2903.0,4222.6,2919.8);
C(4215.9,2942.8,4232.8,2958.4,4292.2,2983.8);
C(4398.9,3029.6,4431.5,3083.8,4418.3,3195.2);
C(4403.8,3317.2,4335.3,3457.3,4260.6,3516.8);
C(4222.6,3547.1,4197.9,3582.0,4197.9,3605.4);
C(4197.9,3658.4,4099.2,3765.8,4015.1,3804.0);
C(3842.5,3882.4,3640.6,3813.8,3613.1,3666.9);
C(3606.7,3633.1,3593.8,3611.7,3582.4,3616.5);
C(3305.8,3734.4,3150.3,3776.3,2906.2,3798.7);
C(2735.6,3814.4,2460.1,3799.4,2357.2,3768.7);
L(2300.9,3752.0);
V(3818.5);
C(2300.9,3856.8,2287.4,3901.1,2268.6,3924.3);
C(2230.1,3971.9,2118.7,3980.6,1874.3,3955.0);
L(1879.8,3954.0);
Z();
M(2215.1,3894.8);
C(2239.0,3887.5,2244.5,3786.5,2222.9,3752.5);
C(2215.7,3741.2,2174.6,3719.2,2131.9,3704.1);
C(2060.6,3678.8,2053.6,3678.9,2037.5,3706.3);
C(2028.2,3722.2,1994.8,3769.7,1963.7,3811.5);
C(1933.0,3852.7,1907.5,3890.4,1907.5,3894.6);
C(1907.5,3914.3,2151.7,3914.2,2216.8,3894.3);
L(2215.1,3894.8);
Z();
M(1847.1,3844.3);
C(1866.9,3830.5,1913.7,3771.1,1951.1,3712.5);
C(2004.2,3629.1,2026.0,3608.3,2053.5,3614.9);
C(2072.3,3619.3,2182.1,3647.9,2297.1,3678.2);
C(2570.2,3750.2,2840.6,3766.2,3054.5,3722.9);
C(3135.2,3706.6,3298.3,3654.4,3416.5,3607.2);
C(3534.2,3560.2,3637.4,3525.5,3645.2,3530.3);
C(3653.0,3535.2,3668.9,3578.4,3680.4,3626.4);
C(3717.5,3781.0,3818.6,3819.0,3986.5,3740.9);
C(4159.6,3660.3,4190.5,3558.1,4089.5,3398.7);
C(3961.4,3196.5,3962.7,3202.2,4028.0,3127.8);
C(4165.7,2971.0,4262.9,2660.0,4283.0,2311.4);
L(4294.1,2120.0);
L(4196.3,2089.1);
C(4110.1,2061.9,4045.3,2010.3,4045.3,1968.8);
C(4045.3,1941.7,4102.7,1956.9,4150.4,1997.0);
C(4212.9,2049.6,4396.6,2085.0,4539.9,2071.6);
C(4827.8,2044.7,5022.4,1887.0,5062.3,1647.9);
C(5100.2,1420.7,4865.2,1185.9,4605.0,1191.8);
C(4554.9,1192.9,4498.4,1181.2,4462.0,1161.8);
C(4415.3,1137.1,4361.2,1130.0,4211.5,1129.1);
L(4021.5,1128.0);
L(3957.3,1189.2);
C(3875.9,1266.9,3854.7,1344.4,3854.5,1564.2);
C(3854.4,1764.1,3839.3,1830.9,3777.1,1904.8);
C(3752.8,1933.7,3728.2,1978.5,3722.6,2004.1);
C(3691.3,2148.4,3627.9,2232.2,3499.7,2298.4);
C(3426.5,2336.3,3387.3,2344.4,3271.8,2345.1);
C(3153.4,2345.9,3118.5,2338.9,3031.8,2296.3);
C(2917.8,2240.3,2838.0,2145.8,2806.9,2029.5);
L(2787.5,1957.0);
L(2647.1,1918.8);
C(2468.0,1870.1,2306.2,1858.2,2179.5,1884.5);
C(1822.3,1958.7,1491.3,2298.1,1434.9,2648.6);
C(1403.7,2842.5,1464.2,3081.8,1593.8,3275.9);
C(1631.2,3331.8,1662.0,3381.5,1662.0,3385.9);
C(1662.0,3390.1,1618.9,3437.1,1566.2,3490.2);
C(1488.3,3568.7,1471.2,3596.6,1471.5,3644.9);
C(1472.0,3718.0,1510.1,3767.2,1610.5,3824.1);
C(1697.4,3873.3,1792.8,3881.1,1847.0,3843.1);
L(1847.1,3844.3);
Z();
M(4206.1,1618.3);
C(4144.5,1585.8,4083.6,1487.3,4083.6,1420.1);
C(4083.6,1294.2,4162.1,1222.0,4299.4,1222.0);
C(4362.0,1222.0,4389.8,1232.4,4435.2,1272.9);
C(4486.0,1318.3,4491.6,1332.7,4491.6,1420.2);
C(4491.6,1504.4,4485.0,1522.8,4437.7,1570.1);
C(4375.9,1631.9,4272.9,1652.8,4205.1,1617.0);
L(4206.1,1618.3);
Z();
M(4318.8,1542.6);
C(4318.8,1524.4,4305.1,1506.8,4288.3,1503.6);
C(4258.3,1497.8,4244.8,1550.6,4270.4,1576.3);
C(4290.5,1596.3,4318.9,1576.9,4318.9,1542.2);
L(4318.8,1542.6);
Z();
M(4414.6,1456.7);
C(4414.6,1442.4,4407.2,1430.6,4398.2,1430.6);
C(4376.8,1430.6,4361.0,1457.8,4375.0,1471.7);
C(4394.1,1490.8,4414.6,1483.2,4414.6,1456.2);
L(4414.6,1456.7);
Z();
M(4310.5,1421.4);
C(4336.7,1389.8,4309.9,1314.1,4266.5,1297.5);
C(4231.2,1284.0,4153.8,1337.1,4153.8,1375.4);
C(4153.8,1443.9,4265.0,1476.5,4311.0,1421.1);
L(4310.5,1421.4);
Z();
M(4336.8,3287.2);
C(4380.3,3140.8,4372.5,3097.8,4294.6,3054.0);
C(4257.9,3033.4,4220.4,3016.2,4211.7,3016.2);
C(4193.0,3016.2,4062.7,3206.2,4070.4,3222.5);
C(4073.3,3228.7,4106.1,3292.5,4143.2,3364.2);
L(4210.2,3493.7);
L(4260.8,3433.1);
C(4288.7,3399.8,4323.0,3333.8,4337.0,3286.6);
L(4336.8,3287.2);
Z();
M(687.4,3416.1);
C(747.1,3384.2,774.9,3297.2,800.2,3062.9);
C(827.5,2811.3,849.9,2723.5,929.8,2556.2);
C(1039.7,2325.8,1308.5,2174.6,1509.9,2230.2);
C(1567.9,2246.2,1576.2,2244.3,1593.7,2210.2);
C(1604.1,2190.0,1628.3,2156.1,1647.1,2135.4);
C(1678.8,2100.5,1679.1,2095.7,1652.2,2054.6);
C(1601.5,1977.2,1518.2,1951.6,1340.6,1959.4);
C(1142.9,1968.1,1056.8,2006.7,913.5,2151.3);
C(840.0,2225.5,787.6,2301.5,731.9,2415.0);
C(657.4,2566.6,652.8,2585.2,627.6,2832.8);
C(598.9,3115.4,581.8,3156.4,492.2,3156.4);
C(425.1,3156.4,329.5,3114.7,306.5,3075.3);
C(292.0,3050.2,287.9,3066.9,287.5,3154.1);
C(286.9,3289.0,325.0,3376.9,398.9,3409.1);
C(462.4,3436.7,641.0,3440.9,688.2,3415.6);
L(687.4,3416.1);
Z();
M(3486.7,2232.5);
C(3568.0,2186.7,3627.1,2114.9,3641.1,2044.5);
L(3650.7,1996.5);
L(3514.3,2003.4);
C(3402.9,2009.0,3369.0,2017.8,3329.5,2051.0);
C(3273.4,2098.2,3184.7,2118.7,3102.5,2103.3);
C(3039.5,2091.5,2958.2,2017.0,2949.1,1962.3);
C(2944.2,1933.1,2955.9,1926.4,3030.4,1916.5);
C(3077.0,1910.4,3131.0,1902.5,3149.1,1899.3);
C(3167.0,1896.1,3185.0,1902.3,3188.9,1913.2);
C(3199.8,1944.1,3143.9,1988.8,3092.8,1989.4);
C(3048.0,1989.9,3047.8,1990.6,3081.7,2016.8);
C(3122.4,2048.2,3237.3,2037.8,3277.7,1997.4);
C(3302.5,1972.6,3301.5,1967.7,3266.2,1943.0);
C(3198.2,1895.4,3170.9,1818.8,3182.7,1706.3);
L(3193.2,1605.8);
H(3106.9);
C(3036.9,1605.8,3011.5,1615.0,2961.4,1658.9);
C(2884.4,1726.5,2859.0,1805.6,2867.4,1952.9);
C(2876.8,2119.2,2952.5,2212.0,3125.6,2268.8);
C(3207.4,2295.6,3409.3,2275.2,3486.2,2232.0);
L(3486.7,2232.5);
Z();
M(3594.5,1934.5);
C(3698.2,1912.2,3761.4,1848.2,3777.4,1749.1);
C(3783.2,1713.7,3794.9,1596.0,3803.3,1488.5);
C(3821.0,1261.8,3861.2,1157.9,3956.4,1093.2);
C(4012.5,1055.0,4028.8,1052.7,4177.4,1061.6);
C(4411.4,1075.5,4477.9,1037.1,4509.5,869.1);
C(4515.6,836.3,4515.4,791.9,4508.8,771.2);
C(4497.8,736.3,4495.1,737.7,4468.1,791.8);
C(4442.6,842.8,4432.5,848.1,4350.2,852.9);
C(4301.7,855.7,4214.3,845.9,4157.0,831.0);
C(4100.2,816.3,4028.5,804.2,3998.1,804.2);
H(3943.1);
L(3948.3,938.7);
L(3952.2,1073.3);
L(3887.9,1086.7);
C(3806.3,1103.6,3746.0,1134.9,3642.0,1214.3);
C(3584.4,1258.3,3554.5,1271.5,3541.5,1258.5);
C(3528.7,1245.7,3531.9,1230.7,3552.4,1208.0);
C(3567.9,1190.9,3581.0,1162.6,3581.0,1145.8);
C(3581.0,1085.3,3536.0,1131.7,3467.8,1263.1);
C(3427.6,1340.5,3372.1,1417.6,3333.6,1449.1);
C(3264.1,1506.1,3242.7,1554.8,3307.3,1509.6);
C(3329.2,1494.3,3381.6,1483.6,3438.4,1483.6);
C(3524.3,1483.6,3537.4,1489.2,3601.6,1553.3);
C(3668.5,1620.3,3670.5,1625.6,3662.9,1718.0);
C(3651.7,1853.2,3615.2,1888.7,3496.6,1878.8);
C(3432.3,1873.5,3393.4,1859.4,3361.9,1829.8);
C(3302.9,1774.4,3311.4,1719.6,3380.5,1712.0);
C(3408.4,1708.9,3449.3,1690.1,3470.6,1670.3);
C(3506.7,1636.8,3509.8,1636.5,3521.0,1665.7);
C(3535.5,1703.4,3510.4,1745.6,3456.4,1773.0);
L(3416.0,1793.4);
L(3463.0,1811.3);
C(3545.8,1842.8,3597.5,1796.5,3597.5,1691.1);
C(3597.5,1629.7,3537.2,1560.0,3468.0,1542.6);
C(3361.8,1516.0,3249.9,1621.8,3249.8,1749.9);
C(3249.7,1858.8,3291.9,1906.7,3415.2,1936.4);
C(3489.9,1954.4,3499.4,1954.3,3595.9,1933.6);
L(3594.5,1934.5);
Z();
M(3726.5,1073.0);
C(3764.8,1049.4,3813.8,1030.0,3835.3,1030.0);
C(3870.8,1029.9,3874.2,1021.7,3874.2,937.2);
C(3874.2,822.5,3833.2,680.9,3787.5,639.6);
C(3753.6,608.9,3752.1,610.1,3709.3,703.6);
C(3647.2,839.3,3603.0,1118.0,3644.5,1116.2);
C(3651.5,1116.0,3688.7,1096.4,3727.1,1072.8);
L(3726.5,1073.0);
Z();
M(4848.2,667.4);
C(4867.5,595.9,4868.0,572.4,4850.9,551.7);
C(4832.4,529.5,4820.5,533.3,4761.6,580.7);
C(4725.2,609.9,4694.6,642.6,4694.6,652.4);
C(4694.6,679.4,4786.2,769.2,4807.0,762.4);
C(4816.7,759.1,4835.8,716.1,4849.1,666.7);
L(4848.2,667.4);
Z();
M(4135.9,741.3);
C(4135.9,735.9,4127.4,715.6,4117.1,696.3);
C(4099.3,663.0,4097.4,662.9,4064.0,693.1);
C(4032.0,722.0,4031.9,724.2,4062.0,736.0);
C(4100.5,751.2,4136.1,753.6,4136.1,740.7);
L(4135.9,741.3);
Z();
M(5000.0,5000.0);
delay(60000);
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
////    stepper_x_step(value);
////  };
////  if (axis == "y") {
////    stepper_y_step(value);
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
//  stepper_y_step(1000);
//  stepper_x_step(1000);
//  stepper_y_step(-1000);
//  stepper_x_step(-1000);
//  delay(100);
//  servo_z.write(180);
//  delay(200);
//  stepper_y_step(-100);
//  stepper_x_step(100);

//  stepper_y_step(random(-1000, 1000));
//  stepper_x_step(random(-1000, 1000));
//  delay(1000);
//  stepper_y_step(-500);
//  stepper_x_step(-500);
//  delay(1000);

//}
