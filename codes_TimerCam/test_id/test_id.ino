String id;

String getShortID() {
  uint64_t mac = ESP.getEfuseMac(); // lecture du registre matériel EFUSE dans l'ESP 32
  uint32_t low = mac & 0xFFFFFF;    // garde que les 24 bits de poids faible => NIC
  char id[7];                       // 6 + caractère de fin de chaine
  sprintf(id, "%06X", low);         // formatage en string : %X pour hexa en majuscule, 0 pour completion par des zéros et 6 pour une longueur de 6
  return String(id);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  id = getShortID();
}

void loop() {
  Serial.print(id);
  delay(5000);
}
