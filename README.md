# RobotDetectionByWebcam

Application Qt6 qui affiche le flux d'une webcam et détecte en temps réel :

- un **carré d'une couleur donnée** ;
- des **marqueurs ArUco** (ou AprilTag), avec leur identifiant.

Pour chacun, l'application donne la position, la taille et l'orientation dans l'image. Une fois la caméra **calibrée** avec un damier, elle donne aussi la **position 3D en millimètres** et l'orientation des marqueurs ArUco.

## Prérequis

- Linux avec une webcam (`/dev/video*`)
- CMake ≥ 3.16 et un compilateur C++17 (g++ ou clang)
- Qt 6 : modules **Widgets** et **Multimedia** (capture et affichage)
- OpenCV ≥ 4.7 : modules **core**, **imgproc**, **calib3d** et **objdetect** (détection et calibration ; ArUco fait partie d'`objdetect` depuis la 4.7)

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

Choisir la caméra dans la liste déroulante si plusieurs webcams sont branchées.

Les réglages sont répartis en trois onglets : **Carré coloré**, **ArUco** et **Calibration**. Dans les deux premiers, la case « Activer la détection » active ou désactive la détection correspondante. Les deux détections peuvent tourner en même temps.

### Carré coloré

1. **Choisir la couleur du carré** :
   - le plus simple : **cliquer sur le carré dans l'image**. La couleur est prise sur une zone de 7×7 pixels et les seuils de saturation et de luminosité sont ajustés automatiquement ;
   - ou cliquer sur le bouton « Couleur » pour ouvrir un sélecteur.
2. **Ajuster les réglages** si besoin, en cochant « Afficher le masque » pour voir les pixels retenus (en blanc).

| Réglage | Rôle |
|---|---|
| Tolérance teinte | Écart de teinte accepté autour de la couleur cible (en degrés) |
| Saturation min | Écarte les zones grises ou délavées |
| Luminosité min | Écarte les zones trop sombres |
| Surface min | Taille minimale d'un carré (en pixels de l'image de travail, 320 px de large) |
| Remplissage min (%) | Part du rectangle orienté que la zone doit remplir ; à augmenter pour mieux écarter les cercles |

L'option « Effet miroir » retourne l'image horizontalement.

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

La calibration est enregistrée dans `~/.config/RobotDetectionByWebcam/calibration.yml` et rechargée automatiquement au démarrage. Elle reste valable si l'on change de résolution, tant que le format d'image (4:3, 16:9...) ne change pas. Il faut la refaire si l'on change de caméra ou si l'on modifie le zoom ou la mise au point.

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

## Principe de la détection

### Carré coloré

Le code se trouve dans `src/squaredetector.cpp` et utilise OpenCV.

1. L'image Qt est convertie en `cv::Mat` sans copie, puis réduite à 320 px de large (`cv::resize`) pour accélérer le traitement.
2. Un léger flou gaussien est appliqué, puis l'image est convertie en HSV (`cv::cvtColor`).
3. Les pixels de la bonne couleur sont sélectionnés avec `cv::inRange`. La teinte est circulaire, donc le rouge autour de 0°/360° est traité en deux plages. OpenCV code la teinte sur 0–179, d'où la division par 2 des degrés.
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

### Calibration

Le code se trouve dans `src/cameracalibration.cpp`.

1. `cv::findChessboardCorners` cherche le damier sur une image réduite à 640 px de large (option `FAST_CHECK` pour rester fluide quand aucun damier n'est visible).
2. À la capture, les coins sont affinés au sous-pixel sur l'image pleine résolution (`cv::cornerSubPix`).
3. `cv::calibrateCamera` calcule la matrice intrinsèque (fx, fy, cx, cy) et les coefficients de distorsion.
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
└── cameracalibration.h/.cpp  calibration de la caméra avec un damier (OpenCV)
```

## Dépannage

- **`Could not find a package configuration file provided by "OpenCV"`** : installer `libopencv-dev`. Pour une installation d'OpenCV compilée à la main, ajouter `-DOpenCV_DIR=<prefix>/lib/cmake/opencv4` à la commande `cmake`.
- **« Aucune webcam détectée »** : vérifier `ls /dev/video*` et que l'utilisateur appartient au groupe `video` (`groups`).
- **Messages « Corrupt JPEG data » dans la console** : ils viennent du flux MJPEG de certaines webcams. Ils sont sans conséquence.
- **Détection instable** : éclairer la scène de façon homogène, cliquer à nouveau sur le carré pour recaler la couleur, et utiliser « Afficher le masque » pour régler les seuils.
- **Le carré n'est pas détecté quand il est loin** : baisser « Surface min ».
- **Un marqueur ArUco n'est pas détecté** : vérifier que le bon dictionnaire est sélectionné, que la marge blanche est présente, que le marqueur est net et assez grand dans l'image (au moins 30 px de côté environ), et qu'il n'y a pas de reflet.
- **Le damier n'est pas détecté** : il doit être entièrement visible avec sa marge blanche. Vérifier que le nombre de **coins intérieurs** saisi correspond au damier (un damier de 10 × 7 cases a 9 × 6 coins intérieurs). Éviter les reflets.
- **Erreur RMS élevée** : damier gondolé, images floues ou vues trop semblables. Recommencer avec « Effacer les captures ».
- **Distances fausses d'un facteur constant** : vérifier la « Taille du marqueur » et le « Côté d'une case », mesurés à la règle sur les impressions.
- **Repartir de zéro** : supprimer `~/.config/RobotDetectionByWebcam/calibration.yml`.
- **Erreur de compilation sur `aruco_detector.hpp`** : la version d'OpenCV est antérieure à 4.7. Installer une version plus récente.
