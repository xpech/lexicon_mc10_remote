#include <lexicon.h>
#include <azur840.h>

#include <FS.h>
#include <LittleFS.h>
#define LEXICON_FS LittleFS

#define LEXICON_RX GPIO_NUM_18
#define LEXICON_TX GPIO_NUM_19

extern WebServer server;

namespace {
constexpr uint8_t kFrameStart = 0x21;
constexpr uint8_t kFrameEnd = 0x0D;
constexpr uint8_t kRc5CommandCode = 0x08;
constexpr uint32_t kDefaultTimeoutMs = 3000;

LexiconComm g_lexicon;
bool g_fsReady = false;

bool parseHexByteArg(const String &name, uint8_t &value) {
  if (!server.hasArg(name)) {
    return false;
  }

  String raw = server.arg(name);
  if (raw.length() == 0) {
    return false;
  }

  char *endPtr = nullptr;
  const unsigned long parsed = strtoul(raw.c_str(), &endPtr, 16);
  if (endPtr == raw.c_str() || *endPtr != '\0' || parsed > 0xFFUL) {
    return false;
  }

  value = static_cast<uint8_t>(parsed);
  return true;
}

bool parseHexDataArg(const String &name, String &data) {
  if (!server.hasArg(name)) {
    return false;
  }

  String raw = server.arg(name);
  raw.replace(" ", "");
  raw.replace("-", "");
  raw.replace(":", "");

  if (raw.length() == 0 || (raw.length() % 2) != 0) {
    return false;
  }

  data = "";
  data.reserve(raw.length() / 2);

  for (size_t i = 0; i < raw.length(); i += 2) {
    const char hi = raw.charAt(i);
    const char lo = raw.charAt(i + 1);
    if (!isxdigit(static_cast<unsigned char>(hi)) || !isxdigit(static_cast<unsigned char>(lo))) {
      return false;
    }

    const String byteText = raw.substring(i, i + 2);
    char *endPtr = nullptr;
    const unsigned long parsed = strtoul(byteText.c_str(), &endPtr, 16);
    if (endPtr == byteText.c_str() || *endPtr != '\0' || parsed > 0xFFUL) {
      return false;
    }

    data += static_cast<char>(parsed);
  }

  return true;
}

void sendCorsHeaders(const char *cacheControl = "no-cache") {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Cache-Control", cacheControl);
}

void handleSharedUiCss() {
  if (!g_fsReady) {
    g_fsReady = LEXICON_FS.begin();
  }

  if (!g_fsReady) {
    server.send(500, "text/plain", "LittleFS mount failed");
    return;
  }

  File f = LEXICON_FS.open("/shared-ui.css", FILE_READ);
  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  sendCorsHeaders("public, max-age=86400");
  const size_t fileSize = f.size();
  const unsigned long streamStartedAt = millis();
  const size_t bytesSent = server.streamFile(f, "text/css");
  Serial.printf("[HTTP] /shared-ui.css: %u/%u bytes sent in %lu ms\n",
                static_cast<unsigned int>(bytesSent),
                static_cast<unsigned int>(fileSize),
                millis() - streamStartedAt);
  f.close();
}

} // namespace

LexiconComm::LexiconComm()
    : initialized_(false), serial_(), lastError_(kOk), pendingCount_(0) {
  lastErrorMessage_[0] = '\0';
}

bool LexiconComm::begin(uint32_t baudrate) {
  serial_.begin(baudrate, SWSERIAL_8N1, LEXICON_RX, LEXICON_TX, false);
  serial_.setTimeout(kDefaultTimeoutMs);
  initialized_ = true;
  setError(kOk, "OK");
  clearInput();
  return true;
}

bool LexiconComm::sendCommand(uint8_t zone, uint8_t command, const String &data, String &responseData, uint8_t &answerCode) {
  if (!initialized_) {
    setError(kNotInitialized, "UART not initialized");
    return false;
  }
  if (data.length() > 255) {
    setError(kInput, "Data too long");
    return false;
  }

  ResponseFrame cached;
  if (dequeueMatchingResponse(zone, command, cached)) {
    responseData = cached.data;
    answerCode = cached.answer;
    setError(kOk, "OK");
    return true;
  }

  serial_.write(kFrameStart);
  serial_.write(zone);
  serial_.write(command);
  serial_.write(static_cast<uint8_t>(data.length()));
  serial_.print(data);
  serial_.write(kFrameEnd);
  serial_.flush();

  const uint32_t startedAt = millis();
  while (millis() - startedAt < kDefaultTimeoutMs) {
    const uint32_t elapsed = millis() - startedAt;
    const uint32_t remaining = kDefaultTimeoutMs - elapsed;

    uint8_t responseZone = 0;
    uint8_t responseCommand = 0;
    uint8_t responseAnswer = 0;
    String responsePayload;

    if (!readFrame(responseZone, responseCommand, responseAnswer, responsePayload, remaining)) {
      return false;
    }

    if (responseZone == zone && responseCommand == command) {
      responseData = responsePayload;
      answerCode = responseAnswer;
      setError(kOk, "OK");
      return true;
    }

    enqueuePendingResponse(responseZone, responseCommand, responseAnswer, responsePayload);
  }

  setError(kTimeout, "No matching response before timeout");
  return false;
}

bool LexiconComm::sendRc5(uint8_t zone, uint8_t command1, uint8_t command2, String &responseHex, uint8_t &answerCode) {
  if (!initialized_) {
    setError(kNotInitialized, "UART not initialized");
    return false;
  }

  ResponseFrame cached;
  if (dequeueMatchingResponse(zone, kRc5CommandCode, cached)) {
    if (cached.data.length() != 2) {
      setError(kLength, "Unexpected RC5 payload length");
      return false;
    }

    char cachedHex[5];
    snprintf(cachedHex, sizeof(cachedHex), "%02X%02X", static_cast<uint8_t>(cached.data[0]), static_cast<uint8_t>(cached.data[1]));
    responseHex = cachedHex;
    answerCode = cached.answer;
    setError(kOk, "OK");
    return true;
  }

  serial_.write(kFrameStart);
  serial_.write(zone);
  serial_.write(kRc5CommandCode);
  serial_.write(static_cast<uint8_t>(2));
  serial_.write(command1);
  serial_.write(command2);
  serial_.write(kFrameEnd);
  serial_.flush();

  const uint32_t startedAt = millis();
  while (millis() - startedAt < kDefaultTimeoutMs) {
    const uint32_t elapsed = millis() - startedAt;
    const uint32_t remaining = kDefaultTimeoutMs - elapsed;

    uint8_t responseZone = 0;
    uint8_t responseCommand = 0;
    uint8_t responseAnswer = 0;
    String responsePayload;

    if (!readFrame(responseZone, responseCommand, responseAnswer, responsePayload, remaining)) {
      return false;
    }

    if (responseZone == zone && responseCommand == kRc5CommandCode) {
      if (responsePayload.length() != 2) {
        setError(kLength, "Unexpected RC5 payload length");
        return false;
      }

      char hexBuffer[5];
      snprintf(hexBuffer, sizeof(hexBuffer), "%02X%02X", static_cast<uint8_t>(responsePayload[0]), static_cast<uint8_t>(responsePayload[1]));
      responseHex = hexBuffer;
      answerCode = responseAnswer;
      setError(kOk, "OK");
      return true;
    }

    enqueuePendingResponse(responseZone, responseCommand, responseAnswer, responsePayload);
  }

  setError(kTimeout, "No matching RC5 response before timeout");
  return false;
}

LexiconComm::ErrorCode LexiconComm::lastError() const {
  return lastError_;
}

const char *LexiconComm::lastErrorMessage() const {
  return lastErrorMessage_;
}

void LexiconComm::clearInput() {
  while (serial_.available() > 0) {
    serial_.read();
  }
}

bool LexiconComm::readByteWithTimeout(uint8_t &value, uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (serial_.available() == 0) {
    if (millis() - start >= timeoutMs) {
      setError(kTimeout, "Serial read timeout");
      return false;
    }
    delay(1);
  }

  const int readValue = serial_.read();
  if (readValue < 0) {
    setError(kFrame, "Serial read failed");
    return false;
  }

  value = static_cast<uint8_t>(readValue);
  return true;
}

bool LexiconComm::readFrame(uint8_t &zone, uint8_t &command, uint8_t &answer, String &data, uint32_t timeoutMs) {
  uint8_t start = 0;
  if (!readByteWithTimeout(start, timeoutMs)) {
    return false;
  }
  if (start != kFrameStart) {
    setError(kFrame, "Invalid frame start");
    return false;
  }

  uint8_t length = 0;
  if (!readByteWithTimeout(zone, timeoutMs) ||
      !readByteWithTimeout(command, timeoutMs) ||
      !readByteWithTimeout(answer, timeoutMs) ||
      !readByteWithTimeout(length, timeoutMs)) {
    return false;
  }

  data = "";
  data.reserve(length);
  for (uint8_t i = 0; i < length; ++i) {
    uint8_t current = 0;
    if (!readByteWithTimeout(current, timeoutMs)) {
      return false;
    }
    data += static_cast<char>(current);
  }

  uint8_t end = 0;
  if (!readByteWithTimeout(end, timeoutMs)) {
    return false;
  }
  if (end != kFrameEnd) {
    setError(kFrame, "Invalid frame end");
    return false;
  }

  return true;
}

bool LexiconComm::dequeueMatchingResponse(uint8_t zone, uint8_t command, ResponseFrame &frame) {
  for (size_t i = 0; i < pendingCount_; ++i) {
    if (pendingFrames_[i].zone == zone && pendingFrames_[i].command == command) {
      frame = pendingFrames_[i];
      for (size_t j = i + 1; j < pendingCount_; ++j) {
        pendingFrames_[j - 1] = pendingFrames_[j];
      }
      --pendingCount_;
      return true;
    }
  }
  return false;
}

void LexiconComm::enqueuePendingResponse(uint8_t zone, uint8_t command, uint8_t answer, const String &data) {
  ResponseFrame frame;
  frame.zone = zone;
  frame.command = command;
  frame.answer = answer;
  frame.data = data;

  if (pendingCount_ < kPendingFrameCount) {
    pendingFrames_[pendingCount_] = frame;
    ++pendingCount_;
    return;
  }

  for (size_t i = 1; i < kPendingFrameCount; ++i) {
    pendingFrames_[i - 1] = pendingFrames_[i];
  }
  pendingFrames_[kPendingFrameCount - 1] = frame;
}

void LexiconComm::setError(ErrorCode code, const char *message) {
  lastError_ = code;
  if (message == nullptr) {
    lastErrorMessage_[0] = '\0';
    return;
  }
  snprintf(lastErrorMessage_, sizeof(lastErrorMessage_), "%s", message);
}

int lexiconSetup() {
  server.on("/lexicon", HTTP_GET, handleLixiconIndex);
  server.on("/shared-ui.css", HTTP_GET, handleSharedUiCss);
  server.on("/lexicon_cmd", HTTP_GET, handleLixiconCommand);
  server.on("/lexicon_rc5", HTTP_GET, handleLixiconRC5Command);

  const int lexiconResult = g_lexicon.begin() ? 0 : -1;
  const int azurResult = azur840Setup();
  return (lexiconResult == 0 && azurResult == 0) ? 0 : -1;
}

void handleLixiconIndex() {
  Serial.print("handleLixiconIndex");
  if (!g_fsReady) {
    g_fsReady = LEXICON_FS.begin();
  }

  if (!g_fsReady) {
    server.send(500, "text/plain", "LittleFS mount failed");
    return;
  }

  File f = LEXICON_FS.open("/lexicon.html", FILE_READ);
  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  Serial.print("send file");

  sendCorsHeaders();
  const size_t fileSize = f.size();
  const unsigned long streamStartedAt = millis();
  const size_t bytesSent = server.streamFile(f, "text/html");
  Serial.printf("[HTTP] /lexicon: %u/%u bytes sent in %lu ms\n",
                static_cast<unsigned int>(bytesSent),
                static_cast<unsigned int>(fileSize),
                millis() - streamStartedAt);
  f.close();
}

int getIntFromHexArg(const String &argName) {
  uint8_t value = 0;
  if (parseHexByteArg(argName, value)) {
    return value;
  }
  return 0;
}

void handleLixiconCommand() {
  uint8_t zone = 0;
  uint8_t command = 0;

  if (!parseHexByteArg("zone", zone) || !parseHexByteArg("command", command)) {
    server.send(400, "text/plain", "Missing or invalid zone/command (hex byte expected)");
    return;
  }

  String data;
  if (server.hasArg("datahex")) {
    if (!parseHexDataArg("datahex", data)) {
      server.send(400, "text/plain", "Invalid datahex (expected even-length hex bytes)");
      return;
    }
  } else {
    data = server.hasArg("data") ? server.arg("data") : "";
  }

  String response;
  uint8_t answerCode = 0;
  if (!g_lexicon.sendCommand(zone, command, data, response, answerCode)) {
    server.send(502, "text/plain", g_lexicon.lastErrorMessage());
    return;
  }

  char answerHex[3];
  snprintf(answerHex, sizeof(answerHex), "%02X", answerCode);
  server.send(200, "text/plain", String("AC=") + answerHex + " DATA=" + response);
}

void handleLixiconRC5Command() {
  uint8_t zone = 0;
  uint8_t command1 = 0;
  uint8_t command2 = 0;

  if (!parseHexByteArg("zone", zone) || !parseHexByteArg("command1", command1) || !parseHexByteArg("command2", command2)) {
    server.send(400, "text/plain", "Missing or invalid RC5 args (hex bytes expected)");
    return;
  }

  String response;
  uint8_t answerCode = 0;
  if (!g_lexicon.sendRc5(zone, command1, command2, response, answerCode)) {
    server.send(502, "text/plain", g_lexicon.lastErrorMessage());
    return;
  }

  char answerHex[3];
  snprintf(answerHex, sizeof(answerHex), "%02X", answerCode);
  server.send(200, "text/plain", String("AC=") + answerHex + " RC5=" + response);
}
