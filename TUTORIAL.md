# Tutoriel — RobotDetectionByWebcam

Ce tutoriel vous fait découvrir **toutes les fonctionnalités** de l'application, pas à pas, avec du matériel simple. Chaque partie se termine par une vérification, pour savoir si tout fonctionne avant de passer à la suite.

Les parties s'enchaînent, mais vous pouvez vous arrêter après n'importe laquelle. Le [guide utilisateur](USER_GUIDE.md) détaille chaque réglage ; ce tutoriel y renvoie quand c'est utile.

| Partie | Ce que vous allez faire | Durée estimée |
|---|---|---|
| [0. Préparer](#0-préparer) | Réunir le matériel, compiler l'application | 15 min |
| [1. Prendre en main la caméra](#1-prendre-en-main-la-caméra) | Choisir la résolution, régler l'image | 10 min |
| [2. Détecter un carré coloré](#2-détecter-un-carré-coloré) | Repérer un carré de papier rouge, puis blanc | 15 min |
| [3. Utiliser les marqueurs ArUco](#3-utiliser-les-marqueurs-aruco) | Imprimer et détecter des marqueurs numérotés | 15 min |
| [4. Suivre des objets quelconques](#4-suivre-des-objets-quelconques) | Encadrer une tasse puis un second objet, suivre leurs trajectoires et leurs mouvements | 10 min |
| [5. Calibrer la caméra](#5-calibrer-la-caméra) | Mesurer l'objectif avec un damier, obtenir la position 3D | 20 min |
| [6. Mesurer sur le sol en millimètres](#6-mesurer-sur-le-sol-en-millimètres) | Tracer un rectangle au ruban adhésif, lire des positions en mm | 15 min |
| [7. Lisser les positions et mesurer la vitesse](#7-lisser-les-positions-et-mesurer-la-vitesse) | Mesurer une vitesse, voir une position prédite | 10 min |
| [8. Envoyer les positions à un programme](#8-envoyer-les-positions-à-un-programme) | Recevoir les positions en Python, calculer un cap vers une cible | 20 min |
| [9. Mise en situation : suivre un robot](#9-mise-en-situation--suivre-un-robot) | Installer le tout pour un vrai robot | 20 min |

---

## 0. Préparer

### Matériel

- Une **webcam** USB, idéalement sur un pied ou une pince : elle devra rester immobile à partir de la partie 5.
- Une **imprimante** et du papier, de préférence **mat** : le papier brillant crée des reflets.
- Une **règle** ou un **mètre ruban**.
- Du **ruban adhésif** de couleur (pour la partie 6).
- Une **feuille de couleur vive**, rouge par exemple, et une feuille blanche, pour découper deux carrés d'environ 5 × 5 cm.
- Un **carton rigide** de format A4 ou plus, pour coller le damier de calibration.
- Un objet quelconque à suivre : une tasse, une petite voiture...
- Pour la partie 8 : **Python 3**, déjà installé sur la plupart des systèmes Linux.

### Compiler et lancer l'application

Si l'application n'est pas encore compilée, suivez le [README](README.md#prérequis). En résumé, sous Debian ou Ubuntu :

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev libopencv-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/RobotDetectionByWebcam
```

✅ **Vérification** : la fenêtre s'ouvre et l'image de la webcam s'affiche à gauche. Sinon, voir [le dépannage](USER_GUIDE.md#12-questions-fréquentes-et-dépannage).

---

## 1. Prendre en main la caméra

**Objectif** : connaître l'écran et régler la caméra pour avoir une image fluide.

### 1.1 Faire le tour de l'écran

- À **gauche**, l'image de la caméra, sur laquelle les détections sont dessinées.
- En **haut à droite**, le groupe **Webcam** : **Caméra**, **Résolution**, **Effet miroir** et le bouton **Réglages de l'image...**.
- Au **milieu à droite**, six onglets : **Carré coloré**, **ArUco**, **Calibration**, **Sol**, **Suivi** et **Envoi UDP**.
- En **bas à droite**, le panneau **Résultat**, qui affiche les mesures, et le bouton **Quitter**.

### 1.2 Choisir la résolution

1. Ouvrez la liste **Résolution**. Chaque ligne indique la taille de l'image, son format et le nombre maximal d'images par seconde, par exemple `640 x 480 (4:3) — 30 i/s`.
2. Choisissez **640 x 480**. C'est un bon point de départ : la détection est rapide.
3. Regardez la première ligne du panneau **Résultat** : `Image : 640 x 480 — ... i/s`.

Le choix est mémorisé : la caméra redémarrera dans cette résolution au prochain lancement.

### 1.3 Régler l'image

Dans une pièce peu éclairée, beaucoup de webcams tombent à 15 images par seconde : la caméra allonge son temps de pose, et un objet qui bouge devient flou.

1. Cliquez sur **Réglages de l'image...**. Une petite fenêtre s'ouvre ; laissez-la à côté de l'application.
2. Dans **Exposition**, passez le **Mode** sur **Manuelle**.
3. Déplacez le curseur **Temps de pose** vers **20 à 30 ms**. La ligne « Appliqué par la caméra » indique la valeur réellement utilisée : la caméra l'arrondit.
4. Regardez le nombre d'images par seconde dans **Résultat** : il doit approcher 30. L'image devient plus sombre ; si elle l'est trop, allongez un peu la pose ou éclairez la pièce.
5. Dans **Autres réglages**, à droite, réglez **Fréquence du secteur** sur **50 Hz** si vous êtes en Europe (60 Hz en Amérique du Nord) : les bandes qui défilent sous un éclairage artificiel disparaissent. Les autres curseurs (luminosité, contraste, gain...) indiquent « (défaut) » à la valeur d'usine.
6. Pour l'instant, cliquez sur **Tout en automatique**, puis sur **Fermer**. Nous reviendrons à ces réglages dans la partie 9.

### 1.4 Essayer l'effet miroir

Cochez **Effet miroir** : l'image se retourne, comme dans un miroir, ce qui est plus naturel quand on se place face à la caméra. Ce réglage ne change **que l'affichage** : les mesures restent celles de l'image réelle. Décochez-le pour la suite.

✅ **Vérification** : l'image est fluide et vous savez où trouver la résolution, les réglages de l'image et le panneau **Résultat**.

---

## 2. Détecter un carré coloré

**Objectif** : repérer un carré de couleur et lire sa position.

### 2.1 Un carré rouge

1. Découpez un carré d'environ **5 × 5 cm** dans la feuille rouge.
2. Posez-le sur un fond **uni et non rouge** (une table, une feuille grise...), bien visible par la caméra.
3. Ouvrez l'onglet **Carré coloré** et vérifiez que **Activer la détection** est coché.
4. **Cliquez sur le carré** dans l'image. Le bouton **Couleur** prend sa couleur, le **Type** passe sur « Couleur vive (teinte) » et les seuils se règlent seuls.

Le carré est entouré **en vert**, avec ses coordonnées en pixels au-dessus. Le panneau **Résultat** affiche :

```
Carré détecté (1 au total)
Centre : (312, 240) px
Côté : 58 px
Angle : 12.0°
```

5. Faites tourner le carré : l'**Angle** change, de 0 à 90° (un carré est identique à lui-même tous les 90°).

### 2.2 Comprendre le masque

1. Cochez **Afficher le masque** : l'image devient noir et blanc. Les pixels reconnus comme « rouges » sont en blanc.
2. Le carré doit être **blanc et plein**, le reste **noir**. S'il est troué, augmentez **Tolérance teinte** ; si d'autres objets apparaissent en blanc, diminuez-la ou augmentez **Saturation min**.
3. Posez un **disque** rouge à côté (une capsule, un rond de papier) : il apparaît en blanc dans le masque, mais il n'est **pas** entouré en vert. L'application vérifie la forme et écarte ce qui n'est pas carré.
4. Décochez **Afficher le masque**.

Le rôle de chaque curseur est expliqué dans le [guide](USER_GUIDE.md#43-les-réglages).

### 2.3 Un carré blanc

Le blanc n'a pas de « teinte » : il se détecte autrement.

1. Posez le carré **blanc** sur un fond **sombre** : le blanc doit contraster avec ce qui le touche.
2. Cliquez dessus. Le **Type** passe sur « **Blanc, gris ou noir** », et les curseurs changent : **Tolérance luminosité** et **Saturation max**.
3. Le carré blanc est entouré en vert.

Sous une lumière chaude (ampoule), le blanc paraît orangé ; si le carré est mal détecté, augmentez **Saturation max**.

✅ **Vérification** : le carré rouge, puis le carré blanc, sont entourés en vert, et le disque est ignoré.

Pour la suite, cliquez à nouveau sur le carré **rouge** pour revenir à cette couleur.

---

## 3. Utiliser les marqueurs ArUco

**Objectif** : détecter des marqueurs numérotés, plus fiables qu'un carré de couleur, et connaître leur orientation complète.

### 3.1 Imprimer deux marqueurs

1. Ouvrez l'onglet **ArUco**. Laissez le **Dictionnaire** sur **4x4 (50)**.
2. Cliquez sur **Générer un marqueur...**, saisissez l'identifiant **1**, puis enregistrez l'image (`aruco_4x4_1.png`).
3. Recommencez avec l'identifiant **2**.
4. Imprimez les deux images de façon à ce que chacune mesure **7,5 cm** de côté, sans les déformer. La partie noire occupe les deux tiers de l'image : elle mesurera donc 5 cm.
5. Découpez-les en **gardant la marge blanche** autour de la partie noire : elle est indispensable à la détection.

### 3.2 Indiquer leur taille

Mesurez à la règle **le côté de la partie noire** d'un marqueur, sans la marge, et saisissez-le dans **Taille du marqueur** (50 mm si l'impression est exacte). Cette valeur servira à la position 3D (partie 5).

### 3.3 Les détecter

1. Montrez le marqueur **1** à la caméra.
2. Il est entouré **en magenta**, avec son numéro. Un **point rouge** marque son coin d'origine (en haut à gauche quand il est droit), et un trait va du centre vers son bord supérieur.
3. Le panneau **Résultat** affiche :

```
1 marqueur(s) ArUco
ID 1 : (320, 241) px, côté 82 px, 0.0°
```

4. Faites tourner le marqueur à plat : l'angle va de **−180 à 180°**. Contrairement au carré coloré, on connaît le sens du marqueur : c'est ce qui permet de connaître la direction d'un robot.
5. Montrez les **deux** marqueurs en même temps : chacun est reconnu par son numéro.

✅ **Vérification** : les marqueurs 1 et 2 sont détectés ensemble, avec leur numéro et leur angle.

**À retenir** : collez le marqueur sur un robot avec son bord supérieur vers l'avant du robot. L'angle du marqueur sera alors la direction du robot.

---

## 4. Suivre des objets quelconques

**Objectif** : suivre un ou plusieurs objets sans couleur particulière ni marqueur, et voir leurs trajectoires.

1. Posez une **tasse** (ou un autre objet bien visible) devant la caméra.
2. Ouvrez l'onglet **Suivi** et laissez la **Méthode** sur **CSRT (précis)**.
3. Cliquez sur **Ajouter des objets**.
4. **Tracez un rectangle** autour de la tasse (cliquer-glisser) : un cadre en pointillés apparaît avec des poignées. Ajustez-le si besoin, puis appuyez sur **Entrée** : le suivi démarre, et « Objet 1 » apparaît dans la liste des objets. Vous pouvez aussi cliquer simplement sur la tasse : l'application la délimite toute seule.
5. Posez un **second objet** (un stylo, un téléphone...) et tracez directement son rectangle, puis appuyez sur **Entrée** : il devient « Objet 2 », dans une autre couleur. Appuyez ensuite sur **Échap** pour terminer l'ajout.
6. Dans la liste des objets, **double-cliquez sur « Objet 1 »** et renommez-le « Tasse ».
7. Déplacez lentement la tasse :
   - elle reste entourée d'un **cadre** de sa couleur, avec son nom et sa vitesse au-dessus ;
   - une **ligne** de la même couleur dessine sa trajectoire sur les 10 dernières secondes ;
   - une **flèche** indique le sens du déplacement.
8. Le panneau **Résultat** affiche `Tasse : en mouvement`, puis `immobile` quand vous la lâchez, avec sa position, sa vitesse et sa direction. Le second objet a son propre paragraphe. Faites défiler l'onglet **Suivi** jusqu'au **Journal des événements** : un « Début de mouvement », puis une « Fin de mouvement » avec la distance parcourue et la durée, y sont inscrits.
9. Cachez entièrement la tasse avec la main : le cadre passe en pointillés, d'abord « Objet (prédit) », puis **rouge** avec « Objet perdu ». L'application cherche alors la tasse dans toute l'image. Retirez la main, ou reposez la tasse **ailleurs** : le suivi reprend tout seul, et le panneau indique « retrouvé automatiquement 1 fois ». Pendant le suivi, la **confiance** affichée dans **Résultat** indique à quel point le cadre ressemble encore à la tasse.
10. Cliquez sur **Tout retirer**.

✅ **Vérification** : les deux objets sont suivis en même temps, chacun dans sa couleur, et l'état de la tasse passe de « en mouvement » à « immobile ».

**À retenir** : ce suivi ne demande aucune préparation, mais il peut perdre un objet, ne donne pas son orientation, et chaque objet ralentit un peu l'application. Pour un robot, les marqueurs ArUco restent plus fiables. Les limites sont détaillées dans le [guide](USER_GUIDE.md#63-limites).

---

## 5. Calibrer la caméra

**Objectif** : mesurer les caractéristiques de l'objectif, pour obtenir la **position 3D** des marqueurs en millimètres.

La calibration se fait une seule fois par caméra. Fixez la caméra si possible.

### 5.1 Préparer le damier

1. Ouvrez l'onglet **Calibration**. Laissez **9** et **6** coins intérieurs.
2. Cliquez sur **Générer un damier...** et enregistrez l'image (`damier_9x6.png`).
3. Imprimez-la **à 100 %**, sans « ajuster à la page ».
4. Collez la feuille **bien à plat** sur le carton rigide.
5. Mesurez **une case** à la règle et saisissez sa taille dans **Côté d'une case**.

### 5.2 Faire les captures

1. Cochez **Mode calibration**.
2. Montrez le damier en entier : ses coins s'affichent en **bleu clair** et `Damier détecté` apparaît en haut de l'image.
3. Cochez **Capture automatique (toutes les 2 s)**.
4. Déplacez **lentement** le damier, en marquant une pause à chaque position. Un **cadre blanc** confirme chaque capture. Variez les positions :
   - proche, puis loin ;
   - au centre, puis dans **chacun des quatre coins** de l'image ;
   - incliné vers la gauche, la droite, le haut et le bas.
5. Arrêtez-vous vers **20 captures** (compteur sous les boutons).

### 5.3 Calculer

1. Cliquez sur **Calibrer**. Après quelques secondes, une fenêtre indique l'**erreur RMS**.
2. En dessous de **0,5 px**, c'est très bon. Au-dessus de 1 px, cliquez sur **Effacer les captures** et recommencez, avec un damier plus plat et des images plus nettes.
3. Décochez **Mode calibration** : il ralentit l'application.

L'onglet affiche maintenant `Caméra calibrée`. La calibration est enregistrée pour cette caméra et rechargée à chaque lancement. Si vous avez plusieurs webcams, calibrez chacune une fois : chaque caméra retrouve la sienne quand vous la choisissez dans la liste **Caméra**.

### 5.4 Lire la position 3D d'un marqueur

1. Montrez le marqueur **1**, face à la caméra. Ses **axes** sont maintenant dessinés : rouge (droite), vert (bas), bleu (vers la caméra).
2. Le panneau **Résultat** ajoute sa position en millimètres :

```
ID 1 : (318, 236) px, côté 80 px, 1.2°
  X 12  Y -8  Z 412 mm (distance 412 mm)
  rotation X 2.1°  Y -1.5°  Z 1.2°
```

3. **Testez la précision** : placez le marqueur à **50 cm** de l'objectif, mesurés au mètre ruban. La **distance** affichée doit être proche de 500 mm (à 1 ou 2 % près). Sinon, vérifiez la **Taille du marqueur** et le **Côté d'une case**.

✅ **Vérification** : la distance affichée correspond à la distance mesurée au mètre ruban.

---

## 6. Mesurer sur le sol en millimètres

**Objectif** : obtenir la position des objets **sur une table ou au sol**, en millimètres, dans un repère que vous choisissez.

Pour cette partie, la caméra doit être **fixe** et voir la table de haut, plutôt à la verticale.

### 6.1 Tracer le rectangle de référence

1. Avec le ruban adhésif, marquez sur la table les **4 coins d'un rectangle de 600 × 400 mm**. Faites de petites croix, précises.
2. Vérifiez que c'est bien un rectangle : les deux diagonales doivent mesurer la même longueur (721 mm).

### 6.2 Définir le repère

1. Ouvrez l'onglet **Sol**.
2. Saisissez **600** dans **Longueur X (point 1 → 2)** et **400** dans **Longueur Y (point 1 → 4)**.
3. Cliquez sur **Définir les 4 points**.
4. Cliquez sur les 4 croix dans l'image, **dans cet ordre**, tel que vous voyez la table à l'écran :
   1. **en bas à gauche** : l'origine (0, 0) ;
   2. **en bas à droite** : le point sur l'axe X (600, 0) ;
   3. **en haut à droite** : le coin opposé (600, 400) ;
   4. **en haut à gauche** : le point sur l'axe Y (0, 400).

   La consigne s'affiche en bas de l'image, et chaque point cliqué est numéroté.

Le rectangle s'affiche en pointillés orange, avec l'**axe X en rouge** et l'**axe Y en vert**. Avec cet ordre, les axes sont orientés comme sur un graphique : X vers la droite, Y vers le haut de l'écran.

### 6.3 Vérifier

1. Posez le marqueur **1** à plat au **centre** du rectangle. Le panneau **Résultat** affiche environ :

```
ID 1 : (322, 245) px, côté 64 px, 0.4°
  sol X 300  Y 200 mm, cap 90.4°
```

2. Déplacez-le de **10 cm vers la droite** : **X** doit augmenter d'environ 100.
3. Faites-le tourner : le **cap** est la direction de son bord supérieur, comptée depuis l'axe X, dans le sens inverse des aiguilles d'une montre avec cet ordre de points. Bord supérieur vers la droite de l'écran : environ 0° ; vers le haut : environ 90°.
4. Posez le carré rouge : il a aussi sa position sur le sol (`Sol : X ...  Y ... mm`).

✅ **Vérification** : au centre du rectangle, le marqueur est à environ (300, 200) mm.

**À retenir** : le repère est défini **au niveau de la table**. Un marqueur collé en hauteur sur un robot sera légèrement décalé. Le [guide](USER_GUIDE.md#84-précision-et-limites) explique comment limiter cet effet.

---

## 7. Lisser les positions et mesurer la vitesse

**Objectif** : obtenir des positions plus stables, la vitesse des objets, et une position estimée quand la détection échoue un instant.

1. Ouvrez l'onglet **Suivi**. Dans le groupe **Lissage des carrés et marqueurs** (actif par défaut), laissez **Force du lissage** sur **50**.
2. Faites glisser le marqueur **1** lentement sur la table. Une **trajectoire rose** le suit, et le panneau **Résultat** ajoute sa vitesse :

```
ID 1 : (402, 250) px, côté 64 px, 0.3°
  sol X 412  Y 198 mm, cap 90.2°
  vitesse 85 mm/s, direction 1°
```

3. Arrêtez-le : la vitesse redescend vers 0.
4. **Cachez le marqueur** avec la main pendant qu'il glisse : un **cercle en pointillés** marqué « 1 (prédit) » continue sur sa lancée pendant une demi-seconde, puis disparaît. Le panneau affiche `ID 1 : non détecté, position prédite (...)`.
5. Essayez la **Force du lissage** à **90** : la position est plus stable à l'arrêt, mais elle prend du retard quand vous déplacez le marqueur vite. Revenez à **50**.

✅ **Vérification** : la vitesse affichée augmente quand vous accélérez, et la position prédite apparaît quand le marqueur est caché.

---

## 8. Envoyer les positions à un programme

**Objectif** : récupérer les positions dans un autre programme, par exemple celui qui pilote un robot.

### 8.1 Activer l'envoi

1. Ouvrez l'onglet **Envoi UDP**. Laissez l'**Adresse** `127.0.0.1` (le même ordinateur) et le **Port** `5005`.
2. Cochez **Envoyer les positions**. Le compteur **Messages envoyés** augmente à chaque image.

### 8.2 Recevoir avec le script fourni

Dans un **second terminal**, depuis le dossier du projet :

```bash
python3 tools/udp_receiver.py
```

Une ligne s'affiche par image :

```
image 812 | carré (310, 242) px 12° | sol X 288 Y 196 mm | ArUco 1 (402, 250) px 0° | sol X 412 Y 198 mm cap 90° | carré vitesse 0 mm/s | ArUco 1 vitesse 85 mm/s
```

Si des objets sont suivis (partie 4), leurs débuts et fins de mouvement s'affichent aussi, par exemple `*** FIN DE MOUVEMENT (342, 299) px, déplacement 92 px en 2.4 s`. Avec `--raw`, le script affiche le message **JSON** brut. Le format complet est décrit dans le [guide](USER_GUIDE.md#103-format-des-messages).

### 8.3 Écrire son propre programme : aller vers une cible

Voici un petit programme qui reçoit les positions et calcule, pour le marqueur **1**, la **distance** et l'**angle à tourner** pour atteindre une cible sur le sol. C'est le cœur d'un programme de pilotage. Enregistrez-le sous `cible.py` :

```python
import json
import math
import socket

CIBLE = (500.0, 300.0)  # position de la cible sur le sol, en mm
MARQUEUR = 1            # identifiant du marqueur collé sur le robot


def ecart_angle(a):
    """Ramène un angle en degrés dans ]-180, 180]."""
    return (a + 180.0) % 360.0 - 180.0


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))
print("En attente des positions... (Ctrl+C pour arrêter)")

while True:
    msg = json.loads(sock.recv(65535))
    for m in msg.get("markers", []):
        if m["id"] != MARQUEUR or "floor" not in m:
            continue
        x, y = m["floor"]["position_mm"]
        cap = m["floor"]["heading_deg"]
        dx, dy = CIBLE[0] - x, CIBLE[1] - y
        distance = math.hypot(dx, dy)
        direction_cible = math.degrees(math.atan2(dy, dx))
        a_tourner = ecart_angle(direction_cible - cap)
        if distance < 30:
            ordre = "arrivé"
        elif abs(a_tourner) > 15:
            ordre = "tourner dans le sens positif" if a_tourner > 0 else "tourner dans le sens négatif"
        else:
            ordre = "avancer"
        print(f"robot ({x:.0f}, {y:.0f}) mm, cap {cap:.0f}° | cible à {distance:.0f} mm, "
              f"tourner de {a_tourner:+.0f}° -> {ordre}")
```

Lancez-le **à la place** de `udp_receiver.py`, car un seul programme peut écouter un port donné :

```bash
python3 cible.py
```

Posez le marqueur **1** dans le rectangle, puis déplacez-le et faites-le tourner : le programme indique s'il faut tourner ou avancer pour atteindre le point (500, 300).

Les angles sont comptés de l'axe X vers l'axe Y. Avec l'ordre de points de la partie 6 (axe Y vers le haut de l'écran), le **sens positif** est le sens **inverse des aiguilles d'une montre**, c'est-à-dire « tourner à gauche » pour un robot vu d'en haut.

✅ **Vérification** : quand le bord supérieur du marqueur pointe vers la cible, le programme affiche « avancer ».

### 8.4 Envoyer vers une autre machine

Pour envoyer les positions à un autre ordinateur ou à une carte (Raspberry Pi...) sur le même réseau, saisissez son adresse IP dans **Adresse**, et lancez le programme de réception sur cette machine. Vérifiez que son pare-feu laisse passer le port UDP choisi.

---

## 9. Mise en situation : suivre un robot

**Objectif** : réunir tout ce qui précède pour suivre un vrai robot qui roule sur une table ou au sol.

### 9.1 Installer la caméra

- Fixez la caméra **au-dessus** de la zone de jeu, si possible à la verticale : sur un pied photo, une potence ou au plafond.
- Elle doit voir **toute la zone**, avec un peu de marge.
- Choisissez une résolution assez haute pour voir les marqueurs de loin, par exemple **1280 x 960**, dans le **même format d'image** que la calibration (4:3 ici).
- Si vous utilisez une autre webcam que dans la partie 5, choisissez-la dans la liste **Caméra** et calibrez-la à son tour : chaque caméra a sa propre calibration et son propre repère du sol. Si la caméra a seulement été déplacée, la calibration reste valable, mais le repère du sol est à redéfinir.

### 9.2 Préparer le robot

- Collez un **marqueur ArUco** sur le dessus du robot, **bien à plat**, avec son **bord supérieur vers l'avant** du robot. Plus il est grand, plus il est détecté de loin : au moins 30 pixels de côté dans l'image.
- Mesurez la **hauteur du marqueur** au-dessus du sol. Le repère du sol est défini au niveau du sol, et un marqueur en hauteur est décalé : si la caméra est calibrée, saisissez cette hauteur dans **Hauteur des repères** (onglet **Sol**) pour corriger le décalage. Sinon, collez le marqueur le plus bas possible.
- Pour plusieurs robots, utilisez un identifiant différent pour chacun.

### 9.3 Définir la zone et régler l'image

1. Marquez un **grand rectangle** au sol, qui couvre la zone de jeu, et définissez le repère (partie 6).
2. Ouvrez **Réglages de l'image...** :
   - **Exposition** en **Manuelle**, avec un temps de pose court (20 à 30 ms) : moins de flou quand le robot roule, et 30 images par seconde ;
   - **Balance des blancs** en **Manuelle**, si vous utilisez aussi le carré coloré : les couleurs ne dérivent plus ;
   - si l'image est trop sombre avec la pose courte, montez un peu le **Gain** (dans **Autres réglages**) plutôt que d'allonger la pose.
3. Éclairez la zone de façon **homogène**.

### 9.4 Lancer le suivi

1. Activez **Envoyer les positions** (onglet **Envoi UDP**), avec l'adresse de la machine qui pilote le robot.
2. Lancez votre programme de pilotage, par exemple une version de `cible.py` qui envoie les ordres au robot au lieu de les afficher.
3. Pendant que le robot roule, vérifiez :
   - le **nombre d'images par seconde** dans **Résultat** (idéalement 25 à 30) ;
   - que le marqueur reste détecté sur toute la zone, y compris dans les coins ;
   - que les positions **prédites** sont rares. Si elles sont fréquentes, le marqueur est trop petit, trop flou ou mal éclairé.

### 9.5 Quitter proprement

Cliquez sur **Quitter**, puis confirmez. Tous les réglages sont enregistrés, et la webcam retrouve son exposition automatique et ses réglages d'origine pour les autres logiciels.

✅ **Vérification finale** : le programme de pilotage reçoit en continu la position et le cap du robot, en millimètres, pendant qu'il se déplace.

---

## Aide-mémoire

| Je veux... | Où | Comment |
|---|---|---|
| Une image plus fluide | **Réglages de l'image...** | Exposition manuelle, 20 à 30 ms |
| Détecter un carré de couleur | Onglet **Carré coloré** | Cliquer sur le carré dans l'image |
| Détecter un carré blanc, gris ou noir | Onglet **Carré coloré** | Cliquer dessus : le type « Blanc, gris ou noir » est choisi |
| Imprimer un marqueur | Onglet **ArUco** | **Générer un marqueur...** |
| Suivre un ou plusieurs objets | Onglet **Suivi** | **Ajouter des objets**, tracer un rectangle par objet en validant chacun avec **Entrée**, puis **Échap** |
| Savoir quand un objet suivi bouge | Onglet **Suivi** | **Journal des événements**, ou liste `events` en UDP |
| La position 3D d'un marqueur | Onglet **Calibration** | Calibrer une fois avec le damier |
| Des positions en mm sur le sol | Onglet **Sol** | **Définir les 4 points** d'un rectangle connu |
| La vitesse d'un marqueur | Onglet **Suivi** | Groupe **Lissage des carrés et marqueurs** |
| Récupérer les positions dans un programme | Onglet **Envoi UDP** | **Envoyer les positions**, puis `python3 tools/udp_receiver.py` |

## Pour aller plus loin

- Le [guide utilisateur](USER_GUIDE.md) détaille chaque réglage et contient une [section de dépannage](USER_GUIDE.md#12-questions-fréquentes-et-dépannage).
- Le [README](README.md) explique l'installation et le fonctionnement interne des détections.
- [IMPROVEMENTS.md](IMPROVEMENTS.md) liste les fonctionnalités et les améliorations envisagées.
