#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  //a veces la dirección no es 0x3f. Cambiar a 0x27 si no funciona.
//Bibliotecas de huellas dactilares
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
int getFingerprintIDez();
SoftwareSerial mySerial(10, 11);  // el pin #2 es IN desde el sensor (cable VERDE)/ el pin #3 es OUT desde el arduino (cable BLANCO)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
//Bibliotecas de reloj en tiempo real
#include <DS3231.h>
DS3231 rtc(SDA, SCL); // Inicia el DS3231 usando la interfaz de hardware
//Bibliotecas de lectura/escritura de tarjeta SD
#include <SPI.h>
#include <SD.h>
File myFile;

/////////////////////////////////////Entradas y salidas////////////////////////////////////////////////
int scan_pin = 13;      //Pin para el botón de escaneo
int add_id_pin = 12;    //Pin para el botón de agregar nuevo ID
int close_door = 9;     //Pin para el botón de cerrar la puerta
int green_led = 8;      //LEDs adicionales para etiquetas de puerta abierta o cerrada
int red_led = 7;
int rele = 26;          //Pin para la señal PWM del rele


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////Variables editables/////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
int main_user_ID = 1;                 //Cambia este valor si deseas un usuario principal diferente
int door_open_degrees = 90;
int door_close_degrees = 0;
String file_name_to_save = "blackBox.txt";
//////////////////////////////////////////////////////////////////////////////////////////////////////

//Otras variables usadas en el código. ¡No cambiar!
bool scanning = false;
int counter = 0;
int id_ad_counter = 0;
bool id_ad = false;
uint8_t num = 1;
bool id_selected = false;
uint8_t id;
bool first_read = false;
bool main_user = false;
bool add_new_id = false;
bool door_locked = true;

void setup() {
  Serial.begin(57600);        //Iniciar comunicación serial para datos TX RX de huellas dactilares.
  rtc.begin();                //Iniciar el reloj en tiempo real (recuerda ajustar la hora antes de este código)
  SD.begin(53);               //Iniciar el módulo de tarjeta SD con el pin CS conectado a D53 del Arduino MEGA. Los otros pines son pines SPI

  //Ahora abrimos el nuevo archivo creado, escribimos los datos y lo cerramos de nuevo//
  myFile = SD.open(file_name_to_save, FILE_WRITE);   //Crear un nuevo archivo en la tarjeta SD con el nombre anterior
  myFile.print("Sistema de bloqueo de puerta iniciado a las ");
  myFile.print(rtc.getTimeStr()); myFile.print(" y día "); myFile.print(rtc.getDateStr());
  myFile.println(" "); myFile.println(" ");
  myFile.close(); 
   
  lcd.init();                     //Iniciar la LCD con comunicación i2c e imprimir texto
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("   Presione SCAN   ");
  lcd.setCursor(0,1);
  lcd.print(" -puerta cerrada- ");

  //Definir pines como salidas o entradas
  pinMode(scan_pin, INPUT);  
  pinMode(add_id_pin, INPUT); 
  pinMode(rele, OUTPUT);  
  pinMode(close_door, INPUT);  
  digitalWrite(rele, HIGH);  //Cerrar puerta
  digitalWrite(red_led, HIGH);         //LED rojo encendido, muestra puerta CERRADA
  digitalWrite(green_led, LOW);        //LED verde apagado
  finger.begin(57600);                // establecer la velocidad de datos para el puerto serial del sensor
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////VOID LOOP////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
/////////////////////////////////////////BOTÓN DE CERRAR PUERTA PRESIONADO///////////////////////////////////
  if (digitalRead(close_door)) {
    door_locked = true;
    delay(300);
    digitalWrite(rele, HIGH); //Cerrar puerta
    digitalWrite(red_led, HIGH);       //LED rojo encendido, muestra puerta CERRADA
    digitalWrite(green_led, LOW);      //LED verde apagado
    lcd.setCursor(0,0);
    lcd.print("  ¡Puerta cerrada!  ");
    lcd.setCursor(0,1);
    lcd.print("                "); 
    delay(2000);     
    lcd.setCursor(0,0);
    lcd.print("   Presione SCAN   ");
    lcd.setCursor(0,1);
    lcd.print(" -puerta cerrada- ");
    myFile = SD.open(file_name_to_save, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- La puerta fue cerrada por un usuario");
    myFile.close(); 
  }


////////////////////////////////Botón de escaneo presionado///////////////////////////////////////////
 if(digitalRead(scan_pin) && !id_ad)
 {
  myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.println(" -- Intento de apertura de puerta");
  myFile.close(); 
  scanning = true;
  lcd.setCursor(0,0);
  lcd.print("  Coloque dedo  ");
  lcd.setCursor(0,1);
  lcd.print("ESCANEANDO------");
 }
  
 while(scanning && counter <= 60)
 {
  getFingerprintID();
  delay(100); 
  counter = counter + 1;
  if(counter == 10)
  {
    lcd.setCursor(0,0);
    lcd.print("  Coloque dedo  ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO  ------");
  }

  if(counter == 20)
  {
    lcd.setCursor(0,0);
    lcd.print("  Coloque dedo  ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO    ----");
  }

  if(counter == 40)
  {
    lcd.setCursor(0,0);
    lcd.print("  Coloque dedo  ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO      --");
  }

  if(counter == 50)
  {
    lcd.setCursor(0,0);
    lcd.print("  Coloque dedo  ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO        ");
  }
  if(counter == 59)
  {
    lcd.setCursor(0,0);
    lcd.print("    ¡Tiempo fuera!    ");
    lcd.setCursor(0,1);
    lcd.print("   ¡Intente de nuevo!   ");
    myFile = SD.open(file_name_to_save, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- ¡Tiempo de escaneo agotado!");
    myFile.close(); 
    delay(2000);
    if(door_locked)
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print(" -puerta cerrada- ");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print("  -puerta abierta-  ");  
    }
   }
   
 }
 scanning = false;
 counter = 0;
///////////////////////////////FIN DE LA PARTE DE ESCANEO



////////////////////////////////Botón de agregar nuevo presionado///////////////////////////////////////////
if(digitalRead(add_id_pin) && !id_ad)
 {
  myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.println(" -- Intento de agregar nuevo usuario. ¡Requiere identificación!");
  myFile.close(); 

  add_new_id = true;


   lcd.setCursor(0,0);
  lcd.print(" Escanear usuario ");
  lcd.setCursor(0,1);
  lcd.print(" principal primero! ");  
  
  while (id_ad_counter < 40 && !main_user)
  {
    getFingerprintID();
    delay(100);  
    id_ad_counter = id_ad_counter + 1;
    if(!add_new_id)
    {
      id_ad_counter = 40;
    }
  }
  id_ad_counter = 0;
  add_new_id = false;

  
  if(main_user)
  {
    lcd.setCursor(0,0);
    lcd.print(" Añadir nuevo ID# ");
    lcd.setCursor(0,1);
    lcd.print(" a la base de datos ");  
    delay(1500);
    print_num(num);  
    id_ad = true;
    myFile = SD.open(file_name_to_save, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- Permiso para agregar nuevo usuario concedido");
    myFile.close(); 
  }
  else
  {
    lcd.setCursor(0,0);
    lcd.print("¡ERROR! Solo el ");
    lcd.setCursor(0,1);
    lcd.print("usuario principal puede agregar IDs");  
    delay(1500); 
    if(door_locked)
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print(" -puerta cerrada- ");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print("  -puerta abierta-  ");  
    }
    id_ad = false;
    myFile = SD.open(file_name_to_save, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- El usuario no tiene permiso para agregar un nuevo usuario");
    myFile.close(); 
  }
 }

if(digitalRead(scan_pin) && id_ad)
 {  
  
  id = num;
  while (!getFingerprintEnroll());
  id_ad = false;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Nuevo ID guardado  ");
  lcd.setCursor(4,1);
  lcd.print("como ID #"); 
  lcd.setCursor(11,1);
  lcd.print(id);  
  delay(3000);
  if(door_locked)
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print(" -puerta cerrada- ");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print("  -puerta abierta-  ");  
    }
  add_new_id = false;
  main_user = false;
  id_ad = false;
 }  


if(digitalRead(add_id_pin) && id_ad)
{
  num = num + 1;
  if(num > 16)
  {
    num = 1;
  }
  print_num(num);  
}

}//fin de void


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////


//Esta función imprimirá los números de ID al agregar un nuevo ID//
void print_num(uint8_t)
{
  if (num == 1)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(">1  2   3   4   ");  
    delay(500);
  }
  if (num == 2)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 1 >2   3   4   ");  
    delay(500);
  }
  if (num == 3)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 1  2  >3   4   ");  
    delay(500);
  }
  if (num == 4)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 1  2   3  >4   ");  
    delay(500);
  }
  if (num == 5)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(">5  6   7   8   ");  
    delay(500);
  }
  if (num == 6)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 5 >6   7   8   ");  
    delay(500);
  }
  if (num == 7)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 5  6  >7   8   ");   
    delay(500);
  }
  if (num == 8)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 5  6   7  >8   ");   
    delay(500);
  }
  if (num == 9)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(">9  10  11  12  ");  
    delay(500);
  }
  if (num == 10)
  {

 lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 9 >10  11  12  ");  
    delay(500);
  }
  if (num == 11)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 9  10 >11  12  ");  
    delay(500);
  }
  if (num == 12)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 9  10  11 >12  ");  
    delay(500);
  }
  if (num == 13)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(">13  14  15  16 ");  
    delay(500);
  }
  if (num == 14)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 13 >14  15  16 ");  
    delay(500);
  }
  if (num == 15)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 13  14 >15  16 ");  
    delay(500);
  }
  if (num == 16)
  {
    lcd.setCursor(0,0);
    lcd.print("Seleccionar ID");
    lcd.setCursor(0,1);
    lcd.print(" 13  14  15 >16 ");  
    delay(500);
  }
}






//Esta función leerá la huella digital colocada en el sensor//
uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
    break;
    case FINGERPRINT_NOFINGER: return p;
    case FINGERPRINT_PACKETRECIEVEERR: return p;
    case FINGERPRINT_IMAGEFAIL: return p;
    default: return p; 
  }
// ¡Éxito!

  p = finger.image2Tz();
  switch (p) 
  {
    case FINGERPRINT_OK: break;    
    case FINGERPRINT_IMAGEMESS: return p;    
    case FINGERPRINT_PACKETRECIEVEERR: return p;  
    case FINGERPRINT_FEATUREFAIL: return p;  
    case FINGERPRINT_INVALIDIMAGE: return p;    
    default: return p;
  }
// ¡Convertido correctamente!

p = finger.fingerFastSearch();

if (p == FINGERPRINT_OK)
{
  scanning = false;
  counter = 0;
  if(add_new_id)
  {
    if(finger.fingerID == main_user_ID)
    {
      main_user = true;
      id_ad = false;
    }
    else
    {
      add_new_id = false;
      main_user = false;
      id_ad = false;
    }
    
  }
  
  else
  {
  digitalWrite(rele, LOW);
  digitalWrite(red_led,LOW);       // LED rojo apagado
  digitalWrite(green_led,HIGH);    // LED verde encendido, indica puerta ABIERTA
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Coincidencia   ");
  
  lcd.setCursor(0,1);
  lcd.print(" ID: #");
  
  lcd.setCursor(6,1);
  lcd.print(finger.fingerID);

  lcd.setCursor(9,1);
  lcd.print("%: ");

  lcd.setCursor(12,1);
  lcd.print(finger.confidence);

  myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" -- Coincidencia de usuario con ID# "); myFile.print(finger.fingerID);
  myFile.print(" con confianza: ");   myFile.print(finger.confidence); myFile.println(" - puerta abierta");
  myFile.close(); 
  door_locked = false;
  delay(4000);
  lcd.setCursor(0,0);
  lcd.print("   Presione SCAN   ");
  lcd.setCursor(0,1);
  lcd.print("  -puerta abierta-  ");
  delay(50);
  }
}// fin finger OK

else if(p == FINGERPRINT_NOTFOUND)
{
  scanning = false;
  id_ad = false;
  counter = 0;
  lcd.setCursor(0,0);
  lcd.print("    No hay coincidencias    ");
  lcd.setCursor(0,1);
  lcd.print("   ¡Intente nuevamente!   ");
  add_new_id = false;
  main_user = false;  

  myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" -- No hay coincidencia para ningún ID. Estado de la puerta sigue igual"); 
  myFile.close(); 
  delay(2000);
  if(door_locked)
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print(" -puerta cerrada- ");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print("   Presione SCAN   ");
      lcd.setCursor(0,1);
      lcd.print("  -puerta abierta-  ");  
    }
  delay(2);
  return p;
}// fin finger error
}// devuelve -1 si falla, de lo contrario devuelve ID #


int getFingerprintIDez() {
uint8_t p = finger.getImage();
if (p != FINGERPRINT_OK) return -1;
p = finger.image2Tz();
if (p != FINGERPRINT_OK) return -1;
p = finger.fingerFastSearch();
if (p != FINGERPRINT_OK) return -1;
// ¡Encontrado un coincidencia!
return finger.fingerID;
}

// Esta función agregará un nuevo ID a la base de datos //
uint8_t getFingerprintEnroll() {
int p = -1;
if (!first_read) {
  lcd.setCursor(0,0);
  lcd.print("Agregar como ID# ");
  lcd.setCursor(14,0);
  lcd.print(id);
  lcd.setCursor(0,1);
  lcd.print(" Coloque dedo  ");
}

while (p != FINGERPRINT_OK) {
  p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" ¡Imagen tomada! ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(100);
      first_read = true;
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0,0);
      lcd.print("Agregar como ID# ");
      lcd.setCursor(14,0);
      lcd.print(id);
      lcd.setCursor(0,1);
      lcd.print(" Coloque dedo  ");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("  Comunicación  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("     -Imagen     ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("    -Desconocido ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
  }
}

// ¡Éxito!

p = finger.image2Tz(1);
switch (p) {
  case FINGERPRINT_OK:
    lcd.setCursor(0,0);
    lcd.print("¡Imagen convertida!");
    lcd.setCursor(0,1);
    lcd.print("                ");
    break;
  case FINGERPRINT_IMAGEMESS:
    lcd.setCursor(0,0);
    lcd.print("¡Imagen demasiado desordenada!");
    lcd.setCursor(0,1);
    lcd.print("                ");
    delay(1000);
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.setCursor(0,0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
  case FINGERPRINT_FEATUREFAIL:
    lcd.setCursor(0,0);
    lcd.print(" No se encontraron ");
    lcd.setCursor(0,1);
    lcd.print(" características de huella digital ");
    delay(1000);
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    lcd.setCursor(0,0);
    lcd.print(" No se encontraron ");
    lcd.setCursor(0,1);
    lcd.print(" características de huella digital ");
    delay(1000);
    return p;
  default:
    lcd.setCursor(0,0);
    lcd.print("    -Desconocido    ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
}

lcd.setCursor(0,0);
lcd.print(" ¡Retire dedo! ");
lcd.setCursor(0,1);
lcd.print("                ");
delay(2000);
p = 0;
while (p != FINGERPRINT_NOFINGER) {
  p = finger.getImage();
}
lcd.setCursor(0,0);
lcd.print(" ¡Retire dedo! ");
lcd.setCursor(0,1);
lcd.print("                ");
delay(2000);
p = 0;
while (p != FINGERPRINT_NOFINGER) {
  p = finger.getImage();
}

lcd.setCursor(0,1);
lcd.print("ID# ");
lcd.setCursor(4,1);
lcd.print(id);

p = -1;
lcd.setCursor(0,0);
lcd.print("Coloque de nuevo");
lcd.setCursor(0,1);
lcd.print("el mismo dedo   ");
while (p != FINGERPRINT_OK) {
  p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" ¡Imagen tomada! ");
      lcd.setCursor(0,1);
      lcd.print("                "); 
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0,0);
      lcd.print("Coloque de nuevo");
      lcd.setCursor(0,1);
      lcd.print("el mismo dedo   ");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("  Comunicación  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("     -Imagen     ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("    -Desconocido ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
  }
}

// ¡Éxito!

p = finger.image2Tz(2);
switch (p) {
  case FINGERPRINT_OK:
    lcd.setCursor(0,0);
    lcd.print("¡Imagen convertida!");
    lcd.setCursor(0,1);
    lcd.print("                ");
    break;
  case FINGERPRINT_IMAGEMESS:
    lcd.setCursor(0,0);
    lcd.print("¡Imagen demasiado desordenada!");
    lcd.setCursor(0,1);
    lcd.print("                ");
    delay(1000);
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.setCursor(0,0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
  case FINGERPRINT_FEATUREFAIL:
    lcd.setCursor(0,0);
    lcd.print(" No se encontraron ");
    lcd.setCursor(0,1);
    lcd.print(" características de huella digital ");
    delay(1000);
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    lcd.setCursor(0,0);
    lcd.print(" No se encontraron ");
    lcd.setCursor(0,1);
    lcd.print(" características de huella digital ");
    delay(1000);
    return p;
  default:
    lcd.setCursor(0,0);
    lcd.print("    -Desconocido    ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
}
  //quesillo//
  
  // ¡OK convertido!
  lcd.setCursor(0, 0);
  lcd.print(" Creando modelo ");
  lcd.setCursor(0, 1);
  lcd.print("para ID# ");
  lcd.setCursor(8, 1);
  lcd.print(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print(" ¡Huella coincidió! ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.setCursor(0, 0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0, 1);
    lcd.print("    ERROR!      ");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    lcd.setCursor(0, 0);
    lcd.print("La huella no ");
    lcd.setCursor(0, 1);
    lcd.print("coincide        ");
    delay(1000);
    return p;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("    -Desconocido ");
    lcd.setCursor(0, 1);
    lcd.print("    ERROR!      ");
    delay(1000);
    return p;
  }   
  
  lcd.setCursor(0, 1);
  lcd.print("ID# ");
  lcd.setCursor(4, 1);
  lcd.print(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print("    Almacenado  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    myFile = SD.open(file_name_to_save, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.print(" -- Nueva huella almacenada para ID# "); myFile.println(id);
    myFile.close(); 
    delay(1000);
    first_read = false;
    id_ad = false;
    
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.setCursor(0, 0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0, 1);
    lcd.print("    ERROR!      ");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    lcd.setCursor(0, 0);
    lcd.print("No se pudo almacenar ");
    lcd.setCursor(0, 1);
    lcd.print("en esa ubicación");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    lcd.setCursor(0, 0);
    lcd.print("Error escribiendo");
    lcd.setCursor(0, 1);
    lcd.print("en la memoria  ");
    delay(1000);
    return p;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("    -Desconocido ");
    lcd.setCursor(0, 1);
    lcd.print("    ERROR!      ");    
    delay(1000);
    return p;
  }   
}
