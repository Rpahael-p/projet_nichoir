import mariadb
from sqlalchemy.orm import DeclarativeBase, mapped_column, relationship
from sqlalchemy import Integer, String, Date, create_engine, ForeignKey, text

class Base(DeclarativeBase): #herite de Declarative Base
    pass

class Film(Base):
    __tablename__ = "Films" #Si on veut un nom différent de la classe

    id = mapped_column(Integer, primary_key = True, nullable=False)
    titre = mapped_column(String(30), nullable=False)
    note = mapped_column(Integer)
    date = mapped_column(Date)
    duree = mapped_column(Integer)
    id_genre = mapped_column(Integer, ForeignKey('Genre.id'), nullable=False)
    genre = relationship("Genre", back_populates = "films")

    def __str__(self):
         return f"ID : {self.id}, film : {self.titre}, note : {self.note}, date de sortie : {self.date}, durée : {self.duree}, genre : {self.genre}"

class Genre(Base):
    __tablename__ = "Genre"
    id = mapped_column(Integer, primary_key = True, nullable=False)
    genre = mapped_column(String(20), nullable=False)
    films = relationship("Film", back_populates="genre")

    def __str__(self):
        return f"id : {self.id}, genre : {self.genre}"

def main():
    engine = create_engine("mariadb+mariadbconnector://nous:123456789@192.168.2.51:3306/FILMS", echo = True) #String de connexion à la BD
    Base.metadata.create_all(engine)

if __name__ == "__main__":
    main()


