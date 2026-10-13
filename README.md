# RobotDetectionByWebcam

Application Qt6 qui affiche le flux d'une webcam et détecte en temps réel :

- un **carré d'une couleur donnée** ;
- des **marqueurs ArUco** (ou AprilTag), avec leur identifiant ;
- **n'importe quel objet encadré à la souris**, suivi d'image en image avec sa trajectoire et sa vitesse.

Pour chacun, l'application donne la position, la taille et l'orientation dans l'image. Une fois la caméra **calibrée** avec un damier, elle donne aussi la **position 3D en millimètres** et l'orientation des marqueurs ArUco.

Le mode d'emploi détaillé (réglages, calibration pas à pas, lecture des résultats, dépannage) se trouve dans le [guide utilisateur](USER_GUIDE.md).

Les améliorations envisagées sont listées dans [IMPROVEMENTS.md](IMPROVEMENTS.md).

## Prérequis

- Linux avec une webcam (`/dev/video*`)
- CMake ≥ 3.16 et un compilateur C++17 (g++ ou clang)
- Qt 6 : modules **Widgets**, **Multimedia** (capture et affichage) et **Network** (envoi UDP)
- OpenCV ≥ 4.7 : modules **core**, **imgproc**, **calib3d**, **objdetect**, **video** et **tracking** (détection, calibration et suivi). ArUco fait partie d'`objdetect` depuis la 4.7. `tracking` vient des modules contrib (paquet `libopencv-contrib-dev`, installé avec `libopencv-dev`).

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

Choisir la caméra dans la liste déroulante si plusieurs webcams sont branchées, puis sa **résolution**. La liste indique pour chaque résolution son format d'image (4:3, 16:9...) et le nombre maximal d'images par seconde. La résolution choisie est mémorisée pour chaque modèle de caméra et réappliquée au lancement suivant. Au premier lancement, la caméra garde son format par défaut et la liste se cale dessus. Une résolution plus basse accélère la détection ; une résolution plus haute permet de détecter les marqueurs de plus loin.

Pour chaque résolution, l'application choisit le format vidéo le plus rapide. Certaines webcams, comme la Logitech C270, envoient toujours leur résolution maximale en MJPEG, quelle que soit la résolution demandée. L'application le détecte et passe alors automatiquement au format non compressé (YUYV) de la même résolution. Ce basculement est mémorisé avec la résolution.

Les réglages sont répartis en six onglets : **Carré coloré**, **ArUco**, **Calibration**, **Sol**, **Suivi** et **Envoi UDP**. Dans les deux premiers, la case « Activer la détection » active ou désactive la détection correspondante. Les deux détections peuvent tourner en même temps.

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

Le bouton « Quitter », en bas à droite, ferme l'application après une confirmation. La même confirmation s'affiche avec la croix de la fenêtre.

Le bouton « Réglages de l'image... » ouvre une fenêtre pour régler l'**exposition** (automatique ou temps de pose manuel) et la **balance des blancs** (automatique ou température manuelle). Une pose courte donne plus d'images par seconde et moins de flou de mouvement dans une pièce peu éclairée, au prix d'une image plus sombre. Une balance des blancs manuelle empêche les couleurs de dériver. Ces réglages sont mémorisés pour chaque modèle de caméra et réappliqués au démarrage de la caméra. À la fermeture normale, l'application remet la webcam en automatique, car ces réglages sont stockés dans la webcam elle-même. Pour l'exposition automatique, l'application écrit directement le contrôle V4L2 (`V4L2_CID_EXPOSURE_AUTO`) : sur les webcams qui n'acceptent que le mode « priorité ouverture » (comme la Logitech C270), Qt ne parvient pas à rétablir l'automatique.

Les réglages (caméra, résolution, exposition, balance des blancs, miroir, couleur et curseurs, dictionnaire et taille des marqueurs, damier, repère du sol, méthode et trajectoire du suivi, taille de la fenêtre) sont enregistrés dans `~/.config/RobotDetectionByWebcam/settings.ini` et restaurés au lancement suivant. La résolution est enregistrée dès qu'on la choisit, le reste à la fermeture de l'application. Supprimer ce fichier rétablit les réglages par défaut.

L'option « Effet miroir » retourne seulement l'image affichée. Les détections se font toujours sur l'image réelle de la caméra : les marqueurs ArUco et le damier restent reconnus, et toutes les mesures (coordonnées, angles, pose 3D) restent dans le repère réel de la caméra, que le miroir soit activé ou non.

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

La calibration est enregistrée dans `~/.config/RobotDetectionByWebcam/calibration.yml` et rechargée automatiquement au démarrage. Elle reste valable si l'on change de résolution, tant que le format d'image (4:3, 16:9...) ne change pas. Si la résolution choisie a un autre format que la calibration, la pose 3D est désactivée et un message l'indique dans le panneau « Résultat ». Il faut la refaire si l'on change de caméra ou si l'on modifie le zoom ou la mise au point.

Changer les dimensions du damier efface les captures en cours.

### Affichage des résultats

- Le carré principal (le plus grand) est entouré **en vert**, avec son centre et ses coordonnées.
- Les autres carrés détectés sont entourés **en jaune**.
- Les marqueurs ArUco sont entourés **en magenta**, avec leur identifiant. Le coin d'origine du marqueur (en haut à gauche) est marqué d'un point rouge.
  - Sans calibration, un trait part du centre vers le bord supérieur pour montrer l'orientation.
  - Avec calibration, les axes 3D du marqueur sont dessinés : **X en rouge** (droite), **Y en vert** (bas), **normale en bleu** (sort du marqueur vers la caméra).
- En mode calibration, les coins du damier détecté sont reliés en bleu clair.
- Le panneau « Résultat » indique le centre (en pixels de l'image d'origine), le côté, l'angle (0–90°), le nombre de carrés et les images par seconde. Pour chaque marqueur ArUco, il indique l'identifiant, le centre, le côté et l'angle : 0° quand le marqueur est droit, positif dans le sens horaire, entre −180° et 180°.

Avec calibration, chaque marqueur a aussi :
- **X, Y, Z** : la position de son centre en mm, dans le repère de la caméra (X vers la droite, Y vers le bas, Z droit devant la caméra) ;
- **distance** : la distance caméra–marqueur en mm ;
- **rotation X, Y, Z** : ses angles d'Euler en degrés. (0, 0, 0) correspond à un marqueur droit, face à la caméra. Z est la rotation dans le plan du marqueur ; X et Y sont ses inclinaisons.

### Positions sur le sol

L'onglet « Sol » définit un repère sur le sol : on saisit les dimensions d'un rectangle tracé au sol (ruban adhésif, coins d'une table...), puis on clique ses 4 coins dans l'image, dans l'ordre : origine, coin sur l'axe X, coin opposé, coin sur l'axe Y. L'application donne alors la position de chaque carré et de chaque marqueur **en mm sur le sol**, avec le cap des marqueurs (direction de leur bord supérieur, de l'axe X vers l'axe Y). Ces valeurs s'affichent dans le panneau « Résultat » et sont envoyées en UDP (champ `floor`).

Le repère fonctionne sans calibration. Si la caméra est calibrée, la distorsion de l'objectif est corrigée en plus. Il est enregistré dans `settings.ini` et reste valable pour toute résolution du même format d'image. La caméra ne doit plus bouger une fois le repère défini. Le repère est défini au niveau du sol, donc un marqueur placé en hauteur sur le robot est légèrement décalé. Le guide utilisateur détaille ce point.

### Suivi d'un objet encadré

Dans l'onglet « Suivi », cliquer sur « Sélectionner un objet », puis tracer un rectangle autour de l'objet dans l'image (cliquer-glisser). L'objet est ensuite suivi d'image en image par un tracker OpenCV :
- **CSRT** : précis, environ 20 ms par image ;
- **KCF** : rapide, environ 5 ms par image.

L'application affiche le cadre, la **trajectoire** (durée réglable, 10 s par défaut) et une flèche dans le sens du déplacement. Le panneau « Résultat » donne la position, l'état (en mouvement ou immobile), la vitesse et la direction, calculées sur la dernière demi-seconde. Si le repère du sol est défini, il donne aussi la position, la vitesse et la direction en mm sur le sol. Ces valeurs sont envoyées en UDP (objet `tracked`).

Le tracker peut perdre l'objet (objet caché, sorti de l'image, trop rapide) ou glisser lentement sur le fond. Il faut alors le resélectionner. Le cadre reste droit : l'orientation de l'objet n'est pas mesurée. Changer de résolution arrête le suivi.

### Envoi des positions en UDP

L'onglet « Envoi UDP » envoie les résultats à un autre programme, par exemple celui qui pilote le robot : un message JSON par image traitée, en UDP, vers l'adresse et le port choisis (`127.0.0.1:5005` par défaut). Cocher « Envoyer les positions » pour l'activer.

Chaque message contient le numéro d'image (`frame`), l'heure (`timestamp_ms`), la taille de l'image et les listes `squares` (le plus grand en premier) et `markers` (triés par identifiant), avec centre, côté, angle, coins, la position au sol (`floor`) si le repère du sol est défini et la pose 3D (`pose`) si la caméra est calibrée. Une liste est absente si la détection correspondante est désactivée. Si un objet est suivi, l'objet `tracked` donne son cadre, sa position, sa vitesse, sa direction et son état (trouvé, en mouvement). Le format complet est décrit dans le [guide utilisateur](USER_GUIDE.md#103-format-des-messages).

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

Le code se trouve dans `src/objecttracker.cpp`.

1. L'image est réduite à 640 px de large au plus, ce qui garde le coût du suivi presque constant quelle que soit la résolution.
2. Le tracker (`cv::TrackerCSRT` ou `cv::TrackerKCF`) est initialisé sur le rectangle sélectionné, puis mis à jour à chaque image. S'il perd l'objet, la dernière position est conservée et il continue de chercher aux images suivantes.
3. Chaque position trouvée est ajoutée à la trajectoire, horodatée. Les points plus anciens que la durée choisie sont retirés.
4. La vitesse est le déplacement entre la position actuelle et la plus ancienne position de la dernière demi-seconde, divisé par le temps écoulé. L'objet est « en mouvement » au-delà de 10 % de la diagonale de son cadre par seconde (au moins 5 px/s).
5. Si le repère du sol est prêt, les deux positions sont projetées sur le sol pour obtenir la vitesse et la direction en mm.

Sur une séquence de synthèse (objet texturé sur fond texturé, 1280 × 960), la vitesse est retrouvée à 299 px/s pour 300 px/s réels, la position à 3 px près, et l'état immobile ou en mouvement est bien distingué.

### Repère du sol

Le code se trouve dans `src/floormapper.cpp`.

1. Les 4 points cliqués sont associés aux coins du rectangle en mm : (0, 0), (X, 0), (X, Y), (0, Y).
2. Si la caméra est calibrée pour ce format d'image, les points sont d'abord corrigés de la distorsion (`cv::undistortPoints`).
3. `cv::getPerspectiveTransform` calcule l'homographie entre l'image et le plan du sol.
4. Le centre de chaque carré ou marqueur est projeté sur le sol (`cv::perspectiveTransform`). Le cap est la direction, sur le sol, du centre vers le milieu du bord supérieur.

Sur une image de synthèse (caméra inclinée filmant un sol de 1000 × 700 mm), les positions sont retrouvées à 1 à 2 mm près et les caps à moins d'1° près.

### Calibration

Le code se trouve dans `src/cameracalibration.cpp`.

1. `cv::findChessboardCorners` cherche le damier sur une image réduite à 640 px de large (option `FAST_CHECK` pour rester fluide quand aucun damier n'est visible).
2. À la capture, les coins sont affinés au sous-pixel sur l'image pleine résolution (`cv::cornerSubPix`).
3. `cv::calibrateCamera` calcule la matrice intrinsèque (fx, fy, cx, cy) et les coefficients de distorsion. Le coefficient radial k3 est bloqué à 0 (`CALIB_FIX_K3`). Laissé libre, il compense k2 dans la zone couverte par le damier, et la correction de distorsion diverge près des bords de l'image.
4. Le résultat est enregistré au format YAML d'OpenCV (`cv::FileStorage`). Si la résolution de la caméra change, la matrice est mise à l'échelle.

## Structure du projet

```
CMakeLists.txt
src/
├── main.cpp             point d'entrée
├── mainwindow.h/.cpp    fenêtre, caméra (QCamera + QVideoSink), réglages, affichage des résultats
├── videowidget.h/.cpp   affichage de l'image en gardant ses proportions, clic pour choisir la couleur
├── squaredetector.h/.cpp  détection du carré coloré (OpenCV)
├── markerdetector.h/.cpp  détection, génération et pose 3D des marqueurs ArUco (OpenCV)
├── cameracalibration.h/.cpp  calibration de la caméra avec un damier (OpenCV)
├── positionsender.h/.cpp  envoi des positions en JSON sur UDP
├── floormapper.h/.cpp   conversion des positions de l'image en mm sur le sol
├── camerasettingsdialog.h/.cpp  réglages de l'image : exposition et balance des blancs
└── objecttracker.h/.cpp  suivi d'un objet encadré, trajectoire et vitesse
tools/
└── udp_receiver.py      récepteur de test des positions UDP
```

## Dépannage

- **Une autre application trouve la webcam très sombre ou très claire** : l'application a été arrêtée brutalement avec une exposition manuelle, réglage que la webcam garde en mémoire. La relancer puis la quitter normalement remet la webcam en automatique.

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
- **Repartir de zéro** : supprimer `~/.config/RobotDetectionByWebcam/calibration.yml`.
- **Erreur de compilation sur `aruco_detector.hpp`** : la version d'OpenCV est antérieure à 4.7. Installer une version plus récente.
