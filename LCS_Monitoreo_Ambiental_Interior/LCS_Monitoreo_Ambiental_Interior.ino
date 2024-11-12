// =============== Importamos librerias para comunicacion con dispositivos ========================================
#include <DallasTemperature.h>         //libreria para comunicacion con sensor DS18B20
#include <Adafruit_BME280.h>           //libreria para comunicacion con sensor BME280
#include <OneWire.h>                   //libreria para comunicacion por protocolo onewire
#include <RTClib.h>                    //libreria para comunicacion con RTC DS3231
#include <MHZ19.h>                     //libreria para comunicación con sensor MHZ19B
#include <math.h>                      //libreria para incluir operaciones matematicas
#include "PMS.h"                       //libreria para comunicacion con sensor PMS5003
#include <SD.h>                        //libreria para comunicacion con tarjeta SD

//============================ Configuración de dispositivos ======================================================== 

//Configuracion del SD
#define chipSelect  53                              //asigamos el PIN digital 53 para chipselect

//Configuracion inicial del RTC DS3231
RTC_DS3231 rtc;                                    //asignamos la instruccion del DS3231 a la variable rtc
DateTime now;                                      //asiganmos la instruccion de solicitud de tiempo a la variable now
#define minute_step 5                              //definimos tiempo de muestreo en minutos para guardar en SD
#define second_step 10                             //definimos tiempo de muestreo en segundos para ver en monitor serial


//Configuracion inicial para el sensor PMS5003 (RX =19, TX =18)
PMS pms(Serial1);                                  //indicamos que la variable pms estará asociada al protocolo UART Serial1
PMS::DATA data;                                    //vinculamos los datos recabos a la variable data

//Configuración inicial para el sensor MHZ19B (RX =17, TX =16)
MHZ19 myMHZ19;                                     //asignamos la instruccion del MHZ19B a la variable myMHZ19
unsigned long timer = 0;                           //contador para temporizar mediciones

//Configuración inicial para el sensor DS18B20
#define pinDatosDQ   45                            //asignamos el pin mediante el cual se comunicará el sensor 
OneWire oneWireObjeto(pinDatosDQ);                 //indicamos que la comunicación será por el protocolo onewire
DallasTemperature sensorDS18B20(&oneWireObjeto);   //indicamos que la información recaba proviene de un sensor DS18B20

//Configuracion del sensor BME280
#define BME280_I2C_ADDRESS  0x76                   //indicamos el numero de direccion I2C para el dispositivo
Adafruit_BME280  bme;                              //asiganmos las instrucciones del sensor a la variable bme

//variables para almacenar informacion
int pm01  = 0;               //datos de particulas PM_01
int pm25  = 0;               //datos de particulas PM_2.5
int pm10  = 0;               //datos de particulas PM_10
int CO2   = 0;               //datos de concentracion de CO2

float temp=0;                //datos de temperatura ambiente
float pres=0;                //datos de presion atmosferica
float rehu=0;                //datos de humedad relativa
float CO  =0;                //datos de concentracion de CO

float MQ7 =0;
int MQ7s  =0;

byte p_minute   =0;           //variable para almacenar el valor de los minutos en la codicional
byte p_second   =0;           //variable para almacenar el valor de los segundos en la codicional
byte contador_m =0;           //contador de minutos
byte contador_s =0;           //contador de segundos

byte segundo = 0;
byte minuto  = 0;
byte hora    = 0;
byte dia     = 0;
byte mes     = 0;
int anno    =  0;


//============================ Inicializamos dispositivos ====================================================

void setup() {
 Serial.begin(9600);                              //inicializamos comunicacion serial con la computadora
 Serial1.begin(9600);                             //inicializamos comunicacion serialcon sensor PMS3005
 Serial1.setTimeout(1500);                        //ajuste de tiempo de medición para el sensor PMS3005

 Serial2.begin(9600);                             //inicialización de la comunicación serial con el MHZ19B
 myMHZ19.begin(Serial2);                          //asignamos la comunición a las instrucciones myMHZ19
 myMHZ19.autoCalibration();                       //autocalibramos el sensor

 sensorDS18B20.begin();                           //inicializamos comunicacion con sensor DS18B20

 rtc.begin();                                     //inicializamos con el dispositivo I2C_1 (RTC)
 //opcion para configurar el RTC de ser necesario
 //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 //rtc.adjust(DateTime(2023, 5, 12, 15, 47, 0)); //(año,mes,dia,hora, min,sec)
 
 bme.begin(BME280_I2C_ADDRESS);                   //inicializamos con el dispositivo I2C_2 (BME280)

 // Inicializamos tarjeta SD
 if (!SD.begin(chipSelect)) {
  }

 File logFile = SD.open("datalog5.txt", FILE_WRITE);
 if (logFile) {
   logFile.println(F("Fecha,Hora,Temp(C),Pr(hPa),HR(%),PM01,PM2.5,PM10,CO2"));
   logFile.close();
  }


 //Configuramos incialmente los datos inciar el registro de tiempo
 now = rtc.now();                                 
 p_second =now.second();
 p_minute =now.minute();
}


//============================ Ejecutamos codigo principal ====================================================
void loop() {

  // ------------------------ CODIGO PARA MEDIR SENSORES ----------------------------------------
  //Obtenemos valores del sensor (DS18B20)
  sensorDS18B20.requestTemperatures();                   // solicitamos la temperatura al sensor
  temp = sensorDS18B20.getTempCByIndex(0);               // almacenamos la temperatura en °C

  //Obtenemos valores del sensor GY-BME280
  pres = bme.readPressure() / 100.0F;                    // Solicitamos y almacenamos la pression barometrica en hPa
  rehu = bme.readHumidity();                             // Solicitamos y almacenamos la humedad relativa en %
  
  //Obtenemos valores de calidad de aire (PMS5003)
  if(pms.readUntil(data)){                               //solicitamos lectura al sensor
    pm01 = data.PM_AE_UG_1_0;                            //almacenamos el valor de pm01  "(ug/m3)"
    pm25 = data.PM_AE_UG_2_5;                            //almacenamos el valor de pm2.5 "(ug/m3)"
    pm10 = data.PM_AE_UG_10_0;}                          //almacenamos el valor de pm10  "(ug/m3)"

  // Obtener la medición de CO2
  if (millis() - timer >= 2000){                         //ejecuamos la solicitud con un tiempo de espera de 2000 milis
    CO2 = myMHZ19.getCO2();                              //obtenemos el valor del CO2
    timer = millis();}                                   //reseteamos el timer

  // ------------------------ CODIGO GUARDAR INFORMACION POR TIME_STEP ----------------------------------------
  now = rtc.now();                                       // solicitamos la informacion de la fecha y hora
  if (p_second !=now.second()){                          //condicional que se activa cuando los minutos son distintos
  contador_s = contador_s+1;                             //incrementamos el contador en un minuto
  p_second =  now.second();                              //guardamo la información del nuevo minuto para comparar

  segundo = now.second();
  minuto  = now.minute();
  hora    = now.hour();
  dia     = now.day();
  mes     = now.month();
  anno    = now.year();}


  if( contador_s == second_step){
  // Imprimmos datos horarios
   Serial.print(anno);
   Serial.print('/');
   Serial.print(mes);
   Serial.print('/');
   Serial.print(dia);
   Serial.print("  ");
   Serial.print(hora);
   Serial.print(':');
   Serial.print(minuto);
   Serial.print(':');
   Serial.print(segundo);
   Serial.print("  ");

  //imprimimos datos de los sensores
  Serial.print(temp);
  Serial.print("  ");
  Serial.print(pres);
  Serial.print("  ");
  Serial.print(rehu);
  Serial.print("  ");
  Serial.print(pm01);
  Serial.print("  ");
  Serial.print(pm25);
  Serial.print("  ");
  Serial.print(pm10);
  Serial.print("  ");
  Serial.println(CO2);
  contador_s =0;
 }
  
  now = rtc.now();                                       // solicitamos la informacion de la fecha y hora
  if (p_minute !=now.minute()){                          //condicional que se activa cuando los minutos son distintos
  contador_m = contador_m+1;                             //incrementamos el contador en un minuto
  p_minute =  now.minute();}                             //guardamo la información del nuevo minuto para comparar

 if( contador_m == minute_step){                         //condicional para guardar datos cada vez que ocurra el time step
 
 File logFile = SD.open("datalog5.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (logFile) {

  logFile.print(anno);
  logFile.print('/');
  logFile.print(mes);
  logFile.print('/');
  logFile.print(dia);
  logFile.print(",");
  logFile.print(hora);
  logFile.print(':');
  logFile.print(minuto);
  logFile.print(':');
  logFile.print(segundo);
  logFile.print(",");

  logFile.print(temp);
  logFile.print(",");
  logFile.print(pres);
  logFile.print(",");
  logFile.print(rehu);
  logFile.print(",");
  logFile.print(pm01);
  logFile.print(",");
  logFile.print(pm25);
  logFile.print(",");
  logFile.print(pm10);
  logFile.print(",");
  logFile.println(CO2);
  
  logFile.close();}

 contador_m =0;}                                           //reseteamos el contador
}
