#include  <Servo.h>



// **********MACROS*********

#define EMERGENCY_STOP    3 
#define IN3               12          // IN3 PIN OF L298 MOTOR DRIVER
#define IN4               13          // IN4 PIN OF L298 MOTOR DRIVER
#define POT               A0          // ADJUST SPEED
#define SYS_START         4           // If pressed once,the system starts and runs until EMERGENCY_STOP is pressed 
#define ENB               6           // ENB PIN OF L298 MOTOR DRIVER
#define IR_CONVEYOR       2           // CONVEYOR SENSOR PIN
#define IR_RIGHT          A1          // RIGHT BOX COUNTER SENSOR
#define IR_LEFT           A2          // LEFT BOX COUNTER SENSOR
#define BUZZER            8           // When order is finished buzzer runs as alert
#define OK_BUTTON         7           // If OK_BUTTON is pressed BUZZER stops 

#define  MAX_PRODUCT_NUM  8           // Number of products.More than number of these products can not be ordered.

#define D_gripper         65          // Robot arm default gripper position
#define D_y_axis          130         // Robot arm default y axis position
#define D_z_axis          92          // Robot arm default z axis position
#define D_x_axis          170         // Robot arm default x axis position

#define P_gripper         0           // Robot arm getting product gripper position
#define P_y_axis          130         // Robot arm getting product y axis position
#define P_z_axis          92          // Robot arm getting product z axis position
#define P_x_axis          130         // Robot arm getting product x axis position

#define L_gripper         30          // Robot arm putting product to the left gripper position
#define L_y_axis          70          // Robot arm putting product to the left y axis position
#define L_z_axis          177         // Robot arm putting product to the left z axis position
#define L_x_axis          180         // Robot arm putting product to the left x axis position

#define R_gripper         30          // Robot arm putting product to the right gripper position
#define R_y_axis          70          // Robot arm putting product to the right y axis position  
#define R_z_axis          2           // Robot arm putting product to the right z axis position         
#define R_x_axis          180         // Robot arm putting product to the right x axis position        



// **********VARIABLES*********

Servo gripper;                        // Gripper object of 4 axis robot arm
Servo y_axis;                         // y_axis object of  4 axis robot arm   
Servo z_axis;                         // z_axis object of  4 axis robot arm   
Servo x_axis;                         // x_axis object of  4 axis robot arm   

int box_flag = 0;                     // for selecting box. (1 -> left box    2 - > right box)

int left_box_order = 0;               // The number of ordered for left box  
int left_box_counter = 0;             // left box counter
int left_box_counter_safety = 0;      // This is for count products safely.Checks whether robotic arm is on the box

int right_box_order = 0;              // The number of ordered for right box  
int right_box_counter = 0;            // right box counter
int right_box_counter_safety = 0;     // This is for count products safely.Checks whether robotic arm is on the box
  
boolean sys_start_flag = false;       // This logically reads the SYS_START button.
boolean emr_stop_flag = false;        // This logically reads the EMERGENCY_STOP button.
boolean ok_button_flag = false;       // This logically reads the OK_BUTTON button.



// **********FUNCTIONS*********

int order_func1();                    // These two functions contain selecting box,taking number of ordered
void order_func2(int choice);         // and some error preventer algorithms about order 
void count();                         // to count products in left or right box
void order_status();                  // This function shows status of order

void motor_on();                      // Motor runs
void motor_off();                     // Motor stops
void MOTOR_EMERGENCY_STOP();          // Emergency stop interrupt function

void robot_active();
// Below 6 functions are branched from this robot_active function
void robot_default(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos);
// Robot arm does not move and waits in its default position to get product
void robot_get_product(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos);
// Robot arm gets the product which is on conveyor belt
void robot_put_to_left(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos);
// Robot arm puts the product to the left box
void robot_turn_left_default();
// Robot arm returns to its default position after putting product to the left box
void robot_put_to_right(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos);
// Robot arm puts the product to the right box
void robot_turn_right_default();
// Robot arm returns to its default position after putting product to the right box




void setup() {
  
  Serial.begin(9600);
  
  pinMode(EMERGENCY_STOP,INPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);
  pinMode(SYS_START,INPUT);
  pinMode(ENB,OUTPUT);
  pinMode(IR_CONVEYOR,INPUT);
  pinMode(IR_LEFT,INPUT);
  pinMode(IR_RIGHT,INPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(OK_BUTTON,INPUT);
  
  gripper.attach(5);    
  y_axis.attach(9);   
  z_axis.attach(10);  
  x_axis.attach(11);  
  
  robot_default(D_gripper,D_y_axis,D_z_axis,D_x_axis);  // Robot arm waits its default position before the system starts

  order_func2(order_func1());                           // Order is taken before the system starts
  
  digitalWrite(IN3,LOW);                                // Motor is in stop mode before the system starts
  digitalWrite(IN4,LOW);                                // to start the system SYS_START button should be pressed once
  
  attachInterrupt(1,MOTOR_EMERGENCY_STOP,RISING);       // Motor emergency stop interrupt function declaration
}


void loop() {
    
    if(digitalRead(SYS_START) == 1){
       sys_start_flag = true;
       emr_stop_flag = false;
    }
    
    if((sys_start_flag) && (digitalRead(IR_CONVEYOR) == 1) && ((right_box_counter + left_box_counter) < (left_box_order + right_box_order))){
       motor_on(); 
    }

    if((digitalRead(IR_CONVEYOR) == 0) && (sys_start_flag)){  
       motor_off();
       robot_active();
    }

    if((right_box_counter + left_box_counter) == (left_box_order + right_box_order)){
      
        if(digitalRead(OK_BUTTON) == 1){
          ok_button_flag = true;
        }
        
        if(ok_button_flag == false){
          digitalWrite(BUZZER,HIGH);

          for(static int i = 0; i < 1; i++){
               Serial.println("********************************************************");
               Serial.print(left_box_counter);
               Serial.println(" pieces of product is in the left box");
               Serial.print(right_box_counter);
               Serial.println(" pieces of product is in the right box");
               Serial.println(".....ORDER IS COMPLETED.....");
               Serial.println("********************************************************");
          }
        }
        
        else if(ok_button_flag == true){
          digitalWrite(BUZZER,LOW);         
        }
    }
}

int order_func1(){
  
  box:
  
  Serial.println("**********************");
  Serial.println("------SELECT BOX------");
  Serial.println("FOR LEFT BOX ----->  1");
  Serial.println("FOR RIGHT BOX -----> 2");
  Serial.println("**********************");

  while(1){
    
    if(Serial.available() > 0){

      box_flag = Serial.read();
      box_flag = box_flag - 48;
      break;
    }
  }
    
    if(box_flag == 1){
      Serial.println("\n-------------LEFT BOX SELECTED------------");
      left_box:
      Serial.println("******************************************");
      Serial.println("Enter the number of orders in the left box");
      Serial.print("Number of max products : ");
      Serial.println(MAX_PRODUCT_NUM);
      Serial.println("******************************************");
      
      while(1){
      
        if(Serial.available() > 0){
    
          left_box_order = Serial.read();
          left_box_order = left_box_order - 48;
          Serial.print("\nEntered number : ");
          Serial.println(left_box_order);

            if(left_box_order > MAX_PRODUCT_NUM){
                Serial.println("This number is greater than number of max products...");
                Serial.println("Left box order unsuccessful order again...\n");
                goto left_box;
            }
            else{
                Serial.println("Left box order successful...\n");
                break;
            }
          
         }
      }
    }

    else if(box_flag == 2){
      Serial.println("\n-------------RIGHT BOX SELECTED------------");
      right_box:
      Serial.println("*******************************************");
      Serial.println("Enter the number of orders in the right box");
      Serial.print("Number of max products : ");
      Serial.println(MAX_PRODUCT_NUM);
      Serial.println("*******************************************");
      
      while(1){
      
        if(Serial.available() > 0){
    
          right_box_order = Serial.read();
          right_box_order = right_box_order - 48;
          Serial.print("\nEntered number : ");
          Serial.println(right_box_order);

            if(right_box_order > MAX_PRODUCT_NUM){
                Serial.println("This number is greater than number of max products...");
                Serial.println("Right box order unsuccessful order again...\n");
                goto right_box;
            }
            else{
                Serial.println("Right box order successful...\n");
                break;
            }
          
         }
      }
    }

    else{
      Serial.println("\nNo box could be selected.Select box again...\n");
      goto box;    
    }

  return box_flag;
}


void order_func2(int choice){

  if((choice == 1) && (left_box_order != MAX_PRODUCT_NUM)){

      right_box:
      Serial.println("**************************************************");
      Serial.println("Enter the number of orders in the right box");
      Serial.print("Number of max available products for right box : ");
      Serial.println(MAX_PRODUCT_NUM - left_box_order);
      Serial.println("**************************************************");
      
      while(1){
      
        if(Serial.available() > 0){
    
          right_box_order = Serial.read();
          right_box_order = right_box_order - 48;
          Serial.print("\nEntered number : ");
          Serial.println(right_box_order);

            if((right_box_order) > (MAX_PRODUCT_NUM - left_box_order)){
                Serial.println("This number is greater than number of max available products...");
                Serial.println("Right box order unsuccessful order again...\n");
                goto right_box;
            }
            else{
                Serial.println("Right box order successful...\n");
                break;
            }
          
         }
      }
  }


  if((choice == 1) && (left_box_order == MAX_PRODUCT_NUM)){

      Serial.println("***********************************");
      Serial.println("ALL PRODUCTS ARE IN THE LEFT BOX...");
      Serial.println("************************************\n");
      right_box_order = 0;
  }


  else if((choice == 2) && (right_box_order != MAX_PRODUCT_NUM)){

      left_box:
      Serial.println("*************************************************");
      Serial.println("Enter the number of orders in the left box");
      Serial.print("Number of max available products for left box : ");
      Serial.println(MAX_PRODUCT_NUM - right_box_order);
      Serial.println("*************************************************");
      
      while(1){
      
        if(Serial.available() > 0){
    
          left_box_order = Serial.read();
          left_box_order = left_box_order - 48;
          Serial.print("\nEntered number : ");
          Serial.println(left_box_order);

            if((left_box_order) > (MAX_PRODUCT_NUM - right_box_order)){
                Serial.println("This number is greater than number of max available products...");
                Serial.println("Left box order unsuccessful order again...\n");
                goto left_box;
            }
            else{
                Serial.println("Left box order successful...\n");
                break;
            }
          
         }
      }
  }

  

  else if((choice == 2) && (right_box_order == MAX_PRODUCT_NUM)){

      Serial.println("************************************");
      Serial.println("ALL PRODUCTS ARE IN THE RIGHT BOX...");
      Serial.println("*************************************\n");
      left_box_order = 0;
  }

  Serial.println("*****************************************************");
  Serial.println("PLEASE CLEAR THE SCREEN IN 8 SECONDS...");
  Serial.println("SYSTEM WILL BE READY TO BE STARTED 8 SECONDS LATER...");
  Serial.println("*****************************************************");
  
  delay(8000);
}

void count(){
  
   if(right_box_counter < right_box_counter_safety){
    
      Serial.println("***********ROBOT PUTS PRODUCT IN RIGHT BOX**************");
      right_box_counter++;
   }

   if(left_box_counter < left_box_counter_safety){
    
      Serial.println("***********ROBOT PUTS PRODUCT IN LEFT BOX***************");
      left_box_counter++;
   }

}

void order_status(){

   Serial.println("**********************ORDER STATUS**********************");

   Serial.print("LEFT BOX ORDERED -->> ");
   Serial.println(left_box_order);
   Serial.print("LEFT BOX COUNTER = ");
   Serial.println(left_box_counter);
   
   if(left_box_order == left_box_counter){
      for(static int i = 0; i < 1; i++){
         Serial.println("LEFT BOX ORDER IS COMPLETED...");
      }
   }
   
   Serial.println("--------------------------------------------");
   
   Serial.print("RIGHT BOX ORDERED -->> ");
   Serial.println(right_box_order);
   Serial.print("RIGHT BOX COUNTER = ");
   Serial.println(right_box_counter);
   
   if(right_box_order == right_box_counter){
      for(static int j = 0; j < 1; j++){
         Serial.println("RIGHT BOX ORDER IS COMPLETED...");
      }
   }
   
   Serial.println("********************************************************\n\n\n");
 }


void motor_on(){
  
    int Avalue = analogRead(POT);    
      
    analogWrite((ENB),(Avalue * 0.249266862));     

    digitalWrite(IN3,HIGH);     
    digitalWrite(IN4,LOW);      
}

void motor_off(){

    analogWrite(ENB,0);      
    digitalWrite(IN3,LOW);   
    digitalWrite(IN4,LOW);   
}

void MOTOR_EMERGENCY_STOP(){   
      
    emr_stop_flag = true;  
    sys_start_flag = false;
    
    motor_off();
}

void robot_active(){
    
    static int L = left_box_order;
    static int R = right_box_order;
    
    if((R != 0) && (L == 0)){
       robot_get_product(P_gripper,P_y_axis,P_z_axis,P_x_axis);
       robot_put_to_right(R_gripper,R_y_axis,R_z_axis,R_x_axis);
       robot_turn_right_default();
  
       R--;
    }
  
    if(L != 0){
       robot_get_product(P_gripper,P_y_axis,P_z_axis,P_x_axis);
       robot_put_to_left(L_gripper,L_y_axis,L_z_axis,L_x_axis);
       robot_turn_left_default();
  
       L--;
   }

   order_status();
}

void robot_default(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos){
  
  gripper.write(gripper_pos);
  y_axis.write(y_axis_pos);
  z_axis.write(z_axis_pos);
  x_axis.write(x_axis_pos);
}



void robot_get_product(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos){

  const int step = 5;
  const int time = 150;
  
  for(int i = y_axis_pos; i >= 100; i-=step){
     y_axis.write(i);
     delay(time);  
  }

  for(int i = D_x_axis; i >= x_axis_pos; i-=step){
     x_axis.write(i);
     delay(time);   
  }

  for(int i = 100; i <= y_axis_pos; i+=step){
     y_axis.write(i);
     delay(time);  
  }

  gripper.write(gripper_pos);
}



void robot_put_to_left(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos){

    left_box_counter_safety++;
    
    const int step = 5;
    const int time = 150;
    
    for(int i = P_x_axis; i <= x_axis_pos; i+=step){
       x_axis.write(i);
       delay(time);  
    }
    
     for(int i = P_z_axis; i <= z_axis_pos; i+=step){
       z_axis.write(i);
       delay(time);   
    }
    
    for(int i = x_axis_pos; i >= 145; i-=step){
       x_axis.write(i);
       delay(time);

       if(digitalRead(IR_LEFT) == 0){
          count();
       }
    }

    gripper.write(gripper_pos);

    for(int i = 145; i <= x_axis_pos; i+=step){
       x_axis.write(i);
       delay(time);  
    }

}

void robot_turn_left_default(){

    const int step = 5;
    const int time = 150;

    for(int i = L_z_axis; i >= D_z_axis; i-= step){
       z_axis.write(i);
       delay(time);  
    }

    for(int i = L_x_axis; i >= D_x_axis; i-= step){
       x_axis.write(i);
       delay(time);  
    }

    gripper.write(D_gripper);
}


void robot_put_to_right(const int gripper_pos,const int y_axis_pos,const int z_axis_pos,const int x_axis_pos){
  
    right_box_counter_safety++;

    const int step = 5;
    const int time = 150;
    
    for(int i = P_x_axis; i <= x_axis_pos; i+=step){
       x_axis.write(i);
       delay(time);  
    }
    
    for(int i = P_z_axis; i >= z_axis_pos; i-=step){
       z_axis.write(i);
       delay(time);   
    }
       
    for(int i = x_axis_pos; i >= 145; i-=step){
       x_axis.write(i);
       delay(time);

       if(digitalRead(IR_RIGHT) == 0){
          count();
       }
    }
    
    gripper.write(gripper_pos);
    
    for(int i = 145; i <= x_axis_pos; i+=step){
       x_axis.write(i);
       delay(time);  
    }
     
}

void robot_turn_right_default(){

    const int step = 5;
    const int time = 150;

    for(int i = R_z_axis; i <= D_z_axis; i+= step){
       z_axis.write(i);
       delay(time);  
    }

    for(int i = R_x_axis; i >= D_x_axis; i-= step){
       x_axis.write(i);
       delay(time);  
    }

    gripper.write(D_gripper);
}
