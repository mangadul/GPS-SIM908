/*
 *  
 *  Version 1.0 - 2015
 *  Author: Abdul Muin
 *  
 *  Board Arduino UNO
 *  GPS/GPRS/GSM Shield V.3
 *  Ref: http://www.dfrobot.com/wiki/index.php/GPS/GPRS/GSM_Module_V3.0_%28SKU:TEL0051%29
 *  
 */

#include <SoftwareSerial.h>

int8_t answer;
int onModulePin = 3;

int baud_rate = 9600;
int pin_gsm = 3;
int pin_gps = 4;
int pin_power = 5;
boolean debug = true;
boolean ready_to_go = false;

char data[100];
int data_size;

char aux_str[30];
char aux;
int x = 0;
char N_S,W_E;

char url[] = "demo.alphamedia.web.id/arduino/gps";
char frame[200];

char latitude[15];
char longitude[15];
char altitude[6];
char date[16];
char time[7];
char satellites[3];
char speedOTG[10];
char course[10];
char imei;
char cimei;

// setting no telp untuk menerima sms dan telepon
char phone_number[]="+6289689321978";  
char aux_string[30];

int ledpin = 13;
int ledgps = 10;
int ledground = 11;
int ledengine = 8;

char inchar;

uint8_t istrack = 0;

void setup(){
    
  Serial.begin(baud_rate);
  delay(5000); 

  if(debug)
  {
    Serial.println(F("\n****************************"));
    Serial.println(F("STARTING SYSTEM Read AT stream"));
    Serial.println(F("******************************"));
  }
  
  pinMode(pin_gsm,OUTPUT);            // Set the pins
  pinMode(pin_gps,OUTPUT);
  pinMode(pin_power,OUTPUT);
  pinMode(ledengine,OUTPUT);
  
  pinMode(ledpin,OUTPUT);
  pinMode(ledgps,OUTPUT);
  pinMode(ledground,OUTPUT);    
  
  digitalWrite(ledground,LOW);

  digitalWrite(ledpin,HIGH);
  powerUpSim908:
  if(powerUpSim908())
  {
    delay(1000);
    
    digitalWrite(ledgps,HIGH);
    
    gps_on();
    
    digitalWrite(ledgps,LOW);
          
    gsm_enable();
    ready_to_go = true;
    
    sendATcommand("AT", "OK", 2000);  
    //sendATcommand("AT+CPIN=?", "OK", 2000);  
    sendATcommand("AT+CPIN?", "OK", 2000);  
    sendATcommand("AT+CSQ", "OK", 2000);
    while( (sendATcommand("AT+CREG?", "+CREG: 0,1", 1000)) == 0 || sendATcommand("AT+CREG?", "+CREG: 0,5", 1000));
    
    sendATcommand("AT+CLIP=1", "OK", 1000); 
    
    digitalWrite(ledgps,HIGH);
    while ( start_GPS() == 0);
    delay(2000);
    digitalWrite(ledgps,LOW);
    digitalWrite(ledpin,LOW);

    sendATcommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
    sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet\"", "OK", 2000);
    sendATcommand("AT+SAPBR=3,1,\"USER\",\"\"", "OK", 2000);
    sendATcommand("AT+SAPBR=3,1,\"PWD\",\"\"", "OK", 2000);
  
    Serial.println("AT+SAPBR=1,1");
    delay(10000);
    Serial.println("AT+HTTPINIT");
    delay(2000);
    Serial.println("AT+HTTPPARA=\"CID\",1");
    delay(2000);              

    while (sendATcommand("AT+SAPBR=4,1", "OK", 20000) == 0)
    {
        delay(5000);
    }

    if(debug)
    {
      Serial.println("AT+CMGD=1,4");
      Serial.println("Hapus pesan masuk....");
      
      Serial.println(F("\n****************************"));
      Serial.println(F("READY TO GO\n"));
      Serial.println(F("****************************\n"));
    }  
  }
  else
  {
    ready_to_go = false;
    if(debug)
    {
      Serial.println(F("\nNOT READY TO GO.\nCheck the battery level.\n"));
      digitalWrite(ledpin,LOW);
    } 
  };
  
}


void loop(){
if(ready_to_go)
{
  
  if(Serial.available()>0)
  {

    // untuk membaca pesan masuk di uriutan pertama
    inchar=Serial.read();
    if(inchar=='T')
    {
      delay(10);
      inchar=Serial.read(); 
      if (inchar=='I')                                      
      {      
        delay(10);
        // read message at position 1 
        Serial.println("AT+CMGR=1");  
        delay(10);
      }
    }
    
    // start and stop engine
    else if (inchar=='S')
    {
      // perintah sms "SE" untuk menghidupkan mesin
      delay(10);
      inchar=Serial.read(); 
      if (inchar=='E')                                     
      {
        Serial.println("(SE) Hidupkan Mesin ...");                   
        delay(10);
        digitalWrite(ledengine,HIGH);                         
        delay(50);        
        Serial.println("AT+CMGD=1,4");    // hapus seluruh pesan masuk (sms inbox)
        delay(100);
      }
      
      // perintah sms "ST" untuk mematikan mesin
      if (inchar=='T')                                    
      {
        Serial.println("(ST) Matikan Mesin ...");                   
        delay(10);
        digitalWrite(ledengine,LOW);                         
        delay(50);
        Serial.println("AT+CMGD=1,4");                   
        delay(100);
      }
    }
    
    // gps
    else if (inchar=='G')
    {
      
      // perintah sms "GN" untuk memulai mode pemindaian GPS dan mengirimkan titik lokasi ke server
      delay(10);            
      inchar=Serial.read(); 
      if (inchar=='N')                                     
      {
        Serial.println("(GN) Kirim koordinat lokasi ...");                   
        delay(10);
        digitalWrite(ledpin,HIGH);                         
        delay(50);
        get_GPS();  
        send_HTTP(); // kirim titik lokasi ke server
        delay(2000);
        Serial.println("AT+CMGD=1,4");                   
        delay(500);
        digitalWrite(ledpin,LOW);  
      }

      // perintah sms "GM" untuk mengirimkan titik lokasi ke no hp
      if (inchar=='M')                                     
      {
        Serial.println("(GS) Kirim koordinat lokasi vis SMS...");                   
        delay(10);
        digitalWrite(ledpin,HIGH);                         
        get_GPS();
        boolean iskirim = false;
        if(!iskirim) {
          sendSMS();
          iskirim = true;
        }
        delay(1000);
        Serial.println("AT+CMGD=1,4"); 
        delay(100);
        digitalWrite(ledpin,LOW);  
      }

      // perintah sms "GS" untuk menghentikan tracking GPS      
      if (inchar=='S')                                    
      {
        Serial.println("(GS) Stop GPS Tracking ...");                   
        delay(10);
        digitalWrite(ledpin,LOW);                         
        delay(50);
        istrack = 0;
        //stop_gps();
        gpsTracking(0);        
        delay(5000);
        Serial.println("AT+CMGD=1,4");                   
        delay(100);
      }

      if (inchar=='T')
      {
        Serial.println("(GT) Start GPS Tracking ...");                   
        delay(10);
        get_GPS();        
        Serial.println("AT+CMGD=1,4"); 
        delay(100);
        gpsTracking(istrack);
      }

      if (inchar=='1')
      {
        Serial.println("(G1) set GPS Tracking ON ...");                   
        istrack=1;
        Serial.println("AT+CMGD=1,4"); 
        delay(100);
      }

      if (inchar=='0')
      {
        Serial.println("(G0) set GPS Tracking OFF ...");                   
        istrack=0;
        Serial.println("AT+CMGD=1,4"); 
        delay(100);
      }
      
    } 
    
    // melakukan panggilan ke no hp
    else if (inchar=='P')
    {
      
        // perintah sms "PC" untuk menghubungi no 
        delay(10);
        inchar=Serial.read(); 
        if (inchar=='C')                                     
        {
          Serial.println("(PC) Lakukan panggilan telepon...");                   
          delay(10);
          digitalWrite(ledpin,HIGH);                         
          delay(50);
          Serial.println("AT");   
          delay(2000);        
          //Make a phone call
          Serial.println("ATD+6289689321978;");
          while(1);
          delay(100);
          Serial.println("AT+CMGD=1,4");                   
          delay(100);
          digitalWrite(ledpin,LOW);                         
        }
        
        // perintah "PS" untuk menutup telepon 
        if (inchar=='S')                                    
        {
          Serial.println("(PS) Tutup panggilan telepon...");                   
          delay(10);
          digitalWrite(ledpin,LOW);                         
          delay(50);
          //Make a phone call
          Serial.println("ATH");
          delay(100);
          Serial.println("AT+CMGD=1,4");                   
          delay(100);
        }
        
     }   
      
  }
  
} //end ready_togo
  
} // end loop

void stop_gps(){
    uint8_t answer=0;
  sendATcommand("AT+CGPSPWR=0", "OK", 2000);  
  gps_stop();
  answer = sendATcommand("AT", "OK", 2000);    
    if (answer == 0)
    {          
        // waits for an answer from the module
        while(answer == 0){  
            answer = sendATcommand("AT", "OK", 2000);    
        }   
    }
}

int8_t start_GPS(){
    unsigned long previous;        
    previous = millis();
    // starts the GPS
    //sendATcommand("AT", "OK", 2000);
    sendATcommand("AT+CGPSPWR=1", "OK", 2000);
    sendATcommand("AT+CGPSRST=0", "OK", 2000);
      
    // waits for fix GPS
    while(( (sendATcommand("AT+CGPSSTATUS?", "2D Fix", 5000) || 
        sendATcommand("AT+CGPSSTATUS?", "3D Fix", 5000)) == 0 ) && 
        ((millis() - previous) < 90000));
 
    if ((millis() - previous) < 90000)
    {
        return 1;
    }
    else
    {
        return 0;    
    }
}

int8_t get_GPS(){

    int8_t counter, answer;
    long previous;

    // First get the NMEA string
    // Clean the input buffer
    while( Serial.available() > 0) Serial.read(); 
    // request Basic string
    sendATcommand("AT+CGPSINF=0", "AT+CGPSINF=0\r\n\r\n", 2000);
    counter = 0;
    answer = 0;
    // Initialize the string
    memset(frame, '\0', 100);    
    previous = millis();
    // this loop waits for the NMEA string
    do{

        if(Serial.available() != 0){    
            frame[counter] = Serial.read();
            counter++;
            // check if the desired answer is in the response of the module
            if (strstr(frame, "OK") != NULL)    
            {
                answer = 1;
            }
        }
        // Waits for the asnwer with time out
    }
    while((answer == 0) && ((millis() - previous) < 2000));  

    frame[counter-3] = '\0'; 
    
    // Parses the string 
    strtok(frame, ",");
    strcpy(longitude,strtok(NULL, ",")); // Gets longitude
    strcpy(latitude,strtok(NULL, ",")); // Gets latitude
    strcpy(altitude,strtok(NULL, ".")); // Gets altitude 
    strtok(NULL, ",");    
    strcpy(date,strtok(NULL, ".")); // Gets date
    strtok(NULL, ",");
    strtok(NULL, ",");  
    strcpy(satellites,strtok(NULL, ",")); // Gets satellites
    strcpy(speedOTG,strtok(NULL, ",")); // Gets speed over ground. Unit is knots.
    strcpy(course,strtok(NULL, "\r")); // Gets course

    convert2Degrees(latitude);
    convert2Degrees(longitude);
    
    return answer;
}

/* convert2Degrees ( input ) - performs the conversion from input 
 * parameters in  DD°MM.mmm’ notation to DD.dddddd° notation. 
 * 
 * Sign '+' is set for positive latitudes/longitudes (North, East)
 * Sign '-' is set for negative latitudes/longitudes (South, West)
 *  
 */
int8_t convert2Degrees(char* input){

    float deg;
    float minutes;
    boolean neg = false;    

    //auxiliar variable
    char aux[10];

    if (input[0] == '-')
    {
        neg = true;
        strcpy(aux, strtok(input+1, "."));

    }
    else
    {
        strcpy(aux, strtok(input, "."));
    }

    // convert string to integer and add it to final float variable
    deg = atof(aux);

    strcpy(aux, strtok(NULL, '\0'));
    minutes=atof(aux);
    minutes/=1000000;
    if (deg < 100)
    {
        minutes += deg;
        deg = 0;
    }
    else
    {
        minutes += int(deg) % 100;
        deg = int(deg) / 100;    
    }

    // add minutes to degrees 
    deg=deg+minutes/60;


    if (neg == true)
    {
        deg*=-1.0;
    }

    neg = false;

    if( deg < 0 ){
        neg = true;
        deg*=-1;
    }
    
    float numeroFloat=deg; 
    int parteEntera[10];
    int cifra; 
    long numero=(long)numeroFloat;  
    int size=0;
    
    while(1){
        size=size+1;
        cifra=numero%10;
        numero=numero/10;
        parteEntera[size-1]=cifra; 
        if (numero==0){
            break;
        }
    }
   
    int indice=0;
    if( neg ){
        indice++;
        input[0]='-';
    }
    for (int i=size-1; i >= 0; i--)
    {
        input[indice]=parteEntera[i]+'0'; 
        indice++;
    }

    input[indice]='.';
    indice++;

    numeroFloat=(numeroFloat-(int)numeroFloat);
    for (int i=1; i<=6 ; i++)
    {
        numeroFloat=numeroFloat*10;
        cifra= (long)numeroFloat;          
        numeroFloat=numeroFloat-cifra;
        input[indice]=char(cifra)+48;
        indice++;
    }
    input[indice]='\0';
}

void send_HTTP(){
    
    uint8_t answer=0;
    // Initializes HTTP service
    sendATcommand("AT+CGATT=1", "OK", 2000);
    sendATcommand("AT+SAPBR=2,1", "+SAPBR:", 2000);
    //answer = sendATcommand("AT+HTTPINIT", "OK", 10000);
    sendATcommand("AT+HTTPINIT", "OK", 5000);
    // Sets CID parameter
    sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 2000);
    Serial.print("AT+HTTPPARA=\"URL\",\"http://demo.alphamedia.web.id/arduino/gps/trak.php?visor=false&latitude=");
    Serial.print(latitude);
    Serial.print("&longitude=");
    Serial.print(longitude);
    Serial.print("&altitude=");
    Serial.print(altitude);              
    Serial.print("&time=");
    Serial.print(date);              
    Serial.print("&satellites=");
    Serial.print(satellites);              
    Serial.print("&speedOTG=");
    Serial.print(speedOTG);              
    Serial.print("&course=");
    Serial.print(course);              
    Serial.println("\"");
    delay(2000);
    Serial.println("AT+HTTPACTION=0"); //now GET action
    delay(2000);    
    Serial.println(F("Done!"));
    sendATcommand("AT+HTTPTERM", "OK", 5000);
    
}

int8_t sendATcommand(char* ATcommand, char* expected_answer1, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;
    // Initialize the string
    memset(response, '\0', 100);    
    delay(100);
    // Clean the input buffer    
    while( Serial.available() > 0) Serial.read();    
    // Send the AT command 
    Serial.println(ATcommand);    

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        if(Serial.available() != 0){    
            response[x] = Serial.read();
            x++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expected_answer1) != NULL)    
            {
                answer = 1;
            }
        }
        // Waits for the asnwer with time out
    }
    while((answer == 0) && ((millis() - previous) < timeout));    
    return answer;
}


boolean sendSMS(){
    delay(3000);    
    Serial.println("Konek ke jaringan...");
    while( (sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || 
            sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0 );

    Serial.print("Setting mode SMS...");
    
    // sets the SMS mode to text
    sendATcommand("AT+CMGF=1", "OK", 1000);    
    Serial.println("Kirim SMS");
    
    sprintf(aux_string,"AT+CMGS=\"%s\"", phone_number);
    // send the SMS number
    answer = sendATcommand(aux_string, ">", 2000);    
    if (answer == 1)
    {
        Serial.print("Posisi sekarang ada di: ");
        Serial.print("Latitude: ");
              int i = 0;
              while(latitude[i]!=0){
                Serial.print(latitude[i]);
                i++;
              }
              Serial.print(" / Longitude: ");
              i = 0;
              while(longitude[i]!=0){
                Serial.print(longitude[i]);
                i++;
              }
        Serial.write(0x1A);
        answer = sendATcommand("", "OK", 20000);
        if (answer == 1)
        {
            Serial.println("AT+CMGD=1,4");                   
            delay(500);          
            Serial.print("sms terkirim ");    
        }
        else
        {
            Serial.print("Error ");
        }
    }
    else
    {
        Serial.print("error ");
        Serial.println(answer, DEC);
    }
    return true;
}

void gps_enable(void)
{
  if(debug)
  {
    Serial.println(F("\nEnabling GPS ..."));
  }
  digitalWrite(pin_gps,LOW);                //Enable GPS mode
  digitalWrite(pin_gsm,HIGH);                //Disable GSM mode
  delay(2000);
}

void gps_stop(void)
{
  if(debug)
  {
    Serial.println(F("\nDisabling GPS ..."));
  }
  digitalWrite(3,HIGH); //Enable the GSM mode
  digitalWrite(4,LOW); //Disable the GPS mode
  delay(2000);      
}


void gsm_enable(void)
{
  if(debug)
  {
    Serial.println(F("\nEnabling GSM ..."));
  }
  digitalWrite(pin_gsm,LOW);                //Enable GSM mode
  digitalWrite(pin_gps,HIGH);               //Disable GPS mode
  delay(2000);
}

boolean powerUpSim908(void)
{
  if(debug)
  {
    Serial.println(F("Powering up SIM908"));  
  }
  boolean turnedON = false;
  //uint8_t answer=0;
  int cont;

  for (cont=0; cont<3; cont++)
  {
    digitalWrite(pin_power,HIGH);
    delay(1500);
    digitalWrite(pin_power,LOW);

    Serial.println(F("Checking if the module is up"));
    if(sendATcommand("AT", "OK", 5000))
    {
    cont = 4; // Leave the loop
    turnedON = true;
    }
    else
    {
      turnedON = false;
      if(debug)
      {
    Serial.println(F("\nTrying agin to turn on SIM908"));  
      }
    };
  }

  if(turnedON)
  {
    if(debug)
    {
      Serial.println(F("Module is tunrned up\n"));
    }
  }
  else
  {
      if(debug)
      {
    Serial.println(F("Module is NOT tunrned ON\n"));  
      }
   }    
    return turnedON;
}

void gps_on()
{
    uint8_t answer=0;  
    uint8_t gpsrst=0;
    Serial.println(F("Turn on GPS Power\n"));  
    answer = sendATcommand("AT+CGPSPWR=1","OK",2000);
    if (answer == 0)
    {
      answer = sendATcommand("AT+CGPSPWR=1","OK",2000);
      while(answer == 0){
        answer = sendATcommand("AT+CGPSPWR=1","OK",2000);
      }
    }
    Serial.println(F("GPS Power ON\n"));  

    Serial.println(F("Set GPS ON autonomy mode...\n"));  
    gpsrst = sendATcommand("AT+CGPSRST=0","OK",2000);
    if (gpsrst == 0)
    {
      gpsrst = sendATcommand("AT+CGPSRST=0","OK",2000);
      while(answer == 0){
        gpsrst = sendATcommand("AT+CGPSRST=0","OK",2000);
      }
    }    
    Serial.println(F("GPS set Mode : Autonomy mode\n"));  
}

void gpsTracking(uint8_t istrack)
{  
  while(istrack == 1)
  {
    digitalWrite(ledpin,HIGH);  
    get_GPS();  
    send_HTTP(); 
    delay(5000);  
    digitalWrite(ledpin,LOW);  
  }
}
