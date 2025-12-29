# MQTT listener et interface Flask

## Préparation de la DB

#### 1)&nbsp;Création de la DB DBNichoir : create database DBNichoir;
#### 2)&nbsp;Création d'un utilisateur : create user 'esp32'@'%' identified by 'nichoir';
#### 3)&nbsp;Donner les privilèges à l'utilisateur sur la base de donnée : grant all privileges on DBNichoir.* to 'esp32'@'%';

## Codes python

#### 1)&nbsp;Création d'un environnement virtuel, dans le dossier du projet : python3 -m venv venv
#### 2)&nbsp;installation des librairies (pip install) : mariadb, sqlalchemy, paho-mqtt, flask

### Lancement manuel

#### 1)&nbsp;Déplacement vers le dossier : cd /home/omarbennanna/Desktop/Projet_Nichoir
#### 2)&nbsp;Activation de l'environnement virtuel : source venv/bin/activate
#### 3)&nbsp;Lancement des trois codes : DB/models.py DB/ServeurDB.py Flask/app.py

### Lancement automatique

La démarrage automatique se fais en créant des services.  
Le fichier models.py ne doit être lancer qu'une seule fois, dès lors la création d'un service n'est pas nécessaire.

#### 1)&nbsp;Déplacement dans le dossier des services : cd /etc/systemd/system  
#### 2)&nbsp;Création des services :  

Les chemins doivent être adaptés vers le dossier choisi  

sudo nano Nichoir_ServeurDB.service  

    [Unit]
    Description=ServeurDB MQTT Listener Nichoir
    After=network.target

    [Service]
    Type=simple
    User=omarbennanna
    WorkingDirectory=/home/omarbennanna/Desktop/Projet_Nichoir
    Environment="PATH=/home/omarbennanna/Desktop/Projet_Nichoir/venv/bin"
    ExecStart=/home/omarbennanna/Desktop/Projet_Nichoir/venv/bin/python3 -u /home/omarbennanna/Desktop/Projet_Nichoir/DB/ServeurDB.py
    Restart=always
    RestartSec=5

    [Install]
    WantedBy=multi-user.target

sudo nano Nichoir_Flask.service  

    [Unit]
    Description=Flask App Nichoir
    After=network.target

    [Service]
    Type=simple
    User=omarbennanna
    WorkingDirectory=/home/omarbennanna/Desktop/Projet_Nichoir
    Environment="PATH=/home/omarbennanna/Desktop/Projet_Nichoir/venv/bin"
    ExecStart=/home/omarbennanna/Desktop/Projet_Nichoir/venv/bin/python3 /home/omarbennanna/Desktop/Projet_Nichoir/Flask/app.py
    Restart=always
    RestartSec=5

    [Install]
    WantedBy=multi-user.target

#### 3)&nbsp;Actualisation des services :  
sudo systemctl daemon-reexec  
sudo systemctl daemon-reload  

#### 4)&nbsp;Activation du démarrage automatique :  
sudo systemctl enable Nichoir_ServeurDB  
sudo systemctl enable Nichoir_Flask  

#### 5)&nbsp;Démarrage des services :  
sudo systemctl start Nichoir_ServeurDB  
sudo systemctl start Nichoir_Flask  

---

#### 6)&nbsp;Vérification de l'état des services :  
systemctl status Nichoir_ServeurDB  
systemctl status Nichoir_Flask  

#### 7)&nbsp;Affichage des logs des services :  
journalctl -u Nichoir_ServeurDB -f  
journalctl -u Nichoir_Flask -f  

#### 8)&nbsp;Redémarrage d'un service :  
sudo systemctl daemon-reload  
sudo systemctl restart Nichoir_Flask  
