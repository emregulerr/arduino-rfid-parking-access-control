/*****************************

Arduino RFID Otopark Erişim Kontrolü
  
******************************/


/* Gerekli kütüphaneleri ekle */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <RFID.h>
#include <Servo.h>
#include <EEPROM.h>


Servo engel;                                // servo nesnesi oluştur
LiquidCrystal_I2C lcd(0x27,16,2);           // lcd nesnesi oluştur
RFID nfc(10,5);                             // kart okuyucu için RFID nesnesi oluştur
int onay=0;                                 // kart onay değişkeni tanımla
boolean gecisOk=0;                          // geçiş durum değişkeni
long sure;                                  // Echo bacağının kac mikro saniyede aktif olduğunu saklayacak olan değişken 
long uzaklik=0; 
int aradakiMesafe;
int trigger = 8;                            // Sensörün Trigger bacağının bağlı olduğu pin
int echo = 7;                               // Sensörün Echo bacağının bağlı olduğu pin
int LDRdeger=0;
int kartsay=0;
int adres=0;


void setup() {
  SPI.begin();                              // SPI iletişimini başlat
  nfc.init();                               // kart okuyucuyu hazırla
  lcd.init();                               // LCD yi hazırla
  lcd.backlight();                          // LCD arka ışığını aç
  lcd.print("KARTINIZI OKUTUN");
  engel.attach(9);                          // Servonun bağlandığı pini belirt
  engel.write(180);                          // Servoyu  pozisyonuna al (Geçişi engelle)
  pinMode(trigger, OUTPUT);                 // Sensörün Trigger bacağına gerilim uygulayabilmemiz için OUTPUT yapıyoruz.
  pinMode(echo, INPUT);                     // Sensörün Echo bacağındaki gerilimi okuyabilmemiz için INPUT yapıyoruz.
  digitalWrite(trigger, LOW);               // Sensör pasif hale getirildi
  Serial.begin(9600);
  kartsay=EEPROM.read(0);
  adres=(kartsay*5)+1;
}

void loop() {
    LDRdeger=analogRead(A0);
    if(LDRdeger>=400){
      lcd.clear();
      lcd.print("KARTINIZI OKUTUN");
      if(nfc.isCard()){                       // Kart okuyucu bir NFC kartı/cihazı algılıyor mu
      if(nfc.readCardSerial()){               // Kartın UID değeri okunabildi mi

       if (nfc.serNum[0] == 85 
       && nfc.serNum[1] == 89
       && nfc.serNum[2] == 109 
       && nfc.serNum[3] == 139
       && nfc.serNum[4] == 234){
        lcd.clear();
        lcd.print("KART KAYIT MODU");
        delay(1000);
        lcd.clear();
        lcd.print("KARTI OKUTUN");
        while(true){
          if(nfc.isCard()){                       // Kart okuyucu bir NFC kartı/cihazı algılıyor mu
            if(nfc.readCardSerial()){
              if (nfc.serNum[0] == 85 
               && nfc.serNum[1] == 89
               && nfc.serNum[2] == 109 
               && nfc.serNum[3] == 139
               && nfc.serNum[4] == 234){        // YÖNETİM KARTI TEKRAR OKUTULURSA
                    for (int i = 0 ; i < EEPROM.length() ; i++) { // EEPROM SIFIRLAMA
                      EEPROM.update(i, 0);
                      kartsay=0;
                    }
                    lcd.clear();
                    lcd.print("SIFIRLANDI!");
                    delay(1500);
               }else{
                  kartsay=EEPROM.read(0);
                  adres=(kartsay*5)+1;
                  EEPROM.write(adres, nfc.serNum[0]);      // EEPROM a UID yi byte byte kaydet
                  EEPROM.write(adres+1, nfc.serNum[1]);
                  EEPROM.write(adres+2, nfc.serNum[2]);
                  EEPROM.write(adres+3, nfc.serNum[3]);
                  EEPROM.write(adres+4, nfc.serNum[4]);
                  lcd.clear();
                  lcd.print("Kart kaydedildi.");
                  kartsay=EEPROM.read(0)+1;
                  EEPROM.write(0,kartsay);
               }
            }
          break;  
          }
        }        
       }else{
        for(int i=0; i<kartsay;i++){
          adres=(i*5)+1;
          if (nfc.serNum[0] == EEPROM.read(adres) 
           && nfc.serNum[1] == EEPROM.read(adres+1)
           && nfc.serNum[2] == EEPROM.read(adres+2) 
           && nfc.serNum[3] == EEPROM.read(adres+3)
           && nfc.serNum[4] == EEPROM.read(adres+4)){// UID tanımlı olan ile aynı mı 
            onay=1; 
          }  
        }
        if(onay==1){                          // UID onayı almış mı
        engel.write(90);                   // Servoyu pozisyonuna al(Geçişi serbest bırak)
        lcd.clear();                        // LCD yi temizle
        lcd.print("HOSGELDINIZ!");
        do
        {
          delay(100);
          digitalWrite(trigger, HIGH);
          delayMicroseconds(10);
          digitalWrite(trigger, LOW);
         
          // Dalga üretildikten sonra geri yansıyıp Echo bacağının HIGH duruma geçireceği süreyi pulseIn fonksiyonu ile 
          // kaydediyoruz. 
          sure = pulseIn(echo, HIGH);
         
          uzaklik= sure /29.1/2; /* ölçülen sure uzaklığa çevriliyor */            
          if(uzaklik > 200) uzaklik = 200;
        } while (uzaklik>10);
        do {
          delay(100);
          digitalWrite(trigger, HIGH);
          delayMicroseconds(10);
          digitalWrite(trigger, LOW);
         
          // Dalga üretildikten sonra geri yansıyıp Echo bacağının HIGH duruma geçireceği süreyi pulseIn fonksiyonu ile 
          // kaydediyoruz. 
          sure = pulseIn(echo, HIGH);
         
          uzaklik= sure /29.1/2; /* ölçülen sure uzaklığa çevriliyor */            
          if(uzaklik > 200) uzaklik = 200;
        }while(uzaklik<=10);
        gecisOk=1;
        if(gecisOk){
          delay(2000);
          engel.write(180);                      
          lcd.clear();                          
          lcd.print("GECIS YAPILDI!");
          onay=0;
          gecisOk=0;
        }
      }
      else{  
        lcd.clear();
        lcd.print("TANIMSIZ KART!");
        delay(2000);
        lcd.clear();
        lcd.print("KARTINIZI OKUTUN");
      }
       }
      
    }
    nfc.halt();                            // RFID iletişimini sonlandır  
    }
    }else{
        lcd.clear();
        lcd.print("KAPALIYIZ!");
    }
    delay(1000);
}
