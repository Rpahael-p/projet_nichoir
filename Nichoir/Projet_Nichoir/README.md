# TimerCAM ESP32 – Nichoir Connecté

## Fonctionnalités principales

- Détection de mouvements avec le capteur PIR
- Capture d’images via la caméra M5TimerCAM
- Transmission des photos et des informations de batterie vers un broker MQTT
- Support des images en **chunks MQTT** pour les gros fichiers
- Interface web pour configurer le Wi-Fi et le broker MQTT
- Mode Access Point pour configuration initiale
- Stockage des paramètres dans la **mémoire non volatile (NVS)**
- Paramétrage avancé de la caméra : résolution, qualité, luminosité, contraste, saturation, sharpness, flip/hmirror

---

## Bibliothèques utilisées

- `M5TimerCAM` – gestion de la caméra et fonctions TimerCAM  
- `WiFi.h` – gestion du Wi-Fi  
- `PubSubClient.h` – client MQTT pour ESP32  
- `WebServer.h` – serveur HTTP embarqué pour configuration  
- `Preferences.h` – stockage non volatile des paramètres  
- `Ticker.h` – gestion du clignotement LED et autres timers  

---

## Paramètres configurables via le web

- **Wi-Fi** : SSID et mot de passe  
- **Broker MQTT** : adresse IP  
- **Caméra** :
  - Résolution
  - Nombre de tentatives de photo
  - Qualité JPEG
  - Luminosité, contraste, saturation, sharpness
  - Flip vertical / miroir horizontal  

---

## MQTT Topics

- `TimerCam/<ID>/photo` – photo capturée (en chunks si nécessaire)  
- `TimerCam/<ID>/photo/end` – indication de fin d’envoi de la photo (1=succès, 0=échec)  
- `TimerCam/<ID>/batterie` – tension de batterie  

> `<ID>` correspond à un identifiant unique généré à partir de l’ESP32 MAC address.

---

## Fonctionnement

1. **Démarrage et mode sélectionné**  
   Au démarrage, le système lit l’état du bouton. Si le bouton est pressé, il active le **mode Access Point (AP)** pour permettre à l’utilisateur de configurer le Wi-Fi et le broker MQTT via une interface web. Sinon, il tente de se connecter au Wi-Fi avec les informations stockées. En mode AP, une LED bleue clignote.

2. **Initialisation de la caméra**  
   La caméra M5TimerCAM est initialisée avec les paramètres stockés en mémoire non volatile. Si la caméra ne peut pas être initialisée, le dispositif passe en mode veille pour économiser la batterie.

3. **Détection de mouvement**  
   Le capteur PIR surveille l’activité dans le nichoir. Si un mouvement est détecté, il déclenche la capture de photo et la transmission des données.

4. **Capture et envoi de données**  
   Une fois connecté au Wi-Fi et au broker MQTT, le dispositif capture les photos et lit la tension de batterie. Les images sont découpées en **chunks** pour le transfert via MQTT. Après transmission réussie de la photo et de la batterie, l’ESP32 se déconnecte du Wi-Fi et se met en **mode veille**.

5. **Mode veille et économie d’énergie**  
   Pendant le sommeil, la caméra et le Wi-Fi sont désactivés, et le TimerCAM utilise son RTC pour programmer le prochain réveil. Le dispositif peut se réveiller soit pour envoyer les informations de batterie périodiquement, soit lorsqu’un mouvement est détecté.

6. **Interface web pour la configuration**  
   En mode AP, l’utilisateur peut accéder à l’interface web pour :
   - Choisir le réseau Wi-Fi et entrer le mot de passe
   - Définir l’adresse du broker MQTT
   - Configurer les paramètres avancés de la caméra (résolution, qualité, orientation, etc.)  
   
   Une fois les informations soumises, le TimerCAM tente de se connecter au réseau Wi-Fi et au broker, puis retourne en fonctionnement normal.
