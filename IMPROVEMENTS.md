# Fonctionnalités et améliorations

Ce fichier recense d'abord les **fonctionnalités réalisées** de RobotDetectionByWebcam, puis les **améliorations envisagées**, classées par thème.

## Fonctionnalités réalisées

Le mode d'emploi détaillé de chaque fonctionnalité se trouve dans le [guide utilisateur](USER_GUIDE.md).

### Caméra et affichage

- Affichage en direct de la webcam (Qt6 Multimedia), avec choix de la caméra. Chaque caméra garde ses propres réglages, sa **calibration** et son **repère du sol**, pour passer de l'une à l'autre sans rien refaire.
- Choix de la **résolution**, avec le format d'image (4:3, 16:9...) et le nombre d'images par seconde. Le format vidéo le plus rapide est choisi automatiquement. Si une webcam n'envoie pas la résolution demandée en MJPEG (cas de la Logitech C270), l'application passe d'elle-même au format non compressé.
- **Réglages de l'image** : exposition automatique ou temps de pose manuel entre les vraies limites de la caméra (valeur appliquée affichée), balance des blancs automatique ou température manuelle, et tous les autres réglages fournis par le pilote V4L2 (luminosité, contraste, gain, fréquence du secteur, voyant...), avec des noms français. La webcam retrouve son état d'origine à la fermeture.
- **Capture d'image** en un clic (F12) : image de la webcam en pleine résolution, ou image avec les détections (Maj+F12), enregistrée en PNG horodaté dans un dossier choisi.
- **Enregistrement vidéo** en MP4 (F9) : flux de la webcam en pleine résolution, ou images affichées avec les détections (Maj+F9).
- **Fenêtre du joueur** : vidéo seule, sur fond noir, en plein écran sur un second écran, avec ou sans les détections, retournable en miroir.
- **Effet miroir**, appliqué seulement à l'affichage : les détections et les mesures restent celles de l'image réelle de la caméra.

### Détection

- **Carré coloré** :
  - couleur choisie d'un clic sur l'image ou dans un sélecteur ;
  - type « couleur vive » (par la teinte) ou « blanc, gris ou noir » (par la luminosité), choisi automatiquement ;
  - seuils réglables et affichage du masque ;
  - forme vérifiée par le rectangle orienté minimal, ce qui écarte les disques et les rectangles.
- **Marqueurs ArUco et AprilTag** : six dictionnaires, coins affinés au sous-pixel, génération de marqueurs à imprimer.
- **Suivi d'un ou plusieurs objets quelconques** désignés à la souris (trackers OpenCV CSRT, KCF, ou réseaux de neurones Nano et Vit dont le modèle est téléchargé au premier choix, avec vérification de l'empreinte), chacun avec son nom et sa couleur :
  - ajout par un rectangle ou d'un clic (délimitation automatique), cadre ajustable avant validation, ajout de plusieurs objets à la suite ; liste des objets (renommer, retirer) ;
  - trajectoire, position lissée, vitesse et direction (filtre de Kalman), position prédite juste après une perte ;
  - indice de confiance, qui détecte le cadre qui glisse sur le fond ou s'accroche à un autre objet ;
  - reprise automatique : un objet perdu est recherché dans toute l'image et le suivi reprend dès qu'il réapparaît ;
  - état « immobile » ou « en mouvement » selon un seuil de distance réglable, et événements (début et fin de mouvement, perte, reprise) dans un journal horodaté exportable en CSV.

### Mesures

- Position, taille et angle dans l'image, pour chaque carré et chaque marqueur.
- **Calibration de la caméra** avec un damier :
  - génération du damier à imprimer ;
  - captures manuelles ou automatiques ;
  - erreur RMS affichée ;
  - calibration enregistrée et rechargée au lancement ;
  - adaptation aux résolutions du même format d'image.
- **Pose 3D des marqueurs** (si la caméra est calibrée) : position en mm, distance et rotations par rapport à la caméra. Les axes du marqueur sont dessinés sur l'image.
- **Positions sur le sol en millimètres** : repère défini par les 4 coins d'un rectangle de dimensions connues, cliqués dans l'image. Avec la calibration, correction de la hauteur des repères (marqueurs posés sur le robot) et hauteur de la caméra au-dessus du sol affichée. Il donne la position et le cap des carrés et des marqueurs, ainsi que la position, la vitesse et la direction de déplacement des objets suivis. Il fonctionne sans calibration. Si la caméra est calibrée, la distorsion de l'objectif est corrigée en plus.
- **Lissage dans le temps** (filtre de Kalman) du carré principal et de chaque marqueur : position lissée, vitesse, position prédite pendant une courte perte de détection, trajectoires.
- **Zones nommées** : rectangles fixes tracés sur l'image, avec l'occupation de chacun (objets suivis, marqueurs, carré) et les entrées et sorties dans le journal et en UDP.
- **Menu Jeu**, « Détection robot dans zone » : le robot (objet suivi) est signalé quand son cadre est entièrement dans une zone, avec bandeau sur l'image, temps passé dans la zone, nombre d'entrées, journal et UDP.
- **Menu Jeu**, « 1, 2, 3 Soleil » : phases « Avancez ! » et « Soleil ! » aux durées aléatoires (trois difficultés), faute si le robot bouge pendant « Soleil ! » après le temps de réaction (+5 s), arrivée dans une zone, chrono et score.
- **Menu Jeu**, « Chasse au trésor » : trésor invisible caché au hasard sur un terrain, indices « chaud / froid » avec tendance, étoile à chaque trésor trouvé, durée de 1 à 5 minutes, trois difficultés.
- **Menu Jeu**, « Course contre la montre » : portes à franchir dans l'ordre sur 1 à 3 tours, chrono au premier mouvement, pénalité pour une mauvaise porte, record par parcours ; premier jeu du cadre commun (`gameframework`), qui construit la fenêtre de chaque jeu à partir de ses réglages.
- **Menu Jeu**, « Le parking » : garer le robot (marqueur ArUco) dans une case avec le bon cap, jugé à l'arrêt ; 5 niveaux où la case rétrécit et la tolérance de cap se resserre ; points selon la précision.
- **Menu Jeu**, « Le fil électrique » : couloir de zones à suivre sans en sortir, touche (+3 s) quand le cadre du robot dépasse, variante « main tremblante » avec vitesse maximale (+2 s par excès).
- **Menu Jeu**, « Curling » : 1 à 4 joueurs lancent le robot vers une cible à anneaux (10, 5, 2 points), jugée à l'arrêt ; manches et classement.
- **Menu Jeu**, « Chat et souris » : deux objets suivis, point au chat quand les cadres se touchent, refuge facultatif, rôles inversés à intervalle régulier.
- **Menu Jeu**, « Football de table » : balle suivie poussée vers deux buts (zones), but pour l'équipe adverse, touche quand la balle sort de l'image, reprise après replacement ; match de 2 à 10 minutes.
- **Menu Jeu**, « Ramassage » : pousser les objets suivis dans une zone de collecte avant la fin du temps, compte sur la position des objets.
- **Menu Jeu**, « Jacques a dit » : ordres à exécuter seulement après « Jacques a dit » (marqueur ArUco pour le cap), jugés sur la position et le cap au début et à la fin du temps imparti ; ordres pièges.
- **Menu Jeu**, « Dessin au sol » : reproduire une forme avec la trajectoire du robot, pourcentage de ressemblance (précision et couverture), meilleur dessin par forme.
- **Menu Jeu**, « Champ de mines » : mines invisibles tirées au hasard entre le départ et l'arrivée, explosions mémorisées, détecteur facultatif, mines et trajectoires dévoilées à la fin.

### Intégration

- **Envoi des positions en JSON sur UDP**, un message par image : carrés, marqueurs, pose 3D, positions au sol, objets suivis (`tracked`), objets lissés (`tracks`) et événements de mouvement (`events`).
- Script de réception `tools/udp_receiver.py`, pour tester l'envoi ou servir de base à un programme de pilotage.

### Confort

- **Traitement des images dans un fil séparé**, avec la détection des marqueurs et le suivi de chaque objet en parallèle : l'interface reste réactive, et davantage d'images sont traitées (13,4 i/s au lieu de 8,1 avec 3 objets, sur images de synthèse).
- Tous les réglages sont **mémorisés** entre deux lancements (`settings.ini`) : la résolution et les réglages de l'image pour chaque modèle de caméra, le repère du sol, etc. Les caméras y portent un nom lisible (`UVC_Camera_046d_0825`), pour modifier le fichier à la main ; les anciens noms encodés sont convertis automatiquement.
- Bouton **Quitter**, avec une confirmation.
- Option pour masquer les messages techniques sans gravité dans le terminal.
- Documentation : [README](README.md) (installation et principes), [tutoriel](TUTORIAL.md) (découverte pas à pas) et [guide utilisateur](USER_GUIDE.md) (référence).

---

## Améliorations envisagées

Chaque entrée indique l'intérêt, une piste de réalisation et un ordre de grandeur de l'effort :

- **S** : quelques heures ;
- **M** : une à deux journées ;
- **L** : plusieurs jours.

Ces efforts sont des estimations.

### Priorité haute

#### Tests automatiques — M

**Intérêt.** La détection et les calculs ont été vérifiés pendant le développement par des programmes de test sur images de synthèse, restés hors du projet. Les intégrer éviterait les régressions.

**Piste.** Un dossier `tests/` exécuté avec CTest (`ctest --test-dir build`), qui reprend ces vérifications :
- carré coloré, y compris blanc, gris et noir, et rejet des disques et des rectangles ;
- marqueurs ArUco dans plusieurs dictionnaires ;
- calibration sur caméra simulée (fx et fy retrouvés à 600 px) ;
- pose 3D ;
- repère du sol (positions à 1 à 2 mm près) ;
- suivi d'objet : vitesse, prédiction, indice de confiance (intrus, rotation), reprise automatique, sélection d'un clic, événements de mouvement ;
- lissage de Kalman (tremblement, retard, prédiction) ;
- format des messages JSON.

Les images de synthèse sont générées par les tests eux-mêmes, sans webcam.

---

### Détection

#### Cap complet avec des couleurs — M

**Intérêt.** Le carré coloré ne donne son angle qu'à 90° près. Une pastille d'une seconde couleur à l'avant du robot donnerait le cap sur 360°, sans marqueur ArUco.

**Piste.** Détecter une seconde couleur près du carré principal. Le cap est la direction du centre du carré vers le centre de la pastille.

#### Plusieurs robots en couleur — M

**Intérêt.** Suivre plusieurs robots sans marqueurs ArUco, un par couleur.

**Piste.** Une liste de couleurs, chacune avec son nom et ses réglages, au lieu d'une couleur unique. Les messages UDP indiqueraient le nom de la couleur.

#### Repère du sol par marqueur ArUco de référence — S

**Intérêt.** Un marqueur fixe posé au sol définirait le repère automatiquement, sans clics. Le repère resterait juste même si la caméra bouge un peu.

**Piste.** Choisir un ID réservé et sa taille. Ses 4 coins donnent l'homographie à chaque image. La précision baisse loin du marqueur : prévoir un grand marqueur, ou plusieurs marqueurs combinés.

### Suivi d'objet

Améliorations du suivi d'un objet encadré (`src/objecttracker.cpp`). Le lissage de Kalman, l'indice de confiance, la reprise automatique, la sélection simplifiée, les événements de mouvement et le suivi de plusieurs objets sont réalisés. Parmi les pistes restantes, l'orientation de l'objet est la plus utile pour suivre un robot sans marqueur.

#### Orientation de l'objet — M

**Intérêt.** Le cadre reste droit : on connaît la direction du déplacement, mais pas celle vers laquelle l'objet est tourné. Pour un robot sans marqueur, c'est pourtant son cap.

**Piste.** Suivre des points caractéristiques à l'intérieur du cadre (`cv::goodFeaturesToTrack`, puis flux optique `cv::calcOpticalFlowPyrLK`). En déduire la rotation et le changement d'échelle entre deux images (`cv::estimateAffinePartial2D`), cumulés depuis la sélection. Afficher un cadre tourné et envoyer l'angle en UDP.

#### Point de contact avec le sol — S

**Intérêt.** La position au sol est calculée à partir du centre du cadre. Pour un objet vu de biais et posé au sol, le point qui touche le sol est plutôt le bas du cadre. Le centre, plus haut, décale la position mesurée.

**Piste.** Une option « Point de référence : centre ou bas du cadre » dans l'onglet Suivi, utilisée pour la position et la vitesse sur le sol.

#### Garder le suivi en changeant de résolution — S

**Intérêt.** Changer de résolution arrête le suivi, même quand le format d'image ne change pas.

**Piste.** Si le format reste le même (4:3 vers 4:3), mettre le cadre et la trajectoire à l'échelle, puis réinitialiser le tracker sur la nouvelle image au lieu de l'arrêter.

#### Statistiques et export de la trajectoire — S

**Intérêt.** Analyser un déplacement après coup.

**Piste.** Afficher la distance parcourue, les vitesses moyenne et maximale et la durée de mouvement, et exporter la trajectoire en CSV (heure, position dans l'image et sur le sol, vitesse).

#### Suivi plus rapide — M

**Intérêt.** Le suivi tourne déjà dans un fil séparé, avec un objet par cœur, mais CSRT prend environ 25 ms de calcul par image et par objet : avec plusieurs objets, le nombre d'images traitées par seconde baisse (environ 13 i/s avec 3 objets en 1280 × 960).

**Piste.**
- Adapter la réduction d'image à la taille de l'objet : un grand objet peut être suivi sur une image plus petite que 640 px de large.
- Mettre à jour les objets immobiles moins souvent (une image sur deux), en gardant toutes les images pour ceux qui bougent.

### Caméra et calibration

#### Vérifier la calibration visuellement — S

**Intérêt.** Contrôler d'un coup d'œil qu'une calibration est bonne.

**Piste.** Une case « Corriger la distorsion » qui affiche l'image redressée (`cv::initUndistortRectifyMap` calculé une fois, puis `cv::remap`) : les lignes droites doivent apparaître droites, surtout sur les bords.

#### Calibration plus robuste et mieux guidée — M

**Intérêt.** La qualité de la calibration dépend de la variété des captures. La calibration de test a montré qu'un damier qui ne couvre pas les coins de l'image fausse la distorsion sur les bords.

**Piste.**
- afficher une carte des zones de l'image déjà couvertes par le damier ;
- indiquer l'erreur de chaque capture et permettre de retirer les mauvaises ;
- proposer une planche ChArUco, qui reste détectée même si elle est partiellement visible ou coupée par le bord de l'image.

---

### Données et intégration

#### Travailler sur une vidéo enregistrée — M

**Intérêt.** Régler la détection tranquillement, rejouer une situation difficile, comparer deux réglages sur les mêmes images, sans le robot sous la caméra.

**Piste.** Les vidéos s'enregistrent déjà (bouton **Vidéo**). Il reste à proposer une source « Fichier vidéo » à la place de la webcam (`QMediaPlayer` relié au même `QVideoSink`), avec pause et retour en arrière.

#### Enregistrement des positions en CSV — S

**Intérêt.** Analyser une trajectoire après coup, dans un tableur ou en Python.

**Piste.** Un bouton « Enregistrer les positions » qui écrit une ligne par objet et par image : heure, ID, position image, position au sol, cap.

#### Autres moyens d'envoi — M

**Intérêt.** UDP convient à la plupart des cas. D'autres liaisons peuvent être plus pratiques selon le robot.

**Piste.**
- **Port série**, pour un Arduino ou un ESP32 relié en USB (`QSerialPort`), avec un format compact ;
- **WebSocket**, pour une page web de supervision ;
- **ROS 2**, en publiant les poses sur un topic.

Pour l'UDP, une option pourrait n'envoyer que les images où quelque chose est détecté, ou limiter le nombre de messages par seconde.

---

### Interface et distribution

#### Traduction des boutons standard de Qt — S

**Intérêt.** Les boutons standard des fenêtres de Qt s'affichent en anglais (« Yes », « Close »...). Plusieurs boutons ont dû être libellés à la main pour l'éviter.

**Piste.** Charger les traductions de Qt au démarrage (`QTranslator` avec `qtbase_fr`, selon la langue du système).

#### Version anglaise de l'interface — M

**Intérêt.** Rendre l'application utilisable par un public non francophone.

**Piste.** Tous les textes passent déjà par `tr()`. Il reste à générer les fichiers de traduction (`lupdate`), à les traduire, puis à les charger selon la langue du système.

#### Paquet d'installation et autres systèmes — L

**Intérêt.** Installer l'application sans la compiler, et l'utiliser sous Windows ou macOS.

**Piste.**
- **Linux** : un paquet `.deb` (CPack) ou une AppImage contenant Qt et OpenCV.
- **Windows et macOS** : le code est portable, sauf l'écriture directe du réglage d'exposition auprès du pilote Linux (V4L2), déjà isolée et désactivée hors Linux. Il faudrait vérifier le comportement de Qt Multimedia et des réglages de caméra sur ces systèmes.
