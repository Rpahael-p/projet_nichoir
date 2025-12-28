from flask import Flask, render_template, request
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from datetime import datetime
from tables import Film, Genre
from sqlalchemy.exc import SQLAlchemyError

app = Flask(__name__)

# --- Connexion à la base ---
engine = create_engine("mariadb+mariadbconnector://nous:123456789@192.168.2.51:3306/FILMS", echo=True)
Session = sessionmaker(bind=engine)
session = Session()

# --- Page d'accueil ---
@app.route('/')
def index():
    return render_template('index.html')


# --- 1) Création de genre ---
@app.route('/genre/new', methods=['GET', 'POST'])
def create_genre():
    if request.method == 'POST':
        try:
            genre_nom = request.form['genre']
            new_genre = Genre(genre=genre_nom)
            session.add(new_genre)
            session.commit()
            return render_template('resultat.html', message=f"✅ Genre '{genre_nom}' ajouté avec succès !")
        except SQLAlchemyError as e:
            session.rollback()
            return f"Erreur lors de l'ajout : {str(e)}"

    return render_template('formulaire_genre.html')


# --- 2) Création de film ---
@app.route('/film/new', methods=['GET', 'POST'])
def create_film():
    genres = session.query(Genre).all()

    if request.method == 'POST':
        try:
            titre = request.form['titre']
            note = int(request.form['note'])
            date_str = request.form['date_sortie']
            date_obj = datetime.strptime(date_str, "%Y-%m-%d").date()
            duree = int(request.form['duree'])
            id_genre = int(request.form['id_genre'])

            film = Film(titre=titre, note=note, date=date_obj, duree=duree, id_genre=id_genre)
            session.add(film)
            session.commit()

            return render_template('resultat.html', message=f"🎬 Film '{titre}' ajouté avec succès !")
        except SQLAlchemyError as e:
            session.rollback()
            return f"Erreur lors de l'ajout : {str(e)}"

    return render_template('formulaire_film.html', genres=genres)


# --- 3) Afficher tous les genres ---
@app.route('/genres')
def liste_genres():
    genres = session.query(Genre).all()
    return render_template('liste_genres.html', genres=genres)


# --- 4) Afficher tous les films avec filtre ---
@app.route('/films')
def liste_films():
    filtre = request.args.get('filtre', '')
    films = session.query(Film).filter(Film.titre.contains(filtre)).all()
    return render_template('liste_films.html', films=films, filtre=filtre)


if __name__ == '__main__':
    app.run(debug=True)
