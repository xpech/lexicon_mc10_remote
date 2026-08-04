(function (global) {
  "use strict";

  var STORAGE_KEY = "lexicon.locale";
  var SUPPORTED = ["fr", "en"];
  var resources = {
    fr: {
      "document.home": "Accueil de la télécommande",
      "document.lexicon": "Commande Lexicon",
      "document.azur": "Commande Azur 840",
      "document.setup": "Configuration",
      "common.skip": "Aller au contenu principal",
      "common.nav": "Navigation principale",
      "common.home": "Accueil",
      "common.lexicon": "Lexicon",
      "common.azur": "Azur",
      "common.setup": "Configuration",
      "nav.label": "Navigation principale",
      "nav.home": "Accueil",
      "nav.lexicon": "Lexicon",
      "nav.azur": "Azur",
      "nav.setup": "Configuration",
      "common.powerOn": "Allumer",
      "common.powerOff": "Éteindre",
      "common.volumeUp": "Volume +",
      "common.volumeDown": "Volume -",
      "common.refresh": "Rafraîchir",
      "common.logs": "Journal",
      "common.localDiagnostics": "diagnostic local",
      "common.unknown": "Inconnu",
      "common.ready": "PRÊT",
      "common.active": "ACTIF",
      "common.inactive": "INACTIF",
      "common.error": "ERREUR",
      "common.sending": "ENVOI",
      "common.send": "Envoyer",
      "home.title": "Centre de commande",
      "home.subtitle": "Accès rapide au Lexicon, à la configuration et aux commandes d’alimentation générale.",
      "home.generalPower": "Alimentation générale",
      "home.unknownState": "ÉTAT INCONNU",
      "home.refreshState": "Rafraîchir l’état",
      "home.powerHelp": "Ces boutons appellent /on et /off. La broche utilisée est celle configurée dans le firmware (POWER_PIN).",
      "home.sensors": "Capteurs",
      "home.localReadings": "mesures locales",
      "home.temperature": "Température",
      "home.humidity": "Humidité",
      "home.refreshSensors": "Rafraîchir les capteurs",
      "home.logLabel": "Journal des opérations",
      "home.operationLog": "Journal des opérations",
      "setup.title": "Configuration",
      "setup.subtitle": "Configuration rapide : Wi-Fi, mise à jour du firmware et mode debug.",
      "setup.languageTitle": "Langue de l’interface",
      "setup.language": "Langue de l’interface",
      "setup.languageBadge": "Langue",
      "setup.languageLabel": "Langue utilisée sur cet appareil",
      "setup.languageHelp": "Ce choix est mémorisé dans ce navigateur et s’applique à toutes les pages.",
      "setup.french": "Français",
      "setup.english": "English",
      "setup.wifiTitle": "Configuration Wi-Fi",
      "setup.wifi": "Configuration Wi-Fi",
      "setup.wifiNetwork": "Réseau Wi-Fi",
      "setup.network": "Réseau Wi-Fi",
      "setup.selectNetwork": "Sélectionner un réseau…",
      "setup.password": "Mot de passe Wi-Fi",
      "setup.connect": "Connecter",
      "setup.scan": "Scanner les réseaux",
      "setup.waiting": "État : en attente",
      "setup.firmwareTitle": "Mise à jour du firmware",
      "setup.firmware": "Mise à jour du firmware",
      "setup.firmwareFile": "Fichier firmware (.bin)",
      "setup.uploadFirmware": "Envoyer le firmware",
      "setup.firmwareHelp": "Sélectionnez un fichier .bin compilé, puis validez l’envoi. L’ESP redémarre à la fin de la mise à jour.",
      "setup.debugTitle": "Mode debug",
      "setup.debug": "Mode debug",
      "setup.inactive": "INACTIF",
      "setup.active": "ACTIF",
      "setup.enable": "Activer",
      "setup.disable": "Désactiver",
      "setup.debugHelp": "Le mode debug active la sortie de diagnostic série du firmware.",
      "setup.navigation": "Navigation",
      "setup.links": "Liens",
      "setup.backHome": "Retour à l’accueil",
      "setup.advanced": "Configuration avancée (historique)",
      "setup.logLabel": "Journal de configuration",
      "setup.configLog": "Journal de configuration",
      "lexicon.quickLabel": "Commandes rapides Lexicon",
      "lexicon.quickGroup": "Commandes rapides Lexicon",
      "lexicon.zone1": "Zone 1",
      "lexicon.zone2": "Zone 2",
      "lexicon.standby": "Veille",
      "lexicon.mute": "Muet",
      "lexicon.title": "Lexicon",
      "lexicon.subtitle": "Télécommande série optimisée pour mobile. Commandes rapides en haut, commandes avancées en bas.",
      "lexicon.linkStatus": "État de la liaison",
      "lexicon.route": "Route : /lexicon_rc5",
      "lexicon.logLabel": "Journal des commandes Lexicon",
      "lexicon.commandLog": "Journal des commandes Lexicon",
      "lexicon.essential": "Actions essentielles",
      "lexicon.touch": "Commandes tactiles",
      "lexicon.sources": "Sources",
      "lexicon.navigation": "Navigation",
      "lexicon.transport": "Transport",
      "lexicon.advanced": "Commandes avancées",
      "lexicon.soundModes": "Modes sonores et affichage",
      "lexicon.audioModes": "Modes sonores et affichage",
      "lexicon.zone2Presets": "Préréglages Zone 2",
      "lexicon.manualRc5": "Commande RC5 manuelle",
      "lexicon.command1": "Commande 1",
      "lexicon.command2": "Commande 2",
      "lexicon.sendRc5": "Envoyer RC5",
      "lexicon.manualDirect": "Commande directe manuelle",
      "lexicon.command": "Commande",
      "lexicon.optionalData": "donnée facultative",
      "lexicon.data": "Donnée",
      "lexicon.sendDirect": "Envoyer la commande directe",
      "lexicon.up": "Haut",
      "lexicon.down": "Bas",
      "lexicon.left": "Gauche",
      "lexicon.right": "Droite",
      "lexicon.menu": "Menu",
      "lexicon.play": "Lecture",
      "lexicon.pause": "Pause",
      "lexicon.stop": "Arrêt",
      "lexicon.display": "Affichage",
      "lexicon.rewind": "Retour rapide",
      "lexicon.forward": "Avance rapide",
      "lexicon.previous": "Précédent",
      "lexicon.next": "Suivant",
      "lexicon.displayOff": "Éteindre l’affichage",
      "lexicon.infoPanel": "Panneau d’informations",
      "lexicon.footer": "Lexicon • interface mobile",
      "azur.quickLabel": "Commandes rapides Azur",
      "azur.quickGroup": "Commandes rapides Azur",
      "azur.title": "Azur 840 RS232",
      "azur.subtitle": "Commandes rapides, protocole structuré et mode brut pour le diagnostic.",
      "azur.serialStatus": "État de la liaison série",
      "azur.linkStatus": "État de la liaison série",
      "azur.decoderWaiting": "Décodeur : en attente d’une trame",
      "azur.pollingOff": "Interrogation : NON",
      "azur.pollingOn": "Interrogation : OUI",
      "azur.interval": "Intervalle",
      "azur.pollingInterval": "Intervalle d’interrogation",
      "azur.power": "Alimentation",
      "azur.mute": "Muet",
      "azur.volume": "Volume",
      "azur.input": "Entrée",
      "azur.balance": "Balance",
      "azur.direct": "Direct",
      "azur.logLabel": "Journal des commandes Azur",
      "azur.commandLog": "Journal des commandes Azur",
      "azur.quickCommands": "Commandes rapides",
      "azur.group1": "Groupe 1",
      "azur.standby": "Veille",
      "azur.muteOn": "Activer le silence",
      "azur.muteOff": "Désactiver le silence",
      "azur.directOn": "Activer Direct",
      "azur.directOff": "Désactiver Direct",
      "azur.inputUp": "Entrée suivante",
      "azur.inputDown": "Entrée précédente",
      "azur.volumeStop": "Arrêter le volume",
      "azur.setVolume20": "Régler le volume à 20",
      "azur.inputs": "Entrées",
      "azur.settings": "Réglages",
      "azur.catalog": "Catalogue Groupe 1 (01–26)",
      "azur.presets": "Préréglages selon la documentation RS232",
      "azur.protocolMode": "Mode protocole",
      "azur.group": "Groupe du protocole",
      "azur.commandNumber": "Numéro de commande",
      "azur.commandData": "Données de la commande",
      "azur.baud": "Vitesse en bauds",
      "azur.timeout": "Délai d’attente en millisecondes",
      "azur.sendStructured": "Envoyer la commande structurée",
      "azur.rawMode": "Mode brut",
      "azur.asciiFrame": "Trame ASCII à transmettre",
      "azur.hexFrame": "Trame hexadécimale à transmettre",
      "azur.suffix": "Suffixe",
      "azur.responseMode": "Format de réponse",
      "azur.sendRaw": "Envoyer en mode brut",
      "azur.prefill": "Préremplir",
      "azur.send": "Envoyer"
    },
    en: {
      "document.home": "Remote Control Home",
      "document.lexicon": "Lexicon Control",
      "document.azur": "Azur 840 Control",
      "document.setup": "Setup",
      "common.skip": "Skip to main content",
      "common.nav": "Main navigation",
      "common.home": "Home",
      "common.lexicon": "Lexicon",
      "common.azur": "Azur",
      "common.setup": "Setup",
      "nav.label": "Main navigation",
      "nav.home": "Home",
      "nav.lexicon": "Lexicon",
      "nav.azur": "Azur",
      "nav.setup": "Setup",
      "common.powerOn": "Power On",
      "common.powerOff": "Power Off",
      "common.volumeUp": "Volume +",
      "common.volumeDown": "Volume -",
      "common.refresh": "Refresh",
      "common.logs": "Logs",
      "common.localDiagnostics": "local diagnostics",
      "common.unknown": "Unknown",
      "common.ready": "READY",
      "common.active": "ACTIVE",
      "common.inactive": "INACTIVE",
      "common.error": "ERROR",
      "common.sending": "SENDING",
      "common.send": "Send",
      "home.title": "Command Center",
      "home.subtitle": "Quick access to Lexicon, setup, and general power controls.",
      "home.generalPower": "General Power",
      "home.unknownState": "UNKNOWN STATE",
      "home.refreshState": "Refresh state",
      "home.powerHelp": "These buttons call /on and /off. The driver pin is configured in the firmware (POWER_PIN).",
      "home.sensors": "Sensors",
      "home.localReadings": "local readings",
      "home.temperature": "Temperature",
      "home.humidity": "Humidity",
      "home.refreshSensors": "Refresh sensors",
      "home.logLabel": "Operation log",
      "home.operationLog": "Operation log",
      "setup.title": "Setup",
      "setup.subtitle": "Quick configuration: Wi-Fi, firmware update, and debug mode.",
      "setup.languageTitle": "Interface language",
      "setup.language": "Interface language",
      "setup.languageBadge": "Language",
      "setup.languageLabel": "Language used on this device",
      "setup.languageHelp": "This choice is saved in this browser and applies to every page.",
      "setup.french": "Français",
      "setup.english": "English",
      "setup.wifiTitle": "Wi-Fi Configuration",
      "setup.wifi": "Wi-Fi Configuration",
      "setup.wifiNetwork": "Wi-Fi Network",
      "setup.network": "Wi-Fi Network",
      "setup.selectNetwork": "Select a network…",
      "setup.password": "Wi-Fi Password",
      "setup.connect": "Connect",
      "setup.scan": "Scan networks",
      "setup.waiting": "Status: waiting",
      "setup.firmwareTitle": "Firmware Update",
      "setup.firmware": "Firmware Update",
      "setup.firmwareFile": "Firmware file (.bin)",
      "setup.uploadFirmware": "Upload firmware",
      "setup.firmwareHelp": "Select a compiled .bin file and confirm the upload. The ESP restarts when the update is complete.",
      "setup.debugTitle": "Debug Mode",
      "setup.debug": "Debug Mode",
      "setup.inactive": "INACTIVE",
      "setup.active": "ACTIVE",
      "setup.enable": "Enable",
      "setup.disable": "Disable",
      "setup.debugHelp": "Debug mode enables the firmware serial diagnostic output.",
      "setup.navigation": "Navigation",
      "setup.links": "Links",
      "setup.backHome": "Back to home",
      "setup.advanced": "Advanced setup (legacy)",
      "setup.logLabel": "Configuration log",
      "setup.configLog": "Configuration log",
      "lexicon.quickLabel": "Lexicon quick commands",
      "lexicon.quickGroup": "Lexicon quick commands",
      "lexicon.zone1": "Zone 1",
      "lexicon.zone2": "Zone 2",
      "lexicon.standby": "Standby",
      "lexicon.mute": "Mute",
      "lexicon.title": "Lexicon",
      "lexicon.subtitle": "Mobile-optimized serial remote. Quick commands at the top and advanced commands below.",
      "lexicon.linkStatus": "Link Status",
      "lexicon.route": "Route: /lexicon_rc5",
      "lexicon.logLabel": "Lexicon command log",
      "lexicon.commandLog": "Lexicon command log",
      "lexicon.essential": "Essential Actions",
      "lexicon.touch": "Touch controls",
      "lexicon.sources": "Sources",
      "lexicon.navigation": "Navigation",
      "lexicon.transport": "Transport",
      "lexicon.advanced": "Advanced Commands",
      "lexicon.soundModes": "Sound modes and display",
      "lexicon.audioModes": "Sound modes and display",
      "lexicon.zone2Presets": "Zone 2 presets",
      "lexicon.manualRc5": "Manual RC5 command",
      "lexicon.command1": "Command 1",
      "lexicon.command2": "Command 2",
      "lexicon.sendRc5": "Send RC5",
      "lexicon.manualDirect": "Manual direct command",
      "lexicon.command": "Command",
      "lexicon.optionalData": "optional data",
      "lexicon.data": "Data",
      "lexicon.sendDirect": "Send direct command",
      "lexicon.up": "Up",
      "lexicon.down": "Down",
      "lexicon.left": "Left",
      "lexicon.right": "Right",
      "lexicon.menu": "Menu",
      "lexicon.play": "Play",
      "lexicon.pause": "Pause",
      "lexicon.stop": "Stop",
      "lexicon.display": "Display",
      "lexicon.rewind": "Rewind",
      "lexicon.forward": "Forward",
      "lexicon.previous": "Previous",
      "lexicon.next": "Next",
      "lexicon.displayOff": "Display Off",
      "lexicon.infoPanel": "Info panel",
      "lexicon.footer": "Lexicon • mobile interface",
      "azur.quickLabel": "Azur quick commands",
      "azur.quickGroup": "Azur quick commands",
      "azur.title": "Azur 840 RS232",
      "azur.subtitle": "Quick commands, structured protocol mode, and raw mode for diagnostics.",
      "azur.serialStatus": "Serial Link Status",
      "azur.linkStatus": "Serial Link Status",
      "azur.decoderWaiting": "Decoder: waiting for frame",
      "azur.pollingOff": "Polling: OFF",
      "azur.pollingOn": "Polling: ON",
      "azur.interval": "Interval",
      "azur.pollingInterval": "Polling interval",
      "azur.power": "Power",
      "azur.mute": "Mute",
      "azur.volume": "Volume",
      "azur.input": "Input",
      "azur.balance": "Balance",
      "azur.direct": "Direct",
      "azur.logLabel": "Azur command log",
      "azur.commandLog": "Azur command log",
      "azur.quickCommands": "Quick Commands",
      "azur.group1": "Group 1",
      "azur.standby": "Standby",
      "azur.muteOn": "Mute On",
      "azur.muteOff": "Mute Off",
      "azur.directOn": "Direct On",
      "azur.directOff": "Direct Off",
      "azur.inputUp": "Input Up",
      "azur.inputDown": "Input Down",
      "azur.volumeStop": "Volume Stop",
      "azur.setVolume20": "Set Volume 20",
      "azur.inputs": "Inputs",
      "azur.settings": "Settings",
      "azur.catalog": "Group 1 Catalog (01–26)",
      "azur.presets": "Presets from the RS232 documentation",
      "azur.protocolMode": "Protocol Mode",
      "azur.group": "Protocol group",
      "azur.commandNumber": "Command number",
      "azur.commandData": "Command data",
      "azur.baud": "Baud rate",
      "azur.timeout": "Timeout in milliseconds",
      "azur.sendStructured": "Send structured command",
      "azur.rawMode": "Raw Mode",
      "azur.asciiFrame": "ASCII frame to send",
      "azur.hexFrame": "Hexadecimal frame to send",
      "azur.suffix": "Suffix",
      "azur.responseMode": "Response format",
      "azur.sendRaw": "Send raw command",
      "azur.prefill": "Prefill",
      "azur.send": "Send"
    }
  };

  function normalize(locale) {
    var value = String(locale || "").slice(0, 2).toLowerCase();
    return SUPPORTED.indexOf(value) >= 0 ? value : null;
  }

  function storedLocale() {
    try {
      return normalize(global.localStorage.getItem(STORAGE_KEY));
    } catch (error) {
      return null;
    }
  }

  var locale = storedLocale() || normalize(global.navigator.language) || "fr";

  function translate(key, values) {
    var text = resources[locale][key] || resources.fr[key] || key;
    Object.keys(values || {}).forEach(function (name) {
      text = text.replace(new RegExp("{{\\s*" + name + "\\s*}}", "g"), String(values[name]));
    });
    return text;
  }

  function apply(root) {
    var scope = root || document;
    scope.querySelectorAll("[data-i18n]").forEach(function (element) {
      element.textContent = translate(element.getAttribute("data-i18n"));
    });
    scope.querySelectorAll("[data-i18n-placeholder]").forEach(function (element) {
      element.setAttribute("placeholder", translate(element.getAttribute("data-i18n-placeholder")));
    });
    scope.querySelectorAll("[data-i18n-aria-label]").forEach(function (element) {
      element.setAttribute("aria-label", translate(element.getAttribute("data-i18n-aria-label")));
    });
    scope.querySelectorAll("[data-i18n-title]").forEach(function (element) {
      element.setAttribute("title", translate(element.getAttribute("data-i18n-title")));
    });
  }

  function setLocale(nextLocale) {
    var normalized = normalize(nextLocale);
    if (!normalized) {
      return;
    }
    locale = normalized;
    try {
      global.localStorage.setItem(STORAGE_KEY, locale);
    } catch (error) {
      // The UI still changes if storage is unavailable.
    }
    document.documentElement.lang = locale;
    apply(document);
    document.dispatchEvent(new CustomEvent("localechange", { detail: { locale: locale } }));
  }

  global.AppI18n = {
    apply: apply,
    getLocale: function () { return locale; },
    setLocale: setLocale,
    supported: SUPPORTED.slice(),
    t: translate
  };

  document.documentElement.lang = locale;
  document.addEventListener("DOMContentLoaded", function () {
    apply(document);
    var selector = document.getElementById("languageSelect");
    if (selector) {
      selector.value = locale;
      selector.addEventListener("change", function () {
        setLocale(selector.value);
        if (selector.hasAttribute("data-locale-reload")) {
          global.location.reload();
        }
      });
    }
  });
})(window);
