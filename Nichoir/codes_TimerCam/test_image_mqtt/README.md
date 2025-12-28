# Explication

La TimerCam se connecte au wifi et au broker MQTT et toutes les 5 secondes envoie un message et une photo.  
Le broker (raspberry ou pc windows) se connecte et s'abonne au topic. Chaque message envoyer au topic déclenche une fonction créant le fichier jpeg dans le dossier spécifié

# Setup

Pour utiliser le code fournit, le fichier config doit être configurer.  
Le code est compatible raspbian et windows.  
Le chemin d'accès vers le dossier doit être sous le format de l'OS utilisé.  
