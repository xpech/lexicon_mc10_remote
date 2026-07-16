# Lexicon MC-10 Remote (ESP32)

Ce projet transforme un ESP32 en telecommande web locale pour un systeme audio haut de gamme:

- Lexicon MC-10 (commande principale)
- Preampli Azur 840 (commande serie)
- Pilotage d'alimentation (mise sous tension globale)

Le projet reutilise une base historique "coolhome" (chauffage/arrosage + remontee de temperature) et l'etend vers un usage audio/video.

## Objectifs

- Fournir une interface web locale simple et rapide pour piloter le Lexicon MC-10.
- Ajouter le pilotage du preampli Azur 840.
- Conserver les briques existantes (reseau, OTA, capteurs, telemetry) quand elles sont utiles.
- Permettre une evolution progressive vers une telecommande "full web" unifiee.

## Stack technique

- Carte cible: ESP32 (environnement PlatformIO: `wemos_d1_mini32`)
- Framework: Arduino
- Serveur HTTP embarque: WebServer
- FS embarque: LittleFS (pages web dans `data/`)
- Liaison serie logicielle:
  - Lexicon: RX GPIO 18, TX GPIO 19
  - Azur 840: RX GPIO 16, TX GPIO 17

## Arborescence utile

- `src/main.cpp`: setup global (Wi-Fi, mDNS, OTA, routes historiques)
- `src/lexicon.cpp`: protocole Lexicon + endpoints web/API Lexicon
- `src/azur840.cpp`: protocole Azur 840 + endpoints web/API Azur
- `data/lexicon.html`: UI web Lexicon
- `data/azur.html`: UI web Azur 840
- `data/shared-ui.css`: styles communs
- `platformio.ini`: configuration build/deps

## Prerequis

- PlatformIO Core installe (`pio` ou `platformio` dans le PATH)
- Carte ESP32 connectee en USB
- Cablage UART adapte aux equipements pilotes

## Build et flash firmware

Depuis la racine du projet:

```bash
pio run -e wemos_d1_mini32
pio run -e wemos_d1_mini32 -t upload
```

Si besoin de preciser le port:

```bash
pio run -e wemos_d1_mini32 -t upload --upload-port /dev/tty.usbserial-XXXX
```

## Upload des assets web (LittleFS)

Les pages web dans `data/` doivent etre envoyees dans LittleFS.

macOS/Linux:

```bash
./upload_data_mac.sh
# ou avec port explicite
./upload_data_mac.sh wemos_d1_mini32 /dev/tty.usbserial-XXXX
```

Windows:

```bat
upload_data_windows.bat
:: ou avec port explicite
upload_data_windows.bat wemos_d1_mini32 COM4
```

## Acces a l'interface

Au demarrage:

- tentative de connexion Wi-Fi sur credentials connus
- sinon demarrage en mode AP (point d'acces)
- publication mDNS (hostname dynamique, service HTTP)

Endpoints principaux:

- `GET /lexicon` : page web Lexicon
- `GET /azur840` : page web Azur
- `GET /shared-ui.css` : feuille de style commune

## API Lexicon

### Commande standard

- `GET /lexicon_cmd?zone=..&command=..&data=...`
- `GET /lexicon_cmd?zone=..&command=..&datahex=...`

Parametres:

- `zone` et `command`: octets en hexadecimal (ex: `01`, `0A`, `FF`)
- `data`: payload ASCII
- `datahex`: payload binaire en hex (longueur paire), separateurs espaces/`:`/`-` acceptes

Reponse type:

- `AC=<code_reponse_hex> DATA=<payload>`

### Commande RC5

- `GET /lexicon_rc5?zone=..&command1=..&command2=..`

Parametres:

- `zone`, `command1`, `command2`: octets en hexadecimal

Reponse type:

- `AC=<code_reponse_hex> RC5=<2 octets hex>`

## API Azur 840

Endpoint:

- `GET /azur840_api`
- `POST /azur840_api`

Modes supportes:

- Mode direct ASCII via `tx`
- Mode hex via `tx_hex`
- Mode structure via `group`, `command`, `data`

Regles mode structure:

- `group` entre 1 et 5
- `command` entre 0 et 99
- `data` max 10 caracteres
- format emis: `#<group>,<command_sur_2_chiffres>,<data>`

## OTA

Endpoint de mise a jour firmware:

- `GET /update`
- `POST /update` (upload binaire)

## Fonctions heritagees (base coolhome)

Le code contient encore des fonctions historiques:

- capteurs DHT / Dallas
- logiques chauffage/arrosage
- remontee periodique de donnees vers un serveur

Ces briques peuvent rester actives selon votre configuration, mais ne sont pas le coeur fonctionnel de la telecommande audio.

## Etat actuel et prochaines etapes

Etat actuel:

- base ESP32 fonctionnelle
- UI web Lexicon et Azur disponibles
- APIs serie en place pour Lexicon et Azur

Prochaines etapes recommandees:

1. Unifier les ecrans web en une telecommande unique (sources, volume, power macros).
2. Stabiliser les macros d'alimentation (ordre d'allumage/extinction securise).
3. Ajouter journalisation et retour d'etat utilisateur (ack, timeout, erreurs).
4. Isoler/retirer progressivement le legacy chauffage-arrosage si non utilise.

## Auteurs

Base historique:

- Pierre Le Noan
- Xavier Péchoultres
