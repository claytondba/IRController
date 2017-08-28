#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
//#include <IRrecv.h>
//#include <IRsend.h>
//#include <IRutils.h>

#define SSID "MIC24"
#define PASSWORD  "-------"
//Configurcao do IR
#define RECV_PIN 12
#define SEND_PIN 14


#define BROKER_MQTT "m13.cloudmqtt.com"
#define BROKER_PORT  16585
#define ID_DISPO "esp-sala-01"
#define BROKER_USER "esp-sala-01"
#define BROKER_PASS "gdsf-ede--ral"
#define SUB_TOPIC "home/sala/ircontroll/#"


#define COPIA 0


WiFiClient espClient;
PubSubClient MQTT(espClient);
decode_results results;
IRrecv irrecv(RECV_PIN); //VARIÁVEL DO TIPO IRrecv
IRsend irsend(SEND_PIN);

void  ircode (decode_results *results)
{
  // Panasonic has an Address
  if (results->decode_type == PANASONIC) {
    Serial.print(results->panasonicAddress, HEX);
    Serial.print(":");
  }

  // Print Code
  Serial.print(results->value, HEX);
}

//+=============================================================================
// Display encoding type
//
void  encoding (decode_results *results)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      Serial.print("UNKNOWN");       break ;
    case NEC:          Serial.print("NEC");           break ;
    case SONY:         Serial.print("SONY");          break ;
    case RC5:          Serial.print("RC5");           break ;
    case RC6:          Serial.print("RC6");           break ;
    case DISH:         Serial.print("DISH");          break ;
    case SHARP:        Serial.print("SHARP");         break ;
    case JVC:          Serial.print("JVC");           break ;
    case SANYO:        Serial.print("SANYO");         break ;
    case MITSUBISHI:   Serial.print("MITSUBISHI");    break ;
    case SAMSUNG:      Serial.print("SAMSUNG");       break ;
    case LG:           Serial.print("LG");            break ;
    case WHYNTER:      Serial.print("WHYNTER");       break ;
    case AIWA_RC_T501: Serial.print("AIWA_RC_T501");  break ;
    case PANASONIC:    Serial.print("PANASONIC");     break ;
  }
}

//+=============================================================================
// Dump out the decode_results structure.
//
void  dumpInfo (decode_results *results)
{
  // Show Encoding standard
  Serial.print("Encoding  : ");
  encoding(results);
  Serial.println("");

  // Show Code & length
  Serial.print("Code      : ");
  ircode(results);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
}

//+=============================================================================
// Dump out the decode_results structure.
//
void  dumpRaw (decode_results *results)
{
  // Print Raw data
  Serial.print("Timing[");
  Serial.print(results->rawlen-1, DEC);
  Serial.println("]: ");

  for (int i = 1;  i < results->rawlen;  i++) {
    unsigned long  x = results->rawbuf[i] * USECPERTICK;
    if (!(i & 1)) {  // even
      Serial.print("-");
      if (x < 1000)  Serial.print(" ") ;
      if (x < 100)   Serial.print(" ") ;
      Serial.print(x, DEC);
    } else {  // odd
      Serial.print("     ");
      Serial.print("+");
      if (x < 1000)  Serial.print(" ") ;
      if (x < 100)   Serial.print(" ") ;
      Serial.print(x, DEC);
      if (i < results->rawlen-1) Serial.print(", "); //',' not needed for last one
    }
    if (!(i % 8))  Serial.println("");
  }
  Serial.println("");                    // Newline
}

//+=============================================================================
// Dump out the decode_results structure.
//
void  dumpCode (decode_results *results)
{
  // Start declaration
  Serial.print("unsigned int  ");          // variable type
  Serial.print("rawData[");                // array name
  Serial.print(results->rawlen - 1, DEC);  // array size
  Serial.print("] = {");                   // Start declaration

  // Dump data
  for (int i = 1;  i < results->rawlen;  i++) {
    Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
    if ( i < results->rawlen-1 ) Serial.print(","); // ',' not needed on last one
    if (!(i & 1))  Serial.print(" ");
  }

  // End declaration
  Serial.print("};");  //

  // Comment
  Serial.print("  // ");
  encoding(results);
  Serial.print(" ");
  ircode(results);

  // Newline
  Serial.println("");

  // Now dump "known" codes
  if (results->decode_type != UNKNOWN) {

    // Some protocols have an address
    if (results->decode_type == PANASONIC) {
      Serial.print("unsigned int  addr = 0x");
      Serial.print(results->panasonicAddress, HEX);
      Serial.println(";");
    }

    // All protocols have data
    Serial.print("unsigned int  data = 0x");
    Serial.print(results->value, HEX);
    Serial.println(";");
  }
}

void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.println("Tentando se conectar ao Broker MQTT: " + String(BROKER_MQTT));

    if (MQTT.connect(ID_DISPO, BROKER_USER, BROKER_PASS))
    {
      Serial.println("Conectado");
      MQTT.subscribe(SUB_TOPIC);
    }
    else
    {
      Serial.println("Falha ao Reconectar :(");
      Serial.println("Tentando se reconectar em 2 segundos");
      Serial.println(WiFi.localIP());
      delay(2000);
    }
  }
}


void recconectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
}


void initWiFi()
{
  delay(10);
  Serial.println("Conectando-se em: " + String(SSID));

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado na Rede " + String(SSID) + " | IP => ");
  Serial.println(WiFi.localIP());
}
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{

  String message;
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    message += c;
  }

  Serial.println("Tópico => " + String(topic) + " | Valor => " + String(message));
  Serial.flush();


}
void initMQTT()
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void dump(decode_results *results)
{
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  }
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  }
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  }
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  }
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->panasonicAddress, HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
  }
  else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  }
  else if (results->decode_type == AIWA_RC_T501) {
    Serial.print("Decoded AIWA RC T501: ");
  }
  else if (results->decode_type == WHYNTER) {
    Serial.print("Decoded Whynter: ");
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 1; i < count; i++) {
    if (i & 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    }
    else {
      Serial.write('-');
      Serial.print((unsigned long) results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Iniciado com sucesso");
  initWiFi();
  initMQTT();

  MQTT.subscribe(SUB_TOPIC);
  Serial.println("Inscrito no tepico: ");
  Serial.println(SUB_TOPIC);
}

// the loop function runs over and over again forever
void loop()
{
  if(COPIA)
  {
    Serial.println("Modo IRCopy");
    irrecv.enableIRIn();
  }
  while (COPIA)
  {
    delay(100);
    //Serial.print(".");
    if (irrecv.decode(&results))
    {
      dumpInfo(&results);           // Output the results
      dumpRaw(&results);            // Output the results in RAW format
      dumpCode(&results);           // Output the results as source code
      Serial.println("");           // Blank line between entries
      irrecv.resume();   // Receive the next value
    }

    /* code */
  }


  if (!MQTT.connected())
  {
    reconnectMQTT();
  }

  recconectWiFi();
  MQTT.loop();

}
