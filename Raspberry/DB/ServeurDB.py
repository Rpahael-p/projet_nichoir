import os
import datetime
import paho.mqtt.client as mqtt
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from models import Base, Camera, Image, Batterie   

# =====================
# CONFIGURATION
# =====================
#BROKER_IP = "10.42.0.1"
BROKER_IP = "192.168.1.60"
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
last_image_id = 0

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
    global image_chunks, last_image_id
    
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
        
        if part == "end":
            if not image_chunks:
                print("END reçu mais aucun chunk")
                return
                
            expected_indices = list(range(max(image_chunks.keys()) + 1))
            received_indices = sorted(image_chunks.keys())
            if expected_indices != received_indices:
                print("Certains chunks sont manquants :", expected_indices, "!= ", received_indices)
                image_chunks.clear()
                return

            filename = os.path.join(PHOTO_SAVE_DIR, f"{demi_mac}_{timestamp.strftime('%y-%m-%d_%H-%M-%S')}.jpg")

            try:
                with open(filename, "wb") as f:
                    for i in sorted(image_chunks.keys()):
                        f.write(image_chunks[i])

                total_size = sum(len(c) for c in image_chunks.values())
                print(f"Image reconstruite : {filename} ({total_size} bytes)")

                image_chunks.clear()
                
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
            finally:
                image_chunks.clear()
                
            return
            
        try:
            idx = int(part)
            if idx == 0:
                image_chunks.clear()
            image_chunks[idx] = msg.payload
            print(f"Chunk {idx} reçu ({len(msg.payload)} bytes)")
        except ValueError:
            print(f"Topic inconnu : {topic}")
        
        

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
