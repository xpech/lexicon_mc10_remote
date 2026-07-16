#ifndef LEXICON_H
#define LEXICON_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <WebServer.h>

class LexiconComm {
public:
	enum ErrorCode {
		kOk = 0,
		kNotInitialized,
		kTimeout,
		kFrame,
		kLength,
		kProtocol,
		kInput
	};

	LexiconComm();
	bool begin(uint32_t baudrate = 38400);

	bool sendCommand(uint8_t zone, uint8_t command, const String &data, String &responseData, uint8_t &answerCode);
	bool sendRc5(uint8_t zone, uint8_t command1, uint8_t command2, String &responseHex, uint8_t &answerCode);

	ErrorCode lastError() const;
	const char *lastErrorMessage() const;

private:
	struct ResponseFrame {
		uint8_t zone;
		uint8_t command;
		uint8_t answer;
		String data;
	};

	static const size_t kPendingFrameCount = 6;

	bool initialized_;
	EspSoftwareSerial::UART serial_;
	ErrorCode lastError_;
	char lastErrorMessage_[64];
	ResponseFrame pendingFrames_[kPendingFrameCount];
	size_t pendingCount_;

	void clearInput();
	bool readByteWithTimeout(uint8_t &value, uint32_t timeoutMs);
	bool readFrame(uint8_t &zone, uint8_t &command, uint8_t &answer, String &data, uint32_t timeoutMs);
	bool dequeueMatchingResponse(uint8_t zone, uint8_t command, ResponseFrame &frame);
	void enqueuePendingResponse(uint8_t zone, uint8_t command, uint8_t answer, const String &data);
	void setError(ErrorCode code, const char *message);
};

int lexiconSetup();
void handleLixiconRC5Command();
void handleLixiconCommand();
void handleLixiconIndex();
int getIntFromHexArg(const String &argName);

#endif // LEXICON_H
