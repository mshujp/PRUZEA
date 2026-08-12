#include "SignalLost.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

static const char SAMPLE_PNS[] = R"PNS(
# PRUZEA Novel Script sample
# Embedded in Flash for PRUZEA 1.2.1.

LABEL start

CHAPTER SIGNAL LOST

NAME SYSTEM

TEXT
A maintenance terminal wakes in an empty station.
The main network is silent.
ENDTEXT

WAIT 0.5

NAME OPERATOR

TEXT
One emergency channel is still open.
Its destination is unknown.
ENDTEXT

CHOICE
Open the channel -> open_channel
Inspect the station first -> inspect_station
ENDCHOICE

LABEL inspect_station

ADD caution 1

CHAPTER STATION LOG

NAME SYSTEM

TEXT
The station clock stopped at 03:17.
A handwritten note says:
"Do not trust the first reply."
ENDTEXT

WAITKEY

NAME OPERATOR

TEXT
The warning may be old.
Or it may have been left for me.
ENDTEXT

JUMP open_channel

LABEL open_channel

CHAPTER FIRST CONTACT

NAME UNKNOWN

TEXT
Can you hear me?
Please answer before the signal disappears.
ENDTEXT

CHOICE
Answer honestly -> honest_reply
Ask for identification -> ask_identity
Remain silent -> silent_reply
ENDCHOICE

LABEL honest_reply

ADD trust 2

NAME OPERATOR

TEXT
I can hear you.
Who are you?
ENDTEXT

JUMP identity_result

LABEL ask_identity

ADD caution 1
ADD trust 1

NAME OPERATOR

TEXT
Identify yourself first.
ENDTEXT

JUMP identity_result

LABEL silent_reply

SUB trust 1
ADD caution 1

WAIT 1.0

NAME UNKNOWN

TEXT
Silence is also an answer.
I will try once more.
ENDTEXT

LABEL identity_result

NAME UNKNOWN

TEXT
My name is Mira.
I am in the observatory above your station.
The doors are locked from the outside.
ENDTEXT

IF caution >= 2 -> cautious_route
IF trust >= 2 -> trust_route
JUMP uncertain_route

LABEL cautious_route

CHAPTER THE WARNING

NAME OPERATOR

TEXT
There is a note here.
It says not to trust the first reply.
ENDTEXT

NAME MIRA

TEXT
Then do not trust me.
Check the observatory power log instead.
Truth should survive inspection.
ENDTEXT

ADD trust 1
JUMP final_choice

LABEL trust_route

CHAPTER A VOICE IN THE DARK

NAME OPERATOR

TEXT
I believe you.
Tell me how to open the observatory.
ENDTEXT

NAME MIRA

TEXT
Restore the northern relay.
But once it starts, the station cannot shut it down remotely.
ENDTEXT

ADD trust 1
JUMP final_choice

LABEL uncertain_route

CHAPTER STATIC

NAME OPERATOR

TEXT
I do not know whether to believe you.
ENDTEXT

NAME MIRA

TEXT
Good.
Blind trust would make this easier for the wrong person.
ENDTEXT

JUMP final_choice

LABEL final_choice

CHAPTER DECISION

NAME SYSTEM

TEXT
The northern relay can be restored.
Estimated station reserve after activation: 11 percent.
ENDTEXT

CHOICE
Restore the relay -> restore_relay
Keep the station alive -> keep_power
Cut the channel -> cut_channel
ENDCHOICE

LABEL restore_relay

IF trust >= 2 -> good_end
JUMP strange_end

LABEL good_end

CHAPTER DAWN

NAME SYSTEM

TEXT
Northern relay online.
Observatory locks released.
ENDTEXT

WAIT 0.8

NAME MIRA

TEXT
I can see the station lights.
Stay where you are.
This time, someone is coming back.
ENDTEXT

END

LABEL strange_end

CHAPTER SECOND SIGNAL

NAME SYSTEM

TEXT
Northern relay online.
No observatory lock response.
ENDTEXT

WAIT 1.0

NAME UNKNOWN

TEXT
Thank you.
The door was never locked.
ENDTEXT

END

LABEL keep_power

CHAPTER LAST LIGHT

NAME OPERATOR

TEXT
I cannot risk the station.
I am sorry.
ENDTEXT

NAME MIRA

TEXT
Then keep the channel open until the batteries fail.
I would rather not be alone.
ENDTEXT

END

LABEL cut_channel

CHAPTER DISCONNECT

NAME SYSTEM

TEXT
Emergency channel closed.
Station reserve stabilized.
ENDTEXT

WAIT 1.0

NAME OPERATOR

TEXT
The terminal is quiet again.
This time, the silence was my choice.
ENDTEXT

END
)PNS";

constexpr PRUZEA::Graphics::Color COLOR_BACKGROUND =
    PRUZEA::Graphics::rgb565(6, 12, 20);
constexpr PRUZEA::Graphics::Color COLOR_PANEL =
    PRUZEA::Graphics::rgb565(12, 25, 38);
constexpr PRUZEA::Graphics::Color COLOR_PANEL_ALT =
    PRUZEA::Graphics::rgb565(18, 37, 52);
constexpr PRUZEA::Graphics::Color COLOR_ACCENT =
    PRUZEA::Graphics::rgb565(74, 220, 210);
constexpr PRUZEA::Graphics::Color COLOR_TEXT =
    PRUZEA::Graphics::rgb565(220, 232, 238);
constexpr PRUZEA::Graphics::Color COLOR_MUTED =
    PRUZEA::Graphics::rgb565(104, 134, 148);
constexpr PRUZEA::Graphics::Color COLOR_SELECTED =
    PRUZEA::Graphics::rgb565(255, 194, 74);
constexpr PRUZEA::Graphics::Color COLOR_ERROR =
    PRUZEA::Graphics::rgb565(255, 92, 92);

constexpr int16_t SCREEN_WIDTH = 320;
constexpr int16_t SCREEN_HEIGHT = 240;
constexpr int16_t SAFE_BOTTOM = 224;

} // namespace

using namespace PRUZEA;

// ============================================================
// MemoryLineReader
// ============================================================

SignalLost::MemoryLineReader::MemoryLineReader()
    : script_(nullptr),
      position_(0) {
}

void SignalLost::MemoryLineReader::attach(const char* script) {
    script_ = script;
    position_ = 0;
}

bool SignalLost::MemoryLineReader::readLine(
    char* outLine,
    size_t outSize,
    uint32_t* outLineOffset
) {
    if (outLine == nullptr || outSize == 0 || script_ == nullptr) {
        return false;
    }

    if (script_[position_] == '\0') {
        outLine[0] = '\0';
        return false;
    }

    if (outLineOffset != nullptr) {
        *outLineOffset = position_;
    }

    size_t written = 0;

    while (script_[position_] != '\0' &&
           script_[position_] != '\n' &&
           script_[position_] != '\r') {
        if (written + 1 < outSize) {
            outLine[written++] = script_[position_];
        }
        ++position_;
    }

    outLine[written] = '\0';

    if (script_[position_] == '\r') {
        ++position_;
        if (script_[position_] == '\n') {
            ++position_;
        }
    } else if (script_[position_] == '\n') {
        ++position_;
    }

    return true;
}

bool SignalLost::MemoryLineReader::seek(uint32_t offset) {
    if (script_ == nullptr) {
        return false;
    }

    uint32_t length = static_cast<uint32_t>(strlen(script_));
    if (offset > length) {
        return false;
    }

    position_ = offset;
    return true;
}

void SignalLost::MemoryLineReader::rewind() {
    position_ = 0;
}

uint32_t SignalLost::MemoryLineReader::position() const {
    return position_;
}

// ============================================================
// Game metadata
// ============================================================

const char* SignalLost::getId() const {
    return "signallost";
}

const char* SignalLost::getName() const {
    return "SIGNAL LOST";
}

const char* SignalLost::getMenuName() const{
    return "SIGNAL LOST";
}

uint16_t SignalLost::getLogicalScreenWidth() const {
    return PRUZEA::Display::ILI9341_SCREEN_W;
}

uint16_t SignalLost::getLogicalScreenHeight() const {
    return PRUZEA::Display::ILI9341_SCREEN_H;
}

uint16_t SignalLost::getTargetScreenWidth() const {
    return PRUZEA::Display::ILI9341_SCREEN_W;
}

uint16_t SignalLost::getTargetScreenHeight() const {
    return PRUZEA::Display::ILI9341_SCREEN_H;
}

// ============================================================
// Initialization
// ============================================================

void SignalLost::onInit(PRUZEA::Storage& storage) {
    (void)storage;

    reader_.attach(SAMPLE_PNS);
    resetRuntime();

    if (!buildLabelTable()) {
        mode_ = Mode::ERROR;
        screenDirty_ = true;
        return;
    }

    if (findLabelIndex("start") < 0) {
        setError(
            ErrorCode::MISSING_START_LABEL,
            0,
            "LABEL start is required"
        );
        return;
    }

    mode_ = Mode::TITLE;
    screenDirty_ = true;
}

void SignalLost::resetRuntime() {
    mode_ = Mode::TITLE;
    modeBeforeWait_ = Mode::EXECUTING;

    errorCode_ = ErrorCode::NONE;
    errorLineNumber_ = 0;
    errorDetail[0] = '\0';

    memset(labels_, 0, sizeof(labels_));
    memset(variables_, 0, sizeof(variables_));
    memset(choices_, 0, sizeof(choices_));

    labelCount_ = 0;
    variableCount_ = 0;
    choiceCount_ = 0;
    selectedChoice_ = 0;

    currentLineNumber_ = 0;
    waitStartMsec_ = 0;
    waitDurationMsec_ = 0;
    finishedStartMsec_ = 0;

    currentChapter_[0] = '\0';
    currentSpeaker_[0] = '\0';
    currentText_[0] = '\0';

    screenDirty_ = true;
    scriptEnded_ = false;

    reader_.rewind();
}

void SignalLost::resetForNewGame() {
    memset(variables_, 0, sizeof(variables_));
    memset(choices_, 0, sizeof(choices_));

    variableCount_ = 0;
    choiceCount_ = 0;
    selectedChoice_ = 0;

    currentLineNumber_ = 0;
    waitStartMsec_ = 0;
    waitDurationMsec_ = 0;
    finishedStartMsec_ = 0;

    currentChapter_[0] = '\0';
    currentSpeaker_[0] = '\0';
    currentText_[0] = '\0';

    errorCode_ = ErrorCode::NONE;
    errorLineNumber_ = 0;
    errorDetail[0] = '\0';

    scriptEnded_ = false;
    screenDirty_ = true;

    beginScript();
}

bool SignalLost::buildLabelTable() {
    reader_.rewind();

    char line[LINE_BUFFER_SIZE];
    uint32_t lineOffset = 0;
    uint32_t lineNumber = 0;

    bool insideTextBlock = false;
    bool insideChoiceBlock = false;

    while (reader_.readLine(line, sizeof(line), &lineOffset)) {
        ++lineNumber;

        removeComment(line);
        char* cleaned = trim(line);

        if (insideTextBlock) {
            if (strcmp(cleaned, "ENDTEXT") == 0) {
                insideTextBlock = false;
            }
            continue;
        }

        if (insideChoiceBlock) {
            if (strcmp(cleaned, "ENDCHOICE") == 0) {
                insideChoiceBlock = false;
            }
            continue;
        }

        if (strcmp(cleaned, "TEXT") == 0) {
            insideTextBlock = true;
            continue;
        }

        if (strcmp(cleaned, "CHOICE") == 0) {
            insideChoiceBlock = true;
            continue;
        }

        if (isBlank(cleaned) ||
            !startsWithCommand(cleaned, "LABEL")) {
            continue;
        }

        char* labelName = commandArguments(cleaned, "LABEL");

        if (!isValidIdentifier(labelName)) {
            setError(
                ErrorCode::SYNTAX_ERROR,
                lineNumber,
                "Invalid LABEL identifier"
            );
            return false;
        }

        const uint32_t targetOffset = reader_.position();

        if (!addLabel(labelName, targetOffset, lineNumber)) {
            return false;
        }
    }

    if (insideTextBlock) {
        setError(
            ErrorCode::MISSING_ENDTEXT,
            lineNumber,
            "ENDTEXT not found"
        );
        return false;
    }

    if (insideChoiceBlock) {
        setError(
            ErrorCode::MISSING_ENDCHOICE,
            lineNumber,
            "ENDCHOICE not found"
        );
        return false;
    }

    reader_.rewind();
    return true;
}

bool SignalLost::beginScript() {
    if (!jumpToLabel("start")) {
        return false;
    }

    mode_ = Mode::EXECUTING;
    executeUntilBlocked();
    return mode_ != Mode::ERROR;
}

bool SignalLost::jumpToLabel(const char* labelName) {
    int16_t index = findLabelIndex(labelName);

    if (index < 0) {
        setError(
            ErrorCode::LABEL_NOT_FOUND,
            currentLineNumber_,
            labelName
        );
        return false;
    }

    if (!reader_.seek(labels_[index].offset)) {
        setError(
            ErrorCode::LABEL_NOT_FOUND,
            currentLineNumber_,
            labelName
        );
        return false;
    }

    currentLineNumber_ = labels_[index].lineNumber;
    return true;
}

// ============================================================
// Main update
// ============================================================

PRUZEA::Game::GameState SignalLost::onUpdate(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio,
    PRUZEA::Storage& storage,
    float deltaSec
) {
    (void)storage;

    switch (mode_) {
    case Mode::TITLE:
        updateTitle(input, audio);
        break;

    case Mode::EXECUTING:
        executeUntilBlocked();
        break;

    case Mode::SHOWING_TEXT:
        updateText(input, audio);
        break;

    case Mode::SHOWING_CHOICES:
        updateChoices(input, audio);
        break;

    case Mode::WAITING_TIME:
        updateTimedWait();
        break;

    case Mode::WAITING_KEY:
        updateKeyWait(input, audio);
        break;

    case Mode::FINISHED:
        updateFinished(input, audio);
        break;

    case Mode::ERROR:
        if (input.justPressed(PRUZEA::Input::A) ||
            input.justPressed(PRUZEA::Input::START)) {
            mode_ = Mode::TITLE;
            screenDirty_ = true;
            audio.playSE(&Audio::SE::NO_2, 0.7f);
        }
        break;
    }

    if (screenDirty_) {
        dirty = true;
    }

    if (mode_ == Mode::FINISHED &&
        !PRUZEA::Platform::elapsed(
            PRUZEA::Platform::getMsec(),
            finishedStartMsec_,
            FINISHED_TWEEN_MSEC
        )) {
        dirty = true;
    }

    return Game::GameState::RUNNING;
}

// ============================================================
// Script execution
// ============================================================

void SignalLost::executeUntilBlocked() {
    constexpr uint16_t COMMAND_GUARD_LIMIT = 256;

    char line[LINE_BUFFER_SIZE];
    uint32_t lineOffset = 0;
    uint16_t commandCount = 0;

    while (mode_ == Mode::EXECUTING) {
        if (++commandCount > COMMAND_GUARD_LIMIT) {
            setError(
                ErrorCode::SYNTAX_ERROR,
                currentLineNumber_,
                "Execution guard reached"
            );
            return;
        }

        if (!reader_.readLine(line, sizeof(line), &lineOffset)) {
            setError(
                ErrorCode::SYNTAX_ERROR,
                currentLineNumber_,
                "Unexpected end of script"
            );
            return;
        }

        ++currentLineNumber_;

        removeComment(line);
        char* cleaned = trim(line);

        if (isBlank(cleaned)) {
            continue;
        }

        if (!executeCommand(cleaned, currentLineNumber_)) {
            return;
        }
    }
}

bool SignalLost::executeCommand(
    char* line,
    uint32_t commandLineNumber
) {
    currentLineNumber_ = commandLineNumber;

    if (startsWithCommand(line, "LABEL")) {
        return executeLabelCommand(commandArguments(line, "LABEL"));
    }

    if (startsWithCommand(line, "JUMP")) {
        return executeJumpCommand(commandArguments(line, "JUMP"));
    }

    if (startsWithCommand(line, "CHAPTER")) {
        return executeChapterCommand(commandArguments(line, "CHAPTER"));
    }

    if (startsWithCommand(line, "NAME")) {
        return executeNameCommand(commandArguments(line, "NAME"));
    }

    if (strcmp(line, "TEXT") == 0) {
        return executeTextCommand();
    }

    if (strcmp(line, "CHOICE") == 0) {
        return executeChoiceCommand();
    }

    if (startsWithCommand(line, "SET")) {
        return executeSetCommand(commandArguments(line, "SET"));
    }

    if (startsWithCommand(line, "ADD")) {
        return executeAddCommand(commandArguments(line, "ADD"));
    }

    if (startsWithCommand(line, "SUB")) {
        return executeSubCommand(commandArguments(line, "SUB"));
    }

    if (startsWithCommand(line, "IF")) {
        return executeIfCommand(commandArguments(line, "IF"));
    }

    if (startsWithCommand(line, "WAIT")) {
        return executeWaitCommand(commandArguments(line, "WAIT"));
    }

    if (strcmp(line, "WAITKEY") == 0) {
        return executeWaitKeyCommand();
    }

    if (strcmp(line, "END") == 0) {
        return executeEndCommand();
    }

    setError(
        ErrorCode::UNKNOWN_COMMAND,
        commandLineNumber,
        line
    );
    return false;
}

bool SignalLost::executeLabelCommand(char* arguments) {
    if (!isValidIdentifier(arguments)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid LABEL"
        );
        return false;
    }

    return true;
}

bool SignalLost::executeJumpCommand(char* arguments) {
    if (!isValidIdentifier(arguments)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid JUMP target"
        );
        return false;
    }

    return jumpToLabel(arguments);
}

bool SignalLost::executeChapterCommand(char* arguments) {
    if (isBlank(arguments)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "CHAPTER requires text"
        );
        return false;
    }

    if (!copyText(
            currentChapter_,
            sizeof(currentChapter_),
            arguments
        )) {
        setError(
            ErrorCode::LINE_TOO_LONG,
            currentLineNumber_,
            "CHAPTER text too long"
        );
        return false;
    }

    screenDirty_ = true;
    return true;
}

bool SignalLost::executeNameCommand(char* arguments) {
    if (!copyText(
            currentSpeaker_,
            sizeof(currentSpeaker_),
            arguments
        )) {
        setError(
            ErrorCode::LINE_TOO_LONG,
            currentLineNumber_,
            "NAME text too long"
        );
        return false;
    }

    screenDirty_ = true;
    return true;
}

bool SignalLost::executeTextCommand() {
    if (!readTextBlock()) {
        return false;
    }

    mode_ = Mode::SHOWING_TEXT;
    screenDirty_ = true;
    return true;
}

bool SignalLost::executeChoiceCommand() {
    if (!readChoiceBlock()) {
        return false;
    }

    selectedChoice_ = 0;
    mode_ = Mode::SHOWING_CHOICES;
    screenDirty_ = true;
    return true;
}

bool SignalLost::executeSetCommand(char* arguments) {
    char* variable = nullptr;
    int32_t value = 0;

    if (!parseVariableOperation(arguments, &variable, &value)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid SET"
        );
        return false;
    }

    return setVariableValue(variable, value);
}

bool SignalLost::executeAddCommand(char* arguments) {
    char* variable = nullptr;
    int32_t value = 0;

    if (!parseVariableOperation(arguments, &variable, &value)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid ADD"
        );
        return false;
    }

    return addVariableValue(variable, value);
}

bool SignalLost::executeSubCommand(char* arguments) {
    char* variable = nullptr;
    int32_t value = 0;

    if (!parseVariableOperation(arguments, &variable, &value)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid SUB"
        );
        return false;
    }

    return addVariableValue(variable, -value);
}

bool SignalLost::executeIfCommand(char* arguments) {
    char* variable = nullptr;
    char* operation = nullptr;
    char* label = nullptr;
    int32_t value = 0;

    if (!parseCondition(
            arguments,
            &variable,
            &operation,
            &value,
            &label
        )) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid IF"
        );
        return false;
    }

    const int32_t currentValue = getVariableValue(variable);

    if (evaluateCondition(currentValue, operation, value)) {
        return jumpToLabel(label);
    }

    return true;
}

bool SignalLost::executeWaitCommand(char* arguments) {
    if (isBlank(arguments)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "WAIT requires seconds"
        );
        return false;
    }

    char* endPointer = nullptr;
    float seconds = strtof(arguments, &endPointer);

    if (endPointer == arguments || *trim(endPointer) != '\0' ||
        seconds < 0.0f) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid WAIT value"
        );
        return false;
    }

    waitDurationMsec_ =
        static_cast<uint32_t>(seconds * 1000.0f);
    waitStartMsec_ = PRUZEA::Platform::getMsec();

    modeBeforeWait_ = Mode::EXECUTING;
    mode_ = Mode::WAITING_TIME;
    return true;
}

bool SignalLost::executeWaitKeyCommand() {
    modeBeforeWait_ = Mode::EXECUTING;
    mode_ = Mode::WAITING_KEY;
    screenDirty_ = true;
    return true;
}

bool SignalLost::executeEndCommand() {
    scriptEnded_ = true;
    finishedStartMsec_ = PRUZEA::Platform::getMsec();
    mode_ = Mode::FINISHED;
    screenDirty_ = true;
    return true;
}

// ============================================================
// Script blocks
// ============================================================

bool SignalLost::readTextBlock() {
    currentText_[0] = '\0';

    char line[LINE_BUFFER_SIZE];
    uint32_t lineOffset = 0;
    bool firstLine = true;

    while (reader_.readLine(line, sizeof(line), &lineOffset)) {
        ++currentLineNumber_;

        char compareLine[LINE_BUFFER_SIZE];
        copyText(compareLine, sizeof(compareLine), line);
        char* cleaned = trim(compareLine);

        if (strcmp(cleaned, "ENDTEXT") == 0) {
            return true;
        }

        if (!firstLine) {
            if (!appendText(
                    currentText_,
                    sizeof(currentText_),
                    "\n"
                )) {
                setError(
                    ErrorCode::TEXT_TOO_LONG,
                    currentLineNumber_,
                    "TEXT block too long"
                );
                return false;
            }
        }

        if (!appendText(
                currentText_,
                sizeof(currentText_),
                line
            )) {
            setError(
                ErrorCode::TEXT_TOO_LONG,
                currentLineNumber_,
                "TEXT block too long"
            );
            return false;
        }

        firstLine = false;
    }

    setError(
        ErrorCode::MISSING_ENDTEXT,
        currentLineNumber_,
        "ENDTEXT not found"
    );
    return false;
}

bool SignalLost::readChoiceBlock() {
    memset(choices_, 0, sizeof(choices_));
    choiceCount_ = 0;

    char line[LINE_BUFFER_SIZE];
    uint32_t lineOffset = 0;

    while (reader_.readLine(line, sizeof(line), &lineOffset)) {
        ++currentLineNumber_;

        removeComment(line);
        char* cleaned = trim(line);

        if (isBlank(cleaned)) {
            continue;
        }

        if (strcmp(cleaned, "ENDCHOICE") == 0) {
            if (choiceCount_ == 0) {
                setError(
                    ErrorCode::SYNTAX_ERROR,
                    currentLineNumber_,
                    "CHOICE requires at least one item"
                );
                return false;
            }

            return true;
        }

        if (choiceCount_ >= MAX_CHOICES) {
            setError(
                ErrorCode::CHOICE_TABLE_FULL,
                currentLineNumber_,
                "Too many choices"
            );
            return false;
        }

        char* choiceText = nullptr;
        char* choiceLabel = nullptr;

        if (!parseChoiceLine(
                cleaned,
                &choiceText,
                &choiceLabel
            )) {
            setError(
                ErrorCode::SYNTAX_ERROR,
                currentLineNumber_,
                "Invalid CHOICE item"
            );
            return false;
        }

        if (!copyText(
                choices_[choiceCount_].text,
                sizeof(choices_[choiceCount_].text),
                choiceText
            ) ||
            !copyText(
                choices_[choiceCount_].label,
                sizeof(choices_[choiceCount_].label),
                choiceLabel
            )) {
            setError(
                ErrorCode::LINE_TOO_LONG,
                currentLineNumber_,
                "CHOICE item too long"
            );
            return false;
        }

        choices_[choiceCount_].used = true;
        ++choiceCount_;
    }

    setError(
        ErrorCode::MISSING_ENDCHOICE,
        currentLineNumber_,
        "ENDCHOICE not found"
    );
    return false;
}

// ============================================================
// Label table
// ============================================================

int16_t SignalLost::findLabelIndex(
    const char* labelName
) const {
    if (labelName == nullptr) {
        return -1;
    }

    for (uint8_t i = 0; i < labelCount_; ++i) {
        if (labels_[i].used &&
            strcmp(labels_[i].name, labelName) == 0) {
            return static_cast<int16_t>(i);
        }
    }

    return -1;
}

bool SignalLost::addLabel(
    const char* labelName,
    uint32_t offset,
    uint32_t lineNumber
) {
    if (findLabelIndex(labelName) >= 0) {
        setError(
            ErrorCode::DUPLICATE_LABEL,
            lineNumber,
            labelName
        );
        return false;
    }

    if (labelCount_ >= MAX_LABELS) {
        setError(
            ErrorCode::LABEL_TABLE_FULL,
            lineNumber,
            "Too many LABEL entries"
        );
        return false;
    }

    LabelEntry& entry = labels_[labelCount_];

    if (!copyText(entry.name, sizeof(entry.name), labelName)) {
        setError(
            ErrorCode::LINE_TOO_LONG,
            lineNumber,
            "LABEL name too long"
        );
        return false;
    }

    entry.offset = offset;
    entry.lineNumber = lineNumber;
    entry.used = true;
    ++labelCount_;

    return true;
}

// ============================================================
// Variables
// ============================================================

int16_t SignalLost::findVariableIndex(
    const char* variableName
) const {
    if (variableName == nullptr) {
        return -1;
    }

    for (uint8_t i = 0; i < variableCount_; ++i) {
        if (variables_[i].used &&
            strcmp(variables_[i].name, variableName) == 0) {
            return static_cast<int16_t>(i);
        }
    }

    return -1;
}

int32_t SignalLost::getVariableValue(
    const char* variableName
) const {
    int16_t index = findVariableIndex(variableName);

    if (index < 0) {
        return 0;
    }

    return variables_[index].value;
}

bool SignalLost::setVariableValue(
    const char* variableName,
    int32_t value
) {
    if (!isValidIdentifier(variableName)) {
        setError(
            ErrorCode::SYNTAX_ERROR,
            currentLineNumber_,
            "Invalid variable name"
        );
        return false;
    }

    int16_t index = findVariableIndex(variableName);

    if (index >= 0) {
        variables_[index].value = value;
        return true;
    }

    if (variableCount_ >= MAX_VARIABLES) {
        setError(
            ErrorCode::VARIABLE_TABLE_FULL,
            currentLineNumber_,
            "Too many variables"
        );
        return false;
    }

    VariableEntry& entry = variables_[variableCount_];

    if (!copyText(
            entry.name,
            sizeof(entry.name),
            variableName
        )) {
        setError(
            ErrorCode::LINE_TOO_LONG,
            currentLineNumber_,
            "Variable name too long"
        );
        return false;
    }

    entry.value = value;
    entry.used = true;
    ++variableCount_;

    return true;
}

bool SignalLost::addVariableValue(
    const char* variableName,
    int32_t value
) {
    const int32_t currentValue =
        getVariableValue(variableName);

    return setVariableValue(
        variableName,
        currentValue + value
    );
}

// ============================================================
// Parsing helpers
// ============================================================

char* SignalLost::trim(char* text) {
    if (text == nullptr) {
        return text;
    }

    while (*text != '\0' &&
           isspace(static_cast<unsigned char>(*text))) {
        ++text;
    }

    char* end = text + strlen(text);

    while (end > text &&
           isspace(static_cast<unsigned char>(end[-1]))) {
        --end;
    }

    *end = '\0';
    return text;
}

void SignalLost::removeComment(char* text) {
    if (text == nullptr) {
        return;
    }

    char* comment = strchr(text, '#');

    if (comment != nullptr) {
        *comment = '\0';
    }
}

bool SignalLost::isBlank(const char* text) {
    if (text == nullptr) {
        return true;
    }

    while (*text != '\0') {
        if (!isspace(static_cast<unsigned char>(*text))) {
            return false;
        }
        ++text;
    }

    return true;
}

bool SignalLost::startsWithCommand(
    const char* line,
    const char* command
) {
    if (line == nullptr || command == nullptr) {
        return false;
    }

    size_t commandLength = strlen(command);

    if (strncmp(line, command, commandLength) != 0) {
        return false;
    }

    char boundary = line[commandLength];

    return boundary == '\0' ||
           boundary == ' ' ||
           boundary == '\t';
}

char* SignalLost::commandArguments(
    char* line,
    const char* command
) {
    if (line == nullptr || command == nullptr) {
        return line;
    }

    return trim(line + strlen(command));
}

bool SignalLost::copyText(
    char* destination,
    size_t destinationSize,
    const char* source
) {
    if (destination == nullptr ||
        destinationSize == 0 ||
        source == nullptr) {
        return false;
    }

    size_t sourceLength = strlen(source);

    if (sourceLength >= destinationSize) {
        destination[0] = '\0';
        return false;
    }

    memcpy(destination, source, sourceLength + 1);
    return true;
}

bool SignalLost::appendText(
    char* destination,
    size_t destinationSize,
    const char* source
) {
    if (destination == nullptr ||
        source == nullptr ||
        destinationSize == 0) {
        return false;
    }

    size_t currentLength = strlen(destination);
    size_t sourceLength = strlen(source);

    if (currentLength + sourceLength >= destinationSize) {
        return false;
    }

    memcpy(
        destination + currentLength,
        source,
        sourceLength + 1
    );

    return true;
}

bool SignalLost::parseInteger(
    const char* text,
    int32_t* outValue
) {
    if (text == nullptr || outValue == nullptr) {
        return false;
    }

    char* endPointer = nullptr;
    long value = strtol(text, &endPointer, 10);

    if (endPointer == text) {
        return false;
    }

    while (*endPointer != '\0' &&
           isspace(static_cast<unsigned char>(*endPointer))) {
        ++endPointer;
    }

    if (*endPointer != '\0') {
        return false;
    }

    *outValue = static_cast<int32_t>(value);
    return true;
}

bool SignalLost::parseVariableOperation(
    char* arguments,
    char** outVariable,
    int32_t* outValue
) {
    if (arguments == nullptr ||
        outVariable == nullptr ||
        outValue == nullptr) {
        return false;
    }

    char* cursor = trim(arguments);

    if (*cursor == '\0') {
        return false;
    }

    char* variable = cursor;

    while (*cursor != '\0' &&
           !isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }

    if (*cursor == '\0') {
        return false;
    }

    *cursor++ = '\0';
    cursor = trim(cursor);

    if (!isValidIdentifier(variable) ||
        !parseInteger(cursor, outValue)) {
        return false;
    }

    *outVariable = variable;
    return true;
}

bool SignalLost::parseChoiceLine(
    char* line,
    char** outText,
    char** outLabel
) {
    if (line == nullptr ||
        outText == nullptr ||
        outLabel == nullptr) {
        return false;
    }

    char* arrow = strstr(line, "->");

    if (arrow == nullptr) {
        return false;
    }

    *arrow = '\0';

    char* text = trim(line);
    char* label = trim(arrow + 2);

    if (isBlank(text) || !isValidIdentifier(label)) {
        return false;
    }

    *outText = text;
    *outLabel = label;
    return true;
}

bool SignalLost::parseCondition(
    char* arguments,
    char** outVariable,
    char** outOperator,
    int32_t* outValue,
    char** outLabel
) {
    if (arguments == nullptr ||
        outVariable == nullptr ||
        outOperator == nullptr ||
        outValue == nullptr ||
        outLabel == nullptr) {
        return false;
    }

    char* arrow = strstr(arguments, "->");

    if (arrow == nullptr) {
        return false;
    }

    *arrow = '\0';

    char* condition = trim(arguments);
    char* label = trim(arrow + 2);

    if (!isValidIdentifier(label)) {
        return false;
    }

    char* cursor = condition;
    char* variable = cursor;

    while (*cursor != '\0' &&
           !isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }

    if (*cursor == '\0') {
        return false;
    }

    *cursor++ = '\0';
    cursor = trim(cursor);

    char* operation = cursor;

    while (*cursor != '\0' &&
           !isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }

    if (*cursor == '\0') {
        return false;
    }

    *cursor++ = '\0';
    cursor = trim(cursor);

    if (!isValidIdentifier(variable) ||
        !parseInteger(cursor, outValue)) {
        return false;
    }

    if (strcmp(operation, "==") != 0 &&
        strcmp(operation, "!=") != 0 &&
        strcmp(operation, "<") != 0 &&
        strcmp(operation, "<=") != 0 &&
        strcmp(operation, ">") != 0 &&
        strcmp(operation, ">=") != 0) {
        return false;
    }

    *outVariable = variable;
    *outOperator = operation;
    *outLabel = label;
    return true;
}

bool SignalLost::evaluateCondition(
    int32_t left,
    const char* operation,
    int32_t right
) {
    if (strcmp(operation, "==") == 0) {
        return left == right;
    }

    if (strcmp(operation, "!=") == 0) {
        return left != right;
    }

    if (strcmp(operation, "<") == 0) {
        return left < right;
    }

    if (strcmp(operation, "<=") == 0) {
        return left <= right;
    }

    if (strcmp(operation, ">") == 0) {
        return left > right;
    }

    if (strcmp(operation, ">=") == 0) {
        return left >= right;
    }

    return false;
}

bool SignalLost::isValidIdentifier(
    const char* identifier
) {
    if (identifier == nullptr || identifier[0] == '\0') {
        return false;
    }

    unsigned char first =
        static_cast<unsigned char>(identifier[0]);

    if (!(first == '_' ||
          (first >= 'a' && first <= 'z'))) {
        return false;
    }

    for (size_t i = 1; identifier[i] != '\0'; ++i) {
        unsigned char c =
            static_cast<unsigned char>(identifier[i]);

        if (!(c == '_' ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9'))) {
            return false;
        }
    }

    return true;
}

// ============================================================
// Error handling
// ============================================================

void SignalLost::setError(
    ErrorCode code,
    uint32_t lineNumber,
    const char* detail
) {
    errorCode_ = code;
    errorLineNumber_ = lineNumber;

    if (detail == nullptr) {
        errorDetail[0] = '\0';
    } else {
        copyText(
            errorDetail,
            sizeof(errorDetail),
            detail
        );
    }

    mode_ = Mode::ERROR;
    screenDirty_ = true;
}

const char* SignalLost::getErrorName() const {
    switch (errorCode_) {
    case ErrorCode::NONE:
        return "PNS000 NONE";
    case ErrorCode::DUPLICATE_LABEL:
        return "PNS001 DUPLICATE LABEL";
    case ErrorCode::LABEL_NOT_FOUND:
        return "PNS002 LABEL NOT FOUND";
    case ErrorCode::SYNTAX_ERROR:
        return "PNS004 SYNTAX ERROR";
    case ErrorCode::UNKNOWN_COMMAND:
        return "PNS005 UNKNOWN COMMAND";
    case ErrorCode::LABEL_TABLE_FULL:
        return "PNS101 LABEL TABLE FULL";
    case ErrorCode::VARIABLE_TABLE_FULL:
        return "PNS102 VARIABLE TABLE FULL";
    case ErrorCode::CHOICE_TABLE_FULL:
        return "PNS103 CHOICE TABLE FULL";
    case ErrorCode::TEXT_TOO_LONG:
        return "PNS104 TEXT TOO LONG";
    case ErrorCode::LINE_TOO_LONG:
        return "PNS105 LINE TOO LONG";
    case ErrorCode::MISSING_ENDTEXT:
        return "PNS106 MISSING ENDTEXT";
    case ErrorCode::MISSING_ENDCHOICE:
        return "PNS107 MISSING ENDCHOICE";
    case ErrorCode::MISSING_START_LABEL:
        return "PNS108 MISSING START";
    }

    return "PNS999 UNKNOWN ERROR";
}

// ============================================================
// Input handling
// ============================================================

void SignalLost::updateTitle(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio
) {
    if (input.justPressed(PRUZEA::Input::START) ||
        input.justPressed(PRUZEA::Input::A)) {
        audio.playSE(&Audio::SE::NO_1, 0.75f);
        resetForNewGame();
    }
}

void SignalLost::updateText(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio
) {
    if (!input.justPressed(PRUZEA::Input::A)) {
        return;
    }

    audio.playSE(&Audio::SE::NO_5, 0.55f);
    mode_ = Mode::EXECUTING;
    screenDirty_ = true;
    executeUntilBlocked();
}

void SignalLost::updateChoices(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio
) {
    bool moved = false;

    if (input.justPressed(PRUZEA::Input::UP) ||
        input.repeat(PRUZEA::Input::UP)) {
        if (selectedChoice_ == 0) {
            selectedChoice_ = choiceCount_ - 1;
        } else {
            --selectedChoice_;
        }
        moved = true;
    }

    if (input.justPressed(PRUZEA::Input::DOWN) ||
        input.repeat(PRUZEA::Input::DOWN)) {
        selectedChoice_ =
            static_cast<uint8_t>(
                (selectedChoice_ + 1) % choiceCount_
            );
        moved = true;
    }

    if (moved) {
        audio.playSE(&Audio::SE::NO_1, 0.45f);
        screenDirty_ = true;
    }

    if (!input.justPressed(PRUZEA::Input::A)) {
        return;
    }

    audio.playSE(&Audio::SE::NO_3, 0.65f);

    char targetLabel[LABEL_NAME_SIZE];
    copyText(
        targetLabel,
        sizeof(targetLabel),
        choices_[selectedChoice_].label
    );

    memset(choices_, 0, sizeof(choices_));
    choiceCount_ = 0;
    selectedChoice_ = 0;

    if (!jumpToLabel(targetLabel)) {
        return;
    }

    mode_ = Mode::EXECUTING;
    screenDirty_ = true;
    executeUntilBlocked();
}

void SignalLost::updateTimedWait() {
    const uint32_t now = PRUZEA::Platform::getMsec();

    if (!PRUZEA::Platform::elapsed(
            now,
            waitStartMsec_,
            waitDurationMsec_
        )) {
        return;
    }

    mode_ = modeBeforeWait_;
    screenDirty_ = true;

    if (mode_ == Mode::EXECUTING) {
        executeUntilBlocked();
    }
}

void SignalLost::updateKeyWait(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio
) {
    if (!input.justPressed(PRUZEA::Input::A)) {
        return;
    }

    audio.playSE(&Audio::SE::NO_5, 0.5f);
    mode_ = modeBeforeWait_;
    screenDirty_ = true;

    if (mode_ == Mode::EXECUTING) {
        executeUntilBlocked();
    }
}

void SignalLost::updateFinished(
    PRUZEA::Input& input,
    PRUZEA::Audio& audio
) {
    if (!input.justPressed(PRUZEA::Input::A) &&
        !input.justPressed(PRUZEA::Input::START)) {
        return;
    }

    audio.playSE(&Audio::SE::NO_1, 0.7f);
    mode_ = Mode::TITLE;
    screenDirty_ = true;
}

// ============================================================
// Drawing
// ============================================================

bool SignalLost::onDraw(
    PRUZEA::Graphics& graphics,
    bool requestFullRedraw
) {
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    graphics.resetViewport();
    const uint32_t now = PRUZEA::Platform::getMsec();

    switch (mode_) {
    case Mode::TITLE:
        drawTitle(graphics);
        break;

    case Mode::FINISHED:
        drawFinished(graphics, now);
        break;

    case Mode::ERROR:
        drawError(graphics);
        break;

    default:
        drawNovel(graphics);
        break;
    }

    screenDirty_ = false;
    dirty = false;
    return true;
}

void SignalLost::drawTitle(
    PRUZEA::Graphics& graphics
) {
    graphics.fillScreen(COLOR_BACKGROUND);

    graphics.fillRoundRect(
        16,
        18,
        288,
        188,
        8,
        COLOR_PANEL
    );

    graphics.drawRoundRect(
        16,
        18,
        288,
        188,
        8,
        2,
        COLOR_ACCENT
    );

    graphics.drawString(
        "PRUZEA NOVEL",
        SCREEN_WIDTH / 2,
        44,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_18,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawString(
        "SIGNAL LOST",
        SCREEN_WIDTH / 2,
        82,
        COLOR_TEXT,
        PRUZEA::Graphics::SIZE_32B,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawLine(
        54,
        110,
        266,
        110,
        COLOR_ACCENT
    );

    graphics.drawString(
        "A CHOICE-DRIVEN STORY",
        SCREEN_WIDTH / 2,
        128,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_13,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawString(
        "PRESS START",
        SCREEN_WIDTH / 2,
        170,
        COLOR_SELECTED,
        PRUZEA::Graphics::SIZE_22B,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawString(
        "PNS 1.2.1 SAMPLE",
        SCREEN_WIDTH / 2,
        218,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_10,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::BOTTOM
    );
}

void SignalLost::drawNovel(
    PRUZEA::Graphics& graphics
) {
    graphics.fillScreen(COLOR_BACKGROUND);

    drawHeader(graphics);
    drawTextWindow(graphics);

    if (mode_ == Mode::SHOWING_CHOICES) {
        drawChoices(graphics);
    }

    drawFooter(graphics);
}

void SignalLost::drawFinished(
    PRUZEA::Graphics& graphics,
    uint32_t now
) {
    drawNovel(graphics);
    graphics.fillRectAlpha(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        112,
        PRUZEA::Graphics::BLACK
    );

    const float t = PRUZEA::Math::clamp(
        static_cast<float>(now - finishedStartMsec_) /
            static_cast<float>(FINISHED_TWEEN_MSEC),
        0.0f,
        1.0f
    );
    const float eased =
        PRUZEA::Tween::apply(t, PRUZEA::Tween::Ease::EASE_OUT_BACK);
    const int16_t panelY = static_cast<int16_t>(
        PRUZEA::Tween::lerp(-170.0f, 30.0f, eased)
    );

    graphics.fillRoundRect(
        24,
        panelY,
        272,
        158,
        8,
        COLOR_PANEL
    );

    graphics.drawRoundRect(
        24,
        panelY,
        272,
        158,
        8,
        2,
        COLOR_ACCENT
    );

    graphics.drawString(
        "END OF TRANSMISSION",
        SCREEN_WIDTH / 2,
        panelY + 40,
        COLOR_TEXT,
        PRUZEA::Graphics::SIZE_25B,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawString(
        currentChapter_[0] != '\0'
            ? currentChapter_
            : "STORY COMPLETE",
        SCREEN_WIDTH / 2,
        panelY + 78,
        COLOR_ACCENT,
        PRUZEA::Graphics::SIZE_18,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    graphics.drawString(
        "PRESS A TO RETURN",
        SCREEN_WIDTH / 2,
        panelY + 126,
        COLOR_SELECTED,
        PRUZEA::Graphics::SIZE_18,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );
}

void SignalLost::drawError(
    PRUZEA::Graphics& graphics
) {
    drawNovel(graphics);
    graphics.fillRectAlpha(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        128,
        PRUZEA::Graphics::BLACK
    );

    graphics.fillRoundRect(
        12,
        16,
        296,
        190,
        6,
        COLOR_PANEL
    );

    graphics.drawRoundRect(
        12,
        16,
        296,
        190,
        6,
        2,
        COLOR_ERROR
    );

    graphics.drawString(
        "PNS ERROR",
        28,
        32,
        COLOR_ERROR,
        PRUZEA::Graphics::SIZE_25B
    );

    graphics.drawString(
        getErrorName(),
        28,
        70,
        COLOR_TEXT,
        PRUZEA::Graphics::SIZE_18
    );

    char lineText[48];
    snprintf(
        lineText,
        sizeof(lineText),
        "LINE: %lu",
        static_cast<unsigned long>(errorLineNumber_)
    );

    graphics.drawString(
        lineText,
        28,
        98,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_18
    );

    drawWrappedText(
        graphics,
        errorDetail,
        28,
        126,
        260,
        3,
        COLOR_TEXT,
        PRUZEA::Graphics::SIZE_18
    );

    graphics.drawString(
        "PRESS A",
        280,
        216,
        COLOR_SELECTED,
        PRUZEA::Graphics::SIZE_13,
        PRUZEA::Graphics::HorizontalAlign::RIGHT,
        PRUZEA::Graphics::VerticalAlign::BOTTOM
    );
}

void SignalLost::drawHeader(
    PRUZEA::Graphics& graphics
) {
    graphics.fillRoundRect(
        10,
        10,
        300,
        34,
        5,
        COLOR_PANEL_ALT
    );

    graphics.drawString(
        currentChapter_[0] != '\0'
            ? currentChapter_
            : "NO CHAPTER",
        20,
        27,
        COLOR_ACCENT,
        PRUZEA::Graphics::SIZE_18,
        PRUZEA::Graphics::HorizontalAlign::LEFT,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );

    char lineText[32];
    snprintf(
        lineText,
        sizeof(lineText),
        "L%lu",
        static_cast<unsigned long>(currentLineNumber_)
    );

    graphics.drawString(
        lineText,
        298,
        27,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_13,
        PRUZEA::Graphics::HorizontalAlign::RIGHT,
        PRUZEA::Graphics::VerticalAlign::MIDDLE
    );
}

void SignalLost::drawTextWindow(
    PRUZEA::Graphics& graphics
) {
    const int16_t panelY =
        mode_ == Mode::SHOWING_CHOICES ? 54 : 58;

    const int16_t panelHeight =
        mode_ == Mode::SHOWING_CHOICES ? 92 : 142;

    graphics.fillRoundRect(
        10,
        panelY,
        300,
        panelHeight,
        6,
        COLOR_PANEL
    );

    graphics.drawRoundRect(
        10,
        panelY,
        300,
        panelHeight,
        6,
        1,
        COLOR_MUTED
    );

    if (currentSpeaker_[0] != '\0') {
        graphics.fillRoundRect(
            22,
            panelY - 8,
            116,
            24,
            4,
            COLOR_ACCENT
        );

        graphics.drawString(
            currentSpeaker_,
            30,
            panelY + 4,
            COLOR_BACKGROUND,
            PRUZEA::Graphics::SIZE_18,
            PRUZEA::Graphics::HorizontalAlign::LEFT,
            PRUZEA::Graphics::VerticalAlign::MIDDLE
        );
    }

    drawWrappedText(
        graphics,
        currentText_,
        24,
        panelY + 24,
        272,
        mode_ == Mode::SHOWING_CHOICES ? 3 : 6,
        COLOR_TEXT,
        PRUZEA::Graphics::SIZE_18
    );
}

void SignalLost::drawChoices(
    PRUZEA::Graphics& graphics
) {
    const int16_t startY = 154;
    const int16_t itemHeight = 26;

    for (uint8_t i = 0; i < choiceCount_; ++i) {
        const int16_t y =
            startY + static_cast<int16_t>(i) * itemHeight;

        const bool selected = i == selectedChoice_;

        graphics.fillRoundRect(
            18,
            y,
            284,
            22,
            4,
            selected ? COLOR_PANEL_ALT : COLOR_PANEL
        );

        graphics.drawRoundRect(
            18,
            y,
            284,
            22,
            4,
            1,
            selected ? COLOR_SELECTED : COLOR_MUTED
        );

        graphics.drawString(
            selected ? ">" : " ",
            28,
            y + 11,
            selected ? COLOR_SELECTED : COLOR_MUTED,
            PRUZEA::Graphics::SIZE_18,
            PRUZEA::Graphics::HorizontalAlign::LEFT,
            PRUZEA::Graphics::VerticalAlign::MIDDLE
        );

        graphics.drawString(
            choices_[i].text,
            48,
            y + 11,
            selected ? COLOR_TEXT : COLOR_MUTED,
            PRUZEA::Graphics::SIZE_18,
            PRUZEA::Graphics::HorizontalAlign::LEFT,
            PRUZEA::Graphics::VerticalAlign::MIDDLE
        );
    }
}

void SignalLost::drawFooter(
    PRUZEA::Graphics& graphics
) {
    const char* footer = "";

    switch (mode_) {
    case Mode::SHOWING_TEXT:
        footer = "A: NEXT";
        break;

    case Mode::SHOWING_CHOICES:
        // footer = "UP/DOWN: SELECT   A: CONFIRM";
        return;
        break;

    case Mode::WAITING_KEY:
        footer = "A: CONTINUE";
        break;

    case Mode::WAITING_TIME:
        footer = "PLEASE WAIT";
        break;

    default:
        footer = "";
        break;
    }

    graphics.drawString(
        footer,
        160,
        SAFE_BOTTOM,
        COLOR_MUTED,
        PRUZEA::Graphics::SIZE_13,
        PRUZEA::Graphics::HorizontalAlign::CENTER,
        PRUZEA::Graphics::VerticalAlign::BOTTOM
    );
}

void SignalLost::drawWrappedText(
    PRUZEA::Graphics& graphics,
    const char* text,
    int16_t x,
    int16_t y,
    uint16_t maxWidth,
    uint8_t maxLines,
    PRUZEA::Graphics::Color color,
    PRUZEA::Graphics::Font font
) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }

    char line[LINE_BUFFER_SIZE];
    size_t lineLength = 0;
    uint8_t lineIndex = 0;
    const uint16_t lineHeight =
        graphics.getTextHeight("", font) + 3;

    const char* cursor = text;

    while (*cursor != '\0' && lineIndex < maxLines) {
        if (*cursor == '\n') {
            line[lineLength] = '\0';

            graphics.drawString(
                line,
                x,
                y + static_cast<int16_t>(lineIndex * lineHeight),
                color,
                font
            );

            lineLength = 0;
            ++lineIndex;
            ++cursor;
            continue;
        }

        char candidate[LINE_BUFFER_SIZE];

        if (lineLength + 1 >= sizeof(line)) {
            line[lineLength] = '\0';

            graphics.drawString(
                line,
                x,
                y + static_cast<int16_t>(lineIndex * lineHeight),
                color,
                font
            );

            lineLength = 0;
            ++lineIndex;
            continue;
        }

        memcpy(candidate, line, lineLength);
        candidate[lineLength] = *cursor;
        candidate[lineLength + 1] = '\0';

        uint16_t width =
            graphics.getTextWidth(candidate, font);

        if (width > maxWidth && lineLength > 0) {
            line[lineLength] = '\0';

            graphics.drawString(
                line,
                x,
                y + static_cast<int16_t>(lineIndex * lineHeight),
                color,
                font
            );

            lineLength = 0;
            ++lineIndex;

            while (*cursor == ' ') {
                ++cursor;
            }

            continue;
        }

        line[lineLength++] = *cursor;
        ++cursor;
    }

    if (lineLength > 0 && lineIndex < maxLines) {
        line[lineLength] = '\0';

        graphics.drawString(
            line,
            x,
            y + static_cast<int16_t>(lineIndex * lineHeight),
            color,
            font
        );
    }
}

void SignalLost::onTerminate(
    PRUZEA::Storage& storage
) {
    (void)storage;
}
