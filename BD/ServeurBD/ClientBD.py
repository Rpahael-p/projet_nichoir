import os
import datetime
import paho.mqtt.client as mqtt
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from models import Base, Camera, Image, Batterie, Etat   

# =====================
# CONFIGURATION
# =====================
BROKER_IP = "10.42.0.1"
BROKER_PORT = 1883
TOPIC_BASE = "TimerCam/#"
PHOTO_SAVE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),"photo")

os.makedirs(PHOTO_SAVE_DIR, exist_ok=True)

# =====================
# BASE DE DONNÉES
# =====================
engine = create_engine(
    "mariadb+mariadbconnector://nous:123456789@10.42.0.1:3306/BDNichoir",
    echo=False
)
Session = sessionmaker(bind=engine)


# -------------------------------------------------
# Fonction : récupérer ou créer une caméra
# -------------------------------------------------
def get_or_create_camera(session, demi_mac):
    cam = session.query(Camera).filter(Camera.DemiMac == demi_mac).first()
    if cam is None:
        cam = Camera(DemiMac=demi_mac, NomPersonalise=None)
        session.add(cam)
        session.commit()
        print(f"[DB] Nouvelle caméra créée : {demi_mac}")
    return cam


# =====================
# MQTT : Connexion
# =====================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker !")
        client.subscribe(TOPIC_BASE)
        print(f"Souscrit à {TOPIC_BASE}")
    else:
        print("Erreur de connexion :", rc)


# =====================
# MQTT : Réception message
# =====================
def on_message(client, userdata, msg):
    """
    Format sujet :
        TimedCam/<DemiMac>/<Type>

    Types possibles :
        photo
        batterie
        etat
    """
    try:
        parts = msg.topic.split("/")
        _, demi_mac, data_type = parts
    except Exception:
        print("Topic invalide :", msg.topic)
        return

    timestamp = datetime.datetime.now()

    session = Session()

    # Assure que la caméra existe
    camera = get_or_create_camera(session, demi_mac)

    # -------------------------------------------------
    # CAS 1 : PHOTO
    # -------------------------------------------------
    if data_type == "photo":
        file_path = os.path.join(PHOTO_SAVE_DIR, f"{demi_mac}_{timestamp.strftime('%y-%m-%d_%H-%M-%S')}.jpg")
        with open(file_path, "wb") as f:
            f.write(msg.payload)

        entry = Image(
            Data=file_path,
            Time=timestamp,
            CameraDemiMac=demi_mac
        )
        session.add(entry)
        session.commit()
        print(f"[PHOTO] Sauvegardée : {file_path}")

    # -------------------------------------------------
    # CAS 2 : BATTERIE
    # -------------------------------------------------
    elif data_type == "batterie":
        try:
            value = int(msg.payload.decode())
        except:
            print("Valeur batterie invalide :", msg.payload)
            return

        entry = Batterie(
            Data=value,
            Time=timestamp,
            CameraDemiMac=demi_mac
        )
        session.add(entry)
        session.commit()
        print(f"[BATTERIE] {demi_mac} = {value}%")

    # -------------------------------------------------
    # CAS 3 : ETAT (ON/OFF)
    # -------------------------------------------------
    elif data_type == "etat":
        try:
            value = bool(int(msg.payload.decode()))
        except:
            print("Valeur état invalide :", msg.payload)
            return

        entry = Etat(
            Data=value,
            Time=timestamp,
            CameraDemiMac=demi_mac
        )
        session.add(entry)
        session.commit()
        print(f"[ETAT] {demi_mac} = {value}")

    else:
        print("Type inconnu :", data_type)
        return

    # -------------------------------------------------
    # Publication update
    # -------------------------------------------------
    client.publish("update", "1")
    print("[MQTT] Sujet update envoyé !")

    session.close()


# =====================
# Lancement MQTT
# =====================
client = mqtt.Client()
client.username_pw_set("esp32","nichoir")
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER_IP, BROKER_PORT, 60)
client.loop_forever()
