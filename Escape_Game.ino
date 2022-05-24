#include <Keypad.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

#define ARDUINO_RX 11//connect to TX pin of the Serial MP3 Player module
#define ARDUINO_TX 12//connect to RX pin of the Serial MP3 Player module
#define CMD_PLAY  0X01
#define CMD_PAUSE 0X02
#define CMD_NEXT_SONG 0X03
#define CMD_PREV_SONG 0X04
#define CMD_VOLUME_UP   0X05
#define CMD_VOLUME_DOWN 0X06
#define CMD_FORWARD 0X0A // >>
#define CMD_REWIND  0X0B // <<
#define CMD_STOP 0X0E
#define CMD_STOP_INJECT 0X0F//stop interruptting with a song, just stop the interlude

/*5 bytes commands*/
#define CMD_SEL_DEV 0X35
#define DEV_TF 0X01
#define CMD_IC_MODE 0X35
#define CMD_SLEEP   0X03
#define CMD_WAKE_UP 0X02
#define CMD_RESET   0X05

/*6 bytes commands*/
#define CMD_PLAY_W_INDEX   0X41
#define CMD_PLAY_FILE_NAME 0X42
#define CMD_INJECT_W_INDEX 0X43

/*Special commands*/
#define CMD_SET_VOLUME 0X31
#define CMD_PLAY_W_VOL 0X31

#define CMD_SET_PLAY_MODE 0X33
#define ALL_CYCLE 0X00
#define SINGLE_CYCLE 0X01

#define CMD_PLAY_COMBINE 0X45//can play combination up to 15 songs

void sendCommand(int8_t command, int16_t dat );

const byte numRows= 4; //Number of rows on the keypad
const byte numCols= 4; //Number of columns on the keypad
String password = "3080"; //Set password
String unlockCode = "";
String part1KeySeq = "";
String part1Password = "3000";
String part2Word = "";
String part2WordSeq = "";
char *guessWord[] = {"HELP", "EARTH", "RESCUE"};
char first, second, third, fourth, fifth, sixth, seventh;
int pos = 0;
int cursorPos = 8;
int part1Pos = 0;
int part2Pos = 0;
int diffLevel = 2;
int seconds;
unsigned int green_led_pin = 10;
unsigned int red_led_pin = 13;
unsigned int dot_duration = 300;

char *letters[] = {
  // The letters A-Z in Morse code  
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",    
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",  
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."         
};

char keyMap[numRows][numCols]=
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char keyMap2[numRows][numCols]=
{
  {'H', 'O', 'N', 'A'},
  {'F', 'P', 'R', 'B'},
  {'S', 'T', 'U', 'C'},
  {'E', 'L', '#', 'D'}
};

byte rowPins[numRows] = {9,8,7,6}; //Rows 0 to 3
byte colPins[numCols]= {5,4,3,2}; //Columns 0 to 3
Keypad keyPad= Keypad(makeKeymap(keyMap), rowPins, colPins, numRows, numCols);
Keypad letterPad = Keypad(makeKeymap(keyMap2), rowPins, colPins, numRows, numCols);
SoftwareSerial myMP3(ARDUINO_RX, ARDUINO_TX);//create a myMP3 object
static int8_t Send_buf[6] = {0};

//LCD Display
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5); //Pins of the LCD. (RS, E, D4, D5, D6, D7)


void setup() {
  lcd.begin(16, 2); //Initialize LCD
  pinMode(green_led_pin, OUTPUT);
  pinMode(red_led_pin, OUTPUT);
  myMP3.begin(9600);//initialize serial communication at 9600 bps
  delay(500);//Wait chip initialization is complete
  sendCommand(CMD_SEL_DEV, DEV_TF);//select the TF card
  delay(200);//wait for 200ms
}

void loop() {
  digitalWrite(red_led_pin, LOW);
  lcd.setCursor(0, 0); //(column, row)
  lcd.print("DOOR LOCKED"); //Print on first row
  lcd.setCursor(0,1); //(column, row)
  lcd.print("unlock: "); //Print on second row, occupying 8/16 columns
  seconds = millis()/1000; //Hidden timer for "randomness"
  char keyNumber = keyPad.waitForKey(); //Get the value of entered key
  
  if (!keyNumber == NO_KEY){ // if got key entered
    lcd.setCursor(cursorPos, 1); //Print from column 11 onwards
    lcd.print(keyNumber); //Print value of pressed number
    unlockCode += String(keyNumber);
    pos++;
    cursorPos++;
  }
  
  if (unlockCode == password){
    lcd.clear();
    lcd.setCursor(0, 0);
    digitalWrite(red_led_pin, HIGH);
    playWithVolume(0X1801);//play mixed audio with volume 15(0x0F) class
    lcd.print("Audio Playing..");
    delay(31000);
    playWithVolume(0X1804);
    pos = 0;
    cursorPos = 8;
    unlockCode = "";
    lcd.clear();
    int keepCodeinLoop = 1;
    
    while (keepCodeinLoop){
      lcd.setCursor(0, 0);
      lcd.print("Enter code: ");
      char keyNumber = keyPad.waitForKey(); //Get the value of entered key
      
      if (!keyNumber == NO_KEY && keyNumber != '#'){ // if got key entered
        lcd.setCursor(part1Pos, 1); //Print from column 11 onwards
        lcd.print(keyNumber); //Print value of pressed number
        part1KeySeq += String(keyNumber);
        part1Pos++;
      }

      if (keyNumber == '#')
      {
        playWithVolume(0X1802);//play mixed audio with volume 15(0x0F) class
      }

      if (part1KeySeq == part1Password){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Check the LIGHT!");
        delay(5000);
        part1Pos = 0;
        keepCodeinLoop = 0; // Exit while loop
      }

      if (part1Pos == 4 && part1KeySeq != part1Password){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Listen again!");
        delay(3000);
        part1Pos = 0;
        part1KeySeq = "";
        lcd.clear();
      }
      delay(100);
    }

    int keepPart2inLoop = 1;
    part2Word = guessWord[seconds%3];
    get_word();
    lcd.clear();

    while (keepPart2inLoop){
      lcd.setCursor(0, 0);
      lcd.print("Guess it!");
      lcd.setCursor(11, 0);
      lcd.print(part2Word.length());

      char keyLetter = letterPad.waitForKey(); // Get letters entered

      if (!keyLetter == NO_KEY && keyLetter != '#'){ // if got key entered
        lcd.setCursor(part2Pos, 1); //Print from column 11 onwards
        lcd.print(keyLetter); //Print value of pressed number
        part2WordSeq += String(keyLetter);
        part2Pos++;
      }

      if (keyLetter == '#')
      {
        lcd.setCursor(0, 1);
        lcd.clear();
        part2Pos = 0;
        lcd.setCursor(part2Pos, 1);
      }

      if (part2WordSeq == part2Word){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sending signal..");
        playWithVolume(0X1803);
        delay(2000);
        for (int i = 0; i < 10; i++)
        {
          digitalWrite(red_led_pin, LOW);
          delay(500);
          digitalWrite(red_led_pin, HIGH);
          delay(500);
        }
        part2Pos = 0;
        lcd.clear();
        keepPart2inLoop = 0; // Exit while loop
      }

      if (part2Pos == part2Word.length() && part2WordSeq != part2Word){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Check again!");
        get_word();
        delay(3000);
        part2Pos = 0;
        part2WordSeq = "";
        lcd.clear();
      }
      delay(100);
    }
  }
  if (pos == 4 && unlockCode != password)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid Key!");
    delay(500);
    unlockCode = "";
    pos = 0;
    cursorPos = 8;
    delay(1000);
    lcd.clear();
  }
  delay(100);
}

void flash_morse_code(char *morse_code) {
    
  unsigned int i = 0;
  while (morse_code[i] != NULL) {
    flash_dot_or_dash(morse_code[i]);
    i++;
  }
  delay(dot_duration * 10); //Delay between letters
}

void flash_dot_or_dash(char dot_or_dash) {
 
  digitalWrite(green_led_pin, HIGH);
  if (dot_or_dash == '.') {
    delay(dot_duration); //Delay for dot           
  }
  else {
    delay(dot_duration * 5); //Delay for dash           
  }
  digitalWrite(green_led_pin, LOW);
  delay(dot_duration); 
}

void get_word(void)
    {
      if (part2Word.length() == 4)
      {
        first = part2Word[1];
        second = part2Word[3];
        third = part2Word[2];
        fourth = part2Word[0];
        // EPLH
        flash_morse_code(letters[first - 'A']);
        flash_morse_code(letters[second - 'A']);
        flash_morse_code(letters[third - 'A']);
        flash_morse_code(letters[fourth - 'A']);
      }
      if (part2Word.length() == 5)
      {
        first = part2Word[4];
        second = part2Word[1];
        third = part2Word[0];
        fourth = part2Word[3];
        fifth = part2Word[2];
        // HAETR
        flash_morse_code(letters[first - 'A']);
        flash_morse_code(letters[second - 'A']);
        flash_morse_code(letters[third - 'A']);
        flash_morse_code(letters[fourth - 'A']);
        flash_morse_code(letters[fifth - 'A']);
      }
      if (part2Word.length() == 6)
      {
        first = part2Word[4];
        second = part2Word[3];
        third = part2Word[0];
        fourth = part2Word[5];
        fifth = part2Word[1];
        sixth = part2Word[2];
        // UCREES
        flash_morse_code(letters[first - 'A']);
        flash_morse_code(letters[second - 'A']);
        flash_morse_code(letters[third - 'A']);
        flash_morse_code(letters[fourth - 'A']);
        flash_morse_code(letters[fifth - 'A']);
        flash_morse_code(letters[sixth - 'A']);
      }
    }

void setVolume(int8_t vol)
{
  mp3_5bytes(CMD_SET_VOLUME, vol);
}
void playWithVolume(int16_t dat)
{
  mp3_6bytes(CMD_PLAY_W_VOL, dat);
}

/*cycle play with an index*/
void cyclePlay(int16_t index)
{
  mp3_6bytes(CMD_SET_PLAY_MODE, index);
}

void setCyleMode(int8_t AllSingle)
{
  mp3_5bytes(CMD_SET_PLAY_MODE, AllSingle);
}


void playCombine(int8_t song[][2], int8_t number)
{
  if (number > 15) return; //number of songs combined can not be more than 15
  uint8_t nbytes;//the number of bytes of the command with starting byte and ending byte
  nbytes = 2 * number + 4;
  int8_t Send_buf[nbytes];
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = nbytes - 2; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = CMD_PLAY_COMBINE;
  for (uint8_t i = 0; i < number; i++) //
  {
    Send_buf[i * 2 + 3] = song[i][0];
    Send_buf[i * 2 + 4] = song[i][1];
  }
  Send_buf[nbytes - 1] = 0xef;
  sendBytes(nbytes);
}


void sendCommand(int8_t command, int16_t dat = 0)
{
  delay(20);
  if ((command == CMD_PLAY_W_VOL) || (command == CMD_SET_PLAY_MODE) || (command == CMD_PLAY_COMBINE))
    return;
  else if (command < 0x10)
  {
    mp3Basic(command);
  }
  else if (command < 0x40)
  {
    mp3_5bytes(command, dat);
  }
  else if (command < 0x50)
  {
    mp3_6bytes(command, dat);
  }
  else return;

}

// MP3 SERIAL FUNCTION
void mp3Basic(int8_t command)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x02; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command;
  Send_buf[3] = 0xef; //
  sendBytes(4);
}
void mp3_5bytes(int8_t command, uint8_t dat)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x03; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command;
  Send_buf[3] = dat; //
  Send_buf[4] = 0xef; //
  sendBytes(5);
}
void mp3_6bytes(int8_t command, int16_t dat)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x04; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command;
  Send_buf[3] = (int8_t)(dat >> 8);//datah
  Send_buf[4] = (int8_t)(dat); //datal
  Send_buf[5] = 0xef; //
  sendBytes(6);
}
void sendBytes(uint8_t nbytes)
{
  for (uint8_t i = 0; i < nbytes; i++) //
  {
    myMP3.write(Send_buf[i]) ;
  }
}
// MP3 SERIAL FUNCTION END
