#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "UserInterface.h"
#include "LTC68041.h"
#include <SPI.h>
//we connect all multiplexers with the same set of control pins
const int selectPins[3] = {2, 3, 4}; // S-pins to Arduino pins: S0~2, S1~3, S2~4

const uint8_t TOTAL_IC = 3;//!<number of ICs in the daisy chain
//buffer for temperature data measurement
double total[3][8];
/******************************************************
 *** Global Battery Variables received from 6804 commands
 These variables store the results from the LTC6804
 register reads and the array lengths must be based
 on the number of ICs on the stack
 ******************************************************/
uint16_t cell_codes[TOTAL_IC][12];
/*!<
 * number of batteries
  The cell codes will be stored in the cell_codes[][12] array in the following format:

  |  cell_codes[0][0]| cell_codes[0][1] |  cell_codes[0][2]|    .....     |  cell_codes[0][11]|  cell_codes[1][0] | cell_codes[1][1]|  .....   |
  |------------------|------------------|------------------|--------------|-------------------|-------------------|-----------------|----------|
  |IC1 Cell 1        |IC1 Cell 2        |IC1 Cell 3        |    .....     |  IC1 Cell 12      |IC2 Cell 1         |IC2 Cell 2       | .....    |
****/

uint16_t aux_codes[TOTAL_IC][6];
/*!<
 The GPIO codes will be stored in the aux_codes[][6] array in the following format:

 |  aux_codes[0][0]| aux_codes[0][1] |  aux_codes[0][2]|  aux_codes[0][3]|  aux_codes[0][4]|  aux_codes[0][5]| aux_codes[1][0] |aux_codes[1][1]|  .....    |
 |-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|---------------|-----------|
 |IC1 GPIO1        |IC1 GPIO2        |IC1 GPIO3        |IC1 GPIO4        |IC1 GPIO5        |IC1 Vref2        |IC2 GPIO1        |IC2 GPIO2      |  .....    |
*/

uint8_t tx_cfg[TOTAL_IC][6];
/*!<
  The tx_cfg[][6] stores the LTC6804 configuration data that is going to be written
  to the LTC6804 ICs on the daisy chain. The LTC6804 configuration data that will be
  written should be stored in blocks of 6 bytes. The array should have the following format:

 |  tx_cfg[0][0]| tx_cfg[0][1] |  tx_cfg[0][2]|  tx_cfg[0][3]|  tx_cfg[0][4]|  tx_cfg[0][5]| tx_cfg[1][0] |  tx_cfg[1][1]|  tx_cfg[1][2]|  .....    |
 |--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|-----------|
 |IC1 CFGR0     |IC1 CFGR1     |IC1 CFGR2     |IC1 CFGR3     |IC1 CFGR4     |IC1 CFGR5     |IC2 CFGR0     |IC2 CFGR1     | IC2 CFGR2    |  .....    |

*/

uint8_t rx_cfg[TOTAL_IC][8];
/*!<
  the rx_cfg[][8] array stores the data that is read back from a LTC6804-1 daisy chain.
  The configuration data for each IC  is stored in blocks of 8 bytes. Below is an table illustrating the array organization:

|rx_config[0][0]|rx_config[0][1]|rx_config[0][2]|rx_config[0][3]|rx_config[0][4]|rx_config[0][5]|rx_config[0][6]  |rx_config[0][7] |rx_config[1][0]|rx_config[1][1]|  .....    |
|---------------|---------------|---------------|---------------|---------------|---------------|-----------------|----------------|---------------|---------------|-----------|
|IC1 CFGR0      |IC1 CFGR1      |IC1 CFGR2      |IC1 CFGR3      |IC1 CFGR4      |IC1 CFGR5      |IC1 PEC High     |IC1 PEC Low     |IC2 CFGR0      |IC2 CFGR1      |  .....    |
*/
const int sensor=1;
/*
 * position of the temperature sensor
 */
double battery_temp=0;
/*
 * Battery sensor that detects the temperature of our battery
 * 
 */
/*!**********************************************************************
 \brief  Inititializes hardware and variables
 ***********************************************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LTC6804_initialize();  //Initialize LTC6804 hardware
  pinMode(sensor, INPUT);
  init_cfg();        //initialize the 6804 configuration array to be written
  //set up multiplexers
  for(int i=0;i<3;i++)
  {
    pinMode(selectPins[i],OUTPUT);
    digitalWrite(selectPins[i],HIGH);
  }
}
/*!*********************************************************************
  \brief main loop

***********************************************************************/
void loop() 
{
  // put your main code here, to run repeatedly:
  if (Serial.available())           // Check for user input
  {
    uint32_t user_command;
    user_command = read_int();      // Read the user command
    Serial.println(user_command);
    run_command(user_command);
  }
}

void run_command(uint32_t cmd)
{
  int8_t error = 0;

  char input = 0;
  switch (cmd)
  {

    case 1:
      wakeup_sleep();
      LTC6804_wrcfg(TOTAL_IC,tx_cfg);
      print_config();
      break;

    case 2:
      wakeup_sleep();
      error = LTC6804_rdcfg(TOTAL_IC,rx_cfg);
      if (error == -1)
      {
        Serial.println("A PEC error was detected in the received data");
      }
      print_rxconfig();
      break;

    case 3:
      wakeup_sleep();
      LTC6804_adcv();
      delay(3);
      Serial.println("cell conversion completed");
      Serial.println();
      break;

    case 4:
      wakeup_sleep();
      error = LTC6804_rdcv(0, TOTAL_IC,cell_codes); // Set to read back all cell voltage registers
      if (error == -1)
      {
        Serial.println("A PEC error was detected in the received data");
      }
      print_cells();
      break;

    case 5:
      wakeup_sleep();
      LTC6804_adax();
      delay(3);
      Serial.println("aux conversion completed");
      Serial.println();
      break;

    case 6:
      wakeup_sleep();
      error = LTC6804_rdaux(0,TOTAL_IC,aux_codes); // Set to read back all aux registers
      if (error == -1)
      {
        Serial.println("A PEC error was detected in the received data");
      }
      print_aux();
      break;

    case 7:
      wakeup_sleep();
      Serial.println("do nothing");
      print_menu();
      break;

    case 8:
       wakeup_sleep();
       battery_temp=temp_convert(analogRead(sensor));
        Serial.print("Current temperature is:");
        Serial.print(battery_temp);
        Serial.println(" C*");
        print_menu();
       break;
       
    case 9:
      wakeup_sleep();
      LTC6804_wrcfg(TOTAL_IC,tx_cfg);
      while(input != 'm')
      { 
        if (Serial.available() > 0)
        {
          input = read_char();
        }
        wakeup_idle();
        LTC6804_adcv();
        delay(10);
        wakeup_idle();
        error = LTC6804_rdcv(0, TOTAL_IC,cell_codes);
        
        temp_measure();
        print_cells();
        
         
         Serial.println();
         
      }
      print_menu();
      break;
      
    default:
      Serial.println("Incorrect Option");
      break;

   case 10:
    print_menu();
    break;
     
  }
}

/*!***********************************
 \brief Initializes the configuration array
 **************************************/
void init_cfg()
{
  for (int i = 0; i<TOTAL_IC; i++)
  {
    tx_cfg[i][0] = 0xFE;
    tx_cfg[i][1] = 0x00 ;
    tx_cfg[i][2] = 0x00 ;
    tx_cfg[i][3] = 0x00 ;
    tx_cfg[i][4] = 0x00 ;
    tx_cfg[i][5] = 0x00 ;
  }

}

/*!*********************************
  \brief Prints the main menu
***********************************/
void print_menu()
{
  Serial.println("Please enter LTC6804 Command");
  Serial.println("Write Configuration: 1");
  Serial.println("Read Configuration: 2");
  Serial.println("Start Cell Voltage Conversion: 3");
  Serial.println("Read Cell Voltages: 4");
  Serial.println("Start Aux Voltage Conversion: 5");
  Serial.println("Read Aux Voltages: 6");
  Serial.println("loop cell voltages: 7");
  Serial.println("Read Cell Temperature: 8");
  Serial.println("loop cell voltages and temperature: 9");
  Serial.println("Please enter command: ");
  Serial.println();
}



/*!************************************************************
  \brief Prints cell coltage codes to the serial port
 *************************************************************/
void print_cells()
{


  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    
    
    for (int i=0; i<12; i++)
    {
      Serial.print(cell_codes[current_ic][i]*0.0001,4);
      Serial.print(" ");
    }
    
  }
  
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes and Vref2 voltage code onto the serial port
 *****************************************************************************/
void print_aux()
{

  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    
    
    for (int i=0; i < 5; i++)
    {
      Serial.print(" GPIO-");
      Serial.print(i+1,DEC);
      Serial.print(":");
      Serial.print(aux_codes[current_ic][i]*0.0001,4);
      Serial.print(",");
    }
    Serial.print(" Vref2");
    Serial.print(":");
    Serial.print(aux_codes[current_ic][5]*0.0001,4);
    Serial.println();
  }
  Serial.println();
}
/*!******************************************************************************
 \brief Prints the configuration data that is going to be written to the LTC6804
 to the serial port.
 ********************************************************************************/
void print_config()
{
  int cfg_pec;

  Serial.println("Written Configuration: ");
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(": ");
    Serial.print("0x");
    serial_print_hex(tx_cfg[current_ic][0]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][1]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][2]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][3]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][4]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][5]);
    Serial.print(", Calculated PEC: 0x");
    cfg_pec = pec15_calc(6,&tx_cfg[current_ic][0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(", 0x");
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println();
  }
  Serial.println();
}

/*!*****************************************************************
 \brief Prints the configuration data that was read back from the
 LTC6804 to the serial port.
 *******************************************************************/
void print_rxconfig()
{
  Serial.println("Received Configuration ");
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(": 0x");
    serial_print_hex(rx_cfg[current_ic][0]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][1]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][2]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][3]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][4]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][5]);
    Serial.print(", Received PEC: 0x");
    serial_print_hex(rx_cfg[current_ic][6]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][7]);
    Serial.println();
  }
  Serial.println();
}

void serial_print_hex(uint8_t data)
{
  if (data< 16)
  {
    Serial.print("0");
    Serial.print((byte)data,HEX);
  }
  else
    Serial.print((byte)data,HEX);
}

double temp_convert(int temp_vol)
{/*
  * cf==1, celsius, cf==0 farenheit  
  */

  float mv=(temp_vol/1024.0)*5000;
  float cel = mv/10;
  float farh=(cel*9)/5 + 32;
 
  return cel;
  
}

void select_sensor(int pin)
{
  for(int i=0;i<3;i++)
  {
    if(pin & 1<<i)
      digitalWrite(selectPins[i],HIGH);
     else
      digitalWrite(selectPins[i],LOW);
  }
}
void temp_measure(){

  double val;
  double dat;
  //record the total temperature
  //we need 24 sensors so I just hard coded 24 sensors
  
  
  //reserved for testing
  //double true_temp=0;
  
    int i,n,k;//i for counting 
    for(k=0;k<3;k++)
      for(n=0;n<8;n++)
      {
         total[k][n]=0;
      }
     for(i=0;i<25;i++)
      { 
       
          for(n=0;n<8;n++)
          { select_sensor(n); 
          //we use a0 a1 and a 2 for three sets of input for mux
            delay(10);
            for(k=0;k<3;k++)
            { delay(8);
              val=analogRead(k);
              dat=temp_convert(val);
              total[k][n]+=dat;
              
            }
            
          }
          delay(40);
         
      } 
      //print data
     for(k=0;k<3;k++)   
       for( n=0;n<8;n++)  
       {
        total[k][n]/=25;
        Serial.print("Sensor");
        Serial.print(k);
        Serial.print(":");
        Serial.print(n);      
        Serial.print(" Temp:");
        Serial.print(total[k][n]); 
        Serial.println("C");
     
       }  


}


