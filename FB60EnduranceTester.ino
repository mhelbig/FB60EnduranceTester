// Libraries:
#include <LiquidCrystal.h>
#include <avr/eeprom.h>

// Constants:
LiquidCrystal lcd(6, 7, 8, 9, 10, 11);                   // LCD I/O assignments
const unsigned long EventInterval = 60;                  // Save Total Rotations to EEPROM every 60 seconds
const unsigned long NumberOfRotationsToTest = 100000;    // Sets the number of rotations to run before stopping the test

// Global Variables:
char SerialCommand;                              // Character buffer from serial port
bool TestRunning = 0;                            // Flag for whether the test is running or not
unsigned long EventLogTimer=0;                   // Event data Timer (in seconds) for serial output interval
long TestRotations = 0;                          // Accumulator for individual test run rotations
long TotalRotations = 0;                         // Accumulator for total rotations
uint32_t EEMEM NonVolatileTotalCounts;           // EEPROM storage for total rotations
uint32_t EEMEM NonVolatileTestCounts;            // EEPROM storage for test rotations
int ButtonState = 0;                             // On/Off button state
int LastButtonState = 0;                         // Previous On/Off button state
int Mode = 0;                                    // On/Off state of the motor

// I/O Ports:
const int ShaftSensor = 2;
const int Motor = 4;
const int LED = 13;
const int PushButton = 12;

void setup()
{                
  // I/O pin setup:
  pinMode(ShaftSensor, INPUT);
  pinMode(Motor, OUTPUT);     
  pinMode(LED, OUTPUT);     
  pinMode(PushButton, INPUT);
  digitalWrite(PushButton, HIGH);               // Enable the built-in pullup resistor on the pushbutton
  
  // LCD setup
  lcd.begin(16, 2);                             // set up the LCD's number of columns and rows

  // Interrupt Setup:
  attachInterrupt(0,AccumulateShaftRotations,RISING);

  // Serial port setup:
  Serial.begin(9600);

  // Check for user holding in the start button when powering up to reset the counts
  ButtonState = digitalRead(PushButton);
  if(ButtonState ==0)
  {
    ResetSensorCounts();
    
    while(ButtonState = 0);  //wait for button release before moving on
    {
      ButtonState = digitalRead(PushButton);
    }
  }
  else
  {
  TotalRotations = eeprom_read_dword(&NonVolatileTotalCounts);   // read in the total number of shaft counts from the EEPROM as a starting point
  TestRotations = eeprom_read_dword(&NonVolatileTestCounts);     // read in the test number of shaft counts from the EEPROM as a starting point
  }

  PrintEventHeader();
  PrintEventData();

}

void loop()
{
  if (Serial.available())
  {
    SerialCommand = Serial.read();
    
    switch (SerialCommand)
    {
      case '1':                                      // 1 = Start the test
        StartMotor();
        break;
        
      case '0':                                      // 0 = Stop the test
        StopMotor();
        break;
        
      case 'r':                                      // r or R = reset counts
      case 'R':
        Serial.println("Counts Reset by user");
        ResetSensorCounts();
        break;
    }  
  }
  
  // Check for someone pressing the On/Off button:
  ButtonState = digitalRead(PushButton);
  
  if(ButtonState ==0 && LastButtonState ==1)
  {
    if (Mode ==0)
    {
      StartMotor();
      Mode = 1;
    }
    else
    {
      StopMotor();
      Mode = 0;
    }
  }

  LastButtonState = ButtonState;
  
  if (TestRunning)
  {
    PrintEventData();
    
// Check number of revolutions to see if we should stop the test
    if (TestRotations > NumberOfRotationsToTest)
    {
      TestRotations = 0;        // Reset these for the next test
      
      TestRunning = 0;
      SetMotor(LOW);
    }
  }
}

void StartMotor(void)
{
  EventLogTimer = Seconds();                   // Synchronize the event timer to the current seconds time
  TestRunning = 1;
  SetMotor(HIGH);
  Serial.println("Test Started by user");
  lcd.setCursor(8,0);              //Set cursor to first position on first row
  lcd.print("0        ");          //clear out any digits that were there
}

void StopMotor(void)
{
  TestRunning = 0;
  SetMotor(LOW);
  eeprom_write_dword(&NonVolatileTotalCounts,TotalRotations);  // Store the Total shaft rotations to EEPROM
  eeprom_write_dword(&NonVolatileTestCounts,TestRotations);    // Store the Test shaft rotations to EEPROM
  Serial.println("Test Paused by user");
}

void ResetSensorCounts(void)
{
  TestRotations = 0;
  TotalRotations = 0;
  
  lcd.setCursor(8,0);              //Set cursor to first position on first row
  lcd.print("0        ");          //clear out any digits that were there
  lcd.setCursor(8,1);              //Set cursor to rotations position on second row
  lcd.print("0        ");          //clear out any digits that were there
}

void PrintEventHeader(void)
{
  Serial.println("FB60 Endurance Tester.  Command set:");
  Serial.println("  '1' = Start Test");
  Serial.println("  '0' = Pause Test");
  Serial.println("  'r' = Reset Counts");

  lcd.setCursor(0,0);       //Set cursor to first position on first row
  lcd.print(" Test");     // Print a header message on the LCD.
  lcd.setCursor(0,1);       //Set cursor to first position on first row
  lcd.print("Total");     // Print a header message on the LCD.
}

void PrintEventData(void)
{
  long CurrentShaftCount;
  
  lcd.setCursor(8,0);              //Set cursor to Test Rotations position on first row
  lcd.print(TestRotations);
  lcd.setCursor(8,1);              //Set cursor to Total Rotations position on second row
  lcd.print(TotalRotations);
  
  if (EventLogTimer < Seconds())
  {
    EventLogTimer += EventInterval;
    eeprom_write_dword(&NonVolatileTotalCounts,TotalRotations);  // Store the total rotations to EEPROM each minute
    eeprom_write_dword(&NonVolatileTestCounts,TestRotations);    // Store the test rotations to EEPROM each minute
    // This should last 1.9 years @ 24/7 operation for 1,000,000 EEPROM write cycles
  }
}

void AccumulateShaftRotations(void)
{
  TotalRotations++;
  TestRotations++;
}

unsigned long Seconds(void)
{
  return (millis()/1000);
}

void SetLED(bool State)
{
  if(State == HIGH) digitalWrite(LED, HIGH);
  else digitalWrite(LED,LOW);
}

void SetMotor(bool State)
{
  if(State == HIGH) digitalWrite(Motor, HIGH);
  else digitalWrite(Motor, LOW);
}
