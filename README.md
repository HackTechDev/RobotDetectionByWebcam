# RobotDetectionByWebcam

Application Qt6 qui affiche le flux d'une webcam et détecte en temps réel :

- un **carré d'une couleur donnée** ;
- des **marqueurs ArUco** (ou AprilTag), avec leur identifiant ;
- **un ou plusieurs objets quelconques**, désignés à la souris (en les encadrant ou d'un clic), suivis d'image en image avec leur trajectoire, leur vitesse et leurs débuts et fins de mouvement.

Pour chacun, l'application donne la position dans l'image, ainsi que la taille et l'orientation des carrés et des marqueurs. Elle peut aussi :
- donner les positions **en millimètres sur le sol**, après avoir cliqué les 4 coins d'un rectangle connu ;
- donner la **position 3D** et l'orientation des marqueurs ArUco, une fois la caméra **calibrée** avec un damier ;
- **lisser** les positions et calculer les **vitesses** (filtre de Kalman) ;
- **envoyer** toutes les mesures en JSON sur UDP à un autre programme, par exemple celui qui pilote le robot.

Pour découvrir l'application pas à pas, suivez le [tutoriel](TUTORIAL.md). Le mode d'emploi détaillé (réglages, calibration pas à pas, lecture des résultats, dépannage) se trouve dans le [guide utilisateur](USER_GUIDE.md).

La liste complète des fonctionnalités et des améliorations envisagées se trouve dans [IMPROVEMENTS.md](IMPROVEMENTS.md).

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

Choisir la caméra dans la liste déroulante si plusieurs webcams sont branchées, puis sa **résolution**. Chaque caméra garde ses propres réglages : résolution, réglages de l'image, **calibration** et **repère du sol**. On peut donc passer de l'une à l'autre sans rien refaire. Changer de caméra arrête le suivi des objets encadrés. Deux caméras du même modèle sont distinguées par leur identifiant système. La liste indique pour chaque résolution son format d'image (4:3, 16:9...) et le nombre maximal d'images par seconde. La résolution choisie est mémorisée pour chaque modèle de caméra et réappliquée au lancement suivant. Au premier lancement, la caméra garde son format par défaut et la liste se cale dessus. Une résolution plus basse accélère la détection ; une résolution plus haute permet de détecter les marqueurs de plus loin.

Pour chaque résolution, l'application choisit le format vidéo le plus rapide. Certaines webcams, comme la Logitech C270, envoient toujours leur résolution maximale en MJPEG, quelle que soit la résolution demandée. L'application le détecte et passe alors automatiquement au format non compressé (YUYV) de la même résolution. Ce basculement est mémorisé avec la résolution.

Les réglages sont répartis en six onglets : **Carré coloré**, **ArUco**, **Calibration**, **Sol**, **Suivi** et **Envoi UDP**. Dans les deux premiers, la case « Activer la détection » active ou désactive la détection correspondante. Les deux détections peuvent tourner en même temps.

### Caméra, réglages et fermeture

Le bouton « Réglages de l'image... » ouvre une fenêtre pour régler l'**exposition** (automatique ou temps de pose manuel) et la **balance des blancs** (automatique ou température manuelle), ainsi que les **autres réglages** fournis par la webcam : luminosité, contraste, saturation, gain, netteté, fréquence du secteur (anti-scintillement), ralentissement des images en exposition automatique, voyant... La liste et les limites (dont celles du temps de pose) sont lues auprès du pilote V4L2 (`VIDIOC_QUERYCTRL`). Une pose courte donne plus d'images par seconde et moins de flou de mouvement dans une pièce peu éclairée, au prix d'une image plus sombre. Une balance des blancs manuelle empêche les couleurs de dériver. Ces réglages sont mémorisés pour chaque modèle de caméra et réappliqués au démarrage de la caméra. À la fermeture normale (et au changement de caméra), l'application remet la webcam en automatique et rend aux autres réglages leur valeur d'avant le lancement, car ces réglages sont stockés dans la webcam elle-même. Pour l'exposition automatique, l'application écrit directement le contrôle V4L2 (`V4L2_CID_EXPOSURE_AUTO`) : sur les webcams qui n'acceptent que le mode « priorité ouverture » (comme la Logitech C270), Qt ne parvient pas à rétablir l'automatique.

L'option « Effet miroir » retourne seulement l'image affichée. Les détections se font toujours sur l'image réelle de la caméra : les marqueurs ArUco et le damier restent reconnus, et toutes les mesures (coordonnées, angles, pose 3D, positions au sol) restent dans le repère réel de la caméra, que le miroir soit activé ou non. Les clics sur l'image (couleur, sélection d'un objet, points du sol) sont convertis automatiquement.

Les réglages (caméra, résolution, réglages de l'image, miroir, couleur et curseurs, dictionnaire et taille des marqueurs, damier, repère du sol, méthode, trajectoire et seuils du suivi, lissage, envoi UDP, taille de la fenêtre) sont enregistrés dans `~/.config/RobotDetectionByWebcam/settings.ini` et restaurés au lancement suivant. Les réglages propres à chaque caméra sont regroupés dans la section `[cameras]` sous un nom lisible : le nom de la caméra réduit aux lettres et aux chiffres, suivi de l'identifiant USB s'il y figure (« UVC Camera (046d:0825) » devient `UVC_Camera_046d_0825`, au lieu du nom encodé par Qt `UVC%20Camera%20%28046d%3A0825%29` des versions précédentes, renommé automatiquement au premier lancement). La résolution, les réglages de l'image et le repère du sol sont enregistrés dès qu'on les modifie, le reste à la fermeture de l'application. Supprimer ce fichier rétablit les réglages par défaut.

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

### Suivi d'objets encadrés

Dans l'onglet « Suivi », cliquer sur « Ajouter des objets », puis tracer un rectangle autour de chaque objet, ou cliquer dessus : un simple clic délimite l'objet automatiquement (`cv::grabCut` sur une zone autour du clic). Le cadre reste ajustable, avec ses poignées ou en le faisant glisser, jusqu'à sa validation : bouton « Valider l'objet », touche Entrée ou double-clic. On reste en mode ajout pour l'objet suivant ; Échap ou « Terminer l'ajout » termine. Chaque objet a un numéro, un nom modifiable (double-clic dans la liste des objets) et une couleur. « Retirer » arrête le suivi de l'objet sélectionné dans la liste, « Tout retirer » celui de tous les objets. Chaque objet est suivi d'image en image par son propre tracker OpenCV :
- **CSRT** : précis, environ 25 ms de calcul par image et par objet (avec 3 objets, environ 13 images traitées par seconde en 1280 × 960 sur 4 cœurs) ;
- **KCF** : rapide, environ 5 ms par image et par objet.

L'application affiche, dans la couleur de chaque objet, le cadre, la **trajectoire** (durée réglable, 10 s par défaut) et une flèche dans le sens du déplacement. Le panneau « Résultat » donne la position, l'état (en mouvement ou immobile), la vitesse et la direction, lissées par un filtre de Kalman (mêmes réglages que le lissage des carrés et des marqueurs). Juste après une perte, la position est prédite à vitesse constante pendant le délai de maintien. Si le repère du sol est défini, il donne aussi la position, la vitesse et la direction en mm sur le sol. Ces valeurs sont envoyées en UDP (liste `tracked`, une entrée par objet).

Un **indice de confiance** compare à chaque image le contenu du cadre à l'apparence de l'objet. S'il chute, l'objet est déclaré perdu, même si le tracker croit encore le suivre : cela arrive quand le cadre glisse sur le fond ou s'accroche à un autre objet passé devant. Le tracker peut aussi perdre l'objet (objet caché, sorti de l'image, trop rapide). Dans les deux cas, l'application **recherche l'objet dans toute l'image** et reprend le suivi dès qu'il réapparaît, même ailleurs. Un objet revenu très tourné ou de taille très différente peut ne pas être reconnu : il faut alors le resélectionner. Le cadre reste droit : l'orientation de l'objet n'est pas mesurée. Changer de résolution arrête le suivi.

L'état « en mouvement » repose sur un **seuil de distance** réglable : 20 mm sur le sol, ou 15 px sans repère du sol. Les **débuts et fins de mouvement**, les pertes et les reprises de chaque objet sont inscrits dans un **journal des événements** horodaté, avec le nom de l'objet, enregistrable en CSV, et envoyés en UDP (liste `events`).

### Lissage dans le temps

Le groupe « Lissage des carrés et marqueurs » de l'onglet « Suivi » suit dans le temps le carré coloré principal et chaque marqueur ArUco, reconnu par son identifiant. Il calcule une position lissée et la **vitesse**, en mm/s sur le sol si le repère est défini. Quand un objet n'est plus détecté pendant un court instant (0,5 s par défaut), sa position est prédite à vitesse constante. Les trajectoires lissées sont dessinées sur l'image. La « Force du lissage » (0 à 100, 50 par défaut) règle le compromis entre la stabilité et le retard sur les mouvements rapides. Ces données sont envoyées en UDP dans la liste `tracks`. Les champs bruts existants ne changent pas.

### Envoi des positions en UDP

L'onglet « Envoi UDP » envoie les résultats à un autre programme, par exemple celui qui pilote le robot : un message JSON par image traitée, en UDP, vers l'adresse et le port choisis (`127.0.0.1:5005` par défaut). Cocher « Envoyer les positions » pour l'activer.

Chaque message contient le numéro d'image (`frame`), l'heure (`timestamp_ms`), la taille de l'image et les listes `squares` (le plus grand en premier) et `markers` (triés par identifiant), avec centre, côté, angle, coins, la position au sol (`floor`) si le repère du sol est défini et la pose 3D (`pose`) si la caméra est calibrée. Une liste est absente si la détection correspondante est désactivée. Si le lissage est actif, la liste `tracks` donne la position lissée, la vitesse et l'état (détecté ou prédit) de chaque carré principal ou marqueur suivi. Si des objets sont suivis, la liste `tracked` donne pour chacun son numéro (`id`), son nom (`name`), son cadre, sa position, sa vitesse, sa direction, son état (trouvé, prédit, en mouvement), son indice de confiance et le nombre de reprises automatiques. La liste `events` signale les débuts et fins de mouvement, les pertes et les reprises de chaque objet, une seule fois chacun, avec `object_id` et `name`. Le format complet est décrit dans le [guide utilisateur](USER_GUIDE.md#103-format-des-messages).

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

Le code se trouve dans `src/objecttracker.cpp`. Chaque objet suivi a sa propre instance d'`ObjectTracker` (tracker, filtre, apparence de référence, événements) : les objets sont indépendants, et la fenêtre les met à jour l'un après l'autre à chaque image.

1. **Sélection d'un clic.** `cv::grabCut` est appliqué sur une zone carrée de 40 % du petit côté de l'image, centrée sur le clic, avec une marge de fond autour (image réduite à 480 px de large). On garde la zone de premier plan qui contient le clic. Si elle est trop petite ou remplit presque toute la zone, un carré est proposé autour du clic. Sur des objets de synthèse (texturé, disque, boîte, petit objet), le cadre obtenu recouvre le vrai cadre à 92 à 100 %, en 70 à 120 ms.
2. L'image est réduite à 640 px de large au plus, ce qui garde le coût du suivi presque constant quelle que soit la résolution.
3. Le tracker (`cv::TrackerCSRT` ou `cv::TrackerKCF`) est initialisé sur le rectangle sélectionné, puis mis à jour à chaque image. S'il perd l'objet, la dernière position est conservée et il continue de chercher aux images suivantes.
4. **Indice de confiance.** À la sélection, l'apparence de l'objet est mémorisée sous deux formes : un histogramme teinte-saturation et un motif en niveaux de gris. À chaque image, le contenu du cadre est comparé à cette référence :
   - par la distance de Bhattacharyya entre histogrammes, peu sensible à la rotation ;
   - par une corrélation normalisée des motifs (`cv::matchTemplate`), qui distingue l'objet d'un fond ou d'un intrus aux couleurs proches.

   La confiance est la moyenne des deux, lissée dans le temps. Sous 0,45 pendant 3 images, l'objet est déclaré perdu (« glissé »). Au-dessus de 0,7, la référence suit lentement l'objet (5 % par image), ce qui accepte une rotation progressive mais pas un intrus qui surgit. Sur une séquence de synthèse, CSRT s'accroche à un intrus passé devant l'objet et le suit en affirmant l'avoir trouvé. La confiance tombe alors à 0,35 et aucune position fausse n'est plus donnée. Pendant une rotation de 90° de l'objet, elle reste au-dessus de 0,72.
5. **Reprise automatique.** Tant que l'objet est perdu, il est recherché dans toute l'image, une image sur trois, sur une image réduite de moitié. Les deux motifs (celui d'origine et celui qui a suivi l'objet) sont cherchés par `cv::matchTemplate` (corrélation normalisée), à trois échelles autour de la dernière taille connue (×0,85, ×1, ×1,2). Le meilleur candidat n'est accepté que si sa corrélation dépasse 0,6 et son indice de confiance 0,55. Le tracker est alors relancé à cet endroit, le filtre de Kalman repart de cette position, et la trajectoire commence un nouveau tronçon. Sur des séquences de synthèse, l'objet est retrouvé après le passage d'un intrus, après un accrochage du tracker à cet intrus, et après une réapparition ailleurs (en 3 images). Aucune fausse reprise n'a lieu quand l'objet est absent et qu'un intrus est présent. Le suivi d'un objet perdu coûte environ 15 ms par image en 1280 × 960.
6. Chaque position trouvée est ajoutée à la trajectoire, horodatée. Les points plus anciens que la durée choisie sont retirés.
7. Le centre du cadre passe dans un filtre de Kalman à vitesse constante (`KalmanFilter2D`, le même que pour le lissage des carrés et des marqueurs). On en tire la position lissée et la vitesse. Quand le tracker perd l'objet, le filtre prédit la position pendant le délai de maintien. Sans lissage, la position est celle du tracker, mais la vitesse reste filtrée (force 50).
8. Si le repère du sol est prêt, la position est projetée sur le sol, ainsi que la position atteinte 0,1 s plus tard à la vitesse estimée, pour obtenir la vitesse et la direction en mm.
9. **Événements de mouvement.** L'objet passe « en mouvement » quand il s'éloigne de sa position de repos de plus du seuil, en mm sur le sol si le repère est prêt, sinon en pixels. Il redevient « immobile » quand, sur la dernière demi-seconde, il est resté dans un rayon de la moitié du seuil. Ce critère de position résiste mieux au tremblement qu'un seuil de vitesse : à l'arrêt, la vitesse mesurée peut atteindre une douzaine de px/s. La fin de mouvement donne la distance nette, la longueur du chemin et la durée, sans la demi-seconde de confirmation. Une perte est signalée une fois la position prédite épuisée, une reprise dès que l'objet est retrouvé. Sur une séquence de synthèse avec bruit de capteur, il n'y a aucun faux départ à l'arrêt ni pour un déplacement de 8 px, et un déplacement de 300 px est mesuré à 304 px.

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
src/
├── main.cpp             point d'entrée
├── mainwindow.h/.cpp    fenêtre, caméra (QCamera + QVideoSink), réglages, affichage des résultats
├── videowidget.h/.cpp   affichage de l'image en gardant ses proportions, clics, sélection et ajustement du cadre
├── squaredetector.h/.cpp  détection du carré coloré (OpenCV)
├── markerdetector.h/.cpp  détection, génération et pose 3D des marqueurs ArUco (OpenCV)
├── cameracalibration.h/.cpp  calibration de la caméra avec un damier (OpenCV)
├── positionsender.h/.cpp  envoi des positions en JSON sur UDP
├── floormapper.h/.cpp   conversion des positions de l'image en mm sur le sol
├── camerasettingsdialog.h/.cpp  réglages de l'image : exposition, balance des blancs, autres réglages
├── v4l2controls.h/.cpp          accès direct aux réglages du pilote Linux des webcams (V4L2)
├── objecttracker.h/.cpp  suivi d'un objet (un par objet suivi) : sélection d'un clic, confiance, reprise, vitesse, événements
└── motionfilter.h/.cpp  lissage dans le temps (filtre de Kalman) des carrés et des marqueurs
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
- **Repartir de zéro** : supprimer le fichier de la caméra dans `~/.config/RobotDetectionByWebcam/calibrations/` (calibration) ou `settings.ini` (réglages, y compris les repères du sol).
- **Erreur de compilation sur `aruco_detector.hpp`** : la version d'OpenCV est antérieure à 4.7. Installer une version plus récente.
