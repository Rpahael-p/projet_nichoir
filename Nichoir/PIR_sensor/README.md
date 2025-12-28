# Capteur de mouvement PIR 5578

Le capteur utilisé pour détecter le mouvement est l'adafruit 5578 ou BS6121

La datasheet du capteur suivie est la [C17724](./C17724.pdf) car indiquée sur le site adafruit, cependant, une autre datasheet [5578](./5578.PDF) est indiqué sur mouser.

Le capteur possède 6 pins :
- VDD : alimentation en 3 V (2.2 à 3.7 V)
- SENS : valeur entre 0 et 31 en fonction de la tension par rapport à VDD. 
Plus le niveau augmente moins le capteur est sensible
Augmente de 1 pour chaque 64ème de VDD => 0 de 0 à 1/64 et 31 de 31 à 32/64.
Si supérieur alors niveau maximum : 31

- ONTIME : Pin dont la tension appliquée défini la durée du signal de sortie
Varie de 2 secondes à 0 V jusque 1 heure à VDD/2. la datasheet fournit un tableau des résistacnes conseillées pour chaque durée d'activation.

- OEN : Pin enable à connecter au VDD (threshold à 1.2 V)
- REL : signal de sortie dont la durée varie en fonction de la pin ONTIME (VDD)
- VSS = ground


