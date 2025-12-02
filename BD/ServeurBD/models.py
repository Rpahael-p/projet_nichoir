import mariadb
from sqlalchemy.orm import DeclarativeBase, mapped_column, relationship
from sqlalchemy import Integer, String, DateTime, create_engine, ForeignKey, text, Boolean

# Base pour toutes les tables
class Base(DeclarativeBase):
    pass

# -----------------------------
# Table Camera
# -----------------------------
class Camera(Base):
    __tablename__ = "Camera"
    
    DemiMac = mapped_column(String(12), primary_key=True, nullable=False, unique=True)
    NomPersonalise = mapped_column(String(255))
    
    # Relations avec les autres tables
    images = relationship("Image", back_populates="camera")
    batteries = relationship("Batterie", back_populates="camera")
    etats = relationship("Etat", back_populates="camera")
    
    def __str__(self):
        return f"DemiMac: {self.DemiMac}, NomPersonalise: {self.NomPersonalise}"

# -----------------------------
# Table Image
# -----------------------------
class Image(Base):
    __tablename__ = "Image"
    
    id = mapped_column(Integer, primary_key=True, nullable=False)
    Data = mapped_column(String(255), nullable=False)
    Time = mapped_column(DateTime, nullable=False)
    
    CameraDemiMac = mapped_column(String(12), ForeignKey("Camera.DemiMac"))
    camera = relationship("Camera", back_populates="images")
    
    def __str__(self):
        return f"{self.Timestamp} - {self.Chemin} - Camera: {self.CameraDemiMac}"

# -----------------------------
# Table Batterie
# -----------------------------
class Batterie(Base):
    __tablename__ = "Batterie"
    
    id = mapped_column(Integer, primary_key=True, nullable=False)
    Data = mapped_column(Integer, nullable=False)
    Time = mapped_column(DateTime, nullable=False)
    
    CameraDemiMac = mapped_column(String(12), ForeignKey("Camera.DemiMac"))
    camera = relationship("Camera", back_populates="batteries")
    
    def __str__(self):
        return f"{self.Timestamp} - Niveau: {self.Niveau} - Camera: {self.CameraDemiMac}"

# -----------------------------
# Table Etat
# -----------------------------
class Etat(Base):
    __tablename__ = "Etat"
    
    id = mapped_column(Integer, primary_key=True, nullable=False)
    Data = mapped_column(Boolean, nullable=False) 
    Time = mapped_column(DateTime, nullable=False)
    
    CameraDemiMac = mapped_column(String(12), ForeignKey("Camera.DemiMac"))
    camera = relationship("Camera", back_populates="etats")
    
    def __str__(self):
        return f"{self.Timestamp} - {self.Data} - Camera: {self.CameraDemiMac}"

# -----------------------------
# Création de la base de données
# -----------------------------
def main():
    engine = create_engine(
        "mariadb+mariadbconnector://nous:123456789@192.168.2.51:3306/BDNichoir",
        echo=True
    )
    Base.metadata.create_all(engine)

if __name__ == "__main__":
    main()