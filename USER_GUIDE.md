# Guide utilisateur — RobotDetectionByWebcam

Ce guide explique comment utiliser l'application au quotidien : repérer un robot avec un carré de couleur ou avec des marqueurs ArUco, calibrer la caméra et lire les résultats.

Pour l'installation et la compilation, voir le [README](README.md). Pour une découverte pas à pas de toutes les fonctionnalités, avec des exercices, voir le [tutoriel](TUTORIAL.md).

## Sommaire

1. [Présentation](#1-présentation)
2. [Démarrage rapide](#2-démarrage-rapide)
3. [L'écran principal](#3-lécran-principal)
4. [Suivre un carré coloré](#4-suivre-un-carré-coloré)
5. [Suivre des marqueurs ArUco](#5-suivre-des-marqueurs-aruco)
6. [Suivre des objets encadrés](#6-suivre-des-objets-encadrés)
7. [Calibrer la caméra](#7-calibrer-la-caméra)
8. [Définir le repère du sol](#8-définir-le-repère-du-sol), les [zones](#85-définir-des-zones) et les jeux ([robot dans une zone](#86-jeu--détecter-le-robot-dans-une-zone), [1, 2, 3 Soleil](#87-jeu--1-2-3-soleil), [chasse au trésor](#88-jeu--chasse-au-trésor), [et les autres](#89-les-autres-jeux--fonctionnement-commun))
9. [Lire les résultats](#9-lire-les-résultats)
10. [Envoyer les positions à un autre programme](#10-envoyer-les-positions-à-un-autre-programme)
11. [Conseils pour une détection fiable](#11-conseils-pour-une-détection-fiable)
12. [Questions fréquentes et dépannage](#12-questions-fréquentes-et-dépannage)
13. [Fichiers utilisés par l'application](#13-fichiers-utilisés-par-lapplication)

---

## 1. Présentation

L'application filme la scène avec une webcam et repère le robot de trois façons, utilisables séparément ou ensemble.

| Méthode | Ce qu'il faut sur le robot | Ce qu'on obtient | Points forts | Limites |
|---|---|---|---|---|
| **Carré coloré** | Un carré d'une couleur vive, ou blanc, gris ou noir (papier, gommette, pièce peinte) | Position et taille en pixels, angle de 0 à 90° | Très simple à mettre en place | Sensible à l'éclairage et aux objets de même couleur ; un seul robot facile à distinguer ; angle connu à 90° près |
| **Objets encadrés** | Rien : on désigne chaque objet à la souris (en l'encadrant ou d'un clic) | Position, trajectoire, vitesse et direction du déplacement, débuts et fins de mouvement ; position et vitesse en mm sur le sol si le repère du sol est défini | Aucune préparation, fonctionne avec n'importe quel objet ; signale quand il ne le reconnaît plus et le retrouve tout seul quand il réapparaît | Moins fiable que les marqueurs (objet qui change d'aspect, qui ressemble au fond) ; pas d'orientation ; chaque objet ralentit l'application |
| **Marqueur ArUco** | Un marqueur noir et blanc imprimé | Identifiant, position et angle complet (−180° à 180°) ; **position 3D en mm** si la caméra est calibrée | Fiable, plusieurs robots distingués par leur identifiant | Le marqueur doit être assez grand et bien visible |

En bref : pour un essai rapide, le carré coloré suffit. Pour suivre un ou plusieurs objets sans rien coller dessus, utilisez le suivi d'objets encadrés. Pour un suivi fiable, ou pour plusieurs robots, utilisez les marqueurs ArUco.

Quelle que soit la méthode, l'application peut aussi :
- donner les positions **en millimètres sur le sol** (section 8) ;
- donner la **position 3D** des marqueurs, une fois la caméra calibrée (section 7) ;
- **lisser** les positions et calculer les **vitesses** (section 6.4) ;
- **envoyer** toutes les mesures à un autre programme, par exemple celui qui pilote le robot (section 10) ;
- signaler ce qui entre dans des **zones** nommées de l'image et en sort (section 8.5) ;
- faire **jouer** le robot : le menu **Jeu** signale quand il est entièrement dans une zone (section 8.6), arbitre une partie de « 1, 2, 3 Soleil » (section 8.7), ou le guide vers des trésors cachés (section 8.8).

L'application sait aussi **capturer une image** ou **enregistrer une vidéo** de la webcam, et afficher la vidéo seule dans une **fenêtre pour le joueur**, sur un second écran (section 3).

---

## 2. Démarrage rapide

1. Branchez la webcam.
2. Lancez l'application :
   ```bash
   ./build/RobotDetectionByWebcam
   ```
3. L'image de la webcam s'affiche à gauche. Si plusieurs caméras sont branchées, choisissez la bonne dans la liste **Caméra**.
4. Au lancement, **aucune détection automatique n'est active** : rien n'est repéré tant que vous ne le demandez pas. Au choix :
   - **objet quelconque** : onglet **Suivi**, **Ajouter des objets**, puis tracez un rectangle autour de l'objet et appuyez sur **Entrée** (voir [section 6](#6-suivre-des-objets-encadrés)) ;
   - **carré coloré** : onglet **Carré coloré**, cochez **Activer la détection**, puis cliquez sur votre carré dans l'image pour choisir sa couleur ;
   - **ArUco** : onglet **ArUco**, cochez **Activer la détection**, puis montrez un marqueur du dictionnaire « 4x4 (50) » (voir [section 5](#5-suivre-des-marqueurs-aruco) pour en imprimer un).
5. Les résultats s'affichent sur l'image et dans le panneau **Résultat**, en bas à droite.

---

## 3. L'écran principal

```
┌───────────────────────────────────────────────────────────────────────┐
│ Jeu                                            (barre de menus)       │
├───────────────────────────────────────┬───────────────────────────────┤
│                                       │ Webcam                        │
│                                       │   Caméra     [........... ▼]  │
│                                       │   Résolution [........... ▼]  │
│                                       │   [ ] Effet miroir            │
│                                       │ [Capture][Vidéo][Joueur][Rég.]│
│          Image de la webcam           ├───────────────────────────────┤
│      (avec les détections dessinées)  │ Carré|ArUco|Cal.|Sol|Zones|...│
│                                       │                               │
│                                       │   Réglages de l'onglet        │
│    Cliquer sur l'image = choisir      │   sélectionné                 │
│    la couleur du carré                │                               │
│                                       ├───────────────────────────────┤
│                                       │ Résultat                      │
│                                       │   Image, images/s, carré,     │
│                                       │   marqueurs...                │
│                                       │                               │
│                                       │         [ Quitter ]           │
├───────────────────────────────────────┴───────────────────────────────┤
│ Capture enregistrée : ...                      (barre d'état)         │
└───────────────────────────────────────────────────────────────────────┘
```

- **Zone d'image** (à gauche) : l'image de la caméra, avec les détections dessinées : carrés, marqueurs, damier, repère du sol, objet suivi et trajectoires. Un clic sur l'image choisit la couleur du carré, sauf pendant l'ajout d'objets à suivre (onglet **Suivi**) ou la saisie des points du sol (onglet **Sol**). Pendant ces deux saisies, la touche **Échap** annule.
- **Webcam** :
  - **Caméra** : la webcam utilisée. Chaque caméra a sa **propre calibration** et son **propre repère du sol**, rechargés quand on la choisit, ainsi que sa résolution et ses réglages de l'image. Changer de caméra arrête le suivi des objets encadrés, dont les cadres ne valaient que pour l'autre caméra ; Deux caméras du même modèle, qui portent le même nom, sont distinguées par leur identifiant système (`/dev/video0`, `/dev/video2`...), ajouté à leur nom dans les réglages (`..._video2`) ; si cet identifiant change (caméras branchées dans un autre ordre), elles peuvent échanger leurs réglages.
  - **Résolution** : la taille des images de la caméra, avec leur format (4:3, 16:9...) et le nombre maximal d'images par seconde annoncé par la caméra. Voir [Choisir la résolution](#choisir-la-résolution) ;
  - **Capture** : enregistre aussitôt l'image de la webcam en PNG (touche **F12**). Voir [Capturer une image](#capturer-une-image) ;
  - **Vidéo** : démarre et arrête l'enregistrement d'une vidéo de la webcam en MP4 (touche **F9**). Voir [Enregistrer une vidéo](#enregistrer-une-vidéo) ;
  - **Joueur** : ouvre une seconde fenêtre avec la vidéo seule, en plein écran sur l'autre écran, pour le joueur. Voir [Fenêtre du joueur](#fenêtre-du-joueur) ;
  - **Réglages de l'image...** : exposition, balance des blancs, luminosité, contraste, gain, fréquence du secteur... Voir [Régler l'image de la webcam](#régler-limage-de-la-webcam) ;
  - **Effet miroir** : retourne l'image affichée, comme dans un miroir. C'est pratique quand on se place face à la caméra. Cette option ne change **que l'affichage** : les détections et les mesures restent celles de l'image réelle de la caméra (voir [section 9.1](#91-coordonnées-dans-limage-en-pixels)).
- **Onglets** :
  - **Carré coloré** : couleur et réglages de la détection du carré ;
  - **ArUco** : dictionnaire et taille des marqueurs ;
  - **Calibration** : damier et calibration de la caméra ;
  - **Sol** : repère du sol, pour avoir les positions en millimètres sur le sol (voir [section 8](#8-définir-le-repère-du-sol)) ;
  - **Zones** : rectangles fixes et nommés de l'image ; l'application indique ce qui s'y trouve et signale les entrées et sorties (voir [8.5](#85-définir-des-zones)) ;
  - **Suivi** : suivi d'un ou plusieurs objets quelconques désignés à la souris, journal de leurs mouvements, et lissage des positions des carrés et des marqueurs (voir [section 6](#6-suivre-des-objets-encadrés)). L'onglet défile s'il ne tient pas en hauteur ;
  - **Envoi UDP** : envoi des positions à un autre programme (voir [section 10](#10-envoyer-les-positions-à-un-autre-programme)).
- **Résultat** : taille de l'image, nombre d'images par seconde (i/s) et mesures détaillées.
- **Menu Jeu**, en haut de la fenêtre : jeux construits sur le suivi et les zones, **Détection robot dans zone...** (voir [8.6](#86-jeu--détecter-le-robot-dans-une-zone)), **1, 2, 3 Soleil...** (voir [8.7](#87-jeu--1-2-3-soleil)), **Chasse au trésor...** (voir [8.8](#88-jeu--chasse-au-trésor)), puis les jeux décrits à partir de la [section 8.9](#89-les-autres-jeux--fonctionnement-commun). Un seul jeu tourne à la fois : en démarrer un arrête les autres.
- **Barre d'état**, tout en bas de la fenêtre : messages brefs, par exemple le chemin d'une image capturée ou d'une vidéo enregistrée.
- **Quitter** : ferme l'application. Une fenêtre demande de confirmer : cliquez sur **Quitter** pour fermer, ou sur **Annuler** (ou appuyez sur Échap) pour continuer. La même confirmation s'affiche si vous fermez la fenêtre avec la croix de la barre de titre.

### Choisir la résolution

La résolution choisie est **mémorisée** pour chaque modèle de caméra et réappliquée au lancement suivant. Au tout premier lancement avec une caméra, celle-ci démarre dans son format par défaut, et la liste **Résolution** se cale sur la taille des images reçues.

| Résolution | Avantages | Inconvénients |
|---|---|---|
| **Haute** (ex. 1280 x 960) | Détecte les marqueurs plus loin ; positions plus précises | Détection plus lente ; certaines caméras donnent moins d'images par seconde |
| **Basse** (ex. 640 x 480) | Détection rapide, fluide | Les marqueurs doivent être plus grands ou plus proches |

Conseils :
- Commencez par **640 x 480** et montez si les marqueurs ne sont pas détectés d'assez loin.
- Restez dans le **même format d'image que votre calibration** (indiqué entre parenthèses) : une calibration faite en 4:3 ne s'applique pas à une image 16:9. Voir [section 7.4](#74-quand-refaire-la-calibration-).
- Le nombre d'images par seconde affiché dans la liste est le **maximum annoncé par la caméra**. Le nombre réel, affiché dans **Résultat**, peut être plus bas, en particulier dans une pièce sombre : la caméra allonge alors son temps de pose. Une **exposition manuelle** courte permet de retrouver le débit maximal (voir ci-dessous).

Certaines webcams n'envoient pas la résolution demandée dans leur format rapide (MJPEG). L'application le détecte et passe automatiquement au format non compressé de la même résolution. Le nombre d'images par seconde indiqué dans la liste est alors mis à jour, et il est souvent plus bas.

### Capturer une image

Le bouton **Capture**, à gauche de **Réglages de l'image...**, enregistre aussitôt l'image de la webcam, sans rien demander. Le chemin du fichier s'affiche quelques secondes en bas de la fenêtre.

- **Un clic sur Capture**, ou la touche **F12** : l'image telle que filmée par la caméra, en pleine résolution, sans les détections dessinées et sans l'effet miroir.
- **La petite flèche** à droite du bouton ouvre un menu :
  - **Image avec les détections** (**Maj+F12**) : l'image telle qu'affichée, avec les carrés, marqueurs, objets suivis, trajectoires et repère du sol, et avec l'effet miroir s'il est coché ;
  - **Choisir le dossier des captures...** : le dossier où les images sont enregistrées (mémorisé) ;
  - **Ouvrir le dossier des captures** : l'ouvre dans le gestionnaire de fichiers.

Par défaut, les captures vont dans `~/Images/RobotDetectionByWebcam/` (le dossier Images de votre session). Chaque fichier est nommé d'après la date et l'heure, par exemple `capture_20261014_214637_393.png`, ou `capture_20261014_214638_160_detections.png` pour une image avec les détections. Une capture ne remplace donc jamais la précédente.

Utile pour garder une trace d'un essai, préparer un fond de terrain pour un jeu, ou joindre une image à un signalement de problème.

### Enregistrer une vidéo

Le bouton **Vidéo**, à droite de **Capture**, enregistre une vidéo au format MP4 (H.264), lisible par tous les lecteurs.

- **Un clic sur Vidéo**, ou la touche **F9** : l'enregistrement démarre. Le bouton devient rouge et affiche **■ Arrêter** avec la durée écoulée. Un nouveau clic, ou **F9**, l'arrête.
- Après l'arrêt, le bouton affiche **Finalisation...** une à deux secondes, le temps de terminer le fichier. Le chemin de la vidéo s'affiche ensuite en bas de la fenêtre.
- **La petite flèche** à droite du bouton ouvre un menu :
  - **Vidéo de la webcam** (**F9**) : le flux de la caméra tel quel, en pleine résolution, à son propre débit, sans les détections ni l'effet miroir ;
  - **Vidéo avec les détections** (**Maj+F9**) : les images telles qu'affichées, avec les carrés, marqueurs, objets suivis, trajectoires et repère du sol. Son débit est celui du traitement (affiché dans **Résultat**), parfois plus bas que celui de la caméra ;
  - **Ouvrir le dossier des captures**.

Les vidéos vont dans le même dossier que les images capturées (par défaut `~/Images/RobotDetectionByWebcam/`, voir [Capturer une image](#capturer-une-image)), avec un nom horodaté : `video_20261014_215256.mp4`, ou `video_20261014_215305_detections.mp4`.

L'enregistrement s'arrête de lui-même si vous changez de caméra ou de résolution, et à la fermeture de l'application, qui attend que le fichier soit terminé. Une vidéo en 1280 × 960 occupe environ 1 Mo par seconde.

### Fenêtre du joueur

Le bouton **Joueur**, à droite de **Vidéo**, ouvre une seconde fenêtre qui ne montre **que la vidéo**, sur fond noir, sans aucun réglage. Elle est faite pour un second écran tourné vers le joueur, pendant que vous gardez la fenêtre principale sur le vôtre.

- **Un clic sur Joueur** ouvre la fenêtre, un nouveau clic la ferme. Le bouton reste enfoncé tant qu'elle est ouverte.
- Elle s'ouvre **en plein écran sur l'autre écran** que celui de la fenêtre principale. Avec un seul écran, elle s'ouvre en fenêtre normale, pour ne pas cacher la fenêtre principale.
- Dans la fenêtre du joueur : **double-clic** ou **F11** bascule le plein écran, **Échap** le quitte. En plein écran, le pointeur de la souris est masqué.
- **La petite flèche** à droite du bouton ouvre un menu :
  - **Afficher les détections** (coché par défaut) : zones, marqueurs, objets suivis, trajectoires... comme dans la fenêtre principale. Décochez pour la vidéo brute de la caméra ;
  - **Retourner l'image (miroir)** : pratique quand le joueur est face à la caméra. Ce réglage est indépendant de l'**Effet miroir** de la fenêtre principale ; avec les détections affichées, les deux s'additionnent ;
  - **Plein écran** ;
  - **Écran** : **Automatique** (l'autre écran) ou un écran précis, choisi dans la liste des écrans branchés.

Ces choix sont mémorisés, ainsi que l'ouverture de la fenêtre : si elle était ouverte en quittant l'application, elle se rouvre au lancement suivant. Si l'écran choisi n'est plus branché, elle s'ouvre sur l'autre écran.

### Régler l'image de la webcam

Le bouton **Réglages de l'image...**, à droite de **Effet miroir**, ouvre une petite fenêtre. Elle peut rester ouverte pendant que l'application fonctionne.

**Exposition** : le temps de pose de chaque image.
- **Automatique** (par défaut) : la caméra s'adapte à la lumière. Dans une pièce peu éclairée, elle allonge son temps de pose, ce qui **ralentit** l'image (souvent 15 images par seconde au lieu de 30) et rend le robot **flou** quand il bouge.
- **Manuelle** : vous fixez le **temps de pose** avec le curseur, entre les limites de la caméra, affichées dessous (0,1 à 1 000 ms pour une Logitech C270). La caméra arrondit la valeur demandée : la valeur réellement **appliquée** s'affiche sous le curseur, avec le nombre maximal d'images par seconde correspondant.

  | Temps de pose | Images par seconde (au plus) | Image |
  |---|---|---|
  | 10 à 20 ms | 30 (la limite de la plupart des webcams) | Nette, mais sombre si la pièce est peu éclairée |
  | 33 ms | 30 | Compromis |
  | 66 ms et plus | 15 et moins | Claire, mais floue si le robot bouge vite |

  Une pose courte demande plus de lumière. Si l'image devient trop sombre, éclairez la scène, ou baissez **Luminosité min** (carré coloré).

**Balance des blancs** : la correction des couleurs selon la lumière de la pièce.
- **Automatique** (par défaut) : la caméra corrige les couleurs en permanence. Elles peuvent alors **dériver** quand la lumière change ou quand un objet coloré entre dans l'image, ce qui peut faire perdre la détection du carré coloré.
- **Manuelle** : vous fixez la **température** de la lumière, de 2000 à 7500 K : basse pour une lumière chaude (ampoule), haute pour la lumière du jour. Les couleurs ne bougent plus. La caméra limite la plage selon son modèle (par exemple 2800 à 6500 K). Réglez la température pour que le blanc paraisse blanc, puis cliquez à nouveau sur le carré pour reprendre sa couleur.

**Tout en automatique** remet l'exposition et la balance des blancs en automatique. Les réglages qu'une caméra ne prend pas en charge sont grisés.

**Autres réglages** (colonne de droite) : la liste dépend de la webcam, qui la fournit elle-même. Pour une Logitech C270 :

| Réglage | Effet |
|---|---|
| **Luminosité**, **Contraste**, **Saturation** | Traitement de l'image par la caméra. Une saturation plus forte rend le carré coloré plus facile à distinguer du décor. |
| **Gain** | Amplification du signal : image plus claire mais plus bruitée. Utile avec une exposition manuelle courte dans une pièce sombre. En exposition automatique, la caméra le règle elle-même. |
| **Fréquence du secteur** | Supprime le scintillement (bandes qui défilent) sous un éclairage artificiel : choisissez **50 Hz** en Europe, **60 Hz** en Amérique du Nord. |
| **Netteté** | Renforcement des contours ; trop fort, il crée des halos. |
| **Compensation du contre-jour** | Éclaircit le sujet devant une fenêtre. |
| **Ralentir les images en exposition auto.** | Autorise la caméra à descendre à 15 images par seconde (ou moins) dans une pièce sombre. Décochez-la pour garder 30 images par seconde en exposition automatique, au prix d'une image plus sombre. |
| **Voyant**, **Clignotement du voyant** | Voyant lumineux de la webcam. |

La valeur d'usine est signalée par « (défaut) » à côté du curseur. **Valeurs par défaut de la webcam** les rétablit toutes.

Tous ces réglages sont **mémorisés pour chaque modèle de caméra** et réappliqués au lancement, ainsi qu'après un changement de résolution.

Ils sont enregistrés **dans la webcam elle-même**. Quand vous quittez l'application normalement (ou changez de caméra), elle remet la webcam en automatique et rend aux autres réglages leur valeur d'avant le lancement, pour que les autres logiciels (visioconférence...) la retrouvent dans son état habituel. Si l'application est arrêtée brutalement (Ctrl+C, plantage), la webcam garde les réglages manuels : relancez l'application puis quittez-la normalement.

---

## 4. Suivre un carré coloré

### 4.1 Préparer le carré

- De préférence, choisissez une **couleur vive et franche** : rouge, vert, bleu, jaune, orange, magenta... C'est le cas le plus fiable.
- Un carré **blanc, gris ou noir** fonctionne aussi (voir [4.3](#43-les-réglages)), mais il faut qu'il **contraste avec le fond** : un carré blanc sur un mur ou un sol clair ne peut pas être distingué. Le blanc est aussi très courant dans une scène (feuilles, murs, reflets), d'où plus de risques de confusion.
- Choisissez une couleur **absente du reste de la scène** (sol, murs, meubles, vêtements).
- Préférez une surface **mate** : les reflets apparaissent blancs et « trouent » le carré.
- Le carré doit être **plein**, sans motif ni inscription.

### 4.2 Régler la détection

1. Ouvrez l'onglet **Carré coloré** et cochez **Activer la détection** (elle est désactivée à chaque lancement).
2. **Choisissez la couleur** :
   - **le plus simple : cliquez sur le carré dans l'image**. L'application prend la couleur moyenne autour du clic, choisit le **Type** adapté (voir ci-dessous) et règle automatiquement les seuils ;
   - ou cliquez sur le bouton **Couleur** (il affiche la couleur actuelle) pour la choisir dans une palette.

   Le **Type** indique comment la couleur est reconnue :
   - **Couleur vive (teinte)** : par sa teinte (rouge, vert, bleu...) ;
   - **Blanc, gris ou noir** : ces couleurs n'ont pas de teinte. Elles sont reconnues par leur **luminosité** et leur **faible saturation**.

   L'application choisit le type automatiquement quand vous choisissez une couleur : « Blanc, gris ou noir » si la couleur est peu saturée. Vous pouvez le changer à la main dans la liste **Type**.
3. Cochez **Afficher le masque** : l'image devient noir et blanc. **En blanc** apparaissent les pixels reconnus comme étant de la bonne couleur.
   - Le but : le carré doit être **blanc et plein**, et le reste de l'image **noir**.
4. Ajustez les curseurs si besoin (voir le tableau ci-dessous), puis décochez **Afficher le masque**.

### 4.3 Les réglages

Les curseurs affichés dépendent du **Type** choisi.

**Type « Couleur vive (teinte) »**

| Réglage | Valeur par défaut | À quoi il sert | Quand le modifier |
|---|---|---|---|
| **Tolérance teinte** | 15 | Écart de couleur accepté autour de la couleur choisie | **Augmenter** si le carré est incomplet dans le masque (ombre, éclairage inégal). **Diminuer** si d'autres objets de couleur proche apparaissent en blanc. |
| **Saturation min** | 90 | Écarte les couleurs ternes ou grisâtres | **Diminuer** si le carré est pâle ou trop éclairé. **Augmenter** si des zones grises ou beiges apparaissent en blanc. |
| **Luminosité min** | 60 | Écarte les zones sombres | **Diminuer** si le carré est dans l'ombre. **Augmenter** si des zones sombres parasites apparaissent. |

**Type « Blanc, gris ou noir »**

| Réglage | Valeur par défaut | À quoi il sert | Quand le modifier |
|---|---|---|---|
| **Tolérance luminosité** | 50 | Écart de luminosité accepté autour de la couleur choisie | **Augmenter** si le carré est incomplet (zones d'ombre sur un carré blanc). **Diminuer** si des zones plus sombres ou plus claires que le carré apparaissent. |
| **Saturation max** | 60 (réglée au clic) | Écarte les zones colorées | **Augmenter** si l'éclairage teinte le carré (lumière chaude qui rend le blanc orangé). **Diminuer** si des objets pâles mais colorés (beige, bois clair) apparaissent. |

**Réglages communs**

| Réglage | Valeur par défaut | À quoi il sert | Quand le modifier |
|---|---|---|---|
| **Surface min** | 120 | Taille minimale d'un carré | **Diminuer** si le carré est loin et petit dans l'image. **Augmenter** pour ignorer les petites taches. |
| **Remplissage min (%)** | 82 | Exige une forme bien carrée et pleine | **Diminuer** (vers 75) si le carré est abîmé ou partiellement masqué. **Augmenter** (vers 90) si des formes rondes sont prises pour des carrés. |

La **Surface min** se mesure sur une version réduite de l'image (320 pixels de large). La valeur par défaut, 120, correspond à un carré d'environ 11 × 11 pixels sur cette image réduite, soit environ 1/30 de la largeur de l'image.

### 4.4 Ce qui s'affiche

- **Contour vert** : le carré principal (le plus grand), avec un point au centre et ses coordonnées en pixels au-dessus.
- **Contours jaunes** : les autres carrés de la même couleur, s'il y en a.
- Dans **Résultat** : `Carré détecté (N au total)`, puis le **centre**, le **côté** et l'**angle** du carré principal.

L'angle d'un carré va de 0° à 90°. Un carré tourné de 90° est identique à lui-même : l'application ne peut donc pas savoir dans quelle direction « regarde » le robot. Si vous avez besoin du cap complet, utilisez un marqueur ArUco.

---

## 5. Suivre des marqueurs ArUco

Un marqueur ArUco est un carré noir et blanc qui contient un code. Chaque marqueur porte un **identifiant** (un numéro). On peut ainsi suivre plusieurs robots en même temps et savoir lequel est lequel.

Dans l'onglet **ArUco**, cochez **Activer la détection** : comme celle du carré coloré, elle est désactivée à chaque lancement.

### 5.1 Choisir un dictionnaire

Un dictionnaire est une famille de marqueurs. L'application ne reconnaît que les marqueurs du dictionnaire sélectionné dans l'onglet **ArUco**.

| Dictionnaire | Nombre de marqueurs | Conseil |
|---|---|---|
| 4x4 (50) | 50 | **Recommandé** : le plus facile à détecter de loin |
| 4x4 (250) | 250 | Si 50 identifiants ne suffisent pas |
| 5x5 (100), 6x6 (250) | 100, 250 | Plus robustes aux erreurs de lecture, mais il faut des marqueurs plus grands |
| ArUco original (1024) | 1024 | Compatibilité avec d'anciens projets |
| AprilTag 36h11 (587) | 587 | Compatibilité avec les projets AprilTag (robotique, ROS) |

Plus un marqueur a de cases, plus il faut l'imprimer grand pour qu'il soit lu de loin. Choisissez le plus petit dictionnaire qui contient assez d'identifiants.

### 5.2 Imprimer un marqueur

1. Dans l'onglet **ArUco**, sélectionnez le dictionnaire.
2. Cliquez sur **Générer un marqueur...**.
3. Saisissez l'**identifiant** (par exemple 1 pour le robot n°1), puis validez.
4. Choisissez où enregistrer l'image PNG. Le nom proposé ressemble à `aruco_4x4_1.png`.
5. Imprimez l'image :
   - **gardez la marge blanche** autour du marqueur, elle est indispensable à la détection ;
   - la taille d'impression est libre, mais l'image ne doit pas être déformée (même largeur et hauteur) ;
   - utilisez du papier **mat** et collez le marqueur **bien à plat** sur le robot.

Taille conseillée : un marqueur doit occuper **au moins 30 pixels de côté** dans l'image pour être détecté. À titre indicatif, avec une webcam ordinaire, un marqueur 4x4 de 5 cm est lisible jusqu'à environ 1,5 à 2 m. Doublez sa taille pour doubler la distance.

### 5.3 Indiquer la taille du marqueur

Mesurez à la règle **le côté de la partie noire** du marqueur imprimé, **sans la marge blanche**. Saisissez cette valeur dans **Taille du marqueur** (en mm).

Cette valeur ne sert qu'aux mesures 3D (après calibration). Si elle est fausse de 10 %, toutes les distances seront fausses de 10 %.

Tous les marqueurs suivis doivent avoir **la même taille imprimée**, car l'application utilise une seule valeur pour tous.

### 5.4 Ce qui s'affiche

- **Contour magenta** autour de chaque marqueur, avec son **identifiant**.
- **Point rouge** : le coin d'origine du marqueur (son coin haut-gauche quand il est droit).
- **Sans calibration** : un trait part du centre vers le bord supérieur du marqueur, pour montrer sa direction.
- **Avec calibration** : les axes 3D du marqueur sont dessinés :
  - **rouge** : axe X (vers la droite du marqueur) ;
  - **vert** : axe Y (vers le bas du marqueur) ;
  - **bleu** : direction perpendiculaire au marqueur, vers la caméra.
- Dans **Résultat**, une ligne par marqueur, triés par identifiant : position et angle dans l'image, puis, si la caméra est calibrée, position 3D, distance et rotations (voir [section 9](#9-lire-les-résultats)).

**Astuce** : collez le marqueur sur le robot avec son bord supérieur orienté vers l'avant du robot. L'angle du marqueur devient alors directement le cap du robot.

---

## 6. Suivre des objets encadrés

Cette méthode suit **n'importe quel objet**, sans couleur particulière ni marqueur : il suffit de l'encadrer dans l'image. L'application le suit ensuite d'image en image d'après son apparence, et donne sa **position**, sa **trajectoire**, sa **vitesse** et la **direction** de son déplacement. **Plusieurs objets** peuvent être suivis en même temps, chacun avec son nom et sa couleur.

L'onglet **Suivi** sert aussi à lisser dans le temps les positions des carrés et des marqueurs, et à calculer leur vitesse (voir [6.4](#64-lisser-les-positions-des-carrés-et-des-marqueurs)).

### 6.1 Ajouter des objets

1. Ouvrez l'onglet **Suivi**.
2. Choisissez la **Méthode** :
   - **CSRT (précis)** : suit mieux l'objet, y compris quand sa taille change. C'est le choix par défaut.
   - **KCF (rapide)** : environ 4 fois plus rapide, mais décroche plus facilement.
   - **Nano (réseau de neurones)** : résiste mieux aux changements d'aspect (rotation, changement de taille) et retrouve plus souvent l'objet après une disparition momentanée. Plus rapide que CSRT (environ 10 ms par image et par objet sur un processeur à 4 cœurs).
   - **Vit (réseau de neurones)** : comparable à Nano, cadre souvent plus précis, un peu plus lent (environ 15 ms).

   Nano et Vit ont besoin d'un **modèle** (fichier de réseau de neurones), téléchargé une seule fois : la première fois que vous les choisissez, l'application propose de le télécharger (1,7 Mo pour Nano, 0,7 Mo pour Vit). Voir [Modèles des réseaux de neurones](#modèles-des-réseaux-de-neurones).
3. Cliquez sur **Ajouter des objets**.
4. **Tracez un rectangle** autour du premier objet (cliquer-glisser), au plus près, avec le moins de fond possible. Vous pouvez aussi **cliquer simplement sur l'objet** : l'application le délimite toute seule autour du point cliqué.
5. Le cadre s'affiche en pointillés avec **8 poignées**. Ajustez-le si besoin :
   - faites glisser une **poignée** pour le redimensionner ;
   - faites glisser l'**intérieur** du cadre pour le déplacer ;
   - tracez **ailleurs** pour recommencer.
6. Validez l'objet, au choix, avec le bouton **Valider l'objet**, la touche **Entrée** ou un **double-clic** dans le cadre. Son suivi démarre et il apparaît dans la **liste des objets**.
7. Vous restez en mode ajout : **tracez directement le rectangle de l'objet suivant**, puis validez-le de la même façon, et ainsi de suite.
8. Quand tous les objets sont ajoutés, appuyez sur **Échap** ou cliquez sur **Terminer l'ajout**. Les clics sur l'image servent de nouveau à choisir la couleur du carré.

Chaque objet reçoit un **numéro**, un **nom** (« Objet 1 », « Objet 2 »...) et une **couleur**, utilisés partout : sur l'image, dans le panneau **Résultat**, dans le journal des événements et dans les messages UDP.

La **liste des objets** permet de les gérer :
- **double-cliquez sur un nom** pour le modifier, par exemple « Robot bleu » ;
- sélectionnez un objet puis cliquez sur **Retirer** pour arrêter son suivi ;
- **Tout retirer** arrête le suivi de tous les objets. La numérotation repart alors de 1.

La délimitation automatique (clic simple) fonctionne bien pour un objet qui se détache du fond. Elle cherche l'objet dans une zone autour du clic (40 % du petit côté de l'image) : un objet plus grand que cette zone, ou qui se confond avec le fond, est mal délimité. Le message « Objet mal délimité » s'affiche alors, et un carré est proposé autour du clic : ajustez-le avec les poignées.

L'image continue de défiler pendant l'ajustement : si l'objet bouge, recadrez-le avant de valider. Le choix de la méthode s'applique aux objets ajoutés ensuite ; les autres réglages (trajectoire, seuils de mouvement, lissage) s'appliquent à tous les objets.

### 6.2 Ce qui s'affiche

Chaque objet est dessiné dans **sa couleur** (cyan, blanc, violet, pêche...) :
- **Cadre plein** autour de l'objet, avec son nom et sa vitesse au-dessus (en mm/s si le repère du sol est défini, sinon en pixels par seconde). La mention « (confiance faible) » s'ajoute quand la **confiance** baisse sous 60 % : le contenu du cadre ressemble moins à l'objet sélectionné.
- **Trajectoire** : la ligne des positions passées, sur la **Durée de la trajectoire** choisie (10 s par défaut). La case **Afficher la trajectoire** la masque.
- **Flèche** dans le sens du déplacement, quand l'objet bouge. Elle est plus longue quand il va plus vite.
- **Cadre en pointillés** et « (prédit) » juste après une perte : pendant le **Maintien après perte** du lissage (0,5 s par défaut), la position est prédite à la même vitesse.
- **Cadre rouge en pointillés** et « (perdu) » ensuite, quand l'objet n'est toujours pas retrouvé. L'application le **recherche alors dans toute l'image**. Dès qu'il réapparaît avec un aspect proche de celui de la sélection, même ailleurs, le suivi reprend tout seul. Pour éviter de confondre l'objet avec autre chose (un visage, un décor), un candidat doit être retrouvé à deux recherches de suite au même endroit. La trajectoire repart alors de la nouvelle position, sans trait à travers l'image.
- **Cadre rouge en pointillés** et « (hors du champ) » quand l'objet est **sorti de l'image** : il a été perdu alors que son cadre touchait le bord, ou son cadre est passé à plus de moitié hors de l'image. Le suivi s'arrête aussitôt, au lieu de s'accrocher à ce qui reste dans l'image (votre main, votre visage...). Comme un objet sorti ne peut revenir que par un bord, l'application ne le cherche plus que **le long des bords de l'image** et reprend le suivi quand il y réapparaît.

Dans le panneau **Résultat**, chaque objet a son paragraphe, précédé d'un carré de sa couleur et de son nom :

| Valeur | Signification |
|---|---|
| **en mouvement / immobile** | L'objet passe « en mouvement » quand il s'éloigne de sa position de repos de plus du **seuil de mouvement** (20 mm sur le sol, ou 15 px dans l'image sans repère du sol). Il redevient « immobile » quand il reste dans un rayon de la moitié du seuil pendant 0,5 s. Voir [6.5](#65-événements-de-mouvement). |
| **Position** lissée, **vitesse, direction** | Centre du cadre en pixels (voir [9.1](#91-coordonnées-dans-limage-en-pixels)), vitesse et direction, estimés par un filtre de Kalman, avec la **Force du lissage** du groupe « Lissage des carrés et marqueurs » (voir [6.4](#64-lisser-les-positions-des-carrés-et-des-marqueurs)). La direction est en degrés, de −180 à 180 : 0° vers la droite, 90° vers le bas de l'image. |
| **Confiance** | Ressemblance du contenu du cadre avec l'objet tel qu'il était à la sélection, de 0 à 100 %. Elle suit lentement les changements progressifs (éclairage, rotation), mais chute si le cadre glisse sur le fond ou s'accroche à un autre objet passé devant. Sous 45 % pendant quelques images, l'objet est déclaré perdu, même si le tracker croit encore le suivre. |
| **Sol** | Si le repère du sol est défini : position en mm, vitesse en mm/s et direction sur le sol, comptée de l'axe X vers l'axe Y (voir [section 8](#8-définir-le-repère-du-sol)) |

L'onglet **Suivi** indique le nombre d'objets suivis et le nombre d'objets perdus. Ces valeurs sont aussi envoyées en UDP (liste `tracked`, voir [10.3](#103-format-des-messages)).

### 6.3 Limites

Le suivi d'objets encadrés est très pratique, mais moins fiable que les marqueurs ArUco :

- **Un objet peut être perdu** s'il est entièrement caché, sort de l'image, bouge très vite ou ressemble trop au fond. L'application le recherche dans toute l'image (le long des bords seulement s'il est sorti de l'image) et reprend le suivi dès qu'il réapparaît. Cette recherche compare l'objet à son aspect mémorisé : un objet qui revient **très tourné**, **beaucoup plus grand ou plus petit** (au-delà d'environ 20 %), ou très différemment éclairé peut ne pas être reconnu. Retirez-le et ajoutez-le de nouveau dans ce cas.
- **Des objets très ressemblants** peuvent être confondus, en particulier lors d'une reprise automatique ou quand ils se croisent. Donnez-leur si possible des aspects différents (couleurs, motifs).
- **Le cadre peut glisser** sur le fond, ou s'accrocher à un autre objet qui passe devant. L'**indice de confiance** le détecte dans la plupart des cas, et l'objet est alors déclaré perdu avec le message « Le cadre ne ressemble plus à l'objet sélectionné ». Un objet qui change très vite d'aspect (retourné d'un coup) peut aussi faire chuter la confiance.
- **Pas d'orientation** : le cadre reste droit. Seule la direction du déplacement est connue, pas celle vers laquelle l'objet est tourné.
- **Changer de résolution arrête le suivi** de tous les objets : il faut les ajouter de nouveau.
- **Vitesse de l'application** : chaque objet demande environ **25 ms de calcul par image avec CSRT** (environ 5 ms avec KCF, 10 ms avec Nano, 15 ms avec Vit). Les objets sont suivis en parallèle, dans le fil de traitement : l'interface reste réactive, mais le nombre d'images traitées par seconde baisse avec le nombre d'objets (environ 13 i/s avec 3 objets en 1280 × 960 sur un processeur à 4 cœurs, d'après un test sur images de synthèse). Choisissez **Nano** ou **KCF** pour suivre plusieurs objets si l'image devient saccadée. L'onglet **Suivi** le rappelle à partir de 3 objets.

Conseils : un objet bien contrasté et texturé (motif, logo, couleurs) se suit mieux qu'un objet uni. Un éclairage stable et une exposition courte (moins de flou, voir [Régler l'image de la webcam](#régler-limage-de-la-webcam)) aident aussi.

### 6.4 Lisser les positions des carrés et des marqueurs

L'onglet **Suivi** contient aussi le groupe **Lissage des carrés et marqueurs**, actif par défaut. Il suit dans le temps le **carré coloré principal** (le plus grand) et **chaque marqueur ArUco**, reconnu par son identifiant. Ses réglages (force et maintien après perte) s'appliquent aussi à l'**objet encadré**. Le lissage apporte :
- une **position lissée**, qui tremble moins d'une image à l'autre ;
- la **vitesse** et la **direction** du déplacement, en mm/s sur le sol si le repère du sol est défini, sinon en pixels par seconde ;
- une **position prédite** quand un objet n'est plus détecté pendant un court instant : il est supposé continuer à la même vitesse ;
- une **trajectoire** dessinée sur l'image : vert clair pour le carré, rose pour les marqueurs.

| Réglage | Valeur par défaut | Rôle |
|---|---|---|
| **Force du lissage** | 50 | De 0 (positions brutes) à 100 (très lissées). Plus fort, les positions sont plus stables, mais elles **prennent du retard** quand l'objet accélère ou tourne. |
| **Maintien après perte** | 0,5 s | Durée pendant laquelle un objet qui n'est plus détecté reste affiché à sa position prédite, avant d'être oublié |
| **Afficher les trajectoires**, **Durée des trajectoires** | oui, 10 s | Affichage et longueur des trajectoires |

Mesures sur des données de synthèse (mesures bruitées à 1,5 px près, 30 images/s) :

| Force | Tremblement à l'arrêt | Retard au démarrage (200 px/s) | Erreur de vitesse (cercle à 236 px/s) |
|---|---|---|---|
| 0 | 1,9 px | 4 px | 67 px/s |
| 50 | 1,4 px | 5 px | 30 px/s |
| 75 | 1,1 px | 9 px | 48 px/s |
| 100 | 1,0 px | 13 px | 83 px/s |

Le gain principal est donc la **vitesse** et la **position prédite**. Pour un robot lent, une force de 75 donne des positions plus stables. Pour un robot rapide ou qui tourne beaucoup, restez vers 50 ou moins.

Dans le panneau **Résultat**, la vitesse s'ajoute sous chaque carré et marqueur. Un objet momentanément perdu apparaît comme `ID 7 : non détecté, position prédite (x, y) px`, et sur l'image comme un cercle en pointillés marqué « (prédit) ». Les valeurs brutes (position, angle, sol, pose 3D) restent celles de la détection. Seules la vitesse et la position prédite viennent du lissage. Le tout est envoyé en UDP dans la liste `tracks` (voir [10.3](#103-format-des-messages)).

Changer de résolution remet le lissage à zéro.

Si vous décochez le groupe, la position de l'objet encadré redevient celle du tracker, sans prédiction. Sa vitesse reste estimée par le filtre, avec une force moyenne, car la vitesse brute est trop bruitée pour distinguer un objet immobile d'un objet en mouvement. Pour la même raison, une force de 0 rend la vitesse instable.

### 6.5 Événements de mouvement

L'application signale les moments où chaque objet suivi **se met en mouvement**, **s'arrête**, est **perdu** ou **retrouvé**. Ces événements s'affichent dans le **Journal des événements** de l'onglet **Suivi**, le plus récent en haut, avec le nom et la couleur de l'objet :

```
20:53:16.1  Objet 1 : Début de mouvement depuis (249, 298) px
20:53:16.1  Robot bleu : Début de mouvement depuis (599, 152) px
20:53:18.5  Objet 1 : Fin de mouvement en (342, 299) px — déplacement 93 px, parcours 95 px, durée 2.0 s
20:53:20.6  Robot bleu : Objet perdu (dernière position (599, 245) px)
20:53:23.1  Robot bleu : Objet retrouvé en (402, 449) px
```

- **Début de mouvement** : l'objet s'est éloigné de sa position de repos de plus du **seuil de mouvement**. La position indiquée est celle d'où il est parti.
- **Fin de mouvement** : l'objet est resté dans un rayon de la moitié du seuil pendant 0,5 s. L'événement donne la position d'arrivée, le **déplacement** (distance en ligne droite depuis le départ), le **parcours** (longueur du chemin suivi) et la **durée** du mouvement.
- **Objet perdu** : une fois la position prédite épuisée (**Maintien après perte**).
- **Objet retrouvé** : dès que le suivi reprend, par le tracker ou par la recherche dans toute l'image.

| Réglage | Valeur par défaut | Rôle |
|---|---|---|
| **Seuil de mouvement (sol)** | 20 mm | Utilisé quand le repère du sol est défini : positions et distances en mm |
| **Seuil de mouvement (image)** | 15 px | Utilisé sans repère du sol : positions et distances en pixels |

Un seuil plus petit signale les moindres déplacements, mais risque de réagir au tremblement du cadre. Un seuil plus grand ne signale que les vrais déplacements.

**Effacer** vide le journal. **Enregistrer...** l'exporte en **CSV** (séparateur `;`), du plus ancien au plus récent, avec les colonnes `heure`, `horodatage_ms`, `objet_id`, `objet` (nom), `type` (`motion_start`, `motion_stop`, `lost`, `found`, `zone_enter`, `zone_exit`), `x`, `y`, `unite` (`mm` ou `px`), `deplacement`, `parcours`, `duree_s`, `zone` (nom de la zone, pour les entrées et sorties, y compris celles du jeu : `game_zone_enter`, `game_zone_exit`, et la zone d'arrivée de `game_win`) et `compte` (numéro de la faute pour `game_fault`, nombre de fautes pour `game_win`, numéro du trésor pour `game_treasure`, total pour `game_over`). Le journal garde les 500 derniers événements et n'est pas conservé entre deux lancements.

Les événements sont aussi envoyés en UDP, dans la liste `events` (voir [10.3](#103-format-des-messages)).

---

## 7. Calibrer la caméra

La calibration mesure les caractéristiques optiques de votre webcam : focale, centre de l'image et déformation de l'objectif. Elle est **indispensable pour obtenir des positions en millimètres**. Elle se fait **une seule fois** par caméra et dure environ 5 minutes.

### 7.1 Préparer le damier

1. Ouvrez l'onglet **Calibration**.
2. Laissez les valeurs par défaut : **9 × 6 coins intérieurs**, soit un damier de 10 × 7 cases.
   - Les **coins intérieurs** sont les points où quatre cases se touchent. Un damier de 10 × 7 cases a 9 × 6 coins intérieurs.
3. Cliquez sur **Générer un damier...** et enregistrez l'image (`damier_9x6.png`).
4. Imprimez-la **à 100 %**, sans « ajuster à la page ».
5. Collez la feuille **parfaitement à plat** sur un support rigide (carton épais, planche, vitre). Un damier gondolé donne une mauvaise calibration.
6. **Mesurez une case à la règle**, précisément, et saisissez la valeur dans **Côté d'une case** (en mm).

Si vous avez déjà un damier, saisissez son nombre de coins intérieurs et la taille de ses cases.

### 7.2 Faire les captures

1. Cochez **Mode calibration**.
2. Montrez le damier à la caméra. Quand il est **entièrement visible**, ses coins s'affichent **en bleu clair** et le texte `Damier détecté` apparaît en haut de l'image. Sinon, le texte `Damier non détecté` s'affiche en rouge.
3. Capturez des images :
   - **manuellement**, avec le bouton **Capturer**, quand le damier est bien placé ;
   - ou **automatiquement**, en cochant **Capture automatique (toutes les 2 s)** : il suffit alors de déplacer lentement le damier.

   Un **cadre blanc** autour de l'image confirme chaque capture. Le compteur augmente sous les boutons.
4. Faites **15 à 25 captures**, en variant les positions :

   | À faire | Pourquoi |
   |---|---|
   | Damier proche et damier éloigné | Mesure de la focale |
   | Damier au centre, dans les 4 coins et sur les bords de l'image | Mesure de la déformation, plus forte sur les bords |
   | Damier incliné vers la gauche, la droite, le haut et le bas (jusqu'à 45° environ) | Indispensable pour un calcul précis |
   | Damier **immobile** au moment de la capture | Une image floue fausse le calcul |

   Une série de captures toutes prises de face, au même endroit, donne une mauvaise calibration, même si l'erreur affichée paraît faible.

### 7.3 Lancer le calcul

1. Cliquez sur **Calibrer**. Le bouton est actif à partir de 5 captures. Le calcul prend quelques secondes.
2. Une fenêtre indique l'**erreur RMS**, en pixels :

   | Erreur RMS | Qualité |
   |---|---|
   | moins de 0,5 px | Très bonne |
   | 0,5 à 1 px | Correcte |
   | plus de 1 px | À refaire : damier gondolé, images floues ou mauvaise taille de damier |

3. La calibration est **enregistrée automatiquement**, pour la caméra choisie, et rechargée à chaque lancement et à chaque fois que vous revenez à cette caméra. Le texte `Caméra calibrée` s'affiche en vert dans l'onglet, avec le chemin du fichier.
4. Décochez **Mode calibration** : il ralentit l'application et n'est plus utile.

Pour recommencer, cliquez sur **Effacer les captures** puis refaites une série.

### 7.4 Quand refaire la calibration ?

- Pour chaque **nouvelle webcam** : chaque caméra a sa propre calibration, conservée quand vous passez de l'une à l'autre (liste **Caméra**). Il n'est donc pas nécessaire de recalibrer en revenant à une caméra déjà calibrée.
- Si l'objectif a un **zoom** ou une **mise au point manuelle** et que vous les modifiez.
- Si vous choisissez une **résolution d'un autre format d'image** (de 4:3 à 16:9, par exemple). La pose 3D est alors désactivée et le panneau **Résultat** affiche `Pas de pose 3D : calibration faite en ...`. Revenez à une résolution du format de la calibration, ou recalibrez.

Changer de résolution **dans le même format** (de 1280 x 960 à 640 x 480, par exemple) ne demande pas de recalibrer : la calibration est adaptée automatiquement. Pour la meilleure précision, calibrez de préférence dans la résolution que vous utiliserez le plus.

### 7.5 Vérifier la calibration

Posez un marqueur à une distance connue de la caméra, mesurée au mètre ruban, face à l'objectif. La **distance** affichée doit correspondre, à 1 ou 2 % près. Sinon, vérifiez la **Taille du marqueur** et le **Côté d'une case**.

---

## 8. Définir le repère du sol

Par défaut, les positions sont données en pixels dans l'image. Une fois le **repère du sol** défini, l'application donne aussi la position de chaque carré et de chaque marqueur **en millimètres sur le sol**, ainsi que son orientation sur le sol. C'est le plus pratique pour piloter un robot qui roule sur une table ou par terre.

Ce repère fonctionne **sans calibration**, pour le carré coloré comme pour les marqueurs ArUco. Si la caméra est calibrée, la déformation de l'objectif est en plus corrigée, ce qui améliore la précision sur les bords de l'image.

### 8.1 Préparer le rectangle de référence

Il faut 4 repères au sol formant un **rectangle de dimensions connues** :
- 4 morceaux de ruban adhésif, les coins d'une table, d'une plaque ou d'une grande feuille...
- **Plus le rectangle est grand, plus les mesures sont précises.** Idéalement, il couvre toute la zone où roule le robot.
- Vérifiez que c'est bien un rectangle : ses deux diagonales doivent avoir la même longueur.
- Mesurez ses deux côtés au mètre ruban, le plus précisément possible.
- Fixez la caméra solidement : **si elle bouge, le repère doit être redéfini**.

### 8.2 Définir le repère

1. Ouvrez l'onglet **Sol**.
2. Saisissez les deux dimensions du rectangle :
   - **Longueur X (point 1 → 2)** : le côté qui va du point 1 au point 2 ;
   - **Longueur Y (point 1 → 4)** : le côté qui va du point 1 au point 4.
3. Cliquez sur **Définir les 4 points**.
4. Cliquez sur les 4 coins du rectangle dans l'image, **dans l'ordre** indiqué en bas de l'image et dans l'onglet :
   1. l'**origine** (0, 0) ;
   2. le coin **sur l'axe X** ;
   3. le **coin opposé** à l'origine ;
   4. le coin **sur l'axe Y**.

   Chaque point cliqué s'affiche, numéroté. Cliquez sur **Annuler** pour recommencer.

Ordre conseillé, **tel que vous voyez la scène à l'écran** : 1 en bas à gauche, 2 en bas à droite, 3 en haut à droite, 4 en haut à gauche. Les axes sont alors orientés comme sur un graphique : X vers la droite et Y vers le haut de l'écran. Les angles se comptent dans le sens inverse des aiguilles d'une montre.

Une fois les 4 points cliqués, le rectangle s'affiche sur l'image en pointillés orange, avec l'**axe X en rouge** et l'**axe Y en vert**, et les lettres O, X et Y. La case **Afficher la zone sur l'image** le masque si besoin. Pendant la saisie, un clic sur l'image place un point au lieu de choisir la couleur du carré.

Le repère est **enregistré immédiatement**, pour la caméra choisie, et rechargé au lancement suivant. Chaque caméra a son propre repère : si vous passez à une autre caméra, définissez son repère une fois, puis chacune retrouve le sien. Changer les dimensions plus tard ne demande pas de recliquer les points. **Effacer le repère** le supprime.

Si la caméra est calibrée, l'onglet indique aussi **la hauteur de la caméra au-dessus du sol**, déduite des 4 points (par exemple « Caméra à 1109 mm du sol »). Comparez-la à une mesure au mètre ruban : un écart important révèle une erreur dans les longueurs saisies, l'ordre des points ou la calibration.

### 8.3 Lire les positions au sol

Dans le panneau **Résultat** apparaissent :
- pour le carré : `Sol : X 698  Y 48 mm, angle 20.7°` ;
- pour chaque marqueur : `sol X 149  Y 101 mm, cap -89.8°`.

| Valeur | Signification |
|---|---|
| **X, Y** | Position du centre en mm, depuis l'origine (point 1), le long des axes X et Y. Les valeurs sont négatives ou dépassent les dimensions du rectangle quand l'objet est en dehors. |
| **cap** (marqueur) | Direction du **bord supérieur** du marqueur, en degrés de −180 à 180, comptée de l'axe X vers l'axe Y. 0° = vers +X, 90° = vers +Y. |
| **angle** (carré) | Orientation du carré, de 0 à 90° seulement (un carré est identique à lui-même tous les 90°) |

Si le haut du marqueur est collé vers l'avant du robot, le **cap** est directement la direction du robot sur le sol.

Ces valeurs sont aussi envoyées en UDP (champ `floor`, voir [10.3](#103-format-des-messages)).

### 8.4 Précision et limites

- **La hauteur compte.** Le repère est défini au niveau du sol. Un marqueur collé sur le dessus du robot est plus haut, donc plus proche de la caméra, et sa position est décalée vers l'extérieur de l'image. Le décalage vaut environ *hauteur du marqueur × distance au point situé sous la caméra ÷ hauteur de la caméra*. Par exemple, avec une caméra à 2 m, un marqueur à 10 cm de haut et à 1 m du point sous la caméra, le décalage est d'environ 5 cm. Pour le corriger :
  - **si la caméra est calibrée**, saisissez la **Hauteur des repères** (onglet **Sol**), mesurée du sol jusqu'à la surface des carrés et des marqueurs. L'application calcule la position de la caméra à partir des 4 points du repère, puis place chaque repère là où son rayon de vue traverse le plan situé à cette hauteur. L'onglet affiche alors « Positions corrigées pour des repères à ... mm du sol ». Sur une scène de synthèse, des marqueurs à 100 mm de haut sont décalés de 36 à 100 mm sans correction, et de 1 mm avec ;
  - **sans calibration**, la hauteur saisie est ignorée (l'onglet le signale) : collez les marqueurs **le plus bas possible** sur le robot, ou définissez le rectangle de référence **à la hauteur des marqueurs**, par exemple avec 4 cales de cette hauteur posées aux coins.

  La hauteur s'applique aux carrés et aux marqueurs, ainsi qu'à leurs positions lissées. Les objets encadrés (section 6) restent rapportés au niveau du sol. Tous les repères doivent être à la même hauteur.
- **Format d'image** : comme pour la calibration, le repère reste valable si vous changez de résolution dans le même format (4:3, 16:9...). Dans un autre format, les positions au sol sont désactivées et le panneau **Résultat** affiche `Pas de position au sol : repère défini en ...`.
- **Précision des clics** : cliquez au centre des repères. Une haute résolution (liste **Résolution**) permet de cliquer plus précisément.
- **Effet miroir** : sans effet sur le repère, les clics sont convertis automatiquement.

### 8.5 Définir des zones

Une **zone** est un rectangle fixe de l'image, avec un nom : « Garage », « Départ », « But rouge »... L'application indique en permanence ce qui se trouve dans chaque zone, et signale chaque **entrée** et chaque **sortie**. Les zones ne demandent ni calibration ni repère du sol.

**Ajouter une zone**

1. Ouvrez l'onglet **Zones** et cliquez sur **Ajouter une zone**.
2. **Tracez un rectangle** sur l'image (cliquer-glisser).
3. Ajustez-le si besoin avec ses **8 poignées**, ou en faisant glisser son intérieur.
4. Validez : **Valider la zone**, touche **Entrée** ou double-clic dans le cadre. **Échap** annule.
5. Donnez-lui un **nom** (« Zone 1 », « Zone 2 »... est proposé), puis cliquez sur **OK**.

La zone s'affiche sur l'image, teintée de sa couleur, avec son nom. La teinte se renforce et le nombre d'occupants s'ajoute au nom quand quelque chose s'y trouve.

**Gérer les zones**
- **Renommer** : double-cliquez sur la zone dans la liste.
- **Retirer** supprime la zone sélectionnée dans la liste, **Tout retirer** toutes les zones de la caméra (après confirmation).
- **Afficher les zones sur l'image** : décochez pour les masquer. Elles continuent de fonctionner.

**Ce qui est surveillé** : les objets suivis (section 6), les marqueurs ArUco (section 5) et le carré principal (section 4). C'est leur **centre** qui compte : un objet est dans la zone quand son centre y est.

**Occupation et événements**
- Sous la liste, chaque zone est suivie de ses occupants, par exemple « **Garage** : Robot bleu, ArUco 7 », ou « vide ».
- Chaque entrée et chaque sortie est inscrite dans le **Journal des événements** de l'onglet **Suivi**, avec la couleur de la zone :

  ```
  22:13:48.667  Robot bleu : Entre dans la zone « Garage » en (600, 450) px
  22:13:50.486  Robot bleu : Sort de la zone « Garage » (dernière position (583, 523) px)
  ```

- Une **entrée** est signalée dès que le centre passe dans la zone. Une **sortie** l'est quand il en est resté dehors, ou n'a plus été vu, pendant **0,3 s**, avec une petite marge autour de la zone. Un objet qui tremble sur le bord, ou un marqueur qui n'est pas détecté sur une image, ne provoque donc pas une suite d'entrées et de sorties.
- Les événements, la liste des zones et leurs occupants sont aussi **envoyés en UDP** (voir [10.3](#103-format-des-messages)).

**Bon à savoir**
- Une zone est **fixe dans l'image** : si la caméra bouge, elle ne correspond plus au même endroit du sol.
- Les zones sont **mémorisées pour chaque caméra**, comme le repère du sol, et restent valables si vous changez de résolution dans le même format d'image (4:3, 16:9...).
- Une zone tracée sur l'image ne suit pas la perspective du sol : vue de biais, une zone rectangulaire au sol apparaît comme un trapèze. Tracez le rectangle qui l'englobe. Si le repère du sol est défini, les coins de la zone sur le sol sont envoyés en UDP.
- **Effet miroir** : sans effet sur les zones, le tracé est converti automatiquement.

### 8.6 Jeu : détecter le robot dans une zone

Le menu **Jeu**, **Détection robot dans zone...**, ouvre une petite fenêtre qui indique en permanence si le robot est **entièrement** dans une zone : il ne suffit pas que son centre y soit, tout son cadre doit être à l'intérieur.

**Préparer**
1. Ajoutez le robot comme **objet suivi** : onglet **Suivi**, **Ajouter des objets**, rectangle autour du robot, **Entrée** (voir [section 6](#6-suivre-des-objets-encadrés)). Encadrez-le au plus près : c'est ce cadre qui doit tenir dans la zone.
2. Tracez au moins une **zone** (voir [8.5](#85-définir-des-zones)), plus grande que le robot.

Tant que l'un des deux manque, la fenêtre de jeu l'indique et **Démarrer** reste grisé.

**Jouer**
1. Choisissez le **Robot** (parmi les objets suivis) et la **Zone** : une zone précise, ou **N'importe quelle zone**.
2. Cliquez sur **Démarrer**.
3. L'état s'affiche en grand dans la fenêtre de jeu et en **bandeau en haut de l'image**, donc aussi dans la [fenêtre du joueur](#fenêtre-du-joueur) :
   - **vert** : « Robot est dans la zone « Garage » ». La zone atteinte est entourée d'un trait vert épais ;
   - **rouge** : « hors zone », y compris quand le robot est à cheval sur le bord ;
   - **orange** : « non trouvé », quand le robot n'est plus suivi (perdu, sorti de l'image).
4. Sous l'état, le **temps passé dans la zone** et le **nombre d'entrées** depuis le départ. **Remettre à zéro** remet ces compteurs à zéro sans arrêter le jeu.
5. **Arrêter** termine la partie. Changer de robot ou de zone pendant la partie la recommence.

**Bon à savoir**
- L'état ne change qu'après **0,2 s** de stabilité : un cadre qui tremble sur le bord ne provoque pas une suite d'entrées et de sorties. Une position seulement prédite (robot caché un instant) ne change rien.
- Chaque entrée et chaque sortie est inscrite dans le **Journal des événements** (`Jeu : entièrement dans la zone « Garage »`) et envoyée en UDP (`game_zone_enter`, `game_zone_exit`). Pendant la partie, chaque message UDP contient aussi l'objet `game` (voir [10.3](#103-format-des-messages)).
- La précision dépend du cadre du suivi : un cadre trop large fait paraître le robot « hors zone » alors qu'il est dedans. Resélectionnez-le au plus près si besoin.

### 8.7 Jeu : 1, 2, 3 Soleil

Le menu **Jeu**, **1, 2, 3 Soleil...**, arbitre la version robot du jeu de cour de récréation : le robot doit atteindre la **zone d'arrivée** en n'avançant qu'au **feu vert**. S'il bouge pendant « **Soleil !** », c'est une **faute**, qui ajoute **5 s** à son score.

**Préparer**
1. Ajoutez le robot comme **objet suivi** (onglet **Suivi**), encadré au plus près.
2. Tracez la **zone d'arrivée** dans l'onglet **Zones**, un peu plus grande que le robot, à distance du point de départ.

**Jouer**
1. Choisissez le **Robot**, l'**Arrivée** et la **Difficulté**, puis cliquez sur **Démarrer**. Ces choix sont figés pendant la partie.
2. Un **compte à rebours** de 3 s s'affiche (« départ dans 3, 2, 1 »), puis le chrono démarre.
3. Les phases alternent, avec des durées tirées au hasard, en **bandeau en haut de l'image** (donc aussi dans la [fenêtre du joueur](#fenêtre-du-joueur)) :
   - **vert, « AVANCEZ ! »** : le robot peut rouler ;
   - **rouge, « SOLEIL ! »** : le robot doit s'arrêter. Il dispose d'un court **temps de réaction** ; ensuite, s'il s'éloigne de sa position de plus que le **seuil de mouvement** (onglet **Suivi** : 20 mm sur le sol, ou 15 px sans repère), c'est une faute ;
   - **violet, « FAUTE ! »** pendant 1,5 s après chaque faute. Une seule faute est comptée par phase « Soleil ! ».
4. La partie se termine quand le robot est **entièrement** dans la zone d'arrivée (entourée de pointillés blancs), confirmé après 0,2 s. Le bandeau devient **bleu** : « GAGNÉ ! 17,6 s — 1 faute(s) — score 22,6 s ». Le score est le temps plus 5 s par faute : le plus petit gagne.
5. **Démarrer** relance une partie, **Arrêter** l'interrompt.

| Difficulté | Feu vert | « Soleil ! » | Temps pour s'arrêter |
|---|---|---|---|
| Facile | 4 à 7 s | 2 à 3 s | 0,8 s |
| Normal | 2 à 5 s | 2 à 4 s | 0,5 s |
| Difficile | 1 à 3 s | 2 à 5 s | 0,3 s |

La fenêtre du jeu affiche aussi le chrono, les fautes, le score et le temps restant avant le prochain changement de phase. Cette dernière information est réservée à l'arbitre : elle n'apparaît pas dans le bandeau.

**Bon à savoir**
- Le cadre de suivi tremble légèrement même quand le robot est arrêté. Si des fautes sont comptées alors qu'il ne bouge pas, augmentez le **seuil de mouvement** (onglet **Suivi**) avant de démarrer la partie.
- Si le robot n'est plus suivi (perdu, sorti de l'image), aucune faute ne peut être comptée ; la fenêtre du jeu le signale.
- Les phases, les fautes et l'arrivée sont inscrites dans le **Journal des événements** et envoyées en UDP (`game_green`, `game_red`, `game_fault`, `game_win`) ; pendant la partie, chaque message contient l'objet `game` avec `mode` = `soleil` (voir [10.3](#103-format-des-messages)).

### 8.8 Jeu : chasse au trésor

Le menu **Jeu**, **Chasse au trésor...**, cache un **trésor invisible** sur le terrain. Le joueur guide le robot grâce à des indices « **chaud / froid** » ; quand le robot passe sur le trésor, il est trouvé, marqué d'une **étoile dorée**, et un nouveau trésor est caché ailleurs. Le but : en trouver le plus possible avant la fin du temps.

**Préparer**
1. Ajoutez le robot comme **objet suivi** (onglet **Suivi**).
2. Facultatif : tracez une zone **Terrain** dans l'onglet **Zones**, pour que les trésors ne soient cachés que là (pas sur les murs, les meubles ou hors de la zone de jeu). Sans zone, les trésors sont cachés dans toute l'image.

**Jouer**
1. Choisissez le **Robot**, le **Terrain** (une zone, ou **Toute l'image**), la **Durée** (1, 2, 3 ou 5 minutes) et la **Difficulté**, puis cliquez sur **Démarrer**. Ces choix sont figés pendant la partie.
2. Le bandeau en haut de l'image (donc aussi dans la [fenêtre du joueur](#fenêtre-du-joueur)) donne l'indice, le temps restant et le nombre de trésors trouvés. Sa couleur va du bleu (loin) au rouge (tout près) :

   | Indice | Distance au trésor (part de la diagonale du terrain) |
   |---|---|
   | **Glacial** | plus de 60 % |
   | **Froid** | 40 à 60 % |
   | **Tiède** | 25 à 40 % |
   | **Chaud** | 12 à 25 % |
   | **BRÛLANT** | moins de 12 % |

   « **▲ plus chaud** » ou « **▼ plus froid** » s'ajoute quand le robot se rapproche ou s'éloigne.
3. Quand le **centre** du robot passe à moins du rayon du trésor, le bandeau affiche « **TRÉSOR TROUVÉ !** » (doré) pendant 1,5 s, une étoile dorée marque l'endroit, et un nouveau trésor est caché, loin du robot.
4. À la fin du temps, le bandeau devient bleu : « **TERMINÉ ! 7 trésors trouvés** ». **Démarrer** relance une partie, **Arrêter** l'interrompt.

| Difficulté | Rayon du trésor (part du petit côté du terrain) |
|---|---|
| Facile | 12 % |
| Normal | 8 % |
| Difficile | 5 % |

**Bon à savoir**
- Le trésor n'est jamais caché tout au bord du terrain (marge de 8 %).
- Si le robot n'est plus suivi, le bandeau affiche « Robot non trouvé » et le temps continue de s'écouler.
- Les trésors trouvés et la fin de la partie sont inscrits dans le **Journal des événements** et envoyés en UDP (`game_treasure`, `game_over`). Pendant la partie, l'objet `game` a `mode` = `treasure` et contient aussi la position du trésor (`treasure_px`) : un programme peut ainsi guider un robot autonome (voir [10.3](#103-format-des-messages)).

### 8.9 Les autres jeux : fonctionnement commun

Les jeux suivants du menu **Jeu** fonctionnent tous de la même façon :

- Chaque jeu ouvre une **fenêtre** qui rappelle la règle et propose ses réglages : objets suivis (le robot, une balle, un second robot...), numéro d'un marqueur ArUco, zones (une zone, ou plusieurs zones à cocher dans l'ordre de l'onglet **Zones**), et choix (durée, difficulté...). Les zones tracées après l'ouverture de la fenêtre y sont ajoutées, cochées.
- **Démarrer** vérifie que tout est prêt : s'il manque un objet suivi ou une zone, un message dit quoi faire. Les réglages sont ensuite figés jusqu'à la fin de la partie.
- L'état s'affiche **en grand dans la fenêtre du jeu** et **en bandeau en haut de l'image**, donc aussi dans la [fenêtre du joueur](#fenêtre-du-joueur), avec les zones et repères du jeu dessinés sur l'image.
- Les événements de la partie sont inscrits dans le **Journal des événements** et envoyés en UDP avec le type `game_event` (`code`, `text`, `count`). Pendant la partie, chaque message contient l'objet `game`, avec `mode` (le nom court du jeu) et les champs décrits dans la section du jeu.
- **Un seul jeu tourne à la fois** : en démarrer un arrête les autres. **Arrêter** interrompt la partie ; à la fin d'une partie, le résultat reste affiché et **Démarrer** en relance une.
- Comme les jeux précédents, ils s'appuient sur les **objets suivis** (onglet **Suivi**) ; le parking et « Jacques a dit », qui ont besoin du **cap** du robot, utilisent un **marqueur ArUco** (onglet **ArUco**, **Activer la détection** cochée).

### 8.10 Jeu : course contre la montre

Le menu **Jeu**, **Course contre la montre...** : le robot franchit des **portes** (des zones) **dans l'ordre**, sur un ou plusieurs tours, le plus vite possible.

**Préparer** : ajoutez le robot comme objet suivi, et tracez au moins **deux portes** dans l'onglet **Zones**, dans l'ordre du parcours (l'ordre de la liste des zones est l'ordre des portes).

**Jouer**
1. Choisissez le **Robot**, cochez les **Portes** du parcours et le nombre de **Tours** (1 à 3), puis **Démarrer**.
2. Le bandeau affiche « Course : partez quand vous voulez ». Le **chrono démarre au premier mouvement** du robot (déplacement supérieur au seuil de mouvement de l'onglet **Suivi**).
3. Les portes sont numérotées sur l'image ; la **prochaine porte** est entourée de **jaune**, celles déjà franchies de vert. Une porte est franchie quand le **centre** du robot y entre. Le bandeau indique « Porte 2/4 — tour 1/2 — 12,3 s ».
4. Entrer dans une **mauvaise porte** (pas celle attendue) coûte **5 s** : « MAUVAISE PORTE ! +5 s » en violet. La porte attendue ne change pas.
5. Un tour se termine à la dernière porte ; le suivant recommence à la première. Après le dernier tour, le bandeau devient bleu : « ARRIVÉE ! 25,7 s », ou « NOUVEAU RECORD ».

Le **tracé** de la course est dessiné sur l'image. Le **record** est conservé pour chaque parcours (portes et nombre de tours) tant que l'application est ouverte.

**UDP** : événements `race_start`, `race_gate` (`count` = numéro de la porte), `race_penalty` et `race_finish` ; objet `game` avec `mode` = `race`, `robot_id`, `gates` (nombre de portes), `next_gate` (numéro de la prochaine porte), `next_zone_id`, `lap`, `laps`, `started`, `finished`, `elapsed_s` (pénalités comprises), `penalties` et `best_s`.

### 8.11 Jeu : le parking

Le menu **Jeu**, **Le parking...** : garer le robot dans une case, **tourné dans la bonne direction**. Ce jeu a besoin du **cap** du robot : il utilise un **marqueur ArUco** collé sur le robot, bord supérieur vers l'avant (onglet **ArUco**, **Activer la détection** cochée).

**Préparer** : tracez la **place de parking** dans l'onglet **Zones**, nettement plus grande que le robot.

**Jouer**
1. Saisissez le numéro du **Marqueur du robot**, choisissez la **Place de parking** et le **Cap demandé** (vers le haut, la droite, le bas ou la gauche de l'écran), puis **Démarrer**.
2. La **case cible** est entourée de **jaune** dans la place, avec une flèche qui montre le cap demandé. Le bandeau indique le niveau et l'écart de cap en direct.
3. Garez le robot et **arrêtez-le** : le stationnement est jugé après **1 s d'immobilité dans la place**. Un arrêt ailleurs (pendant les manœuvres) n'est pas jugé.
4. **Réussi** si le marqueur est **entièrement dans la case cible** et que son cap est à moins de la **tolérance** : « Bien garé ! +92 points » (vert). Les points (10 à 100) diminuent avec l'écart au centre de la case et l'écart de cap. Sinon : « Raté : pas entièrement dans la case » ou « Raté : cap décalé de 35° » (rouge). Pour réessayer, déplacez ou tournez le robot, puis arrêtez-le de nouveau.
5. À chaque réussite, on passe au niveau suivant. Après le niveau 5 : « PARKING TERMINÉ ! 485 points » (bleu).

| Niveau | Case cible (part de la place) | Tolérance de cap |
|---|---|---|
| 1 | 100 % | 30° |
| 2 | 85 % | 25° |
| 3 | 70 % | 20° |
| 4 | 55 % | 15° |
| 5 | 40 % | 10° |

Si le marqueur n'est pas vu (détection ArUco désactivée, marqueur caché ou trop petit), le bandeau affiche « Marqueur n° 7 non vu » en orange.

**UDP** : événements `parking_success` (`count` = points), `parking_miss` (`count` = essais ratés) et `parking_finish` (`count` = score) ; objet `game` avec `mode` = `parking`, `marker_id`, `zone_id`, `level`, `levels`, `score`, `misses`, `target_heading_deg` (angle du marqueur demandé : 0 vers le haut, 90 vers la droite), `tolerance_deg`, `seen`, `heading_error_deg`, `inside` et `finished`.

### 8.12 Jeu : le fil électrique

Le menu **Jeu**, **Le fil électrique...** : suivre un **couloir** sinueux sans en sortir, comme le jeu d'adresse où l'anneau ne doit pas toucher le fil.

**Préparer**
1. Ajoutez le robot comme objet suivi, encadré au plus près.
2. Tracez le couloir dans l'onglet **Zones** avec plusieurs rectangles qui **se touchent ou se chevauchent** (par exemple un départ, deux tronçons, un coude et une arrivée), **dans l'ordre du parcours** : la **première** zone cochée est le **départ**, la **dernière** l'**arrivée**.

**Jouer**
1. Choisissez le **Robot**, cochez les zones du **Couloir** et la **Vitesse maximale** (pas de limite, moyenne ou lente), puis **Démarrer**.
2. Placez le robot **entièrement dans le départ** : le bandeau passe à « Prêt : partez quand vous voulez ». Le **chrono démarre** quand le robot quitte le départ.
3. Le couloir est teinté de bleu sur l'image. Si le **cadre du robot dépasse** du couloir pendant plus de 0,2 s, c'est une **touche** : le couloir passe au rouge, « TOUCHÉ ! Revenez dans le couloir », et **3 s** s'ajoutent au score. Une nouvelle touche ne compte qu'après être revenu dans le couloir.
4. Variante « **main tremblante** » : avec une vitesse maximale (12 % ou 6 % de la largeur de l'image par seconde), chaque **excès** coûte **2 s** (« TROP VITE ! »), et compte une fois jusqu'à ce que le robot ralentisse.
5. La partie se termine quand le robot est **entièrement dans l'arrivée** : « ARRIVÉE ! 14,1 s — 1 touche(s) — score 19,1 s ». Le score est le temps plus les pénalités : le plus petit gagne.

**UDP** : événements `wire_start`, `wire_touch` (`count` = numéro de la touche), `wire_speed` et `wire_finish` ; objet `game` avec `mode` = `wire`, `robot_id`, `phase` (`waiting`, `ready`, `running` ou `finished`), `inside`, `touches`, `overspeeds`, `speed_limit_px_s` (0 sans limite), `elapsed_s` et `score_s`.

### 8.13 Jeu : curling

Le menu **Jeu**, **Curling...** : chacun son tour, lancer le robot vers une **cible** et le laisser s'arrêter au plus près du centre. Seule la **position à l'arrêt** compte.

**Préparer** : ajoutez le robot comme objet suivi, et tracez la **cible** dans l'onglet **Zones** : son centre est le centre de la cible, et son petit côté donne le diamètre des anneaux. Prévoyez une ligne de départ assez loin.

**Jouer**
1. Choisissez le **Robot**, la **Cible**, le nombre de **Joueurs** (1 à 4) et de **Manches** (1, 3 ou 5 ; un lancer par joueur et par manche), puis **Démarrer**.
2. La cible est dessinée avec trois anneaux : **10 points** au centre (tiers du rayon), **5 points** (deux tiers), **2 points** (anneau extérieur) ; **0** en dehors.
3. « Joueur 1 : placez le robot au départ » : posez le robot ; après **1 s d'immobilité**, le bandeau affiche « Joueur 1 : lancez ! » (couleur du joueur).
4. Lancez le robot (télécommande, ou poussée à la main). Le lancer est détecté au premier déplacement, puis jugé quand le robot est **immobile depuis 1 s** : « Joueur 1 : 10 point(s) ». Une pastille de la couleur du joueur marque la position d'arrêt jusqu'à la fin de la manche. Si le robot n'est plus suivi pendant le lancer (sorti de l'image), le lancer vaut 0.
5. C'est au joueur suivant : replacez le robot au départ. Le déplacement pendant le replacement n'est pas compté comme un lancer, car le lancer suivant n'est accepté qu'après 1 s d'immobilité.
6. Après le dernier lancer : « Joueur 1 GAGNE ! » avec le classement (« TERMINÉ ! 25 points » pour un joueur seul).

**UDP** : événements `curling_throw` (`count` = points du lancer) et `curling_finish` ; objet `game` avec `mode` = `curling`, `robot_id`, `state` (`placing`, `ready`, `moving`, `result` ou `finished`), `players`, `rounds`, `round`, `player` (joueur dont c'est le tour), `scores` (liste des totaux) et `last_points`.

### 8.14 Jeu : chat et souris

Le menu **Jeu**, **Chat et souris...** : deux objets suivis jouent l'un contre l'autre, deux robots ou un robot et un objet poussé à la main. Le **chat** marque un point quand il **touche** la souris.

**Préparer** : ajoutez **deux objets suivis** (onglet **Suivi**), et, si vous voulez, tracez un **refuge** dans l'onglet **Zones**.

**Jouer**
1. Choisissez le **Joueur 1** (chat au départ), le **Joueur 2** (souris au départ), le **Refuge** (ou « Aucun refuge »), la **Durée** (1 à 5 minutes) et l'intervalle de **Changement de rôles** (20, 30 ou 60 s), puis **Démarrer**.
2. Le chat est entouré de **rouge** avec « CHAT », la souris de **bleu** avec « SOURIS ». Le bandeau donne le chat, le temps restant et le score.
3. Quand les **cadres** du chat et de la souris **se touchent**, le chat marque un point : « ATTRAPÉE ! +1 pour Rouge ». Il faut ensuite **se séparer** (et attendre 3 s) avant une nouvelle prise : un contact prolongé ne compte qu'une fois.
4. La souris dont le **centre** est dans le **refuge** (teinté de vert) ne peut pas être attrapée : « souris au refuge ».
5. À chaque intervalle, les **rôles s'inversent** : « CHANGEMENT DE RÔLES ! Bleu est le chat ». Les deux joueurs doivent se séparer avant la première prise.
6. À la fin du temps : « TERMINÉ ! Rouge gagne 3 à 2 », ou « Égalité ».

**UDP** : événements `cat_mouse_catch` (`count` = points du chat), `cat_mouse_swap` et `cat_mouse_finish` ; objet `game` avec `mode` = `cat_mouse`, `cat_id`, `mouse_id` (numéros des objets suivis), `scores` (liste de `{id, points}`), `contact`, `mouse_safe`, `remaining_s`, `next_swap_s` et `finished`.

### 8.15 Jeu : football de table

Le menu **Jeu**, **Football de table...** : le robot pousse une **balle** vers les buts. Deux équipes : rouge et bleue.

**Préparer**
1. Ajoutez le robot, puis la **balle**, comme objets suivis (une balle colorée ou texturée se suit mieux).
2. Tracez les **deux buts** dans l'onglet **Zones**, aux deux bouts du terrain.

**Jouer**
1. Choisissez la **Balle** (le deuxième objet suivi est proposé), le **But de l'équipe 1** (rouge), le **But de l'équipe 2** (bleue) et la **Durée** (2 à 10 minutes), puis **Démarrer**.
2. Les buts sont teintés de la couleur de leur équipe, et la balle est entourée de blanc. Le bandeau donne le score et le temps restant.
3. Quand le **centre de la balle** entre dans un but (pendant 0,2 s), c'est **but** pour l'équipe **adverse** : une balle dans le but rouge marque pour l'équipe bleue. « BUT pour l'équipe 2 ! 1 — 1 — replacez la balle ».
4. Une balle qui **sort de l'image** (« hors du champ » du suivi, voir [6.2](#62-ce-qui-saffiche)) est une **touche** : « TOUCHE ! — replacez la balle ».
5. Après un but ou une touche, **replacez la balle** sur le terrain : le jeu reprend quand elle est hors des buts et **immobile depuis 1 s**. Une balle sortie de l'image n'est cherchée que le long des bords : ramenez-la sur le terrain **par un bord** pour qu'elle soit retrouvée.
6. À la fin du temps : « FIN DU MATCH ! L'équipe 1 gagne 3 — 2 », ou « Égalité ».

**UDP** : événements `football_goal` (`count` = numéro de l'équipe qui marque), `football_out`, `football_kickoff` et `football_finish` ; objet `game` avec `mode` = `football`, `ball_id`, `goal1_zone_id`, `goal2_zone_id`, `scores` (`[équipe 1, équipe 2]`), `state` (`play`, `goal`, `out` ou `finished`), `outs` et `remaining_s`.

### 8.16 Jeu : ramassage

Le menu **Jeu**, **Ramassage...** : le robot pousse le plus d'objets possible dans une **zone de collecte** avant la fin du temps. Le compte se fait sur la position des **objets**, pas sur celle du robot.

**Préparer**
1. Ajoutez le **robot**, puis chaque **objet à ramasser** (cubes, balles, bouchons...), comme objets suivis : tous les objets suivis autres que le robot sont à ramasser.
2. Tracez la **zone de collecte** dans l'onglet **Zones**.

**Jouer**
1. Choisissez le **Robot**, la **Zone de collecte** et la **Durée** (1 à 5 minutes), puis **Démarrer**.
2. La zone de collecte est teintée de jaune, avec le compte « Collecte : 2/5 ». Chaque objet à ramasser est entouré d'**orange** tant qu'il est dehors, de **vert** une fois dans la zone.
3. Un objet est **ramassé** quand son **centre** est dans la zone depuis 0,3 s : « Objet « Cube » ramassé ! ». S'il en ressort, il n'est plus compté : « Objet « Balle » ressorti ».
4. Le robot lui-même dans la zone ne compte pas. Un objet caché un instant (par le robot qui le pousse) garde son état.
5. À la fin du temps : « TERMINÉ ! 4 objet(s) ramassé(s) sur 5 ».

Des objets ajoutés au suivi pendant la partie sont aussi à ramasser ; un objet retiré du suivi n'est plus compté.

**UDP** : événements `collect_in`, `collect_out` (`count` = objets dans la zone) et `collect_finish` ; objet `game` avec `mode` = `collect`, `robot_id`, `zone_id`, `in_zone` (numéros des objets dans la zone), `collected`, `total`, `remaining_s` et `finished`.

---

## 9. Lire les résultats

### 9.1 Coordonnées dans l'image (en pixels)

```
(0,0) ──────────────► x
  │
  │      ● (x, y)
  │
  ▼
  y
```

- L'origine (0, 0) est le **coin haut-gauche** de l'image. **x** augmente vers la droite, **y** vers le bas.
- Les valeurs sont données dans la **résolution réelle de la caméra**, affichée sur la première ligne du panneau Résultat (par exemple `Image : 1280 x 960`).
- Les coordonnées et les angles sont toujours ceux de l'**image réelle de la caméra**, même si **Effet miroir** est coché. Avec le miroir, un objet affiché à gauche de l'écran a donc un **x élevé**, car il est à droite dans l'image réelle. De même, les angles affichés correspondent à l'image non retournée.
- **Angle d'un marqueur** : 0° quand le marqueur est droit, positif quand il tourne **dans le sens des aiguilles d'une montre**, de −180° à 180°.
- **Angle d'un carré** : de 0° à 90° seulement (voir [section 4.4](#44-ce-qui-saffiche)).

### 9.2 Position 3D d'un marqueur (en mm, après calibration)

Les positions 3D sont mesurées **depuis la caméra** :

```
              Z (droit devant la caméra)
             ↗
  caméra ●──────► X (vers la droite)
         │
         ▼
         Y (vers le bas)
```

| Valeur | Signification |
|---|---|
| **X** | Décalage vers la droite (négatif = à gauche) |
| **Y** | Décalage vers le bas (négatif = en haut) |
| **Z** | Profondeur : distance devant la caméra, le long de son axe |
| **distance** | Distance en ligne droite entre la caméra et le centre du marqueur |

Exemple : `X 120  Y -40  Z 800 mm (distance 811 mm)` signifie que le marqueur est 12 cm à droite, 4 cm plus haut que l'axe de la caméra, et 80 cm devant elle.

### 9.3 Rotations d'un marqueur (en degrés, après calibration)

La référence (0°, 0°, 0°) est un marqueur **droit, face à la caméra**.

| Rotation | Signification |
|---|---|
| **Z** | Rotation du marqueur dans son propre plan, comme une aiguille de montre |
| **X** | Inclinaison haut/bas (le marqueur bascule vers l'avant ou l'arrière) |
| **Y** | Inclinaison gauche/droite (le marqueur pivote comme une porte) |

**Cas d'une caméra au plafond qui regarde le sol** : les marqueurs posés à plat sur le robot font face à la caméra. **X** et **Y** restent alors proches de 0, **rotation Z** donne le **cap du robot**, et les positions **X** et **Y** donnent sa position sur le sol par rapport à l'axe de la caméra.

Les inclinaisons X et Y sont moins précises (1 à 2°) que la position et la rotation Z : elles se déduisent de la faible déformation du marqueur. Elles deviennent plus fiables quand le marqueur est grand dans l'image.

---

## 10. Envoyer les positions à un autre programme

L'application peut envoyer en temps réel tout ce qu'elle détecte (carrés, marqueurs, positions 3D) à un autre programme, sur le même ordinateur ou sur le réseau. Par exemple, au programme qui pilote votre robot. Les données sont envoyées en **JSON**, par le protocole réseau **UDP**.

### 10.1 Activer l'envoi

1. Ouvrez l'onglet **Envoi UDP**.
2. Saisissez l'**Adresse** de la machine qui reçoit les données :
   - `127.0.0.1` : le même ordinateur (valeur par défaut) ;
   - l'adresse IP d'une autre machine du réseau (par exemple `192.168.1.42`) ou son nom ;
   - `255.255.255.255` : toutes les machines du réseau local.
3. Choisissez le **Port** (5005 par défaut). Le programme qui reçoit doit écouter ce même port.
4. Cochez **Envoyer les positions**.

Sous les réglages, le compteur **Messages envoyés** augmente à chaque image traitée. L'envoi continue quel que soit l'onglet affiché. L'adresse, le port et l'activation sont mémorisés.

### 10.2 Tester la réception

Un programme de test est fourni. Dans un second terminal, lancez :

```bash
python3 tools/udp_receiver.py
```

Il affiche une ligne par image reçue :

```
image 12 | carré (664, 484) px 0° | ArUco 7 (200, 200) px 0° | X -67 Y -33 Z 251 mm
```

Options : `--port 6000` pour écouter un autre port, `--raw` pour afficher le JSON brut.

Ce programme sert aussi de point de départ pour écrire votre propre programme de pilotage : la fonction `handle()` reçoit chaque message déjà décodé.

### 10.3 Format des messages

Chaque image traitée produit **un message**, sur une seule ligne. Voici un exemple, mis en forme pour la lecture :

```json
{
  "frame": 12,
  "timestamp_ms": 1791830806877,
  "image": {"width": 800, "height": 600},
  "squares": [
    {"center": [665.8, 483.8], "side": 87.5, "angle": 0,
     "corners": [[622, 440], [709.5, 440], [709.5, 527.5], [622, 527.5]]}
  ],
  "markers": [
    {"id": 7, "center": [201.5, 199.5], "side": 149.9, "angle": 0,
     "corners": [[126.6, 124.6], [276.4, 124.6], [276.4, 274.4], [126.6, 274.4]],
     "floor": {"position_mm": [150.3, 101.2], "heading_deg": -89.8},
     "pose": {"position_mm": [-67, -33.3, 251.3], "rotation_deg": [-4.3, 5.2, -0.4],
              "distance_mm": 262.3}}
  ]
}
```

| Champ | Contenu |
|---|---|
| `frame` | Numéro de l'image, augmenté de 1 à chaque message (repart de 0 au lancement). Un saut indique un message perdu. |
| `timestamp_ms` | Heure de l'envoi, en millisecondes depuis le 1er janvier 1970 (UTC) |
| `image` | Taille de l'image en pixels |
| `squares` | Carrés détectés, **le plus grand en premier** (le carré principal, entouré en vert). Liste vide si aucun carré n'est détecté. **Absent** si la détection du carré est désactivée. |
| `markers` | Marqueurs ArUco détectés, triés par identifiant. Liste vide si aucun marqueur n'est détecté. **Absent** si la détection ArUco est désactivée. |
| `center` | Centre `[x, y]`, en pixels (voir [9.1](#91-coordonnées-dans-limage-en-pixels)) |
| `side` | Côté moyen, en pixels |
| `angle` | Angle en degrés : de 0 à 90 pour un carré, de −180 à 180 pour un marqueur |
| `corners` | Les 4 coins `[x, y]`, en pixels. Pour un marqueur, le premier est le coin d'origine (point rouge à l'écran). |
| `id` | Identifiant du marqueur |
| `floor` | Uniquement si le repère du sol est défini : `position_mm` `[X, Y]` sur le sol, et `heading_deg` (cap d'un marqueur) ou `angle_deg` (angle d'un carré, de 0 à 90), voir [8.3](#83-lire-les-positions-au-sol) |
| `events` | Uniquement quand un événement survient à cette image (voir [6.5](#65-événements-de-mouvement) et [8.5](#85-définir-des-zones)). Chaque événement n'est envoyé qu'une fois, avec `object_id` et `name` (l'objet concerné : numéro et nom d'un objet suivi, identifiant d'un marqueur et « ArUco 7 », ou 0 et « Carré »), `type` (`motion_start`, `motion_stop`, `lost`, `found`, `zone_enter`, `zone_exit`, `game_zone_enter` et `game_zone_exit` pour le jeu de la [section 8.6](#86-jeu--détecter-le-robot-dans-une-zone), `game_green`, `game_red`, `game_fault` et `game_win` pour « 1, 2, 3 Soleil » ([8.7](#87-jeu--1-2-3-soleil)), `game_treasure` et `game_over` pour la chasse au trésor ([8.8](#88-jeu--chasse-au-trésor)), ou `game_event` pour les autres jeux ([8.9](#89-les-autres-jeux--fonctionnement-commun)), avec `code` (par exemple `race_gate`), `text` (le texte du journal) et `count` ; pour les entrées et sorties de zone s'ajoutent `zone_id` et `zone`, le nom de la zone ; `game_fault` a `fault` (numéro de la faute) ; `game_win` a `faults`, `elapsed_s`, `zone_id` et `zone` ; `game_treasure` a `treasure` (numéro du trésor) et `game_over` a `treasures` (total)), `timestamp_ms` (heure de l'événement) et `position_mm` (sur le sol) ou `position_px` (dans l'image). Pour `motion_stop` s'ajoutent `distance_mm` (ou `_px`), `travelled_mm` (ou `_px`) et `duration_s`. |
| `tracks` | Uniquement si le lissage est actif (voir [6.4](#64-lisser-les-positions-des-carrés-et-des-marqueurs)) : une entrée par carré principal ou marqueur suivi, avec `type` (`square` ou `marker`), `id` (marqueur), `predicted` (non détecté dans cette image), `position_px` lissée, `velocity_px_s`, `speed_px_s`, `angle_deg` lissé et, si le repère du sol est défini, `floor` avec `position_mm`, `velocity_mm_s`, `speed_mm_s`, `direction_deg` et `heading_deg` (marqueur) ou `angle_deg` (carré) |
| `tracked` | Uniquement si au moins un objet est suivi (voir [section 6](#6-suivre-des-objets-encadrés)) : une **liste**, avec une entrée par objet. Chaque entrée contient `id` (numéro de l'objet), `name` (son nom), `found` (trouvé ou perdu), `predicted` (perdu depuis peu, position prédite), `confidence` (indice de confiance, de 0 à 1), `drifted` (perdu parce que le cadre ne ressemble plus à l'objet ou a sauté ailleurs), `out_of_view` (sorti de l'image : recherché seulement le long des bords), `reacquired` (retrouvé à cette image par la recherche dans toute l'image), `reacquisitions` (nombre de reprises automatiques depuis la sélection), `center` (position lissée), `box` `[x, y, largeur, hauteur]` en pixels, `moving`, `velocity_px_s` `[vx, vy]`, `speed_px_s`, `direction_deg` et, si le repère du sol est défini, `floor` avec `position_mm`, `velocity_mm_s`, `speed_mm_s` et `direction_deg` |
| `zones` | Uniquement si au moins une zone est définie (voir [8.5](#85-définir-des-zones)) : une entrée par zone, avec `id`, `name`, `box` `[x, y, largeur, hauteur]` en pixels, `occupants` (noms de ce qui s'y trouve) et, si le repère du sol est défini, `floor_corners_mm` (les 4 coins sur le sol, en mm) |
| `game` | Uniquement pendant une partie d'un jeu. Pour « Détection robot dans zone » (voir [8.6](#86-jeu--détecter-le-robot-dans-une-zone)) : `mode` (`robot_in_zone`), `robot_id` et `robot` (numéro et nom de l'objet suivi), `target_zone_id` (zone visée, 0 pour n'importe laquelle), `found` (robot suivi), `inside` (entièrement dans une zone), `zone_id` et `zone` (si `inside`), `time_inside_s` (temps passé dans la zone) et `entries` (nombre d'entrées). Pour « 1, 2, 3 Soleil » (voir [8.7](#87-jeu--1-2-3-soleil)) : `mode` (`soleil`), `robot_id`, `robot`, `finish_zone_id`, `phase` (`countdown`, `green`, `red` ou `won`), `phase_remaining_s`, `found`, `elapsed_s` (chrono), `faults` et `score_s`. Pour la chasse au trésor (voir [8.8](#88-jeu--chasse-au-trésor)) : `mode` (`treasure`), `robot_id`, `robot`, `terrain_zone_id` (0 pour toute l'image), `phase` (`playing` ou `over`), `hint` (`freezing`, `cold`, `warm`, `hot`, `burning`, `found` ou `unknown`), `trend` (+1 se rapproche, −1 s'éloigne, 0 stable), `found` (trésors trouvés), `remaining_s`, `distance_px` et, pendant la partie, `treasure_px` `[x, y]` et `radius_px`. Pour les autres jeux (section 8.9 et suivantes) : `mode` (nom court du jeu, par exemple `race`), `running` (partie en cours) et les champs propres au jeu, décrits dans sa section |
| `pose` | Uniquement si la caméra est calibrée : `position_mm` `[X, Y, Z]`, `rotation_deg` `[X, Y, Z]` et `distance_mm`, comme dans le panneau Résultat (voir [9.2](#92-position-3d-dun-marqueur-en-mm-après-calibration) et [9.3](#93-rotations-dun-marqueur-en-degrés-après-calibration)) |

Les valeurs sont arrondies au dixième. Comme à l'écran, les coordonnées sont celles de l'image réelle de la caméra, même avec l'effet miroir.

### 10.4 Bon à savoir

- **UDP ne garantit pas la livraison** : un message peut être perdu, surtout en Wi-Fi. Ce n'est pas gênant pour suivre un robot, car le message suivant arrive quelques dizaines de millisecondes plus tard. Utilisez `frame` pour repérer les pertes.
- **Débit** : un message par image traitée, soit autant de messages par seconde que le nombre d'i/s affiché dans **Résultat**. Un message pèse moins d'1 Ko avec quelques marqueurs.
- **Envoi vers une autre machine** : vérifiez que son pare-feu laisse passer le port UDP choisi.
- **Exemple minimal en Python** pour recevoir les messages :

  ```python
  import json, socket

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.bind(("0.0.0.0", 5005))
  while True:
      msg = json.loads(sock.recv(65535))
      for m in msg.get("markers", []):
          print(m["id"], m["center"], m["angle"])
  ```

---

## 11. Conseils pour une détection fiable

**Éclairage**
- Un éclairage **homogène et stable** est le facteur le plus important.
- Évitez le soleil direct, les ombres marquées et les spots qui créent des reflets.
- Évitez les éclairages qui scintillent (certains néons ou LED bon marché). Si des bandes défilent sur l'image, réglez la **Fréquence du secteur** sur **50 Hz** en Europe (60 Hz en Amérique du Nord), dans **Réglages de l'image...**, **Autres réglages**.

**Caméra**
- Fixez la caméra solidement : si elle bouge, toutes les positions bougent.
- Une caméra **au-dessus de la zone, qui regarde vers le bas**, donne les mesures les plus simples à exploiter.
- Laissez l'objectif propre ; une image floue gêne surtout les marqueurs ArUco.

**Robot en mouvement**
- Un déplacement rapide rend l'image floue et peut faire perdre la détection. Une **exposition manuelle courte** (**Réglages de l'image...**, 20 à 30 ms) réduit ce flou ; éclairez davantage si l'image devient trop sombre.

**Performances**
- Le nombre d'images par seconde (i/s) est affiché dans **Résultat**.
- Le traitement des images (détections, suivi, lissage) se fait dans un **fil d'exécution séparé** : l'interface reste réactive même quand le traitement est lent. Si la caméra va plus vite que le traitement, les images intermédiaires sont ignorées et l'affichage montre toujours l'image la plus récente traitée. Le nombre d'images par seconde affiché est celui des images traitées.
- Choisissez une **résolution plus basse** (liste **Résolution**) : la détection est plus rapide.
- Désactivez les détections inutiles (case **Activer la détection** de chaque onglet) et le **Mode calibration** pour gagner en vitesse.
- L'**enregistrement d'une vidéo** (bouton **Vidéo**) occupe aussi le processeur, surtout la vidéo avec les détections : si le nombre d'images par seconde chute pendant l'enregistrement, baissez la résolution.
- Le suivi d'objets avec **CSRT** demande environ 25 ms de calcul par image **et par objet**. Les objets sont suivis en parallèle sur les cœurs du processeur, mais leur coût s'additionne quand même : passez à **Nano** (environ 10 ms par objet) ou **KCF** (environ 5 ms), ou retirez les objets qui ne servent plus.

---

## 12. Questions fréquentes et dépannage

**L'image reste noire avec « En attente de la webcam... »**
- Vérifiez que la webcam est branchée et choisissez-la dans la liste **Caméra**.
- Fermez les autres logiciels qui l'utilisent (visioconférence, navigateur...).
- Vérifiez que votre utilisateur a accès à la caméra : `ls /dev/video*`, et appartenance au groupe `video` (`groups`).

**« Aucune webcam détectée »**
- Aucune caméra n'a été trouvée au lancement. Branchez-la, puis relancez l'application.

**Le carré n'est jamais détecté**
1. Cliquez à nouveau sur le carré dans l'image pour reprendre sa couleur.
2. Cochez **Afficher le masque** :
   - **carré noir ou incomplet** → en type « Couleur vive », augmentez **Tolérance teinte**, diminuez **Saturation min** et **Luminosité min** ; en type « Blanc, gris ou noir », augmentez **Tolérance luminosité** et **Saturation max** ;
   - **carré blanc et plein, mais pas de contour vert** → diminuez **Surface min** (carré trop petit) ou **Remplissage min** (forme imparfaite).

**Un carré blanc, gris ou noir n'est pas détecté**
- Vérifiez que le **Type** est « Blanc, gris ou noir ». Cliquer sur le carré le choisit automatiquement.
- Le carré doit contraster avec ce qui le touche : un carré blanc posé sur une surface claire forme une seule zone avec elle, et n'est donc plus un carré. Posez-le sur un fond sombre, ou ajoutez-lui un bord noir.
- Sous une lumière chaude, le blanc paraît orangé : augmentez **Saturation max**.

**D'autres objets sont détectés comme des carrés**
- Diminuez **Tolérance teinte**, augmentez **Saturation min** ou **Surface min**.
- Le carré principal, en vert, est toujours le plus grand : retirez de la scène les objets de même couleur ou choisissez une couleur plus rare.

**Un marqueur ArUco n'est pas détecté**
- Vérifiez que le **dictionnaire** sélectionné est celui du marqueur imprimé.
- La **marge blanche** autour du marqueur doit être présente et visible.
- Le marqueur doit être net, sans reflet, et mesurer au moins 30 pixels dans l'image : rapprochez-le ou imprimez-le plus grand.

**Les positions 3D (X, Y, Z) ne s'affichent pas**
- La caméra n'est pas calibrée : l'onglet **Calibration** indique `Caméra non calibrée`. Suivez la [section 7](#7-calibrer-la-caméra).
- Le panneau **Résultat** affiche `Pas de pose 3D : calibration faite en ...` : la résolution choisie n'a pas le même format d'image que la calibration. Choisissez une résolution du format indiqué, ou recalibrez.

**Le nombre d'images par seconde est faible**
- Dans une pièce sombre, la webcam allonge son temps de pose et ralentit, quelle que soit la résolution. Décochez **Ralentir les images en exposition auto.**, ou passez l'**exposition en manuel** avec un temps de pose de 33 ms ou moins (**Réglages de l'image...**), et éclairez davantage la scène si l'image devient trop sombre.
- Choisissez une résolution plus basse, et désactivez les détections inutiles et le **Mode calibration**.

**Les couleurs changent toutes seules et le carré coloré est perdu par moments**
- La balance des blancs automatique corrige les couleurs en permanence. Passez-la en **manuel** (**Réglages de l'image...**), puis cliquez à nouveau sur le carré.

**Une autre application trouve la webcam très sombre ou très claire**
- RobotDetectionByWebcam a probablement été arrêtée brutalement alors que l'exposition était en manuel (ou la luminosité, le gain... modifiés) : la webcam a gardé ces réglages. Relancez l'application puis quittez-la normalement avec **Quitter** : elle remet la webcam en automatique, avec ses réglages d'origine.

**Les distances sont fausses**
- **Toujours trop grandes ou trop petites dans la même proportion** → la **Taille du marqueur** est mal saisie, ou le **Côté d'une case** était faux pendant la calibration (recalibrez dans ce cas).
- **Fausses surtout sur les bords de l'image** → la calibration manque de captures avec le damier dans les coins. Refaites-la.

**Le damier n'est pas détecté en mode calibration**
- Le damier doit être **entièrement visible**, marge blanche comprise.
- Vérifiez le nombre de **coins intérieurs** : il y en a un de moins que de cases dans chaque direction.
- Évitez les reflets sur le papier et rapprochez le damier.

**Les captures ont disparu**
- Changer les dimensions du damier (coins ou taille des cases) efface les captures en cours. Elles sont aussi effacées si la résolution de la caméra change pendant la série.

**Les positions tremblent, ou la vitesse affichée varie beaucoup**
- Augmentez la **Force du lissage** (onglet **Suivi**). Vérifiez aussi l'éclairage : une image bruitée fait trembler les détections.

**Les positions lissées sont en retard sur l'objet**
- Diminuez la **Force du lissage**. Un lissage fort ralentit la réaction aux démarrages et aux virages.

**Le journal signale des mouvements alors que l'objet ne bouge pas, ou l'inverse**
- Augmentez le **seuil de mouvement** (onglet **Suivi**) si de faux départs apparaissent : le cadre tremble, surtout avec un objet peu texturé ou une image bruitée. Diminuez-le si de vrais petits déplacements ne sont pas signalés.
- Avec un repère du sol, c'est le seuil en mm qui compte ; sans repère, le seuil en pixels.

**L'objet suivi est perdu ou le cadre glisse sur le fond**
- Resélectionnez l'objet en l'encadrant au plus près, avec le moins de fond possible : le fond compté dans la sélection abaisse la confiance quand l'objet se déplace devant un autre décor.
- Essayez la méthode **Nano** ou **Vit** : elles résistent mieux aux rotations, aux changements de taille et aux disparitions momentanées. À défaut, préférez **CSRT**, plus robuste que KCF.
- Un objet uni, petit ou qui ressemble au fond se suit mal : ajoutez-lui un motif contrasté, ou utilisez un marqueur ArUco.
- Si l'objet bouge vite, réduisez le flou avec une exposition manuelle courte (**Réglages de l'image...**).

**La sélection passe sur ma main ou mon visage quand l'objet sort de l'image**
- L'objet devrait être signalé « **hors du champ** » (cadre rouge) : le suivi s'arrête alors et l'objet n'est plus cherché que le long des bords. Si le cadre glisse quand même sur autre chose, c'est que l'objet a été perdu loin du bord (caché par la main avant de sortir, par exemple) : retirez-le de la liste et ajoutez-le de nouveau quand il revient.
- Évitez de sortir l'objet de l'image en le tenant juste devant votre visage : il est plus facile à distinguer devant un fond neutre.

**Le téléchargement du modèle Nano ou Vit échoue**
- Vérifiez la connexion Internet. Sans connexion, téléchargez les fichiers sur une autre machine (adresses dans la fenêtre de téléchargement et dans [Modèles des réseaux de neurones](#modèles-des-réseaux-de-neurones)), puis copiez-les dans `~/.local/share/RobotDetectionByWebcam/models/`.
- « Empreinte différente » : le fichier reçu n'est pas celui attendu (téléchargement tronqué ou fichier modifié sur le serveur). Réessayez ; si le message persiste, signalez-le.

**Une zone ne signale pas l'entrée d'un objet**
- C'est le **centre** de l'objet qui compte : un objet à moitié dans la zone n'y est pas forcément.
- Seuls les objets suivis, les marqueurs ArUco et le carré principal sont surveillés : un objet qui n'est ni encadré (onglet **Suivi**) ni porteur d'un marqueur n'est pas vu.
- La caméra a bougé depuis le tracé : la zone ne correspond plus au même endroit. Retirez-la et tracez-la de nouveau.

**La vidéo enregistrée est illisible**
- L'application a été arrêtée brutalement (Ctrl+C, plantage) pendant l'enregistrement : le fichier n'a pas pu être terminé. Arrêtez toujours la vidéo avec le bouton **Vidéo**, ou quittez avec **Quitter**, qui attend la fin du fichier.

**La fenêtre du joueur s'ouvre sur le mauvais écran**
- Choisissez l'écran dans le menu du bouton **Joueur** (petite flèche), **Écran**. Le choix est mémorisé.
- Si l'écran choisi est débranché, la fenêtre s'ouvre sur l'autre écran que celui de la fenêtre principale.

**« Détection robot dans zone » indique « hors zone » alors que le robot est dans la zone**
- Le robot doit être **entièrement** dans la zone : c'est son **cadre de suivi** qui compte. Si le cadre est plus large que le robot, ou s'il a glissé sur le décor, il peut dépasser de la zone. Retirez le robot de la liste des objets suivis et encadrez-le de nouveau, au plus près.
- La zone est trop petite pour le cadre : tracez-la un peu plus grande que le robot.
- Vérifiez la **Zone** choisie dans la fenêtre du jeu : avec une zone précise, les autres zones ne comptent pas.
- Même règle pour l'**arrivée** de « 1, 2, 3 Soleil » : la partie ne se termine que lorsque le robot est entièrement dans la zone d'arrivée.

**« 1, 2, 3 Soleil » compte des fautes alors que le robot est arrêté**
- Le cadre de suivi tremble un peu, surtout pour un objet peu texturé ou une image bruitée. Augmentez le **seuil de mouvement** dans l'onglet **Suivi**, puis relancez la partie.
- Le robot met plus de temps à s'arrêter que le temps de réaction : choisissez une difficulté plus facile.

**Chasse au trésor : le robot passe près du trésor sans le trouver**
- C'est le **centre** du robot qui doit passer sur le trésor, pas seulement le bord de son cadre. Suivez les indices jusqu'à « BRÛLANT », puis balayez lentement la zone.
- Choisissez une difficulté plus facile : le trésor est plus grand.

**Le robot n'apparaît pas dans la liste d'un jeu**
- Les jeux ne connaissent que les **objets suivis** (onglet **Suivi**) : un robot repéré seulement par un marqueur ArUco ou par le carré coloré n'y figure pas. Ajoutez-le avec **Ajouter des objets**.

**Pas de position au sol**
- Le repère n'est pas défini : l'onglet **Sol** indique `Repère du sol non défini`. Suivez la [section 8](#8-définir-le-repère-du-sol).
- Le panneau **Résultat** affiche `Pas de position au sol : repère défini en ...` : la résolution choisie n'a pas le même format d'image que lors de la définition du repère.

**Les positions au sol sont décalées**
- La caméra a bougé depuis la définition du repère : redéfinissez-le.
- Vérifiez les **longueurs X et Y** saisies et l'**ordre des 4 points**.
- Le décalage augmente vers les bords de l'image : les marqueurs sont plus hauts que le sol, ou l'objectif déforme l'image. Calibrez la caméra et saisissez la **Hauteur des repères** (voir [8.4](#84-précision-et-limites)).
- La hauteur de la caméra affichée dans l'onglet **Sol** ne correspond pas à la réalité : vérifiez les longueurs saisies, l'ordre des points et la calibration.

**Le programme de réception ne reçoit rien**
- Vérifiez que **Envoyer les positions** est coché et que le compteur **Messages envoyés** augmente.
- Le **port** doit être le même des deux côtés.
- Vers une autre machine : vérifiez l'**adresse** et le **pare-feu** de la machine qui reçoit. Testez d'abord avec `python3 tools/udp_receiver.py` sur cette machine.
- Un seul programme peut écouter un port donné sur une machine : fermez les autres récepteurs.

**Comment repartir de zéro pour la calibration ?**
- Supprimez le fichier de calibration de la caméra, dans `~/.config/RobotDetectionByWebcam/calibrations/` (son chemin est affiché dans l'onglet **Calibration**), puis relancez l'application.

---

## 13. Fichiers utilisés par l'application

| Fichier | Contenu |
|---|---|
| `~/.config/RobotDetectionByWebcam/settings.ini` | Réglages de l'application (voir ci-dessous) |
| `~/.config/RobotDetectionByWebcam/calibrations/<caméra>.yml` | Calibration de chaque caméra, nommée d'après la caméra, par exemple `UVC_Camera_046d_0825.yml` (créée par **Calibrer**, relue au lancement et quand on revient à cette caméra). Peut être copiée sur une autre machine utilisant la même webcam. |
| `~/.config/RobotDetectionByWebcam/settings.ini`, groupe `zones` de chaque caméra | Zones de l'onglet **Zones** : nom et rectangle (`rect` = x, y, largeur, hauteur en fraction de l'image) |
| `~/Images/RobotDetectionByWebcam/capture_<date>.png` | Images enregistrées par **Capture** (dossier modifiable, voir [Capturer une image](#capturer-une-image)) |
| `~/Images/RobotDetectionByWebcam/video_<date>.mp4` | Vidéos enregistrées par **Vidéo** (même dossier, voir [Enregistrer une vidéo](#enregistrer-une-vidéo)) |
| `~/.local/share/RobotDetectionByWebcam/models/` | Modèles des méthodes de suivi Nano et Vit, téléchargés au premier choix de ces méthodes (voir [Modèles des réseaux de neurones](#modèles-des-réseaux-de-neurones)) |
| `~/.config/RobotDetectionByWebcam/calibration.yml` | Ancienne calibration unique, des versions précédentes. Au premier lancement de la version actuelle, elle est copiée pour la caméra active ; l'original est conservé mais n'est plus utilisé. |
| `aruco_<dictionnaire>_<id>.png` | Marqueurs générés, à l'emplacement choisi |
| `damier_<largeur>x<hauteur>.png` | Damiers générés, à l'emplacement choisi |
| `evenements.csv` (nom proposé) | Journal des événements de mouvement, exporté avec **Enregistrer...** (onglet **Suivi**) |

### Modèles des réseaux de neurones

Les méthodes de suivi **Nano** et **Vit** utilisent des réseaux de neurones fournis par OpenCV. Leurs modèles ne sont pas inclus dans l'application :

| Méthode | Fichiers | Taille | Origine |
|---|---|---|---|
| Nano | `nanotrack_backbone_sim.onnx`, `nanotrack_head_sim.onnx` | 1,7 Mo | [NanoTrack](https://github.com/HonglinChu/SiamTrackers/tree/master/NanoTrack/models/nanotrackv2) |
| Vit | `object_tracking_vittrack_2023sep.onnx` | 0,7 Mo | [OpenCV Zoo](https://github.com/opencv/opencv_zoo/tree/main/models/object_tracking_vittrack) |

Quand vous choisissez l'une de ces méthodes pour la première fois, l'application propose de télécharger ses fichiers dans `~/.local/share/RobotDetectionByWebcam/models/`. L'empreinte de chaque fichier est vérifiée : un fichier tronqué ou modifié est refusé. Si vous annulez ou si le téléchargement échoue, la méthode précédente reste choisie.

Sur une machine sans connexion, téléchargez les fichiers ailleurs et copiez-les dans l'un de ces dossiers, cherchés dans cet ordre :
1. `~/.local/share/RobotDetectionByWebcam/models/` ;
2. `models/` à côté de l'exécutable ;
3. `models/` dans le dossier parent de l'exécutable (par exemple `RobotDetectionByWebcam/models/` quand l'application est lancée depuis `build/`).

Si un modèle a été supprimé, l'application revient à **CSRT** au lancement suivant.

### Réglages mémorisés

L'application retrouve au lancement les réglages de la fois précédente :

- **Webcam** : caméra choisie (si elle est branchée), résolution, exposition, balance des blancs et autres réglages de l'image (pour chaque caméra), effet miroir et dossier des captures ;
- **Fenêtre du joueur** : ouverte ou non, détections, miroir, plein écran et écran choisi ;
- **Carré coloré** : couleur et tous les curseurs ;
- **ArUco** : dictionnaire et taille du marqueur ;
- **Calibration** : dimensions du damier et taille des cases ;
- **Sol** : les 4 points du repère, les dimensions du rectangle et la hauteur des repères (pour chaque caméra), et l'affichage de la zone ;
- **Zones** : les zones de chaque caméra (nom et rectangle) et leur affichage ;
- **Suivi** : la méthode, l'affichage et la durée de la trajectoire, les seuils de mouvement (les objets suivis et le journal des événements, eux, ne sont pas mémorisés), et tous les réglages du lissage ;
- **Envoi UDP** : activation, adresse et port ;
- **Fenêtre** : taille, position et onglet ouvert.

Le fichier peut se lire et se modifier à la main (application fermée). Les réglages propres à une caméra sont regroupés dans la section `[cameras]`, sous un nom simplifié : son nom réduit aux lettres et aux chiffres, suivi de son identifiant USB s'il y figure. Une Logitech C270, nommée « UVC Camera (046d:0825) », devient `UVC_Camera_046d_0825` :

```ini
[cameras]
UVC_Camera_046d_0825\exposureMs=33
UVC_Camera_046d_0825\manualExposure=false
UVC_Camera_046d_0825\size=@Size(1280 960)
UVC_Camera_046d_0825\controls\0x00980918=1
UVC_Camera_046d_0825\floor\sizeX=1000
```

Les lignes `controls\0x...` sont les autres réglages de l'image modifiés, repérés par leur identifiant V4L2 (`0x00980918` = fréquence du secteur, ici 50 Hz). Les réglages des versions précédentes, enregistrés sous le nom complet de la caméra (`UVC%20Camera%20%28046d%3A0825%29`), sont renommés automatiquement au premier lancement, ainsi que le fichier de calibration.

Le fichier contient aussi une option sans équivalent dans l'interface : `hideLibraryMessages`, dans la section `[logging]`. Elle vaut `true` par défaut et masque des messages techniques sans gravité dans le terminal (par exemple « Corrupt JPEG data »). Mettez-la à `false` pour les réafficher, par exemple pour signaler un problème. Ce réglage est pris en compte au lancement suivant.

La résolution, les réglages de l'image et le repère du sol sont enregistrés dès qu'on les modifie. Les autres réglages sont enregistrés **à la fermeture de l'application**, une fois la fermeture confirmée. Si l'application est arrêtée brutalement (Ctrl+C dans le terminal, plantage), les changements de la session sont perdus.

Ne sont **pas** mémorisés : l'**activation des détections du carré coloré et des marqueurs ArUco**, désactivées à chaque lancement pour que seuls les objets encadrés à la main soient suivis, et, car ils ne servent que ponctuellement, les parties de jeu (robot, zone, durée, difficulté, compteurs), **Afficher le masque**, **Mode calibration**, **Capture automatique**, les captures du damier en cours, les objets suivis et le journal des événements.

Pour revenir aux réglages par défaut, fermez l'application et supprimez le fichier `settings.ini`. Les calibrations, enregistrées à part dans le dossier `calibrations/`, sont conservées. (Les repères du sol et les zones, eux, sont dans `settings.ini` et sont donc effacés. Les images et vidéos capturées et les modèles Nano et Vit ne sont pas touchés.)
