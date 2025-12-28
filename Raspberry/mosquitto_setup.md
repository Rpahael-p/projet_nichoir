# SETUP du Broker MQTT mosquitto

#### 1)&nbsp;Création d'un fichier de mot de passe :  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;mosquitto_passwd -c passwd &lt;username&gt;  

Cela permet de modifier le mot de passe utilisé. Attention, les codes TimerCam et python doivent être modifiés en conséquence.

#### 2)&nbsp;Modifié le fichier de configuration
- Windows : C:\Program Files\mosquitto\mosquitto.conf
- Linux : /etc/mosquitto/mosquitto.conf

Les trois lignes suivantes doivent être ajoutées:  
&nbsp;&nbsp;listener 1883 0.0.0.0  
&nbsp;&nbsp;allow_anonymous false  
&nbsp;&nbsp;password_file /etc/mosquitto/passwd  

Le fichier complet est trouvable ici : [mosquitto.conf](mosquitto.conf)

---

### Autre commandes

Démarrage manuel sur windows du broker avec le bon fichier de configuration :   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;mosquitto -v -c "C:\Program Files\mosquitto\mosquitto.conf"