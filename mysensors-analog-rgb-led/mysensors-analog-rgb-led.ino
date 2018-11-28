// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable WiFi gateway
#define MY_GATEWAY_ESP8266

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

#define MY_ESP8266_SSID "xxx"
#define MY_ESP8266_PASSWORD "xxx"

#define CHILD_ID_LIGHT 1

#define SN "LED Strip"
#define SV "2.0"

// Enable UDP communication
//#define MY_USE_UDP

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
// #define MY_ESP8266_HOSTNAME "sensor-gateway"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
#define MY_IP_ADDRESS 192,168,0,120

// If using static ip you need to define Gateway and Subnet address as well
#define MY_IP_GATEWAY_ADDRESS 192,168,0,1
#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// The port to keep open on node server mode
#define MY_PORT 8888

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2

// Controller ip address. Enables client mode (default is "server" mode).
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere.
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE

// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3

#if defined(MY_USE_UDP)
#include <WiFiUdp.h>
#endif

#include <ESP8266WiFi.h>

#include <MySensors.h>

MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
MyMessage rgbMsg(CHILD_ID_LIGHT, V_RGB);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);

byte red = 255;
byte green = 255;
byte blue = 255;
byte r0 = 255;
byte g0 = 255;
byte b0 = 255;
char rgbstring[] = "ffffff";

int on_off_status = 0;
int dimmerlevel = 100;
int fadespeed = 1000;
int autofade_mode = 0;

#define REDPIN 0
#define GREENPIN 4
#define BLUEPIN 5

void setup()
{
  // Output pins
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
}

void presentation()
{
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo(SN, SV);
  present(CHILD_ID_LIGHT, S_RGBW_LIGHT);
}


void loop()
{
  static bool first_message_sent = false;
  if ( first_message_sent == false ) {
    Serial.println( "Sending initial state..." );
    set_hw_status(0);
    send_status();
    first_message_sent = true;
  }
  else {
    if(autofade_mode > 0) {
      fade();
    }
    else {
      set_hw_status(1);
      delay(1000);    
    }
  }
}

void receive(const MyMessage &message)
{
  int val;
  
  if (message.type == V_RGB) {
    Serial.println( "V_RGB command: " );
    Serial.println(message.data);
    long number = (long) strtol( message.data, NULL, 16);

    autofade_mode = 0;

    // Save old value
    strcpy(rgbstring, message.data);
    
    // Split it up into r, g, b values
    red = number >> 16;
    green = number >> 8 & 0xFF;
    blue = number & 0xFF;

    send_status();
    set_hw_status(0);

  } else if (message.type == V_RGBW) {    
    Serial.println( "V_RGBW command: " );
    Serial.println(message.data);
    long number = (long) strtol( message.data, NULL, 16);

    autofade_mode = 0;
    
    // Save old value
    strcpy(rgbstring, message.data);
    
    if (strlen(rgbstring) == 6) {
      Serial.println("new rgb value");
      // Split it up into r, g, b values
      red = number >> 16;
      green = number >> 8 & 0xFF;
      blue = number & 0xFF;
    } else if (strlen(rgbstring) == 9) {
      Serial.println("new rgbw value");
      // Split it up into r, g, b values
      red = fromhex (& rgbstring [7]);
      green = fromhex (& rgbstring [7]);
      blue = fromhex (& rgbstring [7]);
      //ToDo: tutaj trzeba zlozyc stringa z r+g+b zeby nie mrugalo jak sie na biale przelacza
      
      //strcpy(rgbstring, message.data);
    } else {
      Serial.println("Wrong length of input");
    }  


    send_status();
    set_hw_status(0);
    
  } else if (message.type == V_LIGHT || message.type == V_STATUS) {
    Serial.println( "V_LIGHT command: " );
    Serial.println(message.data);
    val = atoi(message.data);
    if (val == 0 or val == 1) {
      autofade_mode = 0;
      on_off_status = val;
      send_status();
      set_hw_status(0);
    }
    
  } else if (message.type == V_DIMMER || message.type == V_PERCENTAGE) {
    Serial.print( "V_DIMMER command: " );
    Serial.println(message.data);
    val = atoi(message.data);
    if (val >= 0 and val <=100) {
      autofade_mode = 0;
      dimmerlevel = val;
      send_status();
      set_hw_status(0);
    }
    
  } else if (message.type == V_VAR1 ) {
    Serial.print( "V_VAR1 command: " );
    Serial.println(message.data);
    val = atoi(message.data);
    if (val >= 0 and val <= 2000) {
      fadespeed = val;
      autofade_mode = 0;
    }
        
  } else if (message.type == V_VAR2 ) {
    Serial.print( "V_VAR2 command: " );
    Serial.println(message.data);
    val = atoi(message.data);
    if (val >= 0 and val <= 10) {
      autofade_mode = val;
      Serial.print( "Value: " );
      Serial.println( val );
    }
    else {
      Serial.println( "invalid Value!" );
    }
    
  } else {
    Serial.println( "Invalid command received..." );
    return;
  }

}

// converts hex char to byte
byte fromhex (const char * str)
{
  char c = str [0] - '0';
  if (c > 9)
    c -= 7;
  int result = c;
  c = str [1] - '0';
  if (c > 9)
    c -= 7;
  return (result << 4) | c;
}

void set_rgb(int r, int g, int b) {
  double r_proz = ((double)100/(double)254)*(double)r;
  r_proz = ((double)1023/(double)100)*r_proz;
  int r_set = (int)r_proz;
  
  double g_proz = ((double)100/(double)254)*(double)g;
  g_proz = ((double)1023/(double)100)*g_proz;
  int g_set = (int)g_proz;
  
  double b_proz = ((double)100/(double)254)*(double)b;
  b_proz = ((double)1023/(double)100)*b_proz;
  int b_set = (int)b_proz;
  
  analogWrite(REDPIN, r_set);
  analogWrite(GREENPIN, g_set);
  analogWrite(BLUEPIN, b_set);
}

void set_hw_status(int is_refresh) {
  if(autofade_mode == 0) {
    int r = on_off_status * (int)(red * dimmerlevel/100.0);
    int g = on_off_status * (int)(green * dimmerlevel/100.0);
    int b = on_off_status * (int)(blue * dimmerlevel/100.0);
  
    if(is_refresh == 0) {
      if (fadespeed > 0) {
        
        float dr = (r - r0) / float(fadespeed);
        float db = (b - b0) / float(fadespeed);
        float dg = (g - g0) / float(fadespeed);
        
        for (int x = 0;  x < fadespeed; x++) {
          set_rgb(r0 + dr*x, g0 + dg*x, b0 + db*x);
          delay(1);
        }
      }
    }
  
    set_rgb(r, g, b);
   
    r0 = r;
    b0 = b;
    g0 = g;  
  }
}

void fade() {
set_rgb(254, 0, 0);
  switch(autofade_mode) {
    case 1:
Serial.println( "fadeR" );
      fadeR();
      break;
    case 2:
Serial.println( "fadeG" );
      fadeG();
      break;
    case 3:
Serial.println( "fadeB" );
      fadeB();
      break;
    case 4:
Serial.println( "fadeRGB" );
      fadeRGB();
      break;
    case 5:
Serial.println( "fadeRGB" );
      fadeRGBLight();
      break;
    default:
Serial.println( "wrong function!" );
      break;
  }
  
}

void fadeR() {  
  int r = on_off_status * 0;
  int g = on_off_status * 0;
  int b = on_off_status * 0;
  int autofade_mode_now = autofade_mode;

  while((autofade_mode > 0) && (autofade_mode_now == autofade_mode)) {
    for (r;  r < 250; r++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r > 1; r--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }    
  }
}

void fadeG() {  
  int r = on_off_status * 0;
  int g = on_off_status * 0;
  int b = on_off_status * 0;
  int autofade_mode_now = autofade_mode;

  while((autofade_mode > 0) && (autofade_mode_now == autofade_mode)) {
    for (g;  g < 250; g++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (g;  g > 1; g--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }    
  }
}

void fadeB() {  
  int r = on_off_status * 0;
  int g = on_off_status * 0;
  int b = on_off_status * 0;
  int autofade_mode_now = autofade_mode;

  while((autofade_mode > 0) && (autofade_mode_now == autofade_mode)) {
    for (b;  b < 250; b++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (b;  b > 1; b--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }    
  }
}

void fadeRGB() {  
  int r = on_off_status * 0;
  int g = on_off_status * 0;
  int b = on_off_status * 0;
  int autofade_mode_now = autofade_mode;
  
  while((autofade_mode > 0) && (autofade_mode_now == autofade_mode)) {

    for (b;  b > 1; b--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r < 250; r++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }

    for (g; g < 250; g++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r > 1; r--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (b; b < 250; b++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (g;  g > 1; g--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r < 250; r++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }

  }
}

void fadeRGBLight() {  
  int r = on_off_status * 0;
  int g = on_off_status * 0;
  int b = on_off_status * 0;
  int autofade_mode_now = autofade_mode;

  while((autofade_mode > 0) && (autofade_mode_now == autofade_mode)) {

    for (b;  b > 20; b--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r < 100; r++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }

    for (g; g < 100; g++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r > 20; r--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (b; b < 100; b++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (g;  g > 20; g--) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }
    
    for (r;  r < 100; r++) {
      set_rgb(r, g, b);
      wait(5);
      if(autofade_mode == 0) {
        return;
      }
    }

  }
}


void send_status() {
  send(rgbMsg.set(rgbstring));
  send(lightMsg.set(on_off_status));
  send(dimmerMsg.set(dimmerlevel));
}
