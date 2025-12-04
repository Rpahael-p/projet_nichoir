from flask import Flask, render_template, request, redirect, url_for ,send_file
from datetime import datetime, timedelta
from urllib.parse import unquote
import mariadb
import os

# Chemin ABSOLU vers les images stockées sur le Raspberry
IMAGE_FOLDER = "/home/pi/images/"

app = Flask(__name__, template_folder="./templates")

# -----------------------------
# Connexion MariaDB
# -----------------------------
def get_db():
    return mariadb.connect(
        user="nous",
        password="123456789",
        host="localhost",
        port=3306,
        database="BDNichoir"
    )

# -----------------------------
# Page principale : liste Image
# -----------------------------
@app.route("/")
def index():
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
    SELECT Image.id, Image.Data, Image.Time, Camera.NomPersonalise
    FROM Image
    LEFT JOIN Camera ON Camera.DemiMac = Image.CameraDemiMac
    ORDER BY Image.Time DESC
""")
    Images = [
    {
        "id": row[0],
        "Data": row[1],
        "Time": row[2],
        "CameraName": row[3] or "Caméra inconnue"
    }
    for row in cur.fetchall()
]

    cur.close()
    conn.close()

    return render_template("index.html", Images=Images)
    
# -----------------------------
# Suppression d'une image
# -----------------------------
@app.route("/image/<int:photo_id>")
def show_image(photo_id):
    conn = get_db()
    cur = conn.cursor()
    
    cur.execute("SELECT Data FROM Image WHERE id = ?", (photo_id,))
    row = cur.fetchone()
    
    cur.close()
    conn.close()
    
    if not row:
        return "Image inconnue", 404
        
    image_path = row[0]
    
    if not os.path.exists(image_path):
        return "fichier introuvable", 404
        
    return send_file(image_path, mimetype="image/jpeg")


# -----------------------------
# Suppression d'une image
# -----------------------------
@app.route("/delete/<int:Image_id>", methods=["POST"])
def delete_Image(Image_id):
    conn = get_db()
    cur = conn.cursor()

    # Récupérer le chemin dans la DB
    cur.execute("SELECT Data FROM Image WHERE id = ?", (Image_id,))
    row = cur.fetchone()

    if row:
        filepath = os.path.join(IMAGE_FOLDER, row[0])

        # Supprimer fichier si existe
        if os.path.exists(filepath):
            os.remove(filepath)

        # Supprimer l'entrée de la DB
        cur.execute("DELETE FROM Image WHERE id = ?", (Image_id,))
        conn.commit()

    cur.close()
    conn.close()

    return redirect(url_for("index"))

# -----------------------------
# Page Batterie : graphique
# -----------------------------
import json

@app.route("/batterie")
def batterie():
    conn = get_db()
    cur = conn.cursor()

    cur.execute("SELECT time, data FROM Batterie ORDER BY time ASC")
    rows = cur.fetchall()

    times = [str(r[0]) for r in rows]
    values = [r[1] for r in rows]

    cur.close()
    conn.close()

    # Convertir en vrais tableaux JSON pour JS
    return render_template(
        "batterie.html",
        times_json=json.dumps(times),
        values_json=json.dumps(values)
    )

# -----------------------------
# Page Caméras
# -----------------------------
@app.route("/cameras", methods=["GET", "POST"])
def cameras():
    conn = get_db()
    cur = conn.cursor()

    # ---- Renommage ----
    if request.method == "POST":
        mac = request.form.get("mac")
        new_name = request.form.get("new_name")
        cur.execute("UPDATE Camera SET NomPersonalise = ? WHERE DemiMac = ?", (new_name, mac))
        conn.commit()

    # ---- Liste des caméras ----
    cur.execute("SELECT DemiMac, NomPersonalise FROM Camera")
    cameras_raw = cur.fetchall()

    cameras = []

    for mac, name in cameras_raw:

        # -------- Compter les photos --------
        cur.execute("SELECT COUNT(*) FROM Image WHERE CameraDemiMac = ?", (mac,))
        img_count = cur.fetchone()[0]

        # -------- Derniére image --------
        cur.execute("""
            SELECT Time
            FROM Image
            WHERE CameraDemiMac = ?
            ORDER BY Time DESC
            LIMIT 1
        """, (mac,))
        last_image = cur.fetchone()
        last_image_time = last_image[0] if last_image else None

        # -------- Dernier message batterie --------
        cur.execute("""
            SELECT Time
            FROM Batterie
            WHERE CameraDemiMac = ?
            ORDER BY Time DESC
            LIMIT 1
        """, (mac,))
        last_batt = cur.fetchone()
        last_batt_time = last_batt[0] if last_batt else None

        # -------- Dernier message global --------
        times = [t for t in (last_image_time, last_batt_time) if t is not None]

        if times:
            last_msg = max(times)        
        else:
            last_msg = None

        # -------- Déterminer l'état --------
        if last_msg:

            if isinstance(last_msg, str):
                last_msg = datetime.fromisoformat(last_msg)

            is_ok = (datetime.now() - last_msg) <= timedelta(hours=24)
        else:
            is_ok = False

        cameras.append({
            "mac": mac,
            "name": name,
            "count": img_count,
            "etat": is_ok,           # True = vert, False = rouge
            "etat_time": last_msg    # date du dernier message
        })

    cur.close()
    conn.close()

    return render_template("cameras.html", cameras=cameras)
# -----------------------------
# Lancement : seulement local
# -----------------------------
if __name__ == "__main__":
    # Bind sur IP locale du Raspberry
    app.run(port=5000, debug=True)

    print(os.listdir("templates"))
