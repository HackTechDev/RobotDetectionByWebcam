# Guide utilisateur — RobotDetectionByWebcam

Ce guide explique comment utiliser l'application au quotidien : repérer un robot avec un carré de couleur ou avec des marqueurs ArUco, calibrer la caméra et lire les résultats.

Pour l'installation et la compilation, voir le [README](README.md).

## Sommaire

1. [Présentation](#1-présentation)
2. [Démarrage rapide](#2-démarrage-rapide)
3. [L'écran principal](#3-lécran-principal)
4. [Suivre un carré coloré](#4-suivre-un-carré-coloré)
5. [Suivre des marqueurs ArUco](#5-suivre-des-marqueurs-aruco)
6. [Calibrer la caméra](#6-calibrer-la-caméra)
7. [Définir le repère du sol](#7-définir-le-repère-du-sol)
8. [Lire les résultats](#8-lire-les-résultats)
9. [Envoyer les positions à un autre programme](#9-envoyer-les-positions-à-un-autre-programme)
10. [Conseils pour une détection fiable](#10-conseils-pour-une-détection-fiable)
11. [Questions fréquentes et dépannage](#11-questions-fréquentes-et-dépannage)
12. [Fichiers utilisés par l'application](#12-fichiers-utilisés-par-lapplication)

---

## 1. Présentation

L'application filme la scène avec une webcam et repère le robot de deux façons, utilisables séparément ou ensemble.

| Méthode | Ce qu'il faut sur le robot | Ce qu'on obtient | Points forts | Limites |
|---|---|---|---|---|
| **Carré coloré** | Un carré d'une couleur vive, ou blanc, gris ou noir (papier, gommette, pièce peinte) | Position et taille en pixels, angle de 0 à 90° | Très simple à mettre en place | Sensible à l'éclairage et aux objets de même couleur ; un seul robot facile à distinguer ; angle connu à 90° près |
| **Marqueur ArUco** | Un marqueur noir et blanc imprimé | Identifiant, position et angle complet (−180° à 180°) ; **position 3D en mm** si la caméra est calibrée | Fiable, plusieurs robots distingués par leur identifiant | Le marqueur doit être assez grand et bien visible |

En bref : pour un essai rapide, le carré coloré suffit. Pour un suivi fiable, ou pour plusieurs robots, utilisez les marqueurs ArUco.

---

## 2. Démarrage rapide

1. Branchez la webcam.
2. Lancez l'application :
   ```bash
   ./build/RobotDetectionByWebcam
   ```
3. L'image de la webcam s'affiche à gauche. Si plusieurs caméras sont branchées, choisissez la bonne dans la liste **Caméra**.
4. Les deux détections sont actives dès le lancement :
   - **carré coloré** : montrez un carré rouge à la caméra, ou cliquez sur votre carré dans l'image pour choisir sa couleur ;
   - **ArUco** : montrez un marqueur du dictionnaire « 4x4 (50) » (voir [section 5](#5-suivre-des-marqueurs-aruco) pour en imprimer un).
5. Les résultats s'affichent sur l'image et dans le panneau **Résultat**, en bas à droite.

---

## 3. L'écran principal

```
┌───────────────────────────────────────┬───────────────────────────────┐
│                                       │ Webcam                        │
│                                       │   Caméra     [........... ▼]  │
│                                       │   Résolution [........... ▼]  │
│                                       │   [ ] Effet miroir            │
│          Image de la webcam           ├───────────────────────────────┤
│      (avec les détections dessinées)  │ [Carré][ArUco][Cal.][Sol][UDP]│
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
└───────────────────────────────────────┴───────────────────────────────┘
```

- **Zone d'image** (à gauche) : l'image de la caméra, avec les contours des carrés, des marqueurs et du damier détectés. Le curseur en croix rappelle qu'un clic sert à choisir une couleur.
- **Webcam** :
  - **Caméra** : la webcam utilisée ;
  - **Résolution** : la taille des images de la caméra, avec leur format (4:3, 16:9...) et le nombre maximal d'images par seconde annoncé par la caméra. Voir [Choisir la résolution](#choisir-la-résolution) ;
  - **Effet miroir** : retourne l'image affichée, comme dans un miroir. C'est pratique quand on se place face à la caméra. Cette option ne change **que l'affichage** : les détections et les mesures restent celles de l'image réelle de la caméra (voir [section 8.1](#81-coordonnées-dans-limage-en-pixels)).
- **Onglets** :
  - **Carré coloré** : couleur et réglages de la détection du carré ;
  - **ArUco** : dictionnaire et taille des marqueurs ;
  - **Calibration** : damier et calibration de la caméra ;
  - **Sol** : repère du sol, pour avoir les positions en millimètres sur le sol (voir [section 7](#7-définir-le-repère-du-sol)) ;
  - **Envoi UDP** : envoi des positions à un autre programme (voir [section 9](#9-envoyer-les-positions-à-un-autre-programme)).
- **Résultat** : taille de l'image, nombre d'images par seconde (i/s) et mesures détaillées.
- **Quitter** : ferme l'application. Une fenêtre demande de confirmer : cliquez sur **Quitter** pour fermer, ou sur **Annuler** (ou appuyez sur Échap) pour continuer. La même confirmation s'affiche si vous fermez la fenêtre avec la croix de la barre de titre.

### Choisir la résolution

La résolution choisie est **mémorisée** pour chaque modèle de caméra et réappliquée au lancement suivant. Au tout premier lancement avec une caméra, celle-ci démarre dans son format par défaut, et la liste **Résolution** se cale sur la taille des images reçues.

| Résolution | Avantages | Inconvénients |
|---|---|---|
| **Haute** (ex. 1280 x 960) | Détecte les marqueurs plus loin ; positions plus précises | Détection plus lente ; certaines caméras donnent moins d'images par seconde |
| **Basse** (ex. 640 x 480) | Détection rapide, fluide | Les marqueurs doivent être plus grands ou plus proches |

Conseils :
- Commencez par **640 x 480** et montez si les marqueurs ne sont pas détectés d'assez loin.
- Restez dans le **même format d'image que votre calibration** (indiqué entre parenthèses) : une calibration faite en 4:3 ne s'applique pas à une image 16:9. Voir [section 6.4](#64-quand-refaire-la-calibration-).
- Le nombre d'images par seconde affiché dans la liste est le **maximum annoncé par la caméra**. Le nombre réel, affiché dans **Résultat**, peut être plus bas, en particulier dans une pièce sombre : la caméra allonge alors son temps de pose.

Certaines webcams n'envoient pas la résolution demandée dans leur format rapide (MJPEG). L'application le détecte et passe automatiquement au format non compressé de la même résolution. Le nombre d'images par seconde indiqué dans la liste est alors mis à jour, et il est souvent plus bas.

---

## 4. Suivre un carré coloré

### 4.1 Préparer le carré

- De préférence, choisissez une **couleur vive et franche** : rouge, vert, bleu, jaune, orange, magenta... C'est le cas le plus fiable.
- Un carré **blanc, gris ou noir** fonctionne aussi (voir [4.3](#43-les-réglages)), mais il faut qu'il **contraste avec le fond** : un carré blanc sur un mur ou un sol clair ne peut pas être distingué. Le blanc est aussi très courant dans une scène (feuilles, murs, reflets), d'où plus de risques de confusion.
- Choisissez une couleur **absente du reste de la scène** (sol, murs, meubles, vêtements).
- Préférez une surface **mate** : les reflets apparaissent blancs et « trouent » le carré.
- Le carré doit être **plein**, sans motif ni inscription.

### 4.2 Régler la détection

1. Ouvrez l'onglet **Carré coloré** et vérifiez que **Activer la détection** est coché.
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
- Dans **Résultat**, une ligne par marqueur, triés par identifiant : position et angle dans l'image, puis, si la caméra est calibrée, position 3D, distance et rotations (voir [section 8](#8-lire-les-résultats)).

**Astuce** : collez le marqueur sur le robot avec son bord supérieur orienté vers l'avant du robot. L'angle du marqueur devient alors directement le cap du robot.

---

## 6. Calibrer la caméra

La calibration mesure les caractéristiques optiques de votre webcam : focale, centre de l'image et déformation de l'objectif. Elle est **indispensable pour obtenir des positions en millimètres**. Elle se fait **une seule fois** par caméra et dure environ 5 minutes.

### 6.1 Préparer le damier

1. Ouvrez l'onglet **Calibration**.
2. Laissez les valeurs par défaut : **9 × 6 coins intérieurs**, soit un damier de 10 × 7 cases.
   - Les **coins intérieurs** sont les points où quatre cases se touchent. Un damier de 10 × 7 cases a 9 × 6 coins intérieurs.
3. Cliquez sur **Générer un damier...** et enregistrez l'image (`damier_9x6.png`).
4. Imprimez-la **à 100 %**, sans « ajuster à la page ».
5. Collez la feuille **parfaitement à plat** sur un support rigide (carton épais, planche, vitre). Un damier gondolé donne une mauvaise calibration.
6. **Mesurez une case à la règle**, précisément, et saisissez la valeur dans **Côté d'une case** (en mm).

Si vous avez déjà un damier, saisissez son nombre de coins intérieurs et la taille de ses cases.

### 6.2 Faire les captures

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

### 6.3 Lancer le calcul

1. Cliquez sur **Calibrer**. Le bouton est actif à partir de 5 captures. Le calcul prend quelques secondes.
2. Une fenêtre indique l'**erreur RMS**, en pixels :

   | Erreur RMS | Qualité |
   |---|---|
   | moins de 0,5 px | Très bonne |
   | 0,5 à 1 px | Correcte |
   | plus de 1 px | À refaire : damier gondolé, images floues ou mauvaise taille de damier |

3. La calibration est **enregistrée automatiquement** et rechargée à chaque lancement. Le texte `Caméra calibrée` s'affiche en vert dans l'onglet.
4. Décochez **Mode calibration** : il ralentit l'application et n'est plus utile.

Pour recommencer, cliquez sur **Effacer les captures** puis refaites une série.

### 6.4 Quand refaire la calibration ?

- Quand vous **changez de webcam**.
- Si l'objectif a un **zoom** ou une **mise au point manuelle** et que vous les modifiez.
- Si vous choisissez une **résolution d'un autre format d'image** (de 4:3 à 16:9, par exemple). La pose 3D est alors désactivée et le panneau **Résultat** affiche `Pas de pose 3D : calibration faite en ...`. Revenez à une résolution du format de la calibration, ou recalibrez.

Changer de résolution **dans le même format** (de 1280 x 960 à 640 x 480, par exemple) ne demande pas de recalibrer : la calibration est adaptée automatiquement. Pour la meilleure précision, calibrez de préférence dans la résolution que vous utiliserez le plus.

### 6.5 Vérifier la calibration

Posez un marqueur à une distance connue de la caméra, mesurée au mètre ruban, face à l'objectif. La **distance** affichée doit correspondre, à 1 ou 2 % près. Sinon, vérifiez la **Taille du marqueur** et le **Côté d'une case**.

---

## 7. Définir le repère du sol

Par défaut, les positions sont données en pixels dans l'image. Une fois le **repère du sol** défini, l'application donne aussi la position de chaque carré et de chaque marqueur **en millimètres sur le sol**, ainsi que son orientation sur le sol. C'est le plus pratique pour piloter un robot qui roule sur une table ou par terre.

Ce repère fonctionne **sans calibration**, pour le carré coloré comme pour les marqueurs ArUco. Si la caméra est calibrée, la déformation de l'objectif est en plus corrigée, ce qui améliore la précision sur les bords de l'image.

### 7.1 Préparer le rectangle de référence

Il faut 4 repères au sol formant un **rectangle de dimensions connues** :
- 4 morceaux de ruban adhésif, les coins d'une table, d'une plaque ou d'une grande feuille...
- **Plus le rectangle est grand, plus les mesures sont précises.** Idéalement, il couvre toute la zone où roule le robot.
- Vérifiez que c'est bien un rectangle : ses deux diagonales doivent avoir la même longueur.
- Mesurez ses deux côtés au mètre ruban, le plus précisément possible.
- Fixez la caméra solidement : **si elle bouge, le repère doit être redéfini**.

### 7.2 Définir le repère

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

Le repère est **enregistré immédiatement** et rechargé au lancement suivant. Changer les dimensions plus tard ne demande pas de recliquer les points. **Effacer le repère** le supprime.

### 7.3 Lire les positions au sol

Dans le panneau **Résultat** apparaissent :
- pour le carré : `Sol : X 698  Y 48 mm, angle 20.7°` ;
- pour chaque marqueur : `sol X 149  Y 101 mm, cap -89.8°`.

| Valeur | Signification |
|---|---|
| **X, Y** | Position du centre en mm, depuis l'origine (point 1), le long des axes X et Y. Les valeurs sont négatives ou dépassent les dimensions du rectangle quand l'objet est en dehors. |
| **cap** (marqueur) | Direction du **bord supérieur** du marqueur, en degrés de −180 à 180, comptée de l'axe X vers l'axe Y. 0° = vers +X, 90° = vers +Y. |
| **angle** (carré) | Orientation du carré, de 0 à 90° seulement (un carré est identique à lui-même tous les 90°) |

Si le haut du marqueur est collé vers l'avant du robot, le **cap** est directement la direction du robot sur le sol.

Ces valeurs sont aussi envoyées en UDP (champ `floor`, voir [9.3](#93-format-des-messages)).

### 7.4 Précision et limites

- **La hauteur compte.** Le repère est défini au niveau du sol. Un marqueur collé sur le dessus du robot est plus haut, donc plus proche de la caméra, et sa position est légèrement décalée vers l'extérieur de l'image. Le décalage vaut environ *hauteur du marqueur × distance au point situé sous la caméra ÷ hauteur de la caméra*. Par exemple, avec une caméra à 2 m, un marqueur à 10 cm de haut et à 1 m du point sous la caméra, le décalage est d'environ 5 cm. Pour l'éviter :
  - collez les marqueurs **le plus bas possible** sur le robot ;
  - ou définissez le rectangle de référence **à la hauteur des marqueurs**, par exemple avec 4 cales de cette hauteur posées aux coins.
- **Format d'image** : comme pour la calibration, le repère reste valable si vous changez de résolution dans le même format (4:3, 16:9...). Dans un autre format, les positions au sol sont désactivées et le panneau **Résultat** affiche `Pas de position au sol : repère défini en ...`.
- **Précision des clics** : cliquez au centre des repères. Une haute résolution (liste **Résolution**) permet de cliquer plus précisément.
- **Effet miroir** : sans effet sur le repère, les clics sont convertis automatiquement.

---

## 8. Lire les résultats

### 8.1 Coordonnées dans l'image (en pixels)

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

### 8.2 Position 3D d'un marqueur (en mm, après calibration)

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

### 8.3 Rotations d'un marqueur (en degrés, après calibration)

La référence (0°, 0°, 0°) est un marqueur **droit, face à la caméra**.

| Rotation | Signification |
|---|---|
| **Z** | Rotation du marqueur dans son propre plan, comme une aiguille de montre |
| **X** | Inclinaison haut/bas (le marqueur bascule vers l'avant ou l'arrière) |
| **Y** | Inclinaison gauche/droite (le marqueur pivote comme une porte) |

**Cas d'une caméra au plafond qui regarde le sol** : les marqueurs posés à plat sur le robot font face à la caméra. **X** et **Y** restent alors proches de 0, **rotation Z** donne le **cap du robot**, et les positions **X** et **Y** donnent sa position sur le sol par rapport à l'axe de la caméra.

Les inclinaisons X et Y sont moins précises (1 à 2°) que la position et la rotation Z : elles se déduisent de la faible déformation du marqueur. Elles deviennent plus fiables quand le marqueur est grand dans l'image.

---

## 9. Envoyer les positions à un autre programme

L'application peut envoyer en temps réel tout ce qu'elle détecte (carrés, marqueurs, positions 3D) à un autre programme, sur le même ordinateur ou sur le réseau. Par exemple, au programme qui pilote votre robot. Les données sont envoyées en **JSON**, par le protocole réseau **UDP**.

### 9.1 Activer l'envoi

1. Ouvrez l'onglet **Envoi UDP**.
2. Saisissez l'**Adresse** de la machine qui reçoit les données :
   - `127.0.0.1` : le même ordinateur (valeur par défaut) ;
   - l'adresse IP d'une autre machine du réseau (par exemple `192.168.1.42`) ou son nom ;
   - `255.255.255.255` : toutes les machines du réseau local.
3. Choisissez le **Port** (5005 par défaut). Le programme qui reçoit doit écouter ce même port.
4. Cochez **Envoyer les positions**.

Sous les réglages, le compteur **Messages envoyés** augmente à chaque image traitée. L'envoi continue quel que soit l'onglet affiché. L'adresse, le port et l'activation sont mémorisés.

### 9.2 Tester la réception

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

### 9.3 Format des messages

Chaque image traitée produit **un message**, sur une seule ligne. Voici un exemple, mis en forme pour la lecture :

```json
{
  "frame": 12,
  "timestamp_ms": 1791830806877,
  "image": {"width": 800, "height": 600},
  "squares": [
    {"center": [664.8, 483.8], "side": 87.5, "angle": 0,
     "corners": [[621, 440], [708.5, 440], [708.5, 527.5], [621, 527.5]]}
  ],
  "markers": [
    {"id": 7, "center": [200.5, 199.5], "side": 149.9, "angle": 0,
     "corners": [[125.6, 124.6], [275.4, 124.6], [275.4, 274.4], [125.6, 274.4]],
     "floor": {"position_mm": [149.3, 101.2], "heading_deg": -89.8},
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
| `center` | Centre `[x, y]`, en pixels (voir [8.1](#81-coordonnées-dans-limage-en-pixels)) |
| `side` | Côté moyen, en pixels |
| `angle` | Angle en degrés : de 0 à 90 pour un carré, de −180 à 180 pour un marqueur |
| `corners` | Les 4 coins `[x, y]`, en pixels. Pour un marqueur, le premier est le coin d'origine (point rouge à l'écran). |
| `id` | Identifiant du marqueur |
| `floor` | Uniquement si le repère du sol est défini : `position_mm` `[X, Y]` sur le sol, et `heading_deg` (cap d'un marqueur) ou `angle_deg` (angle d'un carré, de 0 à 90), voir [7.3](#73-lire-les-positions-au-sol) |
| `pose` | Uniquement si la caméra est calibrée : `position_mm` `[X, Y, Z]`, `rotation_deg` `[X, Y, Z]` et `distance_mm`, comme dans le panneau Résultat (voir [8.2](#82-position-3d-dun-marqueur-en-mm-après-calibration) et [8.3](#83-rotations-dun-marqueur-en-degrés-après-calibration)) |

Les valeurs sont arrondies au dixième. Comme à l'écran, les coordonnées sont celles de l'image réelle de la caméra, même avec l'effet miroir.

### 9.4 Bon à savoir

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

## 10. Conseils pour une détection fiable

**Éclairage**
- Un éclairage **homogène et stable** est le facteur le plus important.
- Évitez le soleil direct, les ombres marquées et les spots qui créent des reflets.
- Évitez les éclairages qui scintillent (certains néons ou LED bon marché).

**Caméra**
- Fixez la caméra solidement : si elle bouge, toutes les positions bougent.
- Une caméra **au-dessus de la zone, qui regarde vers le bas**, donne les mesures les plus simples à exploiter.
- Laissez l'objectif propre ; une image floue gêne surtout les marqueurs ArUco.

**Robot en mouvement**
- Un déplacement rapide rend l'image floue et peut faire perdre la détection. Un meilleur éclairage réduit ce flou, car la caméra peut alors prendre ses images plus rapidement.

**Performances**
- Le nombre d'images par seconde (i/s) est affiché dans **Résultat**.
- Choisissez une **résolution plus basse** (liste **Résolution**) : la détection est plus rapide.
- Désactivez les détections inutiles (case **Activer la détection** de chaque onglet) et le **Mode calibration** pour gagner en vitesse.

---

## 11. Questions fréquentes et dépannage

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
- La caméra n'est pas calibrée : l'onglet **Calibration** indique `Caméra non calibrée`. Suivez la [section 6](#6-calibrer-la-caméra).
- Le panneau **Résultat** affiche `Pas de pose 3D : calibration faite en ...` : la résolution choisie n'a pas le même format d'image que la calibration. Choisissez une résolution du format indiqué, ou recalibrez.

**Le nombre d'images par seconde est faible**
- Dans une pièce sombre, la webcam allonge son temps de pose et ralentit, quelle que soit la résolution : éclairez davantage la scène.
- Choisissez une résolution plus basse, et désactivez les détections inutiles et le **Mode calibration**.

**Les distances sont fausses**
- **Toujours trop grandes ou trop petites dans la même proportion** → la **Taille du marqueur** est mal saisie, ou le **Côté d'une case** était faux pendant la calibration (recalibrez dans ce cas).
- **Fausses surtout sur les bords de l'image** → la calibration manque de captures avec le damier dans les coins. Refaites-la.

**Le damier n'est pas détecté en mode calibration**
- Le damier doit être **entièrement visible**, marge blanche comprise.
- Vérifiez le nombre de **coins intérieurs** : il y en a un de moins que de cases dans chaque direction.
- Évitez les reflets sur le papier et rapprochez le damier.

**Les captures ont disparu**
- Changer les dimensions du damier (coins ou taille des cases) efface les captures en cours. Elles sont aussi effacées si la résolution de la caméra change pendant la série.

**Pas de position au sol**
- Le repère n'est pas défini : l'onglet **Sol** indique `Repère du sol non défini`. Suivez la [section 7](#7-définir-le-repère-du-sol).
- Le panneau **Résultat** affiche `Pas de position au sol : repère défini en ...` : la résolution choisie n'a pas le même format d'image que lors de la définition du repère.

**Les positions au sol sont décalées**
- La caméra a bougé depuis la définition du repère : redéfinissez-le.
- Vérifiez les **longueurs X et Y** saisies et l'**ordre des 4 points**.
- Le décalage augmente vers les bords de l'image : les marqueurs sont plus hauts que le sol (voir [7.4](#74-précision-et-limites)), ou l'objectif déforme l'image (calibrez la caméra).

**Le programme de réception ne reçoit rien**
- Vérifiez que **Envoyer les positions** est coché et que le compteur **Messages envoyés** augmente.
- Le **port** doit être le même des deux côtés.
- Vers une autre machine : vérifiez l'**adresse** et le **pare-feu** de la machine qui reçoit. Testez d'abord avec `python3 tools/udp_receiver.py` sur cette machine.
- Un seul programme peut écouter un port donné sur une machine : fermez les autres récepteurs.

**Comment repartir de zéro pour la calibration ?**
- Supprimez le fichier `~/.config/RobotDetectionByWebcam/calibration.yml`, puis relancez l'application.

---

## 12. Fichiers utilisés par l'application

| Fichier | Contenu |
|---|---|
| `~/.config/RobotDetectionByWebcam/settings.ini` | Réglages de l'application (voir ci-dessous) |
| `~/.config/RobotDetectionByWebcam/calibration.yml` | Calibration de la caméra (créé par **Calibrer**, relu à chaque lancement). Peut être copié sur une autre machine utilisant la même webcam. |
| `aruco_<dictionnaire>_<id>.png` | Marqueurs générés, à l'emplacement choisi |
| `damier_<largeur>x<hauteur>.png` | Damiers générés, à l'emplacement choisi |

### Réglages mémorisés

L'application retrouve au lancement les réglages de la fois précédente :

- **Webcam** : caméra choisie (si elle est branchée), résolution (pour chaque modèle de caméra) et effet miroir ;
- **Carré coloré** : activation, couleur et tous les curseurs ;
- **ArUco** : activation, dictionnaire et taille du marqueur ;
- **Calibration** : dimensions du damier et taille des cases ;
- **Sol** : les 4 points du repère, les dimensions du rectangle et l'affichage de la zone ;
- **Envoi UDP** : activation, adresse et port ;
- **Fenêtre** : taille, position et onglet ouvert.

Le fichier contient aussi une option sans équivalent dans l'interface : `hideLibraryMessages`, dans la section `[logging]`. Elle vaut `true` par défaut et masque des messages techniques sans gravité dans le terminal (par exemple « Corrupt JPEG data »). Mettez-la à `false` pour les réafficher, par exemple pour signaler un problème. Ce réglage est pris en compte au lancement suivant.

La résolution est enregistrée dès qu'on la choisit. Les autres réglages sont enregistrés **à la fermeture de l'application**, une fois la fermeture confirmée. Si l'application est arrêtée brutalement (Ctrl+C dans le terminal, plantage), les changements de la session sont perdus.

Ne sont **pas** mémorisés, car ils ne servent que ponctuellement : **Afficher le masque**, **Mode calibration**, **Capture automatique** et les captures du damier en cours.

Pour revenir aux réglages par défaut, fermez l'application et supprimez le fichier `settings.ini`. La calibration, enregistrée à part dans `calibration.yml`, est conservée.
