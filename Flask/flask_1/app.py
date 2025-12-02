from flask import Flask, render_template, request, redirect, url_for ,send_file
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

    # --- Récupérer la liste des caméras ---
    cur.execute("SELECT DemiMac, NomPersonalise FROM Camera ORDER BY NomPersonalise ASC")
    Cameras = [{"mac": r[0], "name": r[1]} for r in cur.fetchall()]

    # --- Lire les filtres ---
    selected_camera = request.args.get("camera", "")
    date_start = request.args.get("start", "")
    date_end = request.args.get("end", "")

    # --- Construire la requéte dynamique ---
    query = """
        SELECT Image.id, Image.Data, Image.Time, Camera.NomPersonalise, Camera.DemiMac
        FROM Image
        LEFT JOIN Camera ON Camera.DemiMac = Image.CameraDemiMac
        WHERE 1=1
    """
    params = []

    if selected_camera:
        query += " AND Image.CameraDemiMac = ?"
        params.append(selected_camera)

    if date_start:
        query += " AND Image.Time >= ?"
        params.append(date_start + " 00:00:00")

    if date_end:
        query += " AND Image.Time <= ?"
        params.append(date_end + " 23:59:59")

    query += " ORDER BY Image.Time DESC"

    cur.execute(query, params)

    Images = [
        {
            "id": row[0],
            "Data": row[1],
            "Time": row[2],
            "CameraName": row[3],
            "CameraMac": row[4]
            
        }
        for row in cur.fetchall()
    ]

    cur.close()
    conn.close()

    return render_template(
        "index.html",
        Images=Images,
        Cameras=Cameras,
        selected_camera=selected_camera,
        date_start=date_start,
        date_end=date_end
    )
    
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
# Lancement : seulement local
# -----------------------------
if __name__ == "__main__":
    # Bind sur IP locale du Raspberry
    app.run(port=5000, debug=True)

    print(os.listdir("templates"))
