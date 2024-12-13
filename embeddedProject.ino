#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// WiFi credentials
// #define WIFI_SSID "DIU_Daffodil Smart City"
// #define WIFI_PASSWORD "diu123456789"

// #define WIFI_SSID "Redmi Note 11"
// #define WIFI_PASSWORD "Bishal123!"

#define WIFI_SSID "TOM"
#define WIFI_PASSWORD "TOM@5780"


// Firebase project details
#define FIREBASE_HOST "https://esp32-fire-project-default-rtdb.firebaseio.com"
#define FIREBASE_API_KEY "AIzaSyDj0OgjEXxuFNnDAP7Go0vqxKiMJEfwc-E"


// ultrasonic sensor
#define TRIG_PIN 12
#define ECHO_PIN 14


// Pin definitions
#define SENSOR_PIN 16  // MQ2
#define SERVO_PIN 18
#define RESET_PIN 22
#define SERVO2_PIN 19
#define SENSOR2_PIN 17  // MQ6
#define SENSOR3_PIN 5   // MQ135
// #define SENSOR4_PIN 21  // MQ3
#define SENSOR4_PIN 34 //MQ3
#define LED_PIN 21

bool state = HIGH;
bool servoMoving = false;  // Tracks if the servo is moving
unsigned long lastResetTime = 0;
const unsigned long debounceDelay = 200;

Servo myServo;
Servo servo2;

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);
  pinMode(SENSOR3_PIN, INPUT);
  pinMode(SENSOR3_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RESET_PIN, INPUT);
  myServo.attach(SERVO_PIN);
  servo2.attach(SERVO2_PIN);
  servo2.write(0);
  myServo.write(0);
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.begin(9600);
}


void hcsr04Function(){
    long duration;
  float distance;

  // Clear the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send a 10 microsecond pulse to trigPin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the echoPin pulse width in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in centimeters
  distance = (duration * 0.0343) / 2;

  // Print the distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

    // Determine detection state and send to Firebase
  if (distance <= 4.0 && distance > 0) { // Ensure valid distance
    Serial.println("Object detected within 4 cm");
    sendDataToFirebase("/ultrasonic", "{\"detected\": 1}");
  } else {
    Serial.println("No object detected within 4 cm");
    sendDataToFirebase("/ultrasonic", "{\"detected\": 0}");
  }

  // Wait a bit before the next measurement
  delay(100);
}


void sendDataToFirebase(String path, String jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(FIREBASE_HOST) + path + ".json?auth=" + FIREBASE_API_KEY;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.PUT(jsonData);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}


String getDataFromFirebase(String path) {
  String payload = "";
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(FIREBASE_HOST) + path + ".json?auth=" + FIREBASE_API_KEY;
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      payload = http.getString();
      Serial.print("Data received: ");
      Serial.println(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
  return payload;
}


void resetServo() {
  myServo.write(0);  // Move servo to initial position
  delay(500);
}

void loop() {
  int mq6State = digitalRead(SENSOR2_PIN);
  int mq2State = digitalRead(SENSOR_PIN);
  int mq135State = digitalRead(SENSOR3_PIN);
  int mq3State = analogRead(SENSOR4_PIN);
  // int mq3State = digitalRead(SENSOR4_PIN);
  hcsr04Function();
  delay(100);
  Serial.print("MQ3:");
  Serial.print(mq3State);
  int reset = digitalRead(RESET_PIN);
  String resetValue = getDataFromFirebase("/manual_reset");

  delay(100);
    if (resetValue == "1") {
    Serial.println("Manual reset triggered from Firebase");
    resetServo();
    sendDataToFirebase("/manual_reset", "0");  // Reset the manual reset flag in Firebase
    state = HIGH;
  }
  delay(100);
  if (reset == HIGH) {
    Serial.println("Manual reset triggered via button");
    resetServo();
    sendDataToFirebase("/manual_reset", "0");  // Optionally synchronize Firebase reset
    state = HIGH;
  }
  // Create JSON data
  String jsonData = "{\"MQ2\":" + String(mq2State) + ",\"MQ6\":" + String(mq6State) + ",\"MQ135\":" + String(mq135State) + ",\"MQ3\":" + String(mq3State) + "}";

  // Send sensor data to Firebase
  sendDataToFirebase("/sensors", jsonData);


  if (state) {
    if (mq2State == LOW) {
      for (int pos = 0; pos <= 180; pos += 5) {
        myServo.write(pos);
        delay(15);
      }  // Set the desired position directly
      state = LOW;
      delay(100);
    } else if (mq6State == LOW) {
      for (int pos = 0; pos <= 180; pos += 5) {
        myServo.write(pos);
        delay(15);
      }  // Set the desired position directly
      state = LOW;
      delay(100);
    }


  }
if (mq3State >= 2300 && mq3State <= 4000) {
  // Turn the LED on
  digitalWrite(LED_PIN, HIGH);
  // Turn the LED off
  delay(1000);
  // digitalWrite(LED_PIN, LOW);
}
else{
  digitalWrite(LED_PIN, LOW);
}
    // Turn the LED off
  // digitalWrite(LED_PIN, LOW);
  delay(100);
}
