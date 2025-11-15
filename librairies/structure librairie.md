# Fonctionnement de la librairie M5TimerCam

# TimerCam

Lors de l'include de la librairie, il y a création automatique d'un objet TimerCam contenant:

## Objets
- Power_Class [Power](#power)
- RTC8563_Class [Rtc](#rtc)
- Camera_Class [Camera](#camera)

## Fonctions
- **begin(bool enableRTC = false)**

    &ensp; Power.begin()  
    &ensp; Serial.begin(115200)  
    &ensp; if enableRTC : Rtc.begin()  

<br>
<br>

<hr style="height: 8px; border: none; background: #ffffffff;">

<br>
<br>

# Power

L'objet Power contient:

## Fonctions

- **begin()**

    &ensp; Active le maintient de la tension (GPIO 33)
        configure ledc  (qu'est ce que ledc ???????)

- **powerOff()**

    &ensp; Coupe la tension vers l'esp32

- **getBatteryVoltage()**

    &ensp; Lis la tension de la batterie à l'aide d'un pont diviseur et d'un ADC

- **getBatteryLevel()**

    &ensp; Lis la tension et la convertit en pourcentage entre 3,3 V et 4,15 V

- **setLed(uint8_t brightnesss)**

    &ensp; Met à jour le duty cycle de la led (quelle LED?????)

- **timerSleep(int seconds)**
- **timerSleep(const rtc_time_t& time)**
- **timerSleep(const rtc_date_t& date, const rtc_time_t& time)**

    &ensp; Configure par I2C la RTC pour l'IRQ de reveil  
    &ensp; Coupe l'alimentation de l'esp32  
    &ensp; Selon :  
    &ensp; &ensp; &ensp; ● sleep en secondes  
    &ensp; &ensp; &ensp; ● à une heure précise chaque jour  
    &ensp; &ensp; &ensp; ● Une date et heure précise  

<br>
<br>

<hr style="height: 8px; border: none; background: #ffffffff;">

<br>
<br>

# Rtc

L'objet Rtc contient:

## Fonctions

- **begin()**

    &ensp; Configure l'I2C

- **setTime(), setDate(), set_tm()**

    &ensp; Configuration de l'heure du RTC

- **getTime(), getDate(), getDateTime()**

    &ensp; Obtention de l'heure du RTC

- **bcd2ToByte(), byteToBcd2()**

    &ensp; La RTC utilise le bcd

- **getVoltLow()**

    &ensp; Retourne True si la tension de la pile du RTC est faible, False sinon





<br>
<br>

<hr style="height: 8px; border: none; background: #ffffffff;">

<br>
<br>

# Camera