# RobotDetectionByWebcam

Application Qt6 qui affiche le flux d'une webcam et détecte en temps réel :

- un **carré d'une couleur donnée** ;
- des **marqueurs ArUco** (ou AprilTag), avec leur identifiant ;
- **un ou plusieurs objets quelconques**, désignés à la souris (en les encadrant ou d'un clic), suivis d'image en image avec leur trajectoire, leur vitesse et leurs débuts et fins de mouvement.

Pour chacun, l'application donne la position dans l'image, ainsi que la taille et l'orientation des carrés et des marqueurs. Elle peut aussi :
- donner les positions **en millimètres sur le sol**, après avoir cliqué les 4 coins d'un rectangle connu ;
- donner la **position 3D** et l'orientation des marqueurs ArUco, une fois la caméra **calibrée** avec un damier ;
- **lisser** les positions et calculer les **vitesses** (filtre de Kalman) ;
- **envoyer** toutes les mesures en JSON sur UDP à un autre programme, par exemple celui qui pilote le robot ;
- signaler ce qui entre dans des **zones** nommées de l'image et en sort ;
- **capturer une image** ou **enregistrer une vidéo** de la webcam ;
- afficher la vidéo seule dans une **fenêtre pour le joueur**, en plein écran sur un second écran ;
- proposer des **jeux** (menu **Jeu**) : détecter quand le robot est entièrement dans une zone, arbitrer une partie de « 1, 2, 3 Soleil », et une chasse au trésor guidée par « chaud / froid ».

Les objets encadrés peuvent être suivis par les trackers classiques d'OpenCV (CSRT, KCF) ou par des **réseaux de neurones** (Nano, Vit), plus robustes. Les réglages de la webcam (exposition, balance des blancs, luminosité, gain, fréquence du secteur...) se font depuis l'application.

Pour découvrir l'application pas à pas, suivez le [tutoriel](TUTORIAL.md). Le mode d'emploi détaillé (réglages, calibration pas à pas, lecture des résultats, dépannage) se trouve dans le [guide utilisateur](USER_GUIDE.md).

La liste complète des fonctionnalités et des améliorations envisagées se trouve dans [IMPROVEMENTS.md](IMPROVEMENTS.md). Des idées de jeux avec un petit robot sont réunies dans [GAMES.md](GAMES.md).

## Prérequis

- Linux avec une webcam (`/dev/video*`)
- CMake ≥ 3.16 et un compilateur C++17 (g++ ou clang)
- Qt 6 : modules **Widgets**, **Multimedia** (capture, affichage et enregistrement vidéo, avec le moteur FFmpeg de Qt), **Network** (envoi UDP, téléchargement des modèles) et **Concurrent** (traitement dans un fil séparé)
- OpenCV ≥ 4.7 : modules **core**, **imgproc**, **calib3d**, **objdetect**, **video** et **tracking** (détection, calibration et suivi). ArUco fait partie d'`objdetect` depuis la 4.7. `tracking` vient des modules contrib (paquet `libopencv-contrib-dev`, installé avec `libopencv-dev`). Les trackers Nano et Vit du module `video` demandent un OpenCV compilé avec le module **dnn** (c'est le cas des paquets Debian et Ubuntu).

Sur Debian/Ubuntu :

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev libopencv-dev
```

## Compilation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Lancement

```bash
./build/RobotDetectionByWebcam
```

## Utilisation

Choisir la caméra dans la liste déroulante si plusieurs webcams sont branchées, puis sa **résolution**. Chaque caméra garde ses propres réglages : résolution, réglages de l'image, **calibration** et **repère du sol**. On peut donc passer de l'une à l'autre sans rien refaire. Changer de caméra arrête le suivi des objets encadrés. Deux caméras du même modèle sont distinguées par leur identifiant système. La liste indique pour chaque résolution son format d'image (4:3, 16:9...) et le nombre maximal d'images par seconde. La résolution choisie est mémorisée pour chaque modèle de caméra et réappliquée au lancement suivant. Au premier lancement, la caméra garde son format par défaut et la liste se cale dessus. Une résolution plus basse accélère la détection ; une résolution plus haute permet de détecter les marqueurs de plus loin.

Pour chaque résolution, l'application choisit le format vidéo le plus rapide. Certaines webcams, comme la Logitech C270, envoient toujours leur résolution maximale en MJPEG, quelle que soit la résolution demandée. L'application le détecte et passe alors automatiquement au format non compressé (YUYV) de la même résolution. Ce basculement est mémorisé avec la résolution.

Les réglages sont répartis en sept onglets : **Carré coloré**, **ArUco**, **Calibration**, **Sol**, **Zones**, **Suivi** et **Envoi UDP**. Le menu **Jeu**, en haut de la fenêtre, regroupe les jeux, et une barre d'état, en bas, affiche les messages brefs (chemin d'une capture...). Dans les deux premiers, la case « Activer la détection » active ou désactive la détection correspondante. Les deux détections peuvent tourner en même temps. Elles sont **désactivées à chaque lancement** (leur état n'est pas mémorisé) : au départ, seuls les objets encadrés à la main dans l'onglet « Suivi » sont suivis.

### Caméra, réglages et fermeture

Le bouton « Capture » (touche F12) enregistre aussitôt l'image de la webcam en PNG, en pleine résolution, dans `~/Images/RobotDetectionByWebcam/` (dossier modifiable). Son menu propose aussi l'image avec les détections dessinées (Maj+F12) et l'ouverture du dossier.

Le bouton « Vidéo » (touche F9) enregistre une vidéo MP4 (H.264) dans le même dossier ; un nouveau clic l'arrête. La vidéo de la webcam est enregistrée par `QMediaRecorder` sur la session de la caméra, à son débit et en pleine résolution. La vidéo avec les détections (Maj+F9) est faite des images affichées, envoyées une à une à un second enregistreur par `QVideoFrameInput` et horodatées à l'heure réelle : elle garde la vitesse du traitement. L'enregistrement s'arrête au changement de caméra ou de résolution, et la fermeture attend que le fichier soit terminé.

Le bouton « Joueur » ouvre une seconde fenêtre (`src/playerwindow.cpp`) qui ne montre que la vidéo, sur fond noir, pour un second écran tourné vers le joueur. Elle s'ouvre en plein écran sur l'autre écran que celui de la fenêtre principale (ou sur l'écran choisi dans le menu du bouton), avec ou sans les détections dessinées, éventuellement retournée en miroir. Double-clic ou F11 : plein écran ; Échap : le quitte. Ses réglages et son ouverture sont mémorisés.

Le bouton « Réglages de l'image... » ouvre une fenêtre pour régler l'**exposition** (automatique ou temps de pose manuel) et la **balance des blancs** (automatique ou température manuelle), ainsi que les **autres réglages** fournis par la webcam : luminosité, contraste, saturation, gain, netteté, fréquence du secteur (anti-scintillement), ralentissement des images en exposition automatique, voyant... La liste et les limites (dont celles du temps de pose) sont lues auprès du pilote V4L2 (`VIDIOC_QUERYCTRL`). Une pose courte donne plus d'images par seconde et moins de flou de mouvement dans une pièce peu éclairée, au prix d'une image plus sombre. Une balance des blancs manuelle empêche les couleurs de dériver. Ces réglages sont mémorisés pour chaque modèle de caméra et réappliqués au démarrage de la caméra. À la fermeture normale (et au changement de caméra), l'application remet la webcam en automatique et rend aux autres réglages leur valeur d'avant le lancement, car ces réglages sont stockés dans la webcam elle-même. Pour l'exposition automatique, l'application écrit directement le contrôle V4L2 (`V4L2_CID_EXPOSURE_AUTO`) : sur les webcams qui n'acceptent que le mode « priorité ouverture » (comme la Logitech C270), Qt ne parvient pas à rétablir l'automatique.

L'option « Effet miroir » retourne seulement l'image affichée. Les détections se font toujours sur l'image réelle de la caméra : les marqueurs ArUco et le damier restent reconnus, et toutes les mesures (coordonnées, angles, pose 3D, positions au sol) restent dans le repère réel de la caméra, que le miroir soit activé ou non. Les clics sur l'image (couleur, sélection d'un objet, points du sol) sont convertis automatiquement.

Les réglages (caméra, résolution, réglages de l'image, miroir, dossier des captures, fenêtre du joueur, couleur et curseurs, dictionnaire et taille des marqueurs, damier, repère du sol, zones, méthode, trajectoire et seuils du suivi, lissage, envoi UDP, taille de la fenêtre) sont enregistrés dans `~/.config/RobotDetectionByWebcam/settings.ini` et restaurés au lancement suivant. Les réglages propres à chaque caméra sont regroupés dans la section `[cameras]` sous un nom lisible : le nom de la caméra réduit aux lettres et aux chiffres, suivi de l'identifiant USB s'il y figure (« UVC Camera (046d:0825) » devient `UVC_Camera_046d_0825`, au lieu du nom encodé par Qt `UVC%20Camera%20%28046d%3A0825%29` des versions précédentes, renommé automatiquement au premier lancement). La résolution, les réglages de l'image, le repère du sol et les zones sont enregistrés dès qu'on les modifie, le reste à la fermeture de l'application. Supprimer ce fichier rétablit les réglages par défaut.

Le bouton « Quitter », en bas à droite, ferme l'application après une confirmation. La même confirmation s'affiche avec la croix de la fenêtre.

### Carré coloré

1. **Choisir la couleur du carré** :
   - le plus simple : **cliquer sur le carré dans l'image**. La couleur est prise sur une zone de 7×7 pixels, le type de couleur est choisi et les seuils sont ajustés automatiquement ;
   - ou cliquer sur le bouton « Couleur » pour ouvrir un sélecteur.

   Le **Type** de couleur est choisi automatiquement d'après la saturation de la couleur, et peut être changé à la main :
   - **Couleur vive (teinte)** : sélection par la teinte ;
   - **Blanc, gris ou noir** : ces couleurs n'ont pas de teinte. Elles sont sélectionnées par leur luminosité et leur faible saturation. Le carré doit contraster avec le fond.
2. **Ajuster les réglages** si besoin, en cochant « Afficher le masque » pour voir les pixels retenus (en blanc).

| Réglage | Rôle |
|---|---|
| Tolérance teinte | *Couleur vive* : écart de teinte accepté autour de la couleur cible (en degrés) |
| Saturation min | *Couleur vive* : écarte les zones grises ou délavées |
| Luminosité min | *Couleur vive* : écarte les zones trop sombres |
| Tolérance luminosité | *Blanc, gris ou noir* : écart de luminosité accepté autour de la couleur cible |
| Saturation max | *Blanc, gris ou noir* : écarte les zones colorées ; à augmenter sous une lumière chaude qui teinte le blanc |
| Surface min | Taille minimale d'un carré (en pixels de l'image de travail, 320 px de large) |
| Remplissage min (%) | Part du rectangle orienté que la zone doit remplir ; à augmenter pour mieux écarter les cercles |

### Marqueurs ArUco

1. **Choisir le dictionnaire** : 4x4, 5x5, 6x6, ArUco original ou AprilTag 36h11. Le nombre entre parenthèses est le nombre de marqueurs du dictionnaire. Seuls les marqueurs du dictionnaire choisi sont détectés.
2. **Générer un marqueur à imprimer** : cliquer sur « Générer un marqueur... », choisir un identifiant, puis enregistrer l'image PNG (600 px avec une marge blanche).
3. Imprimer le marqueur **sans le redimensionner de façon déformée** et **en gardant la marge blanche**, indispensable à la détection. Le coller bien à plat sur le robot.

Conseils :
- Plus le dictionnaire a peu de cases (4x4), plus le marqueur est détecté de loin. Plus il contient de marqueurs, plus le risque de confusion augmente : prendre le plus petit dictionnaire qui suffit.
- Éviter les reflets : préférer un papier mat.

**Taille du marqueur** : indiquer le côté de la partie noire du marqueur imprimé, sans la marge blanche, mesuré à la règle. Cette valeur sert uniquement à la position 3D ; une erreur de taille fausse les distances dans la même proportion.

### Calibration de la caméra

La calibration mesure les caractéristiques optiques de la webcam (focale, centre optique, distorsion). Elle est nécessaire pour obtenir la position 3D des marqueurs. Elle se fait une seule fois par caméra.

1. **Imprimer un damier** : dans l'onglet « Calibration », régler le nombre de coins intérieurs (9 × 6 par défaut, soit un damier de 10 × 7 cases), puis cliquer sur « Générer un damier... ». Imprimer à 100 %, sans « ajuster à la page ».
2. **Mesurer une case à la règle** et saisir sa taille dans « Côté d'une case ».
3. Coller le damier **bien à plat** sur un support rigide (carton, planche).
4. Cocher **« Mode calibration »**. Quand le damier est vu en entier, ses coins s'affichent en bleu clair.
5. **Capturer 15 à 25 images** avec « Capturer » ou « Capture automatique (toutes les 2 s) ». Un cadre blanc confirme chaque capture. Varier les vues :
   - damier proche et éloigné ;
   - dans le centre et dans les coins de l'image ;
   - incliné dans plusieurs directions (jusqu'à 45° environ) ;
   - garder le damier immobile au moment de la capture pour éviter le flou.
6. Cliquer sur **« Calibrer »**. L'erreur RMS s'affiche : en dessous de 0,5 px, c'est très bon ; au-dessus de 1 px, il vaut mieux recommencer.

La calibration est propre à chaque caméra : elle est enregistrée dans `~/.config/RobotDetectionByWebcam/calibrations/<caméra>.yml` (par exemple `UVC_Camera_046d_0825.yml`), et rechargée au démarrage et à chaque changement de caméra. L'ancienne calibration unique (`calibration.yml`, versions précédentes) est copiée pour la caméra active au premier lancement ; l'original est conservé. Elle reste valable si l'on change de résolution, tant que le format d'image (4:3, 16:9...) ne change pas. Si la résolution choisie a un autre format que la calibration, la pose 3D est désactivée et un message l'indique dans le panneau « Résultat ». Il faut la refaire si l'on change de caméra ou si l'on modifie le zoom ou la mise au point.

Changer les dimensions du damier efface les captures en cours.

### Affichage des résultats

- Le carré principal (le plus grand) est entouré **en vert**, avec son centre et ses coordonnées.
- Les autres carrés détectés sont entourés **en jaune**.
- Les marqueurs ArUco sont entourés **en magenta**, avec leur identifiant. Le coin d'origine du marqueur (en haut à gauche) est marqué d'un point rouge.
  - Sans calibration, un trait part du centre vers le bord supérieur pour montrer l'orientation.
  - Avec calibration, les axes 3D du marqueur sont dessinés : **X en rouge** (droite), **Y en vert** (bas), **normale en bleu** (sort du marqueur vers la caméra).
- En mode calibration, les coins du damier détecté sont reliés en bleu clair.
- Le repère du sol est dessiné en pointillés orange, avec l'axe X en rouge et l'axe Y en vert.
- Chaque objet suivi est entouré dans **sa couleur**, avec son nom (pointillés quand sa position est prédite, rouge en pointillés quand il est perdu), sa trajectoire et une flèche dans le sens du déplacement.
- Les trajectoires lissées du carré (vert clair) et des marqueurs (rose) sont tracées, et la position prédite d'un objet momentanément perdu est marquée d'un cercle en pointillés.
- Le panneau « Résultat » indique le centre (en pixels de l'image d'origine), le côté, l'angle (0–90°), le nombre de carrés et les images par seconde. Pour chaque marqueur ArUco, il indique l'identifiant, le centre, le côté et l'angle : 0° quand le marqueur est droit, positif dans le sens horaire, entre −180° et 180°.

Avec calibration, chaque marqueur a aussi :
- **X, Y, Z** : la position de son centre en mm, dans le repère de la caméra (X vers la droite, Y vers le bas, Z droit devant la caméra) ;
- **distance** : la distance caméra–marqueur en mm ;
- **rotation X, Y, Z** : ses angles d'Euler en degrés. (0, 0, 0) correspond à un marqueur droit, face à la caméra. Z est la rotation dans le plan du marqueur ; X et Y sont ses inclinaisons.

### Positions sur le sol

L'onglet « Sol » définit un repère sur le sol : on saisit les dimensions d'un rectangle tracé au sol (ruban adhésif, coins d'une table...), puis on clique ses 4 coins dans l'image, dans l'ordre : origine, coin sur l'axe X, coin opposé, coin sur l'axe Y. L'application donne alors la position de chaque carré et de chaque marqueur **en mm sur le sol**, avec le cap des marqueurs (direction de leur bord supérieur, de l'axe X vers l'axe Y). Ces valeurs s'affichent dans le panneau « Résultat » et sont envoyées en UDP (champ `floor`).

Le repère est propre à chaque caméra, comme la calibration. Il fonctionne sans calibration. Si la caméra est calibrée, la distorsion de l'objectif est corrigée en plus. Il est enregistré dans `settings.ini` et reste valable pour toute résolution du même format d'image. La caméra ne doit plus bouger une fois le repère défini. Le repère est défini au niveau du sol. Un marqueur placé en hauteur sur le robot est donc décalé, jusqu'à plusieurs centimètres. Si la caméra est calibrée, le champ « Hauteur des repères » corrige ce décalage pour les carrés et les marqueurs. L'onglet affiche alors aussi la hauteur de la caméra au-dessus du sol, vérifiable au mètre ruban.

### Zones

L'onglet « Zones » définit des rectangles fixes de l'image, chacun avec un nom : on clique sur « Ajouter une zone », on trace le rectangle (ajustable avec ses poignées), on valide, puis on le nomme. Les zones se renomment dans la liste (double-clic) et sont mémorisées pour chaque caméra, en fraction de l'image (valables pour toute résolution du même format). L'application indique ce qui se trouve dans chaque zone (objets suivis, marqueurs ArUco, carré principal, d'après leur centre) et inscrit chaque entrée et sortie dans le journal des événements (`zone_enter`, `zone_exit`). Une sortie n'est signalée qu'après 0,3 s hors de la zone, avec une marge de 1 % de l'image : un point qui tremble sur le bord ou un marqueur manqué sur une image ne crée pas de fausses sorties. Les zones, leurs occupants et, si le repère du sol est défini, leurs coins sur le sol en mm sont envoyés en UDP (liste `zones`). Le code est dans `src/zonemanager.cpp`.

### Jeux

Le menu « Jeu » propose des jeux construits sur le suivi d'objets, les marqueurs et les zones : « Détection robot dans zone », « 1, 2, 3 Soleil », « Chasse au trésor », puis les jeux du cadre commun décrits plus bas. Dans chacun, le robot est un objet suivi (onglet « Suivi ») ; l'état de la partie s'affiche dans une petite fenêtre propre au jeu et en bandeau en haut de l'image, donc aussi dans la fenêtre du joueur ; les événements vont dans le journal et en UDP, et l'objet `game` décrit la partie en cours. Un seul jeu tourne à la fois : en démarrer un arrête les autres.

### Jeu : détection du robot dans une zone

Le menu « Jeu », « Détection robot dans zone... », ouvre une fenêtre où l'on choisit le robot (un objet suivi) et la zone visée (ou n'importe laquelle). Le robot est « dans la zone » quand son cadre de suivi y est **entièrement contenu** (`QRectF::contains`), pas seulement son centre. L'état (dans la zone, hors zone, non trouvé) ne change qu'après 0,2 s de stabilité, et une position seulement prédite ne le modifie pas. Il s'affiche en grand dans la fenêtre de jeu et en bandeau sur l'image (donc aussi dans la fenêtre du joueur), avec le temps passé dans la zone et le nombre d'entrées. Les entrées et sorties vont dans le journal (`game_zone_enter`, `game_zone_exit`) et l'état de la partie est envoyé en UDP (objet `game`). Le code est dans `src/robotzonegame.cpp`.

### Jeu : 1, 2, 3 Soleil

Le menu « Jeu », « 1, 2, 3 Soleil... », arbitre une course vers une zone d'arrivée : le robot (un objet suivi) ne doit avancer qu'au feu vert. Après un compte à rebours de 3 s, les phases « AVANCEZ ! » et « SOLEIL ! » alternent avec des durées tirées au hasard (`QRandomGenerator`), selon trois difficultés. Pendant « SOLEIL ! », après un temps de réaction (0,8, 0,5 ou 0,3 s), la position du robot est mémorisée ; s'il s'en éloigne de plus que le seuil de mouvement du suivi (20 mm ou 15 px par défaut), une faute est comptée, une seule par phase, et ajoute 5 s au score. La partie se termine quand le cadre du robot est entièrement dans la zone d'arrivée (confirmé après 0,2 s) : temps, fautes et score s'affichent en bandeau. Les phases, fautes et l'arrivée vont dans le journal et en UDP (`game_green`, `game_red`, `game_fault`, `game_win`, objet `game` avec `mode` = `soleil`). Le code est dans `src/soleilgame.cpp`.

### Jeu : chasse au trésor

Le menu « Jeu », « Chasse au trésor... », cache un trésor invisible à un endroit tiré au hasard (`QRandomGenerator`), dans une zone choisie comme terrain ou dans toute l'image, à au moins 8 % du bord. Le bandeau donne un indice selon la distance du centre du robot au trésor, rapportée à la diagonale du terrain (Glacial au-delà de 60 %, Froid, Tiède, Chaud, BRÛLANT sous 12 %), et la tendance (« plus chaud » ou « plus froid »), mesurée sur 0,7 s. Quand le centre du robot passe à moins du rayon du trésor (12, 8 ou 5 % du petit côté du terrain selon la difficulté), le trésor est trouvé, marqué d'une étoile dorée, et un nouveau est caché à au moins 30 % de la diagonale du robot. La partie dure 1 à 5 minutes. Les trésors trouvés et la fin vont dans le journal et en UDP (`game_treasure`, `game_over`, objet `game` avec `mode` = `treasure`, qui donne aussi la position du trésor pour un robot autonome). Le code est dans `src/treasuregame.cpp`.

### Jeux du cadre commun

Les jeux suivants reposent sur un cadre commun (`src/gameframework.cpp`) : chaque jeu (classe `Game`) déclare ses réglages (objets suivis, marqueur, zones, choix), et la fenêtre du jeu (`GameDialog`) est construite à partir de cette déclaration. À chaque image, le jeu reçoit les objets suivis, les marqueurs et les zones (`GameContext`), et fournit son bandeau, son dessin sur l'image, ses événements (type `game_event`, avec un `code` et un texte) et son objet `game` pour l'UDP. « Démarrer » vérifie d'abord que les réglages sont utilisables.

- **Course contre la montre** (`src/racegame.cpp`) : portes (zones cochées, dans l'ordre de l'onglet Zones) à franchir dans l'ordre, sur 1 à 3 tours. Une porte est franchie quand le centre du robot y entre ; le chrono démarre au premier mouvement (seuil de mouvement du suivi) ; une mauvaise porte coûte 5 s. Le record est conservé par parcours (portes et tours) pendant la session.

- **Le parking** (`src/parkinggame.cpp`) : garer le robot, repéré par un marqueur ArUco pour connaître son cap, dans une case avec le cap demandé. Le stationnement est jugé après 1 s d'immobilité dans la place : marqueur entièrement dans la case cible et écart de cap sous la tolérance. Les points (10 à 100) dépendent de l'écart au centre et de l'écart de cap ; sur 5 niveaux, la case cible rétrécit de 100 % à 40 % de la place et la tolérance passe de 30° à 10°.

- **Le fil électrique** (`src/wiregame.cpp`) : suivre un couloir formé de zones qui se touchent (la première est le départ, la dernière l'arrivée) sans que le cadre du robot en dépasse. Le cadre est jugé dans le couloir quand ses quatre coins et son centre sont dans l'union des zones ; une sortie confirmée après 0,2 s est une touche (+3 s). Variante « main tremblante » : vitesse maximale (12 % ou 6 % de la largeur de l'image par seconde), chaque excès +2 s. Le chrono va de la sortie du départ à l'arrivée complète.

- **Curling** (`src/curlinggame.cpp`) : 1 à 4 joueurs lancent le robot à tour de rôle vers une cible (une zone) à trois anneaux, 10, 5 et 2 points selon la distance au centre (tiers, deux tiers et totalité du rayon). Le lancer est détecté au premier déplacement après 1 s d'immobilité, et jugé quand le robot est de nouveau immobile depuis 1 s ; un robot perdu pendant le lancer vaut 0. Sur 1, 3 ou 5 manches, avec le classement à la fin.

- **Chat et souris** (`src/catmousegame.cpp`) : deux objets suivis ; le chat marque un point quand les cadres se touchent (marge de 5 %), sauf si le centre de la souris est dans le refuge (zone facultative). Après une prise, il faut se séparer et attendre 3 s ; les rôles s'inversent toutes les 20, 30 ou 60 s, sans prise possible avant séparation. Partie de 1 à 5 minutes.

- **Football de table** (`src/footballgame.cpp`) : le robot pousse une balle (objet suivi) vers deux buts (zones). Un but est compté quand le centre de la balle reste 0,2 s dans un but, pour l'équipe adverse ; une balle « hors du champ » du suivi est une touche. Le jeu reprend quand la balle replacée est hors des buts et immobile depuis 1 s. Match de 2 à 10 minutes.

### Suivi d'objets encadrés

Dans l'onglet « Suivi », cliquer sur « Ajouter des objets », puis tracer un rectangle autour de chaque objet, ou cliquer dessus : un simple clic délimite l'objet automatiquement (`cv::grabCut` sur une zone autour du clic). Le cadre reste ajustable, avec ses poignées ou en le faisant glisser, jusqu'à sa validation : bouton « Valider l'objet », touche Entrée ou double-clic. On reste en mode ajout pour l'objet suivant ; Échap ou « Terminer l'ajout » termine. Chaque objet a un numéro, un nom modifiable (double-clic dans la liste des objets) et une couleur. « Retirer » arrête le suivi de l'objet sélectionné dans la liste, « Tout retirer » celui de tous les objets. Chaque objet est suivi d'image en image par son propre tracker OpenCV :
- **CSRT** : précis, environ 25 ms de calcul par image et par objet (avec 3 objets, environ 13 images traitées par seconde en 1280 × 960 sur 4 cœurs) ;
- **KCF** : rapide, environ 5 ms par image et par objet ;
- **Nano** (`cv::TrackerNano`, réseau de neurones NanoTrack) : plus robuste aux changements d'aspect et aux disparitions momentanées, environ 10 ms par image et par objet ;
- **Vit** (`cv::TrackerVit`, réseau de neurones VitTrack) : comparable, environ 15 ms.

Nano et Vit demandent un modèle ONNX (1,7 Mo et 0,7 Mo), que l'application propose de télécharger au premier choix de la méthode, dans `~/.local/share/RobotDetectionByWebcam/models/`, en vérifiant son empreinte SHA-256. Les modèles sont aussi cherchés dans `models/` à côté de l'exécutable et dans le dossier parent (lancement depuis `build/`). Sur une séquence de synthèse (objet texturé qui tourne de 90°, change de taille, disparaît 12 images puis passe derrière un poteau), l'objet est trouvé sur 175 images visibles sur 187 avec Nano, 169 avec Vit, 168 avec CSRT et 147 avec KCF. Leur propre score de suivi reste élevé même sur un mauvais objet : la perte est détectée par l'indice de confiance de l'application, comme pour les autres trackers.

L'application affiche, dans la couleur de chaque objet, le cadre, la **trajectoire** (durée réglable, 10 s par défaut) et une flèche dans le sens du déplacement. Le panneau « Résultat » donne la position, l'état (en mouvement ou immobile), la vitesse et la direction, lissées par un filtre de Kalman (mêmes réglages que le lissage des carrés et des marqueurs). Juste après une perte, la position est prédite à vitesse constante pendant le délai de maintien. Si le repère du sol est défini, il donne aussi la position, la vitesse et la direction en mm sur le sol. Ces valeurs sont envoyées en UDP (liste `tracked`, une entrée par objet).

Un **indice de confiance** compare à chaque image le contenu du cadre à l'apparence de l'objet. S'il chute, l'objet est déclaré perdu, même si le tracker croit encore le suivre : cela arrive quand le cadre glisse sur le fond ou s'accroche à un autre objet passé devant. Le tracker peut aussi perdre l'objet (objet caché, sorti de l'image, trop rapide). Dans les deux cas, l'application **recherche l'objet dans toute l'image** et reprend le suivi dès qu'il réapparaît, même ailleurs. Un objet perdu alors que son cadre touchait le bord est déclaré **hors du champ** : son tracker est arrêté (il s'accrocherait à la main ou au visage restés dans l'image) et l'objet n'est plus cherché que le long des bords, par où il reviendra. Un objet revenu très tourné ou de taille très différente peut ne pas être reconnu : il faut alors le resélectionner. Le cadre reste droit : l'orientation de l'objet n'est pas mesurée. Changer de résolution arrête le suivi.

L'état « en mouvement » repose sur un **seuil de distance** réglable : 20 mm sur le sol, ou 15 px sans repère du sol. Les **débuts et fins de mouvement**, les pertes et les reprises de chaque objet sont inscrits dans un **journal des événements** horodaté, avec le nom de l'objet, enregistrable en CSV, et envoyés en UDP (liste `events`).

### Lissage dans le temps

Le groupe « Lissage des carrés et marqueurs » de l'onglet « Suivi » suit dans le temps le carré coloré principal et chaque marqueur ArUco, reconnu par son identifiant. Il calcule une position lissée et la **vitesse**, en mm/s sur le sol si le repère est défini. Quand un objet n'est plus détecté pendant un court instant (0,5 s par défaut), sa position est prédite à vitesse constante. Les trajectoires lissées sont dessinées sur l'image. La « Force du lissage » (0 à 100, 50 par défaut) règle le compromis entre la stabilité et le retard sur les mouvements rapides. Ces données sont envoyées en UDP dans la liste `tracks`. Les champs bruts existants ne changent pas.

### Envoi des positions en UDP

L'onglet « Envoi UDP » envoie les résultats à un autre programme, par exemple celui qui pilote le robot : un message JSON par image traitée, en UDP, vers l'adresse et le port choisis (`127.0.0.1:5005` par défaut). Cocher « Envoyer les positions » pour l'activer.

Chaque message contient le numéro d'image (`frame`), l'heure (`timestamp_ms`), la taille de l'image et les listes `squares` (le plus grand en premier) et `markers` (triés par identifiant), avec centre, côté, angle, coins, la position au sol (`floor`) si le repère du sol est défini et la pose 3D (`pose`) si la caméra est calibrée. Une liste est absente si la détection correspondante est désactivée. Si le lissage est actif, la liste `tracks` donne la position lissée, la vitesse et l'état (détecté ou prédit) de chaque carré principal ou marqueur suivi. Si des objets sont suivis, la liste `tracked` donne pour chacun son numéro (`id`), son nom (`name`), son cadre, sa position, sa vitesse, sa direction, son état (trouvé, prédit, en mouvement), son indice de confiance et le nombre de reprises automatiques. La liste `events` signale les débuts et fins de mouvement, les pertes et les reprises de chaque objet, ainsi que les entrées et sorties de zone (`zone_enter`, `zone_exit`), une seule fois chacun, avec `object_id` et `name`. Pendant une partie, l'objet `game` donne l'état du jeu : robot dans une zone ou non, temps passé et entrées pour « Détection robot dans zone » ; phase, chrono, fautes et score pour « 1, 2, 3 Soleil » ; indice, trésors trouvés, temps restant et position du trésor pour la chasse au trésor. La liste `zones` donne pour chaque zone son rectangle, ses occupants et, si le repère du sol est défini, ses coins sur le sol. L'état `out_of_view` de chaque objet suivi indique qu'il est sorti de l'image. Le format complet est décrit dans le [guide utilisateur](USER_GUIDE.md#103-format-des-messages).

Pour tester la réception :

```bash
python3 tools/udp_receiver.py          # résumé lisible, port 5005
python3 tools/udp_receiver.py --raw    # JSON brut
```

## Principe de la détection

### Carré coloré

Le code se trouve dans `src/squaredetector.cpp` et utilise OpenCV.

1. L'image Qt est convertie en `cv::Mat` sans copie, puis réduite à 320 px de large (`cv::resize`) pour accélérer le traitement.
2. Un léger flou gaussien est appliqué, puis l'image est convertie en HSV (`cv::cvtColor`).
3. Les pixels de la bonne couleur sont sélectionnés avec `cv::inRange` :
   - **couleur vive** : par la teinte, avec une saturation et une luminosité minimales. La teinte est circulaire, donc le rouge autour de 0°/360° est traité en deux plages. OpenCV code la teinte sur 0–179, d'où la division par 2 des degrés ;
   - **blanc, gris ou noir** : toutes teintes confondues, saturation inférieure à « Saturation max » et luminosité dans « Tolérance luminosité » autour de la cible. Une couleur est classée ainsi quand sa saturation est inférieure à 64 sur 255.
4. Une ouverture puis une fermeture morphologiques 3×3 (`cv::morphologyEx`) suppriment le bruit et bouchent les petits trous.
5. Les contours extérieurs sont extraits (`cv::findContours`), puis le **rectangle orienté de surface minimale** est calculé pour chacun (`cv::minAreaRect`).
6. Un contour est accepté comme carré si :
   - sa surface (`cv::contourArea`) dépasse « Surface min » ;
   - le rapport petit côté / grand côté est ≥ 0,75 ;
   - le taux de remplissage (surface du contour / surface du rectangle) est ≥ au seuil réglé, 82 % par défaut. Un disque ne remplit qu'environ 78 %, il est donc rejeté.

### Marqueurs ArUco

Le code se trouve dans `src/markerdetector.cpp` et utilise `cv::aruco::ArucoDetector` (module `objdetect`).

1. L'image est convertie en niveaux de gris, à pleine résolution pour détecter les petits marqueurs.
2. `detectMarkers` trouve les quadrilatères candidats, lit leur grille noir/blanc et la compare au dictionnaire choisi.
3. Les coins sont affinés au sous-pixel (`CORNER_REFINE_SUBPIX`).
4. L'angle est celui du bord supérieur du marqueur (coin 0 vers coin 1).

**Pose 3D** (si la caméra est calibrée) :

5. `cv::solvePnP` (méthode `SOLVEPNP_IPPE_SQUARE`, spécialisée pour les carrés) calcule la position et l'orientation du marqueur à partir de ses 4 coins, de sa taille réelle et de la calibration.
6. La rotation est convertie en angles d'Euler (convention Z-Y-X), dans un repère où (0, 0, 0) correspond à un marqueur face à la caméra.

Sur une image de synthèse, la position est retrouvée à environ 1 mm près à 40 cm. L'inclinaison (rotations X et Y) est moins précise, de l'ordre de 1 à 2°, car elle se déduit de la faible déformation perspective d'un seul marqueur.

### Suivi d'objet

Le code se trouve dans `src/objecttracker.cpp`. Chaque objet suivi a sa propre instance d'`ObjectTracker` (tracker, filtre, apparence de référence, événements) : les objets sont indépendants, et ils sont mis à jour en parallèle à chaque image, dans le fil de traitement.

1. **Sélection d'un clic.** `cv::grabCut` est appliqué sur une zone carrée de 40 % du petit côté de l'image, centrée sur le clic, avec une marge de fond autour (image réduite à 480 px de large). On garde la zone de premier plan qui contient le clic. Si elle est trop petite ou remplit presque toute la zone, un carré est proposé autour du clic. Sur des objets de synthèse (texturé, disque, boîte, petit objet), le cadre obtenu recouvre le vrai cadre à 92 à 100 %, en 70 à 120 ms.
2. L'image est réduite à 640 px de large au plus, ce qui garde le coût du suivi presque constant quelle que soit la résolution.
3. Le tracker (`cv::TrackerCSRT`, `cv::TrackerKCF`, `cv::TrackerNano` ou `cv::TrackerVit`) est initialisé sur le rectangle sélectionné, puis mis à jour à chaque image. S'il perd l'objet, la dernière position est conservée et il continue de chercher aux images suivantes.
4. **Indice de confiance.** À la sélection, l'apparence de l'objet est mémorisée sous deux formes : un histogramme teinte-saturation et un motif en niveaux de gris. À chaque image, le contenu du cadre est comparé à cette référence :
   - par la distance de Bhattacharyya entre histogrammes, peu sensible à la rotation ;
   - par une corrélation normalisée des motifs (`cv::matchTemplate`), qui distingue l'objet d'un fond ou d'un intrus aux couleurs proches.

   La confiance est la moyenne des deux, lissée dans le temps. Sous 0,45 pendant 3 images, l'objet est déclaré perdu (« glissé »). Au-dessus de 0,7, la référence suit lentement l'objet (5 % par image), ce qui accepte une rotation progressive mais pas un intrus qui surgit. Sur une séquence de synthèse, CSRT s'accroche à un intrus passé devant l'objet et le suit en affirmant l'avoir trouvé. La confiance tombe alors à 0,35 et aucune position fausse n'est plus donnée. Pendant une rotation de 90° de l'objet, elle reste au-dessus de 0,72.
5. **Sortie du champ.** Si le cadre trouvé est visible à moins de 50 %, ou si l'objet est perdu alors que son dernier cadre était à moins de 2 % de la largeur d'un bord, l'objet est « hors du champ » (`out_of_view`) : le tracker est arrêté, aucune position n'est prédite, et la recherche ne garde que les positions dont le cadre est à moins de 20 % d'un bord. La taille de référence de la recherche n'est mise à jour que pour un cadre entièrement dans l'image : au bord, le cadre de certains trackers (KCF) rétrécit avec la partie visible de l'objet. Le bord et la dernière position connue ne sont mis à jour que sur des images fiables (confiance au-dessus du seuil) : un cadre qui glisse du bord vers un visage voisin avec une confiance déjà basse compte comme une sortie. Un cadre qui saute de plus de 1,5 fois sa taille en une image est rejeté, et la référence d'apparence ne suit l'objet que s'il est entièrement visible et ressemble encore un peu (au moins 0,35) à l'objet d'origine. Sur 36 séquences de synthèse (3 objets, dont un couleur peau, 4 méthodes, un visage loin de la sortie, tout près du bord, ou une main qui emporte l'objet puis revient vers le visage), l'ancien comportement donnait dans 4 cas une position fausse sur le visage pendant 110 à 153 images. Désormais aucune position fausse n'est donnée, et l'objet est retrouvé à son retour dans les 36 cas (51 à 55 images sur 60).
6. **Reprise automatique.** Tant que l'objet est perdu, il est recherché dans toute l'image, une image sur trois, sur une image réduite de moitié. Les deux motifs (celui d'origine et celui qui a suivi l'objet) sont cherchés par `cv::matchTemplate` (corrélation normalisée), à trois échelles autour de la dernière taille connue (×0,85, ×1, ×1,2), en gardant les trois meilleurs emplacements de chaque recherche : une tache du décor peut corréler mieux qu'un objet tourné. Les six meilleurs candidats dont la corrélation dépasse 0,6 sont vérifiés par l'indice de confiance, et le plus ressemblant est retenu s'il dépasse 0,55. Il n'est accepté que s'il est retrouvé à deux recherches de suite au même endroit (recouvrement d'au moins 30 %). Le tracker est alors relancé à cet endroit, le filtre de Kalman repart de cette position, et la trajectoire commence un nouveau tronçon. Sur des séquences de synthèse, l'objet est retrouvé après le passage d'un intrus, après un accrochage du tracker à cet intrus, et après une réapparition ailleurs (en 3 images). Aucune fausse reprise n'a lieu quand l'objet est absent et qu'un intrus est présent. Le suivi d'un objet perdu coûte environ 15 ms par image en 1280 × 960.
7. Chaque position trouvée est ajoutée à la trajectoire, horodatée. Les points plus anciens que la durée choisie sont retirés.
8. Le centre du cadre passe dans un filtre de Kalman à vitesse constante (`KalmanFilter2D`, le même que pour le lissage des carrés et des marqueurs). On en tire la position lissée et la vitesse. Quand le tracker perd l'objet, le filtre prédit la position pendant le délai de maintien. Sans lissage, la position est celle du tracker, mais la vitesse reste filtrée (force 50).
9. Si le repère du sol est prêt, la position est projetée sur le sol, ainsi que la position atteinte 0,1 s plus tard à la vitesse estimée, pour obtenir la vitesse et la direction en mm.
10. **Événements de mouvement.** L'objet passe « en mouvement » quand il s'éloigne de sa position de repos de plus du seuil, en mm sur le sol si le repère est prêt, sinon en pixels. Il redevient « immobile » quand, sur la dernière demi-seconde, il est resté dans un rayon de la moitié du seuil. Ce critère de position résiste mieux au tremblement qu'un seuil de vitesse : à l'arrêt, la vitesse mesurée peut atteindre une douzaine de px/s. La fin de mouvement donne la distance nette, la longueur du chemin et la durée, sans la demi-seconde de confirmation. Une perte est signalée une fois la position prédite épuisée, une reprise dès que l'objet est retrouvé. Sur une séquence de synthèse avec bruit de capteur, il n'y a aucun faux départ à l'arrêt ni pour un déplacement de 8 px, et un déplacement de 300 px est mesuré à 304 px.

Sur une séquence de synthèse (objet texturé sur fond texturé, 1280 × 960, bruit de capteur simulé), la vitesse est retrouvée à 299 px/s pour 300 px/s réels. À l'arrêt, la vitesse mesurée est de 6 px/s avec le lissage par défaut, contre 30 px/s sans filtre.

### Lissage dans le temps

Le code se trouve dans `src/motionfilter.cpp`.

1. Chaque marqueur (par identifiant) et le carré principal ont leur propre **filtre de Kalman** à vitesse constante. L'état est la position et la vitesse sur chaque axe, avec une accélération modélisée par un bruit blanc. Le pas de temps est l'intervalle réel entre les images.
2. La force du lissage règle le bruit d'accélération du modèle, de 10^5,5 à 10^2 px²/s³. La variance de la mesure est fixée à 1,5 px d'écart-type.
3. Un saut plus grand que deux fois la taille de l'objet (au moins 50 px) remet le filtre à zéro, par exemple quand le plus grand carré devient un autre objet.
4. Sans détection, le filtre ne fait que prédire. Au-delà du délai de maintien, l'objet est oublié.
5. L'angle est lissé par une moyenne exponentielle d'un vecteur unitaire, ce qui évite le saut de ±180°. Pour un carré, défini à 90° près, c'est l'angle multiplié par 4 qui est lissé.
6. La vitesse sur le sol est obtenue en projetant la position lissée et la position atteinte 0,1 s plus tard à la vitesse estimée.

Une variante adaptative, qui lissait plus fort à l'arrêt, a été essayée. Elle réduisait le tremblement à l'arrêt, mais triplait le retard au démarrage. Le filtre simple a été conservé.

### Repère du sol

Le code se trouve dans `src/floormapper.cpp`.

1. Les 4 points cliqués sont associés aux coins du rectangle en mm : (0, 0), (X, 0), (X, Y), (0, Y).
2. Si la caméra est calibrée pour ce format d'image, les points sont d'abord corrigés de la distorsion (`cv::undistortPoints`).
3. `cv::getPerspectiveTransform` calcule l'homographie entre l'image et le plan du sol.
4. Le centre de chaque carré ou marqueur est projeté sur le sol (`cv::perspectiveTransform`). Le cap est la direction, sur le sol, du centre vers le milieu du bord supérieur.
5. **Correction de hauteur** (caméra calibrée). La pose de la caméra par rapport au sol est calculée à partir des 4 points (`cv::solvePnP`, méthode `SOLVEPNP_IPPE` pour un plan), ce qui donne sa position dans le repère du sol. Pour un repère à la hauteur *h*, le point de l'image est converti en rayon de vue (`cv::undistortPoints`, puis rotation dans le repère du sol). Sa position est l'intersection de ce rayon avec le plan *z = h*, du côté de la caméra. Sur une scène de synthèse (caméra inclinée à 1,1 m du sol, marqueurs à 100 mm de haut), l'erreur passe de 36 à 100 mm sans correction à 1 mm avec, et la hauteur de la caméra est retrouvée exactement.

Sur une image de synthèse (caméra inclinée filmant un sol de 1000 × 700 mm), les positions sont retrouvées à 1 à 2 mm près et les caps à moins d'1° près.

### Calibration

Le code se trouve dans `src/cameracalibration.cpp`.

1. `cv::findChessboardCorners` cherche le damier sur une image réduite à 640 px de large (option `FAST_CHECK` pour rester fluide quand aucun damier n'est visible).
2. À la capture, les coins sont affinés au sous-pixel sur l'image pleine résolution (`cv::cornerSubPix`).
3. `cv::calibrateCamera` calcule la matrice intrinsèque (fx, fy, cx, cy) et les coefficients de distorsion. Le coefficient radial k3 est bloqué à 0 (`CALIB_FIX_K3`). Laissé libre, il compense k2 dans la zone couverte par le damier, et la correction de distorsion diverge près des bords de l'image.
4. Le résultat est enregistré au format YAML d'OpenCV (`cv::FileStorage`). Si la résolution de la caméra change, la matrice est mise à l'échelle.

### Traitement dans un fil séparé

Le traitement de chaque image se fait hors du fil de l'interface (`QtConcurrent::run`) : détection des carrés et des marqueurs, repère du sol, suivi des objets, lissage, recherche du damier. Le fil principal reçoit les images, puis affiche le résultat : dessin, panneau « Résultat », journal, envoi UDP.

- **Une image à la fois.** Si une image arrive pendant un traitement, elle est mise de côté. Seule la plus récente est gardée, et elle est traitée dès que le traitement en cours se termine.
- **Parallélisme dans l'image.** La détection des marqueurs et le suivi de chaque objet tournent en parallèle, sur un groupe de fils dédié (`QThreadPool`), pendant que le fil de traitement détecte les carrés. Ce groupe est distinct de celui qui exécute le traitement, pour éviter tout blocage.
- **Sécurité des données.** Le fil de traitement n'accède à aucun widget : les réglages dont il a besoin sont lus dans l'interface au lancement du traitement. Toute action de l'interface qui modifie un objet utilisé par le traitement (détecteurs, calibration, repère du sol, objets suivis, lissage) attend d'abord la fin du traitement en cours, ce qui prend au plus une image (`waitForProcessing()`). L'affichage n'a lieu que lorsque le traitement est à l'arrêt, et le traitement suivant ne démarre qu'après lui.
- **Les clics portent sur l'image affichée**, et non sur la dernière image reçue, pour que la couleur, le cadre d'un objet ou un point du sol correspondent à ce que l'utilisateur voit.

Mesures sur des images de synthèse 1280 × 960 envoyées à 30 i/s, avec 3 objets suivis en CSRT, un marqueur et un carré, sur un processeur à 4 cœurs :

| | Avant (tout dans l'interface) | Après |
|---|---|---|
| Images traitées par seconde | 8,1 | 13,4 |
| Retard moyen d'un minuteur de 10 ms (blocage de l'interface) | 140 ms | 14 ms |

La détection des marqueurs ArUco est déjà répartie sur les cœurs par OpenCV : son coût dépend beaucoup du contenu de l'image (environ 9 ms sur une image peu texturée, beaucoup plus sur un fond très texturé). Le mode de détection rapide d'OpenCV (`useAruco3Detection`) a été essayé, mais il manque les petits marqueurs (24 à 32 px de côté dans le test, alors que le mode normal les trouve) : il n'est pas utilisé.

## Structure du projet

```
CMakeLists.txt
README.md                installation, utilisation et principes (ce fichier)
TUTORIAL.md              découverte pas à pas de toutes les fonctionnalités
USER_GUIDE.md            guide utilisateur : référence de chaque réglage, dépannage
IMPROVEMENTS.md          fonctionnalités réalisées et améliorations envisagées
GAMES.md                 idées de jeux avec un petit robot
src/
├── main.cpp             point d'entrée
├── mainwindow.h/.cpp    fenêtre, caméra (QCamera + QVideoSink), réglages, affichage des résultats, capture et vidéo
├── videowidget.h/.cpp   affichage de l'image en gardant ses proportions, clics, sélection et ajustement du cadre
├── squaredetector.h/.cpp  détection du carré coloré (OpenCV)
├── markerdetector.h/.cpp  détection, génération et pose 3D des marqueurs ArUco (OpenCV)
├── cameracalibration.h/.cpp  calibration de la caméra avec un damier (OpenCV)
├── positionsender.h/.cpp  envoi des positions en JSON sur UDP
├── floormapper.h/.cpp   conversion des positions de l'image en mm sur le sol
├── camerasettingsdialog.h/.cpp  réglages de l'image : exposition, balance des blancs, autres réglages
├── v4l2controls.h/.cpp          accès direct aux réglages du pilote Linux des webcams (V4L2)
├── objecttracker.h/.cpp  suivi d'un objet (un par objet suivi) : sélection d'un clic, confiance, reprise, vitesse, événements
├── playerwindow.h/.cpp   fenêtre du joueur : vidéo seule, plein écran sur un second écran
├── treasuregame.h/.cpp   jeu « Chasse au trésor » : trésor caché, indices, score et fenêtre du jeu
├── soleilgame.h/.cpp     jeu « 1, 2, 3 Soleil » : phases, fautes, arrivée et fenêtre du jeu
├── robotzonegame.h/.cpp  jeu « Détection robot dans zone » : règle, compteurs et fenêtre du jeu
├── zonemanager.h/.cpp    zones fixes nommées : occupation, entrées et sorties, mémorisation
├── trackermodels.h/.cpp  modèles des trackers Nano et Vit : emplacement, téléchargement vérifié
└── motionfilter.h/.cpp  lissage dans le temps (filtre de Kalman) des carrés et des marqueurs
tools/
└── udp_receiver.py      récepteur de test des positions UDP
```

## Dépannage

- **Une autre application trouve la webcam très sombre ou très claire** : l'application a été arrêtée brutalement avec une exposition manuelle (ou la luminosité, le gain... modifiés), réglages que la webcam garde en mémoire. La relancer puis la quitter normalement remet la webcam en automatique, avec ses réglages d'origine.
- **Vidéo enregistrée illisible** : l'application a été arrêtée brutalement pendant l'enregistrement, et le fichier MP4 n'a pas été terminé. Arrêter la vidéo avec le bouton « Vidéo » ou quitter avec « Quitter », qui attend la fin du fichier.
- **Le modèle Nano ou Vit ne se télécharge pas** : sans connexion, télécharger les fichiers ONNX ailleurs (adresses dans le [guide](USER_GUIDE.md#modèles-des-réseaux-de-neurones)) et les copier dans `~/.local/share/RobotDetectionByWebcam/models/` ou dans `models/` à côté de l'exécutable.
- **`Could not find a package configuration file provided by "OpenCV"`** : installer `libopencv-dev`. Pour une installation d'OpenCV compilée à la main, ajouter `-DOpenCV_DIR=<prefix>/lib/cmake/opencv4` à la commande `cmake`.
- **« Aucune webcam détectée »** : vérifier `ls /dev/video*` et que l'utilisateur appartient au groupe `video` (`groups`).
- **Messages dans la console** : par défaut, l'application masque deux types de messages sans gravité, la version de FFmpeg utilisée par Qt Multimedia (`qt.multimedia.ffmpeg`) et les « Corrupt JPEG data » (`qt.gui.imageio.jpeg`) que produisent certaines webcams en MJPEG. Pour les réafficher, mettre `hideLibraryMessages=false` dans la section `[logging]` de `~/.config/RobotDetectionByWebcam/settings.ini`. La variable d'environnement `QT_LOGGING_RULES` reste prioritaire, par exemple `QT_LOGGING_RULES="qt.gui.imageio.jpeg.warning=true" ./build/RobotDetectionByWebcam`.
- **`Failed to open VDPAU backend libvdpau_va_gl.so`** : FFmpeg cherche une accélération matérielle du décodage vidéo et ne trouve pas le pilote VDPAU des cartes Intel. Ce message est sans conséquence. Il est écrit directement par la bibliothèque système, donc l'application ne peut pas le masquer. Pour le supprimer : `sudo apt install libvdpau-va-gl1`.
- **Détection instable** : éclairer la scène de façon homogène, cliquer à nouveau sur le carré pour recaler la couleur, et utiliser « Afficher le masque » pour régler les seuils.
- **Le carré n'est pas détecté quand il est loin** : baisser « Surface min ».
- **Un marqueur ArUco n'est pas détecté** : vérifier que le bon dictionnaire est sélectionné, que la marge blanche est présente, que le marqueur est net et assez grand dans l'image (au moins 30 px de côté environ), et qu'il n'y a pas de reflet.
- **Le damier n'est pas détecté** : il doit être entièrement visible avec sa marge blanche. Vérifier que le nombre de **coins intérieurs** saisi correspond au damier (un damier de 10 × 7 cases a 9 × 6 coins intérieurs). Éviter les reflets.
- **Erreur RMS élevée** : damier gondolé, images floues ou vues trop semblables. Recommencer avec « Effacer les captures ».
- **Distances fausses d'un facteur constant** : vérifier la « Taille du marqueur » et le « Côté d'une case », mesurés à la règle sur les impressions.
- **Un jeu indique « hors zone », ou « 1, 2, 3 Soleil » ne détecte pas l'arrivée, alors que le robot est dans la zone** : le cadre de suivi du robot doit tenir entièrement dans la zone. Réencadrer le robot au plus près, ou tracer une zone un peu plus grande. Dans tous les jeux, seuls les objets suivis (onglet « Suivi ») peuvent jouer le rôle du robot.
- **« 1, 2, 3 Soleil » compte des fautes alors que le robot est arrêté** : le cadre de suivi tremble légèrement. Augmenter le seuil de mouvement (onglet « Suivi ») avant de démarrer la partie, ou choisir une difficulté plus facile (temps de réaction plus long).
- **Repartir de zéro** : supprimer le fichier de la caméra dans `~/.config/RobotDetectionByWebcam/calibrations/` (calibration) ou `settings.ini` (réglages, y compris les repères du sol et les zones).
- **Erreur de compilation sur `aruco_detector.hpp`** : la version d'OpenCV est antérieure à 4.7. Installer une version plus récente.
