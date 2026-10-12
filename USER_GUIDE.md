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
7. [Lire les résultats](#7-lire-les-résultats)
8. [Conseils pour une détection fiable](#8-conseils-pour-une-détection-fiable)
9. [Questions fréquentes et dépannage](#9-questions-fréquentes-et-dépannage)
10. [Fichiers utilisés par l'application](#10-fichiers-utilisés-par-lapplication)

---

## 1. Présentation

L'application filme la scène avec une webcam et repère le robot de deux façons, utilisables séparément ou ensemble.

| Méthode | Ce qu'il faut sur le robot | Ce qu'on obtient | Points forts | Limites |
|---|---|---|---|---|
| **Carré coloré** | Un carré d'une couleur vive (papier, gommette, pièce peinte) | Position et taille en pixels, angle de 0 à 90° | Très simple à mettre en place | Sensible à l'éclairage et aux objets de même couleur ; un seul robot facile à distinguer ; angle connu à 90° près |
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
│                                       │   Caméra [............... ▼]  │
│                                       │   [ ] Effet miroir            │
│                                       ├───────────────────────────────┤
│          Image de la webcam           │ [Carré coloré][ArUco][Calibr.]│
│      (avec les détections dessinées)  │                               │
│                                       │   Réglages de l'onglet        │
│    Cliquer sur l'image = choisir      │   sélectionné                 │
│    la couleur du carré                │                               │
│                                       ├───────────────────────────────┤
│                                       │ Résultat                      │
│                                       │   Image, images/s, carré,     │
│                                       │   marqueurs...                │
└───────────────────────────────────────┴───────────────────────────────┘
```

- **Zone d'image** (à gauche) : l'image de la caméra, avec les contours des carrés, des marqueurs et du damier détectés. Le curseur en croix rappelle qu'un clic sert à choisir une couleur.
- **Webcam** :
  - **Caméra** : la webcam utilisée ;
  - **Effet miroir** : retourne l'image horizontalement, comme dans un miroir. ⚠️ À laisser **décoché** pour les marqueurs ArUco et la calibration : un marqueur vu en miroir n'est plus reconnu.
- **Onglets** :
  - **Carré coloré** : couleur et réglages de la détection du carré ;
  - **ArUco** : dictionnaire et taille des marqueurs ;
  - **Calibration** : damier et calibration de la caméra.
- **Résultat** : taille de l'image, nombre d'images par seconde (i/s) et mesures détaillées.

---

## 4. Suivre un carré coloré

### 4.1 Préparer le carré

- Choisissez une **couleur vive et franche** : rouge, vert, bleu, jaune, orange, magenta...
- Évitez le blanc, le noir et le gris : ils n'ont pas de « teinte » et ne peuvent pas être détectés.
- Choisissez une couleur **absente du reste de la scène** (sol, murs, meubles, vêtements).
- Préférez une surface **mate** : les reflets apparaissent blancs et « trouent » le carré.
- Le carré doit être **plein**, sans motif ni inscription.

### 4.2 Régler la détection

1. Ouvrez l'onglet **Carré coloré** et vérifiez que **Activer la détection** est coché.
2. **Choisissez la couleur** :
   - **le plus simple : cliquez sur le carré dans l'image**. L'application prend la couleur moyenne autour du clic, puis règle automatiquement la saturation et la luminosité minimales ;
   - ou cliquez sur le bouton **Couleur** (il affiche la couleur actuelle) pour la choisir dans une palette.
3. Cochez **Afficher le masque** : l'image devient noir et blanc. **En blanc** apparaissent les pixels reconnus comme étant de la bonne couleur.
   - Le but : le carré doit être **blanc et plein**, et le reste de l'image **noir**.
4. Ajustez les curseurs si besoin (voir le tableau ci-dessous), puis décochez **Afficher le masque**.

### 4.3 Les réglages

| Réglage | Valeur par défaut | À quoi il sert | Quand le modifier |
|---|---|---|---|
| **Tolérance teinte** | 15 | Écart de couleur accepté autour de la couleur choisie | **Augmenter** si le carré est incomplet dans le masque (ombre, éclairage inégal). **Diminuer** si d'autres objets de couleur proche apparaissent en blanc. |
| **Saturation min** | 90 | Écarte les couleurs ternes ou grisâtres | **Diminuer** si le carré est pâle ou trop éclairé. **Augmenter** si des zones grises ou beiges apparaissent en blanc. |
| **Luminosité min** | 60 | Écarte les zones sombres | **Diminuer** si le carré est dans l'ombre. **Augmenter** si des zones sombres parasites apparaissent. |
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
- Dans **Résultat**, une ligne par marqueur, triés par identifiant : position et angle dans l'image, puis, si la caméra est calibrée, position 3D, distance et rotations (voir [section 7](#7-lire-les-résultats)).

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

1. Décochez **Effet miroir**.
2. Cochez **Mode calibration**.
3. Montrez le damier à la caméra. Quand il est **entièrement visible**, ses coins s'affichent **en bleu clair** et le texte `Damier détecté` apparaît en haut de l'image. Sinon, le texte `Damier non détecté` s'affiche en rouge.
4. Capturez des images :
   - **manuellement**, avec le bouton **Capturer**, quand le damier est bien placé ;
   - ou **automatiquement**, en cochant **Capture automatique (toutes les 2 s)** : il suffit alors de déplacer lentement le damier.

   Un **cadre blanc** autour de l'image confirme chaque capture. Le compteur augmente sous les boutons.
5. Faites **15 à 25 captures**, en variant les positions :

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
- Si la caméra passe à un **format d'image différent** (de 4:3 à 16:9, par exemple). Un simple changement de résolution dans le même format ne demande pas de recalibrer.

### 6.5 Vérifier la calibration

Posez un marqueur à une distance connue de la caméra, mesurée au mètre ruban, face à l'objectif. La **distance** affichée doit correspondre, à 1 ou 2 % près. Sinon, vérifiez la **Taille du marqueur** et le **Côté d'une case**.

---

## 7. Lire les résultats

### 7.1 Coordonnées dans l'image (en pixels)

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
- **Angle d'un marqueur** : 0° quand le marqueur est droit, positif quand il tourne **dans le sens des aiguilles d'une montre**, de −180° à 180°.
- **Angle d'un carré** : de 0° à 90° seulement (voir [section 4.4](#44-ce-qui-saffiche)).

### 7.2 Position 3D d'un marqueur (en mm, après calibration)

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

### 7.3 Rotations d'un marqueur (en degrés, après calibration)

La référence (0°, 0°, 0°) est un marqueur **droit, face à la caméra**.

| Rotation | Signification |
|---|---|
| **Z** | Rotation du marqueur dans son propre plan, comme une aiguille de montre |
| **X** | Inclinaison haut/bas (le marqueur bascule vers l'avant ou l'arrière) |
| **Y** | Inclinaison gauche/droite (le marqueur pivote comme une porte) |

**Cas d'une caméra au plafond qui regarde le sol** : les marqueurs posés à plat sur le robot font face à la caméra. **X** et **Y** restent alors proches de 0, **rotation Z** donne le **cap du robot**, et les positions **X** et **Y** donnent sa position sur le sol par rapport à l'axe de la caméra.

Les inclinaisons X et Y sont moins précises (1 à 2°) que la position et la rotation Z : elles se déduisent de la faible déformation du marqueur. Elles deviennent plus fiables quand le marqueur est grand dans l'image.

---

## 8. Conseils pour une détection fiable

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
- Désactivez les détections inutiles (case **Activer la détection** de chaque onglet) et le **Mode calibration** pour gagner en vitesse.

---

## 9. Questions fréquentes et dépannage

**L'image reste noire avec « En attente de la webcam... »**
- Vérifiez que la webcam est branchée et choisissez-la dans la liste **Caméra**.
- Fermez les autres logiciels qui l'utilisent (visioconférence, navigateur...).
- Vérifiez que votre utilisateur a accès à la caméra : `ls /dev/video*`, et appartenance au groupe `video` (`groups`).

**« Aucune webcam détectée »**
- Aucune caméra n'a été trouvée au lancement. Branchez-la, puis relancez l'application.

**Le carré n'est jamais détecté**
1. Cliquez à nouveau sur le carré dans l'image pour reprendre sa couleur.
2. Cochez **Afficher le masque** :
   - **carré noir ou incomplet** → augmentez **Tolérance teinte**, diminuez **Saturation min** et **Luminosité min** ;
   - **carré blanc et plein, mais pas de contour vert** → diminuez **Surface min** (carré trop petit) ou **Remplissage min** (forme imparfaite).

**D'autres objets sont détectés comme des carrés**
- Diminuez **Tolérance teinte**, augmentez **Saturation min** ou **Surface min**.
- Le carré principal, en vert, est toujours le plus grand : retirez de la scène les objets de même couleur ou choisissez une couleur plus rare.

**Un marqueur ArUco n'est pas détecté**
- Vérifiez que le **dictionnaire** sélectionné est celui du marqueur imprimé.
- Vérifiez que **Effet miroir** est décoché.
- La **marge blanche** autour du marqueur doit être présente et visible.
- Le marqueur doit être net, sans reflet, et mesurer au moins 30 pixels dans l'image : rapprochez-le ou imprimez-le plus grand.

**Les positions 3D (X, Y, Z) ne s'affichent pas**
- La caméra n'est pas calibrée : l'onglet **Calibration** indique `Caméra non calibrée`. Suivez la [section 6](#6-calibrer-la-caméra).

**Les distances sont fausses**
- **Toujours trop grandes ou trop petites dans la même proportion** → la **Taille du marqueur** est mal saisie, ou le **Côté d'une case** était faux pendant la calibration (recalibrez dans ce cas).
- **Fausses surtout sur les bords de l'image** → la calibration manque de captures avec le damier dans les coins. Refaites-la.

**Le damier n'est pas détecté en mode calibration**
- Le damier doit être **entièrement visible**, marge blanche comprise.
- Vérifiez le nombre de **coins intérieurs** : il y en a un de moins que de cases dans chaque direction.
- Évitez les reflets sur le papier et rapprochez le damier.

**Les captures ont disparu**
- Changer les dimensions du damier (coins ou taille des cases) efface les captures en cours. Elles sont aussi effacées si la résolution de la caméra change pendant la série.

**Comment repartir de zéro pour la calibration ?**
- Supprimez le fichier `~/.config/RobotDetectionByWebcam/calibration.yml`, puis relancez l'application.

---

## 10. Fichiers utilisés par l'application

| Fichier | Contenu |
|---|---|
| `~/.config/RobotDetectionByWebcam/calibration.yml` | Calibration de la caméra (créé par **Calibrer**, relu à chaque lancement). Peut être copié sur une autre machine utilisant la même webcam. |
| `aruco_<dictionnaire>_<id>.png` | Marqueurs générés, à l'emplacement choisi |
| `damier_<largeur>x<hauteur>.png` | Damiers générés, à l'emplacement choisi |

Les réglages de détection (couleur, curseurs, dictionnaire, taille du marqueur) **ne sont pas enregistrés** : ils reprennent leurs valeurs par défaut à chaque lancement.
