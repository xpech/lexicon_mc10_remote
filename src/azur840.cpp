#include <azur840.h>

#include <FS.h>
#include <LittleFS.h>

#define AZUR840_RX GPIO_NUM_16
#define AZUR840_TX GPIO_NUM_17

extern WebServer server;

namespace {
constexpr uint32_t kAzurDefaultTimeoutMs = 1200;
constexpr uint32_t kAzurInterByteTimeoutMs = 40;
constexpr size_t kAzurMaxResponseBytes = 256;
constexpr size_t kAzurMaxCommandDataChars = 10;

EspSoftwareSerial::UART g_azurSerial;
bool g_azurReady = false;
uint32_t g_azurBaud = 9600;
bool g_azurFsReady = false;

String bytesToHex(const uint8_t *buffer, size_t len) {
  const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out += hex[(buffer[i] >> 4) & 0x0F];
    out += hex[buffer[i] & 0x0F];
  }
  return out;
}

bool parseHexPayload(const String &input, uint8_t *buffer, size_t maxLen, size_t &outLen) {
  outLen = 0;
  uint8_t current = 0;
  bool haveHighNibble = false;

  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == ' ' || c == ':' || c == '-' || c == ',') {
      continue;
    }

    uint8_t value = 0;
    if (c >= '0' && c <= '9') {
      value = static_cast<uint8_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      value = static_cast<uint8_t>(10 + (c - 'a'));
    } else if (c >= 'A' && c <= 'F') {
      value = static_cast<uint8_t>(10 + (c - 'A'));
    } else {
      return false;
    }

    if (!haveHighNibble) {
      current = static_cast<uint8_t>(value << 4);
      haveHighNibble = true;
    } else {
      current = static_cast<uint8_t>(current | value);
      if (outLen >= maxLen) {
        return false;
      }
      buffer[outLen++] = current;
      haveHighNibble = false;
    }
  }

  return !haveHighNibble;
}

bool azurEnsureReady(uint32_t baudrate) {
  if (g_azurReady && baudrate == g_azurBaud) {
    return true;
  }

  g_azurSerial.begin(baudrate, SWSERIAL_8N1, AZUR840_RX, AZUR840_TX, false);
  g_azurSerial.setTimeout(kAzurDefaultTimeoutMs);
  g_azurBaud = baudrate;
  g_azurReady = true;
  while (g_azurSerial.available() > 0) {
    g_azurSerial.read();
  }
  return true;
}

bool azurReadResponse(uint8_t *buffer, size_t maxLen, size_t &outLen, uint32_t firstByteTimeoutMs) {
  outLen = 0;
  const uint32_t startedAt = millis();
  while (g_azurSerial.available() == 0) {
    if (millis() - startedAt >= firstByteTimeoutMs) {
      return false;
    }
    delay(1);
  }

  uint32_t lastDataAt = millis();
  while (millis() - lastDataAt <= kAzurInterByteTimeoutMs) {
    while (g_azurSerial.available() > 0) {
      const int value = g_azurSerial.read();
      if (value < 0) {
        continue;
      }
      if (outLen >= maxLen) {
        return true;
      }
      buffer[outLen++] = static_cast<uint8_t>(value);
      lastDataAt = millis();
    }
    delay(1);
  }

  return outLen > 0;
}

bool parseUintArg(const String &name, uint32_t &value) {
  if (!server.hasArg(name)) {
    return false;
  }
  const String raw = server.arg(name);
  if (raw.length() == 0) {
    return false;
  }

  char *endPtr = nullptr;
  const unsigned long parsed = strtoul(raw.c_str(), &endPtr, 10);
  if (endPtr == raw.c_str() || *endPtr != '\0') {
    return false;
  }

  value = static_cast<uint32_t>(parsed);
  return true;
}

String zeroPad2(uint32_t value) {
  if (value < 10) {
    return String("0") + String(value);
  }
  return String(value);
}

bool buildAzurCommand(String &outCommand, String &errorMessage) {
  uint32_t group = 0;
  uint32_t command = 0;
  const bool hasGroup = parseUintArg("group", group);
  const bool hasCommand = parseUintArg("command", command);

  if (!hasGroup && !hasCommand) {
    return false;
  }

  if (!hasGroup || !hasCommand) {
    errorMessage = "Provide both group and command for structured mode";
    return true;
  }

  if (group < 1 || group > 5) {
    errorMessage = "group must be between 1 and 5";
    return true;
  }
  if (command > 99) {
    errorMessage = "command must be between 0 and 99";
    return true;
  }

  const String data = server.hasArg("data") ? server.arg("data") : "";
  if (data.length() > kAzurMaxCommandDataChars) {
    errorMessage = "data length must be <= 10 characters";
    return true;
  }

  outCommand = String("#") + String(group) + "," + zeroPad2(command) + "," + data;
  return true;
}

String parseAzurFrameAscii(const String &ascii) {
  int start = ascii.indexOf('#');
  if (start < 0) {
    return "";
  }

  int end = ascii.indexOf('\r', start);
  if (end < 0) {
    end = ascii.indexOf('\n', start);
  }
  if (end < 0) {
    end = ascii.length();
  }

  const String frame = ascii.substring(start, end);
  int c1 = frame.indexOf(',');
  if (c1 < 0) {
    return "";
  }
  int c2 = frame.indexOf(',', c1 + 1);
  if (c2 < 0) {
    return "";
  }

  const String group = frame.substring(1, c1);
  const String command = frame.substring(c1 + 1, c2);
  const String data = frame.substring(c2 + 1);
  return String("RX_PARSED group=") + group + " command=" + command + " data=" + data;
}

} // namespace

int azur840Setup() {
  server.on("/azur840", HTTP_GET, handleAzur840Index);
  server.on("/azur.html", HTTP_GET, handleAzur840Index);
  server.on("/azur", HTTP_GET, handleAzur840Index);
  server.on("/azur840.html", HTTP_GET, handleAzur840Index);
  server.on("/azur840_api", HTTP_GET, handleAzur840Api);
  server.on("/azur840_api", HTTP_POST, handleAzur840Api);
  return 0;
}

void handleAzur840Index() {
  if (!g_azurFsReady) {
    g_azurFsReady = LittleFS.begin();
  }

  if (!g_azurFsReady) {
    server.send(500, "text/plain", "LittleFS mount failed");
    return;
  }

  File f = LittleFS.open("/azur.html", FILE_READ);
  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Cache-Control", "no-cache");
  const size_t fileSize = f.size();
  const unsigned long streamStartedAt = millis();
  const size_t bytesSent = server.streamFile(f, "text/html");
  Serial.printf("[HTTP] /azur840: %u/%u bytes sent in %lu ms\n",
                static_cast<unsigned int>(bytesSent),
                static_cast<unsigned int>(fileSize),
                millis() - streamStartedAt);
  f.close();
}

int32_t volume = -1;



void handleAzur840Api() {
  String txAscii = server.hasArg("tx") ? server.arg("tx") : "";
  const String txHex = server.hasArg("tx_hex") ? server.arg("tx_hex") : "";

  String structuredCommand;
  String structuredError;
  const bool structuredRequested = buildAzurCommand(structuredCommand, structuredError);
  if (structuredRequested) {
    if (structuredError.length() > 0) {
      server.send(400, "text/plain", structuredError);
      return;
    }
    txAscii = structuredCommand;
  }

  if (txAscii.length() == 0 && txHex.length() == 0) {
    server.send(400, "text/plain", "Missing payload: provide group+command(+data) or tx=<ascii> or tx_hex=<hex bytes>");
    return;
  }

  uint32_t baudrate = 9600;
  if (server.hasArg("baud")) {
    const unsigned long candidate = strtoul(server.arg("baud").c_str(), nullptr, 10);
    if (candidate >= 1200 && candidate <= 115200) {
      baudrate = static_cast<uint32_t>(candidate);
    }
  }

  uint32_t timeoutMs = kAzurDefaultTimeoutMs;
  if (server.hasArg("timeout")) {
    const unsigned long candidate = strtoul(server.arg("timeout").c_str(), nullptr, 10);
    if (candidate >= 100 && candidate <= 10000) {
      timeoutMs = static_cast<uint32_t>(candidate);
    }
  }

  String suffix = server.hasArg("suffix") ? server.arg("suffix") : "cr";
  suffix.toLowerCase();

  if (structuredRequested && !server.hasArg("suffix")) {
    // The official protocol requires CR termination.
    suffix = "cr";
  }

  String responseMode = server.hasArg("response") ? server.arg("response") : "ascii";
  responseMode.toLowerCase();

  if (!azurEnsureReady(baudrate)) {
    server.send(500, "text/plain", "Azur840 serial init failed");
    return;
  }

  while (g_azurSerial.available() > 0) {
    g_azurSerial.read();
  }

  String txSentHex;
  if (txHex.length() > 0) {
    uint8_t txBytes[kAzurMaxResponseBytes];
    size_t txLen = 0;
    if (!parseHexPayload(txHex, txBytes, sizeof(txBytes), txLen) || txLen == 0) {
      server.send(400, "text/plain", "Invalid tx_hex payload");
      return;
    }

    for (size_t i = 0; i < txLen; ++i) {
      g_azurSerial.write(txBytes[i]);
    }
    txSentHex = bytesToHex(txBytes, txLen);
  } else {
    g_azurSerial.print(txAscii);
    txSentHex = bytesToHex(reinterpret_cast<const uint8_t *>(txAscii.c_str()), txAscii.length());
  }

  if (suffix == "cr") {
    g_azurSerial.write('\r');
    txSentHex += "0D";
  } else if (suffix == "lf") {
    g_azurSerial.write('\n');
    txSentHex += "0A";
  } else if (suffix == "crlf") {
    g_azurSerial.write('\r');
    g_azurSerial.write('\n');
    txSentHex += "0D0A";
  }
  g_azurSerial.flush();

  uint8_t rxBytes[kAzurMaxResponseBytes];
  size_t rxLen = 0;
  if (!azurReadResponse(rxBytes, sizeof(rxBytes), rxLen, timeoutMs)) {
    server.send(504, "text/plain", String("TX_HEX=") + txSentHex + "\nTIMEOUT waiting Azur840 response");
    return;
  }

  const String rxHex = bytesToHex(rxBytes, rxLen);
  String rxAscii;
  rxAscii.reserve(rxLen);
  for (size_t i = 0; i < rxLen; ++i) {
    const char c = static_cast<char>(rxBytes[i]);
    if ((c >= 32 && c <= 126) || c == '\r' || c == '\n' || c == '\t') {
      rxAscii += c;
    } else {
      rxAscii += '.';
    }
  }

  if (responseMode == "hex") {
    server.send(200, "text/plain", String("TX_HEX=") + txSentHex + "\nRX_HEX=" + rxHex);
    return;
  }

  const String parsed = parseAzurFrameAscii(rxAscii);
  String body = String("TX_HEX=") + txSentHex + "\nRX_ASCII=" + rxAscii + "\nRX_HEX=" + rxHex;
  if (parsed.length() > 0) {
    body += "\n" + parsed;
  }
  server.send(200, "text/plain", body);

}
