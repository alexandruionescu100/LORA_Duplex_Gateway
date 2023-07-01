/*
  LoRa Simple Gateway/Node Exemple

  This code uses InvertIQ function to create a simple Gateway/Node logic.

  Gateway - Sends messages with enableInvertIQ()
          - Receives messages with disableInvertIQ()

  Node    - Sends messages with disableInvertIQ()
          - Receives messages with enableInvertIQ()

  With this arrangement a Gateway never receive messages from another Gateway
  and a Node never receive message from another Node.
  Only Gateway to Node and vice versa.

  This code receives messages and sends a message every second.

  InvertIQ function basically invert the LoRa I and Q signals.

  See the Semtech datasheet, http://www.semtech.com/images/datasheet/sx1276.pdf
  for more on InvertIQ register 0x33.

  created 05 August 2018
  by Luiz H. Cassettari

  translated by Akshaya Bali
*/

#include "stdlib.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "stdio.h"
#include "string.h"
#include <sstream>

#include <string>
//Inlude Library
#include "LoRa-RP2040.h"
#include "ssd1306.h"
#include "sd_card.h"
#include "ff.h"

#define SENSORDATA0 100

#define SEARCH_NODE 0                              //read sensor data, save data to SD card, LORA module in Listen mode
#define BEGIN_RECEIVE_DATA_FROM_NODE 1                     //Connect to the gateway, receive comands from the gateway(what data to receive), send to the Gateway the size of the data)
#define ONGOING_RECEIVE_DATA_FROM_NODE 2                   //send data to the Gateway
#define END_RECEIVE_DATA_FROM_NODE 3                       //check if all the data was sent/received

#define SEND_DATA_SENSOR_1 1
#define READY_2_RECEIVE 2
#define END_TRANSMISSION 3
#define GOOD_BYE 4

using std::string;

using namespace std;
const long frequency = 433E6;  // LoRa Frequency

const int csPin = 8;          // LoRa radio chip select
const int resetPin = 9;        // LoRa radio reset
const int irqPin = 13;          // change for your board; must be a hardware interrupt pin
const char *words[]= {"SSD1306", "DISPLAY", "DRIVER"};
const char *call[]= {"Call111","Call222","Call333"};
ssd1306_t disp ={};


//sensor data from 55 to 154
char sensor_data_test[SENSORDATA0]= {};
char data_index = 0; //sensor_data_test vector index; 0 - 99

//Network settings
uint8_t localAddress = 0x7A;      // destination to send to | 0x7A = z //GATEWAY
uint8_t destination = 0x78;     // address of this device | 0x78 = x //NODE X

//Parse message data
uint8_t received_localAddress = 0x00;
uint8_t received_destination = 0x00;
uint8_t received_mes_counter = 0x00;


//system state
char SystemState = SEARCH_NODE;

//message info variables
uint8_t msgCount = 0;            // count of outgoing messages

//incoming data
uint32_t size_of_data = 0;
uint32_t current_nr_of_received_data = 0;

char data_container[2400]={};
int data_cnt_idx=0;
char print_data = 0;


int counter=0;

void setup_gpios(void);
int32_t ParseInt32(const char (&buf)[5]);
void SerializeInt32(char (&buf)[5], int32_t val);
int32_t ParseInt32(const char (&buf)[5]);
void save_data_to_sd(int size, char * buffer);

//SD card data
  FRESULT fr;
  FATFS fs;
  FIL fil;
  int ret;
  char buf[128];
  char filename[] = "temp_data_gateway.txt";
  int sd_ptr_ind = 0;
  int nr_of_data_entries_sd = 0;
void LoRa_rxMode(){
  //LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  //LoRa.disableInvertIQ();               // normal mode
  LoRa.enableInvertIQ();                // active invert I and Q signals
}

void LoRa_sendMessage(string message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write((msgCount%10)+48);            // add message ID - transformed from integer to string 0...9
  
  switch (SystemState)
  {
    case SEARCH_NODE:
      LoRa.write(SEND_DATA_SENSOR_1+'0');
    break;

    //case BEGIN_RECEIVE_DATA_FROM_NODE:
      //LoRa.write(READY_2_RECEIVE+'0');
    //break;

    case ONGOING_RECEIVE_DATA_FROM_NODE:
      //Nothing to do, just listen
    break;

    //case END_RECEIVE_DATA_FROM_NODE:
      //LoRa.write(END_TRANSMISSION+'0');
    //break;
  
  default:
      LoRa.write(END_TRANSMISSION+'0');
    break;
  }
  LoRa.print(message.c_str());                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
  msgCount++;
}

void system_state_transition(char command)
{
  switch (SystemState)
  {
  case SEARCH_NODE:
      if(command=='S' or command=='D')
      {
        SystemState = ONGOING_RECEIVE_DATA_FROM_NODE;
        printf("-----line 153----");
      }
    break;
    /*
  case BEGIN_RECEIVE_DATA_FROM_NODE:
      if(command=='D')
      {
        SystemState = ONGOING_RECEIVE_DATA_FROM_NODE;
      }
    break;
    */

  case ONGOING_RECEIVE_DATA_FROM_NODE:
      if(command=='D')
      {
        SystemState = ONGOING_RECEIVE_DATA_FROM_NODE;
      }else if(command=='C'){
        //SystemState = END_RECEIVE_DATA_FROM_NODE;
        SystemState = SEARCH_NODE;
      }
    break;
  
  default:
  SystemState = SEARCH_NODE;
    break;
  }
}

void PacketAnalyzer(string message)
{
  int str_length = message.length();
  int index = 0;
  int arr[96] = { 0 };
  char data_size[5]={'0','0','0','0','\0'};
  char data_chunk[5]={'0','0','0','0','\0'};
  char node_state = 0;
  int current_chunk = 0;

  received_destination = message[index++];
  received_localAddress = message[index++];
  received_mes_counter = message[index++]-48;
  node_state =  message[index++];
 

system_state_transition(node_state);


  switch (SystemState)
  {
    case SEARCH_NODE:
      printf("SYSTEM state is SEARCH_NODE----");
      if(node_state=='C')
    {
      printf("\n current_nr_of_received_data = %d \n", current_nr_of_received_data);
      printf("\n Transmision will be closed by the Node\n");
      //check here again the seze of the sent data and after this send "good bye"
      print_data = 1;
    }
    break;

    case ONGOING_RECEIVE_DATA_FROM_NODE:
    if(node_state=='S')
    {
      data_size[0] = message[index++];
      data_size[1] = message[index++];
      data_size[2] = message[index++];
      data_size[3] = message[index++];
      size_of_data = ParseInt32(data_size);
      current_nr_of_received_data = 0;
      printf("The node will send %d data", size_of_data);
      SystemState = ONGOING_RECEIVE_DATA_FROM_NODE;
    }else if(node_state=='D')
    {
      data_chunk[0] = message[index++];
      data_chunk[1] = message[index++];
      data_chunk[2] = message[index++];
      data_chunk[3] = message[index++];
      current_chunk = ParseInt32(data_chunk);
      printf("current chunk %d of data\n", current_chunk);
      int i=0;
      for(;i<24;i++)
        {
          arr[i]=message[index++];
          data_container[data_cnt_idx++]=arr[i];
        }
      current_nr_of_received_data = current_nr_of_received_data + i;
    } else if(node_state=='C')
    {
      printf("\n current_nr_of_received_data = %d \n", current_nr_of_received_data);
      printf("\n Transmision will be closed by the Node\n");
      //check here again the seze of the sent data and after this send "good bye"
    }

     break;
  
  default:
      
    break;
  }


printf("\n received_localAddress = %d, received_destination = %d \n", received_localAddress,received_destination);
printf("\n received_mes_counter = %d, node_state = %c \n", received_mes_counter,node_state);
printf("\nstr_length = %d, Internal SystemState = %d \n", str_length,SystemState);

for(int i=0;i<96;i++)
{
  printf(" arr[%d]=%d ;", i, arr[i]);
}


}

void onReceive(int packetSize) {
  string message = "";


  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  printf("Gateway Receive: ");
  printf("%s\n",message.c_str());

  PacketAnalyzer(message);

  
   if(counter<3)
  {
    ssd1306_draw_string(&disp, 8, 24, 2, call[counter]);
    counter++;
  } else
  {
    counter=0;
    ssd1306_draw_string(&disp, 8, 24, 2, call[counter]);
    counter++;
  }

  ssd1306_show(&disp);
  ssd1306_clear(&disp);
  
}

void onTxDone() {
  printf("TxDone\n");
  LoRa_rxMode();
}

bool runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = to_ms_since_boot(get_absolute_time());
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

bool runEvery2(unsigned long interval)
{
  static unsigned long previousMillis2 = 0;
  unsigned long currentMillis = to_ms_since_boot(get_absolute_time());
  if (currentMillis - previousMillis2 >= interval)
  {
    previousMillis2 = currentMillis;
    return true;
  }
  return false;
}

bool runEvery3(unsigned long interval)
{
  static unsigned long previousMillis3 = 0;
  unsigned long currentMillis = to_ms_since_boot(get_absolute_time());
  if (currentMillis - previousMillis3 >= interval)
  {
    previousMillis3 = currentMillis;
    return true;
  }
  return false;
}

bool runEvery4(unsigned long interval)
{
  static unsigned long previousMillis4 = 0;
  unsigned long currentMillis = to_ms_since_boot(get_absolute_time());
  if (currentMillis - previousMillis4 >= interval)
  {
    previousMillis4 = currentMillis;
    return true;
  }
  return false;
}


int main(){

  string message = "";

  stdio_init_all();
  setup_default_uart();
  setup_gpios();

  disp.external_vcc=false;
  ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
  ssd1306_clear(&disp);

for(int i=0; i<sizeof(words)/sizeof(char *); ++i) {
            ssd1306_draw_string(&disp, 8, 24, 2, words[i]);
            ssd1306_show(&disp);
            sleep_ms(800);
          ssd1306_clear(&disp);
        }
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {
    printf("LoRa init failed. Check your connections.\n");
    while (true);                       // if failed, do nothing
  }

  printf("LoRa init succeeded.\n");
  printf("\n");
  printf("LoRa Simple Gateway\n");
  printf("Only receive messages from nodes\n");
  printf("Tx: invertIQ enable\n");
  printf("Rx: invertIQ disable\n");

  printf("\n");

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();
  
   //sd begin
    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        //while (true);
    }
   

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        //while (true);
    }

    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_EXISTING| FA_OPEN_APPEND); 
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        //while (true);
    }

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        //while (true);
    }

    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        //while (true);
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf(buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        //while (true);
    }


    // Unmount drive
    f_unmount("0:");

  //sd end

  while (1)  {
    /*
    if (runEvery(10000)) { // repeat every 1000 millis

      message = "HeLoRa World! ";
      message += "I'm a Gateway! ";
      message += to_ms_since_boot(get_absolute_time());

      LoRa_sendMessage(message); // send a message
      printf("Send Message!");
    }
    */

    switch (SystemState)
    {
    case SEARCH_NODE:
      if (runEvery(15000))
      {
        message = "Gateway";
        LoRa_sendMessage(message); // send a message
        printf("\nSEARCH_NODE:\n"); 
        if(print_data==1)
        {
          for(int i=0; i<2400; i++)
          {
            printf("\ndata_container [ %d] = %c", i, data_container[i]);
          }
          print_data=0;
        }
      }
    break;

    case BEGIN_RECEIVE_DATA_FROM_NODE:
      if(runEvery2(100))
      {
        message = "Gateway";
        LoRa_sendMessage(message); // send a message
        printf("\nBEGIN_RECEIVE_DATA_FROM_NODE:\n");        
      }
    break;

    case ONGOING_RECEIVE_DATA_FROM_NODE:
      if(runEvery3(30000))
      {
        printf("\nONGOING_RECEIVE_DATA_FROM_NODE\n");
      }
    break;

    case END_RECEIVE_DATA_FROM_NODE:
      if(runEvery4(1000))
      {
        for(int i=0; i<2400; i++)
          {
            printf("\ndata_container[%d]=%c\n",i,data_container[i]);
          }
          save_data_to_sd(data_cnt_idx, data_container);
          data_cnt_idx = 0;
        message = "Gateway";
        LoRa_sendMessage(message); // send a message
        printf("\nEND_RECEIVE_DATA_FROM_NODE\n");
      }

    break;
    
    default:
      break;
    }
  }

  return 0;
}
void setup_gpios(void) {
    i2c_init(i2c1, 400000);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
}


void SerializeInt32(char (&buf)[5], int32_t val)
{
    uint32_t uval = val;
    buf[0] = uval;
    buf[1] = uval >> 8;
    buf[2] = uval >> 16;
    buf[3] = uval >> 24;
    //end of string
    buf[4] = '\0';
}

int32_t ParseInt32(const char (&buf)[5])
{
    // This prevents buf[i] from being promoted to a signed int.
    uint32_t u0 = buf[0], u1 = buf[1], u2 = buf[2], u3 = buf[3];
    uint32_t uval = u0 | (u1 << 8) | (u2 << 16) | (u3 << 24);
    return uval;
}
void save_data_to_sd(int size, char * buffer)
{
  static uint16_t data_counter = 0;
    //sd begin
    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        //while (true);
    }
   

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        //while (true);
    }


         // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_EXISTING| FA_OPEN_APPEND); 
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        //while (true);
    }

    for(int i=0; i<size; i++)
    {
      ret = f_putc(buffer[i], &fil);
    }

    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        //while (true);
    }
    
    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        //while (true);
    }

    f_unmount("0:");
}