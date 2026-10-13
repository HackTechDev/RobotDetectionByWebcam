# Améliorations possibles

Liste des améliorations envisagées pour RobotDetectionByWebcam, classées par thème. Chaque entrée indique l'intérêt, une piste de réalisation et un ordre de grandeur de l'effort :

- **S** : quelques heures ;
- **M** : une à deux journées ;
- **L** : plusieurs jours.

Ces efforts sont des estimations.

## Déjà réalisé

- Envoi des positions en JSON sur UDP (onglet « Envoi UDP »).
- Positions sur le sol en millimètres (onglet « Sol »).
- Réglages manuels de l'exposition et de la balance des blancs (« Réglages de l'image... »).
- Suivi d'un objet encadré à la souris, avec trajectoire et vitesse (onglet « Suivi »).
- Lissage dans le temps des carrés et des marqueurs : position lissée, vitesse, position prédite (onglet « Suivi »).

---

## Priorité haute

### Correction de la hauteur des repères sur le sol — M

**Intérêt.** Le repère du sol est défini au niveau du sol. Un marqueur collé sur le dessus du robot est plus haut, donc décalé vers l'extérieur de l'image : environ 5 cm d'erreur pour un marqueur à 10 cm de haut, à 1 m du point situé sous une caméra placée à 2 m. C'est aujourd'hui la principale source d'erreur des positions au sol.

**Piste.** Ajouter un champ « Hauteur des repères (mm) » dans l'onglet Sol. Avec une caméra calibrée, retrouver la pose de la caméra par rapport au sol à partir des 4 points (`cv::solvePnP`), puis intersecter le rayon de chaque repère avec le plan situé à cette hauteur. Sans calibration, il est possible de définir un second rectangle à la hauteur des repères.

### Tests automatiques — M

**Intérêt.** La détection et les calculs ont été vérifiés pendant le développement par des programmes de test sur images de synthèse, restés hors du projet. Les intégrer éviterait les régressions.

**Piste.** Un dossier `tests/` exécuté avec CTest (`ctest --test-dir build`), qui reprend ces vérifications :
- carré coloré, y compris blanc, gris et noir, et rejet des disques et des rectangles ;
- marqueurs ArUco dans plusieurs dictionnaires ;
- calibration sur caméra simulée (fx et fy retrouvés à 600 px) ;
- pose 3D ;
- repère du sol (positions à 1 à 2 mm près) ;
- format des messages JSON.

Les images de synthèse sont générées par les tests eux-mêmes, sans webcam.

---

## Détection

### Cap complet avec des couleurs — M

**Intérêt.** Le carré coloré ne donne son angle qu'à 90° près. Une pastille d'une seconde couleur à l'avant du robot donnerait le cap sur 360°, sans marqueur ArUco.

**Piste.** Détecter une seconde couleur près du carré principal. Le cap est la direction du centre du carré vers le centre de la pastille.

### Plusieurs robots en couleur — M

**Intérêt.** Suivre plusieurs robots sans marqueurs ArUco, un par couleur.

**Piste.** Une liste de couleurs, chacune avec son nom et ses réglages, au lieu d'une couleur unique. Les messages UDP indiqueraient le nom de la couleur.

### Repère du sol par marqueur ArUco de référence — S

**Intérêt.** Un marqueur fixe posé au sol définirait le repère automatiquement, sans clics. Le repère resterait juste même si la caméra bouge un peu.

**Piste.** Choisir un ID réservé et sa taille. Ses 4 coins donnent l'homographie à chaque image. La précision baisse loin du marqueur : prévoir un grand marqueur, ou plusieurs marqueurs combinés.

### Suivi d'objet : plusieurs objets et reprise automatique — M

**Intérêt.** Le suivi d'un objet encadré ne gère qu'un objet à la fois. Quand l'objet est perdu, il faut le resélectionner à la main.

**Piste.**
- Une liste d'objets suivis, chacun avec son nom et sa couleur d'affichage. Le coût augmente avec le nombre d'objets (environ 20 ms par objet avec CSRT).
- Mémoriser l'apparence de l'objet au moment de la sélection et la rechercher dans toute l'image quand il est perdu (`cv::matchTemplate` ou points caractéristiques ORB).
- Proposer les trackers à base de réseaux de neurones d'OpenCV (Nano, Vit), plus robustes, qui demandent de télécharger un modèle.

### Détection dans un fil d'exécution séparé — M

**Intérêt.** En haute résolution, ou avec le suivi CSRT, la détection ralentit l'interface. Un fil dédié garderait l'affichage fluide et utiliserait un second cœur du processeur.

**Piste.** Déplacer la détection dans un `QThread` qui traite toujours l'image la plus récente, en ignorant celles reçues pendant le traitement, et renvoie les résultats par signal.

---

## Caméra et calibration

### Vérifier la calibration visuellement — S

**Intérêt.** Contrôler d'un coup d'œil qu'une calibration est bonne.

**Piste.** Une case « Corriger la distorsion » qui affiche l'image redressée (`cv::initUndistortRectifyMap` calculé une fois, puis `cv::remap`) : les lignes droites doivent apparaître droites, surtout sur les bords.

### Calibration plus robuste et mieux guidée — M

**Intérêt.** La qualité de la calibration dépend de la variété des captures. La calibration de test a montré qu'un damier qui ne couvre pas les coins de l'image fausse la distorsion sur les bords.

**Piste.**
- afficher une carte des zones de l'image déjà couvertes par le damier ;
- indiquer l'erreur de chaque capture et permettre de retirer les mauvaises ;
- proposer une planche ChArUco, qui reste détectée même si elle est partiellement visible ou coupée par le bord de l'image.

### Autres réglages de la webcam — S

**Intérêt.** Le gain, la luminosité, le contraste et la mise au point influencent aussi la détection. Par exemple, quand l'exposition repasse en manuel, la webcam garde le gain choisi par l'exposition automatique.

**Piste.** Lire la liste des contrôles V4L2 de la webcam (`VIDIOC_QUERYCTRL`) et afficher un curseur pour chacun. Cela donnerait aussi les vraies limites du temps de pose, que Qt ne fournit pas.

---

## Données et intégration

### Travailler sur une vidéo enregistrée — M

**Intérêt.** Régler la détection tranquillement, rejouer une situation difficile, comparer deux réglages sur les mêmes images, sans le robot sous la caméra.

**Piste.** Un bouton d'enregistrement (`QMediaRecorder`) et une source « Fichier vidéo » à la place de la webcam (`QMediaPlayer` relié au même `QVideoSink`).

### Enregistrement des positions en CSV — S

**Intérêt.** Analyser une trajectoire après coup, dans un tableur ou en Python.

**Piste.** Un bouton « Enregistrer les positions » qui écrit une ligne par objet et par image : heure, ID, position image, position au sol, cap.

### Autres moyens d'envoi — M

**Intérêt.** UDP convient à la plupart des cas. D'autres liaisons peuvent être plus pratiques selon le robot.

**Piste.**
- **Port série**, pour un Arduino ou un ESP32 relié en USB (`QSerialPort`), avec un format compact ;
- **WebSocket**, pour une page web de supervision ;
- **ROS 2**, en publiant les poses sur un topic.

Pour l'UDP, une option pourrait n'envoyer que les images où quelque chose est détecté, ou limiter le nombre de messages par seconde.

---

## Interface et distribution

### Traduction des boutons standard de Qt — S

**Intérêt.** Les boutons standard des fenêtres de Qt s'affichent en anglais (« Yes », « Close »...). Plusieurs boutons ont dû être libellés à la main pour l'éviter.

**Piste.** Charger les traductions de Qt au démarrage (`QTranslator` avec `qtbase_fr`, selon la langue du système).

### Version anglaise de l'interface — M

**Intérêt.** Rendre l'application utilisable par un public non francophone.

**Piste.** Tous les textes passent déjà par `tr()`. Il reste à générer les fichiers de traduction (`lupdate`), à les traduire, puis à les charger selon la langue du système.

### Lisibilité du fichier de réglages — S

**Intérêt.** Dans `settings.ini`, Qt encode le nom des caméras (`UVC%20Camera%20%28046d%3A0825%29`), ce qui rend le fichier peu lisible si on veut le modifier à la main.

**Piste.** Utiliser une clé simplifiée, par exemple l'identifiant USB `046d_0825` quand il est présent dans le nom, ou le nom réduit aux lettres et aux chiffres.

### Paquet d'installation et autres systèmes — L

**Intérêt.** Installer l'application sans la compiler, et l'utiliser sous Windows ou macOS.

**Piste.**
- **Linux** : un paquet `.deb` (CPack) ou une AppImage contenant Qt et OpenCV.
- **Windows et macOS** : le code est portable, sauf l'écriture directe du réglage d'exposition auprès du pilote Linux (V4L2), déjà isolée et désactivée hors Linux. Il faudrait vérifier le comportement de Qt Multimedia et des réglages de caméra sur ces systèmes.
