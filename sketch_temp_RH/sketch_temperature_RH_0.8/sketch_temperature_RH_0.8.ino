#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// Modbus slave address (typically 1 for XY-MD02)
#define SLAVE_ID 1

// Modbus register addresses
#define TEMP_REGISTER 0x01   // Temperature register (0x01)
#define HUMIDITY_REGISTER 0x02 // Humidity register (0x02)

// RS485 communication pins (Arduino UNO R4 WiFi)
#define RS485_DIR_PIN 3      // DE/RE pin for RS485 transceiver
#define RS485_RX_PIN 10      // RX pin (connect to RO)
#define RS485_TX_PIN 11      // TX pin (connect to DI)

// Create ModbusMaster object
ModbusMaster node;

// Variables to store sensor data
float temperature = 0.0;
float humidity = 0.0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize RS485 communication
  Serial1.begin(9600, SERIAL_8N1);
  
  // Initialize Modbus
  node.begin(SLAVE_ID, Serial1);
  
  // Configure RS485 direction control
  pinMode(RS485_DIR_PIN, OUTPUT);
  digitalWrite(RS485_DIR_PIN, LOW); // Receive mode
  
  Serial.println("XY-MD02 Modbus Reader Started");
}

void loop() {
  // Read temperature and humidity
  if (readSensorData()) {
    Serial.print("Temperature: ");
    Serial.print(temperature, 2);
    Serial.println(" °C");
    
    Serial.print("Humidity: ");
    Serial.print(humidity, 2);
    Serial.println(" %");
    
    Serial.println("---");
  } else {
    Serial.println("Failed to read sensor data");
  }
  
  // Wait before next reading
  delay(2000);
}

bool readSensorData() {
  uint16_t data[2]; // Buffer to store raw data
  uint8_t result;
  
  // Read 2 registers starting from address 0x01 (Temperature and Humidity)
  result = node.readInputRegisters(0x01, 2);
  
  if (result == node.ku8MBSuccess) {
    // Extract temperature and humidity from registers
    data[0] = node.getResponseBuffer(0); // Temperature register
    data[1] = node.getResponseBuffer(1); // Humidity register
    
    // Convert to decimal values (XY-MD02 typically outputs 10x the actual value)
    temperature = (float)data[0] / 10.0;
    humidity = (float)data[1] / 10.0;
    
    return true;
  } else {
    // Handle error
    Serial.print("Modbus error: ");
    Serial.println(result);
    return false;
  }
}
