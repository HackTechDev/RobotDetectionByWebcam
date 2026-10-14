#!/usr/bin/env python3
"""Reçoit et affiche les positions envoyées en UDP par RobotDetectionByWebcam.

Exemples :
    python3 tools/udp_receiver.py              # écoute le port 5005
    python3 tools/udp_receiver.py --port 6000
    python3 tools/udp_receiver.py --raw        # affiche le JSON brut

Sert aussi de point de départ pour un programme de pilotage : la fonction
handle() reçoit chaque message déjà décodé.
"""

import argparse
import json
import socket


def describe(msg):
    """Résumé d'un message sur une ligne."""
    parts = [f"image {msg['frame']}"]

    squares = msg.get("squares")
    if squares is not None:
        if squares:
            s = squares[0]
            text = f"carré ({s['center'][0]:.0f}, {s['center'][1]:.0f}) px {s['angle']:.0f}°"
            floor = s.get("floor")
            if floor:
                x, y = floor["position_mm"]
                text += f" | sol X {x:.0f} Y {y:.0f} mm"
            parts.append(text)
        else:
            parts.append("pas de carré")

    for m in msg.get("markers", []):
        text = f"ArUco {m['id']} ({m['center'][0]:.0f}, {m['center'][1]:.0f}) px {m['angle']:.0f}°"
        floor = m.get("floor")
        if floor:
            x, y = floor["position_mm"]
            text += f" | sol X {x:.0f} Y {y:.0f} mm cap {floor['heading_deg']:.0f}°"
        pose = m.get("pose")
        if pose:
            x, y, z = pose["position_mm"]
            text += f" | X {x:.0f} Y {y:.0f} Z {z:.0f} mm"
        parts.append(text)

    # Objets suivis (liste « tracked ») : nom, position et état de chacun.
    for t in msg.get("tracked", []):
        state = "perdu" if not t["found"] and not t["predicted"] else "prédit" if t["predicted"] else \
            "en mouvement" if t["moving"] else "immobile"
        parts.append(f"{t['name']} ({t['center'][0]:.0f}, {t['center'][1]:.0f}) px {state}")

    # Événements des objets suivis (liste « events »).
    names = {"motion_start": "DÉBUT DE MOUVEMENT", "motion_stop": "FIN DE MOUVEMENT",
             "lost": "OBJET PERDU", "found": "OBJET RETROUVÉ"}
    for e in msg.get("events", []):
        unit = "mm" if "position_mm" in e else "px"
        x, y = e.get("position_mm") or e.get("position_px")
        text = f"*** {e.get('name', 'objet')} : {names.get(e['type'], e['type'])} ({x:.0f}, {y:.0f}) {unit}"
        if e["type"] == "motion_stop":
            text += f", déplacement {e['distance_' + unit]:.0f} {unit} en {e['duration_s']:.1f} s"
        parts.append(text)

    # Vitesses issues du lissage (liste « tracks »).
    for t in msg.get("tracks", []):
        name = "carré" if t["type"] == "square" else f"ArUco {t['id']}"
        floor = t.get("floor")
        speed = f"{floor['speed_mm_s']:.0f} mm/s" if floor else f"{t['speed_px_s']:.0f} px/s"
        parts.append(f"{name} {'prédit' if t['predicted'] else 'vitesse'} {speed}")

    return " | ".join(parts)


def handle(msg):
    """Appelée pour chaque message reçu : à adapter pour piloter un robot."""
    print(describe(msg))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port", type=int, default=5005, help="port UDP (défaut : 5005)")
    parser.add_argument("--host", default="0.0.0.0",
                        help="adresse d'écoute (défaut : toutes les interfaces)")
    parser.add_argument("--raw", action="store_true", help="affiche le JSON brut")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    print(f"En écoute sur {args.host}:{args.port} (Ctrl+C pour arrêter)")

    try:
        while True:
            data, sender = sock.recvfrom(65535)
            if args.raw:
                print(data.decode("utf-8"))
                continue
            try:
                handle(json.loads(data))
            except (ValueError, KeyError) as e:
                print(f"Message invalide de {sender[0]} : {e}")
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
