# SETUP

Liste des commandes utiles :  

mosquitto_passwd -c passwd <username>  
    Permet la création d'un fichier de password pour mosquitto

Cela permet de modifier le mot de passe utilisé. Attention, les codes TimerCam et python doivent être modifiés en conséquence.

mosquitto -v -c "C:\Program Files\mosquitto\mosquitto.conf"
    Permet de démarrer mosquitto avec le bon fichier de configuration