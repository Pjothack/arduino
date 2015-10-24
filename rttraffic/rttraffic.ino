#include <SPI.h>
#include <Ethernet.h>

byte MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

#define HOST          "95.215.47.190"
#define PORT          1234
#define RETRY_DELAY   (5 * 1000)

EthernetClient client;

#define TRIG_PIN 3
#define ECHO_PIN 2

#define CYCLES_PER_SECOND (10)
#define CYCLE_DURATION    (1000 / CYCLES_PER_SECOND)
#define DIFF_SAMPLES      (60 * CYCLES_PER_SECOND)
#define DIFF_SENSITIVITY  (2 * 29.1 * 10)

unsigned long total_cycles = 0;
unsigned long change_cycles = 0;
double changes_per_cycle = 0.0;

long previous_distance = -1;
long current_distance = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  dhcp_init();
  delay(1000);
  connection_init();
  
  Serial.print("Cycles per second: ");
  Serial.print(CYCLE_DURATION);
  Serial.print(" ms   Sampling size: ");
  Serial.println(DIFF_SAMPLES);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void dhcp_init() {
  Serial.println("Configuring DHCP...");
  while (Ethernet.begin(MAC) == 0) {
    Serial.println("Failed to configure DHCP. Retrying in 10 seconds...");
    delay(RETRY_DELAY);
  }

  Serial.println("DHCP configured successfully.");
}

void dhcp_renew() {
  Serial.println("Renewing DHCP lease...");
  int status = Ethernet.maintain();
  while (status == 1 || status == 3) {
    Serial.println("Failed to renew DHCP lease. Retrying in 5 seconds...");
    delay(RETRY_DELAY);
    status = Ethernet.maintain();
  }
  
  Serial.println("DHCP lease renewed successfully.");
}

void connection_init() {
  Serial.print("Connecting to host ");
  Serial.print(HOST);
  Serial.print(" on port ");
  Serial.print(PORT);
  Serial.println("...");
  while (!client.connect(HOST, PORT)) {
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Connection failed. Retrying in 5 seconds...");
    delay(RETRY_DELAY);
  }

  Serial.println("Connection established!");
}

void loop() {
  if (!client.connected()) {
    Serial.println("Connection failed.");
    client.flush();
    client.stop();
    dhcp_renew();
    delay(1000);
    connection_init();
  }
  
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  double changes_per_cycle = (double)change_cycles / (double)DIFF_SAMPLES;
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  long current_distance = (duration / 2) / DIFF_SENSITIVITY;

  if (current_distance != previous_distance && previous_distance != -1)
    change_cycles++;
  
  previous_distance = current_distance;
  total_cycles++;

  if (total_cycles == DIFF_SAMPLES) {
    changes_per_cycle = (double)change_cycles / (double)DIFF_SAMPLES;
    //Serial.println(100 * changes_per_cycle);
    client.println(change_cycles);
    total_cycles = 0;
    change_cycles = 0;
  }

  // Wait before repeating.
  delay(CYCLE_DURATION);
}
