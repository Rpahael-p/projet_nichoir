import os
import datetime
import paho.mqtt.client as mqtt
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from models import Base, Camera, Image, Batterie   

# =====================
# CONFIGURATION
# =====================
BROKER_IP = "127.0.0.1"
BROKER_PORT = 1883
TOPIC_BASE = "TimerCam/#"
PHOTO_SAVE_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "photo")

os.makedirs(PHOTO_SAVE_DIR, exist_ok=True)

# =========================
# IMAGE BUFFER
# =========================
image_chunks = {}

# =====================
# BASE DE DONNÉES
# =====================
engine = create_engine(
    "mariadb+mariadbconnector://esp32:nichoir@127.0.0.1:3306/DBNichoir",
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
    """
    print(repr(msg.topic))
    global image_chunks
    
    try:
        parts = msg.topic.split("/")
        _, demi_mac, data_type, *rest = parts
    except Exception:
        print("Topic invalide :", msg.topic)
        return

    session = Session()
    # Assure que la caméra existe
    camera = get_or_create_camera(session, demi_mac)
    timestamp = datetime.datetime.now()

    # -------------------------------------------------
    # CAS 1 : PHOTO
    # -------------------------------------------------
    if data_type == "photo":
        part = rest[0]

        # Créer le buffer pour cette caméra si nécessaire
        if demi_mac not in image_chunks:
            image_chunks[demi_mac] = {}

        cam_chunks = image_chunks[demi_mac]

        if part == "end":
            if not cam_chunks:
                print(f"END reçu mais aucun chunk pour {demi_mac}")
                return

            expected_indices = list(range(max(cam_chunks.keys()) + 1))
            received_indices = sorted(cam_chunks.keys())
            if expected_indices != received_indices:
                print(f"Certains chunks sont manquants pour {demi_mac}: {expected_indices} != {received_indices}")
                del image_chunks[demi_mac]
                return

            filename = os.path.join(PHOTO_SAVE_DIR, f"{demi_mac}_{timestamp.strftime('%y-%m-%d_%H-%M-%S')}.jpg")

            try:
                with open(filename, "wb") as f:
                    for i in sorted(cam_chunks.keys()):
                        f.write(cam_chunks[i])

                total_size = sum(len(c) for c in cam_chunks.values())
                print(f"Image reconstruite pour {demi_mac}: {filename} ({total_size} bytes)")

                # Supprimer uniquement les chunks de cette caméra
                del image_chunks[demi_mac]

                entry = Image(
                    Data=filename,
                    Time=timestamp,
                    CameraDemiMac=demi_mac
                )
                session.add(entry)
                session.commit()
                print(f"[PHOTO] Sauvegardée : {filename}")
                return
            except Exception as e:
                print(f"Erreur lors de la sauvegarde de l'image : {e}")
                del image_chunks[demi_mac]
            return

        # Réception d'un chunk normal
        try:
            idx = int(part)
            if idx == 0:
                cam_chunks.clear()
            cam_chunks[idx] = msg.payload
            print(f"Chunk {idx} reçu pour {demi_mac} ({len(msg.payload)} bytes)")
        except ValueError:
            print(f"Topic inconnu : {msg.topic}")
            
        

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
        print(f"[BATTERIE] {demi_mac} = {value} mV")

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
