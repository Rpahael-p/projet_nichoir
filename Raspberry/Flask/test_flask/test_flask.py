from flask import Flask, render_template, request, send_file
import os

folder = "C:/Users/theod/Desktop/SmartPhotos/"

app = Flask("test flask", template_folder="./templates")
#app.config['upload_folder'] 

@app.route('/')
def index():
    image_path = os.path.join(folder, "photo_11169.jpg")
    print(f"image path = {image_path}")
    return render_template('index.html', image = image_path)

@app.route("/image")
def load_image():
    path = request.args.get("path")

    if not path:
        return "Aucun chemin fourni", 400

    if not os.path.exists(path):
        return "Fichier introuvable : " + path, 404

    return send_file(path, mimetype="image/jpeg")
app.run(debug=True)