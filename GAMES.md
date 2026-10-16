# Idées de jeux avec un petit robot

Ces jeux s'appuient sur ce que RobotDetectionByWebcam mesure déjà et envoie en UDP (voir le [format des messages](USER_GUIDE.md#103-format-des-messages)). Un petit programme à côté, par exemple en Python à partir de [`tools/udp_receiver.py`](tools/udp_receiver.py), peut tenir le score et arbitrer.

Les jeux marqués « **Dans l'application** » sont aussi intégrés au menu **Jeu** de l'application, qui arbitre la partie et affiche l'état en bandeau sur l'image (voir la [section 8 du guide](USER_GUIDE.md#86-jeu--détecter-le-robot-dans-une-zone)).

## Sommaire

- [Données utiles](#données-utiles)
- [Adresse et pilotage](#adresse-et-pilotage)
- [Jeux à plusieurs objets](#jeux-à-plusieurs-objets)
- [Mémoire et réflexes](#mémoire-et-réflexes)
- [Exploration](#exploration)
- [Conseils pratiques](#conseils-pratiques)

## Données utiles

| Donnée | Où la trouver dans le message UDP | Sert à |
|---|---|---|
| Position sur le sol, en mm | `floor.position_mm` d'un marqueur, d'un carré ou d'un objet suivi | Zones, portes, distances, cibles |
| Cap du robot | `floor.heading_deg` d'un marqueur ArUco | Parking, ordres « tourne », dessin |
| Vitesse et direction | `tracks` (marqueurs et carrés, lissage actif) ou `tracked` (objets suivis) | Limite de vitesse, détection d'arrêt |
| Débuts et fins de mouvement | `events` (`motion_start`, `motion_stop`) des objets suivis | Chrono, arbitre de « 1, 2, 3, soleil » |
| Perte et sortie de l'image | `events` (`lost`, `found`) et `out_of_view` des objets suivis | Sorties de terrain, touches |
| Plusieurs objets | Listes `markers` et `tracked` | Jeux à deux robots, balle, objets à ramasser |

Les événements de mouvement ne concernent que les **objets suivis** (onglet **Suivi**). Pour un robot porteur d'un marqueur, le programme de jeu peut détecter lui-même l'arrêt et le départ à partir de la vitesse lissée (`tracks`), ou encadrer aussi le robot dans l'onglet **Suivi**.

## Adresse et pilotage

### 1. Course contre la montre

**Dans l'application** : menu **Jeu**, « Course contre la montre » (voir la [section 8.10 du guide](USER_GUIDE.md#810-jeu--course-contre-la-montre)).

**Principe.** Des portes virtuelles, définies en mm dans le repère du sol, sont à franchir dans l'ordre. Le chrono démarre au premier mouvement et s'arrête à la dernière porte.

**Données.** Position sur le sol, début de mouvement.

**Pistes.** Afficher le meilleur tour et le tracé de chaque essai ; ajouter des pénalités pour une porte manquée ; varier les circuits.

### 2. Le parking

**Dans l'application** : menu **Jeu**, « Le parking » (voir la [section 8.11 du guide](USER_GUIDE.md#811-jeu--le-parking)).

**Principe.** Arrêter le robot dans une case dessinée au sol, avec le bon cap. Le score dépend de l'écart de position et d'angle, mesuré à l'arrêt. Les cases rétrécissent à chaque niveau.

**Données.** Position et cap (marqueur ArUco), fin de mouvement.

**Pistes.** Créneau entre deux « voitures » (zones interdites) ; marche arrière imposée.

### 3. Le fil électrique

**Dans l'application** : menu **Jeu**, « Le fil électrique » (voir la [section 8.12 du guide](USER_GUIDE.md#812-jeu--le-fil-électrique)).

**Principe.** Suivre un parcours sinueux sans sortir du couloir. Chaque sortie coûte une pénalité. Variante « main tremblante » : une vitesse maximale à ne pas dépasser.

**Données.** Position sur le sol, vitesse en mm/s.

**Pistes.** Couloir de plus en plus étroit ; parcours tiré au hasard.

### 4. Curling

**Dans l'application** : menu **Jeu**, « Curling » (voir la [section 8.13 du guide](USER_GUIDE.md#813-jeu--curling)).

**Principe.** Lancer le robot vers une cible et le laisser s'arrêter au plus près du centre. Seule la position à l'arrêt compte. On joue à plusieurs, chacun son tour.

**Données.** Position sur le sol, fin de mouvement.

**Pistes.** Cible à anneaux (10, 5, 2 points) ; manches et classement.

## Jeux à plusieurs objets

### 5. Chat et souris

**Dans l'application** : menu **Jeu**, « Chat et souris » (voir la [section 8.14 du guide](USER_GUIDE.md#814-jeu--chat-et-souris)).

**Principe.** Deux robots, ou un robot et un objet poussé à la main, sont suivis en même temps. Le « chat » marque un point quand la distance entre eux passe sous un seuil. Les rôles s'inversent toutes les 30 secondes.

**Données.** Positions des deux objets (deux marqueurs, ou deux objets suivis).

**Pistes.** Zones refuges où la souris ne peut pas être prise ; durée de survie comme score.

### 6. Football de table

**Principe.** Le robot pousse une balle, suivie comme objet encadré, vers des buts virtuels. Le but est compté quand la balle franchit la ligne. Une balle qui sort par un bord de l'image est détectée « hors du champ » : c'est une touche.

**Données.** Position de la balle, état `out_of_view`, position du robot.

**Pistes.** Deux robots, un par équipe ; temps de possession.

### 7. Ramassage

**Principe.** Des objets sont posés au sol et suivis. Le robot doit en pousser le plus possible dans une zone en un temps donné. Le compte se fait sur la position des objets, pas sur celle du robot.

**Données.** Positions des objets suivis, position du robot.

**Pistes.** Objets de valeurs différentes (selon leur couleur ou leur nom) ; zone qui se déplace.

## Mémoire et réflexes

### 8. Jacques a dit

**Principe.** Le programme annonce un ordre : « avance de 30 cm », « tourne d'un quart de tour », « arrête-toi ». La mesure dit si l'ordre a été respecté, en distance, en angle et au moment de l'arrêt. Un ordre sans « Jacques a dit » ne doit pas être exécuté.

**Données.** Position et cap (marqueur ArUco), débuts et fins de mouvement.

**Pistes.** Annonces à la voix (synthèse vocale) ; enchaînements d'ordres de plus en plus longs.

### 9. 1, 2, 3, soleil

**Dans l'application** : menu **Jeu**, « 1, 2, 3 Soleil » (voir la [section 8.7 du guide](USER_GUIDE.md#87-jeu--1-2-3-soleil)).

**Principe.** Le robot ne doit pas bouger quand le programme dit « soleil ». L'événement de début de mouvement sert d'arbitre, et le seuil de mouvement réglable fixe la sévérité. Le but est d'atteindre une ligne d'arrivée.

**Données.** Début de mouvement, position sur le sol.

**Pistes.** Durées de « soleil » aléatoires ; retour au départ en cas de faute.

### 10. Dessin au sol

**Principe.** Reproduire une forme (carré, cercle, lettre) avec la trajectoire du robot. Le programme compare le tracé au modèle et donne un pourcentage de ressemblance.

**Données.** Suite des positions sur le sol.

**Pistes.** Formes de plus en plus complexes ; galerie des meilleurs dessins.

## Exploration

### 11. Chasse au trésor

**Dans l'application** : menu **Jeu**, « Chasse au trésor » (voir la [section 8.8 du guide](USER_GUIDE.md#88-jeu--chasse-au-trésor)).

**Principe.** Un trésor est caché à une position secrète. Le programme guide le robot par « chaud / froid » selon la distance, à la voix ou par la couleur d'un affichage.

**Données.** Position sur le sol.

**Pistes.** Plusieurs trésors ; nombre limité d'indices.

### 12. Champ de mines

**Principe.** Des zones interdites invisibles sont placées sur le sol. Le robot doit traverser la zone en mémorisant les « explosions » des essais précédents. Les trajectoires sont affichées à la fin.

**Données.** Position sur le sol, trajectoire.

**Pistes.** Mines qui changent à chaque partie ; détecteur qui « bipe » à l'approche.

## Conseils pratiques

- **Un marqueur ArUco sur le robot** est le plus fiable, et il donne le cap, indispensable pour le parking ou « Jacques a dit ». Voir la [section 5 du guide](USER_GUIDE.md#5-suivre-des-marqueurs-aruco).
- **Définissez le repère du sol** pour travailler en mm. Si la caméra est calibrée, renseignez la **Hauteur des repères** pour une position exacte du marqueur posé sur le robot. Voir la [section 8 du guide](USER_GUIDE.md#8-définir-le-repère-du-sol).
- **Commencez par les jeux 1, 2 et 9** : un seul objet et des zones fixes, peu de code côté récepteur.
- **Pour un jeu où le programme pilote le robot** (« Jacques a dit » automatique, chasse au trésor guidée), il faut pouvoir commander le robot depuis l'ordinateur : Bluetooth, Wi-Fi ou autre, selon le robot.
