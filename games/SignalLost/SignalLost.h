#pragma once

#include "PRUZEA.h"

#include <stddef.h>
#include <stdint.h>

class SignalLost final : public PRUZEA::Game {
public:
    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;

    uint16_t getLogicalScreenWidth() const override;
    uint16_t getLogicalScreenHeight() const override;
    uint16_t getTargetScreenWidth() const override;
    uint16_t getTargetScreenHeight() const override;

    void onInit(PRUZEA::Storage& storage) override;

    PRUZEA::Game::GameState onUpdate(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio,
        PRUZEA::Storage& storage,
        float deltaSec
    ) override;

    bool onDraw(
        PRUZEA::Graphics& graphics,
        bool requestFullRedraw
    ) override;

    void onTerminate(PRUZEA::Storage& storage) override;

private:
    // ============================================================
    // PNS sample limits
    // ============================================================

    static constexpr uint8_t MAX_LABELS = 32;
    static constexpr uint8_t MAX_VARIABLES = 16;
    static constexpr uint8_t MAX_CHOICES = 4;

    static constexpr uint8_t LABEL_NAME_SIZE = 24;
    static constexpr uint8_t VARIABLE_NAME_SIZE = 24;
    static constexpr uint8_t SPEAKER_NAME_SIZE = 24;
    static constexpr uint8_t CHAPTER_NAME_SIZE = 32;
    static constexpr uint8_t CHOICE_TEXT_SIZE = 40;
    static constexpr uint8_t LINE_BUFFER_SIZE = 96;
    static constexpr uint16_t TEXT_BUFFER_SIZE = 384;

    static constexpr uint8_t TEXT_LINES_VISIBLE = 5;
    static constexpr uint8_t TEXT_CHARS_PER_LINE = 32;
    static constexpr uint32_t FINISHED_TWEEN_MSEC = 250;

    // ============================================================
    // Internal game states
    // ============================================================

    enum class Mode : uint8_t {
        TITLE,
        EXECUTING,
        SHOWING_TEXT,
        SHOWING_CHOICES,
        WAITING_TIME,
        WAITING_KEY,
        FINISHED,
        ERROR
    };

    enum class ErrorCode : uint8_t {
        NONE = 0,
        DUPLICATE_LABEL,
        LABEL_NOT_FOUND,
        SYNTAX_ERROR,
        UNKNOWN_COMMAND,
        LABEL_TABLE_FULL,
        VARIABLE_TABLE_FULL,
        CHOICE_TABLE_FULL,
        TEXT_TOO_LONG,
        LINE_TOO_LONG,
        MISSING_ENDTEXT,
        MISSING_ENDCHOICE,
        MISSING_START_LABEL
    };

    // ============================================================
    // Script source abstraction
    // ============================================================

    class LineReader {
    public:
        virtual bool readLine(
            char* outLine,
            size_t outSize,
            uint32_t* outLineOffset
        ) = 0;

        virtual bool seek(uint32_t offset) = 0;
        virtual void rewind() = 0;
        virtual uint32_t position() const = 0;

    protected:
        virtual ~LineReader() = default;
    };

    class MemoryLineReader final : public LineReader {
    public:
        MemoryLineReader();

        void attach(const char* script);

        bool readLine(
            char* outLine,
            size_t outSize,
            uint32_t* outLineOffset
        ) override;

        bool seek(uint32_t offset) override;
        void rewind() override;
        uint32_t position() const override;

    private:
        const char* script_;
        uint32_t position_;
    };

    // ============================================================
    // PNS runtime data
    // ============================================================

    struct LabelEntry {
        char name[LABEL_NAME_SIZE];
        uint32_t offset;
        uint32_t lineNumber;
        bool used;
    };

    struct VariableEntry {
        char name[VARIABLE_NAME_SIZE];
        int32_t value;
        bool used;
    };

    struct ChoiceEntry {
        char text[CHOICE_TEXT_SIZE];
        char label[LABEL_NAME_SIZE];
        bool used;
    };

    // ============================================================
    // Runtime state
    // ============================================================

    Mode mode_;
    Mode modeBeforeWait_;

    ErrorCode errorCode_;
    uint32_t errorLineNumber_;
    char errorDetail[LINE_BUFFER_SIZE];

    MemoryLineReader reader_;

    LabelEntry labels_[MAX_LABELS];
    VariableEntry variables_[MAX_VARIABLES];
    ChoiceEntry choices_[MAX_CHOICES];

    uint8_t labelCount_;
    uint8_t variableCount_;
    uint8_t choiceCount_;
    uint8_t selectedChoice_;

    uint32_t currentLineNumber_;
    uint32_t waitStartMsec_;
    uint32_t waitDurationMsec_;
    uint32_t finishedStartMsec_;

    char currentChapter_[CHAPTER_NAME_SIZE];
    char currentSpeaker_[SPEAKER_NAME_SIZE];
    char currentText_[TEXT_BUFFER_SIZE];

    bool screenDirty_;
    bool scriptEnded_;

    // ============================================================
    // Initialization and reset
    // ============================================================

    void resetRuntime();
    void resetForNewGame();

    bool buildLabelTable();
    bool beginScript();
    bool jumpToLabel(const char* labelName);

    // ============================================================
    // Script execution
    // ============================================================

    void executeUntilBlocked();

    bool executeCommand(
        char* line,
        uint32_t commandLineNumber
    );

    bool executeLabelCommand(char* arguments);
    bool executeJumpCommand(char* arguments);
    bool executeChapterCommand(char* arguments);
    bool executeNameCommand(char* arguments);
    bool executeTextCommand();
    bool executeChoiceCommand();
    bool executeSetCommand(char* arguments);
    bool executeAddCommand(char* arguments);
    bool executeSubCommand(char* arguments);
    bool executeIfCommand(char* arguments);
    bool executeWaitCommand(char* arguments);
    bool executeWaitKeyCommand();
    bool executeEndCommand();

    // ============================================================
    // Script blocks
    // ============================================================

    bool readTextBlock();
    bool readChoiceBlock();

    // ============================================================
    // Label table
    // ============================================================

    int16_t findLabelIndex(const char* labelName) const;

    bool addLabel(
        const char* labelName,
        uint32_t offset,
        uint32_t lineNumber
    );

    // ============================================================
    // Variables
    // ============================================================

    int16_t findVariableIndex(const char* variableName) const;
    int32_t getVariableValue(const char* variableName) const;

    bool setVariableValue(
        const char* variableName,
        int32_t value
    );

    bool addVariableValue(
        const char* variableName,
        int32_t value
    );

    // ============================================================
    // Parsing helpers
    // ============================================================

    static char* trim(char* text);
    static void removeComment(char* text);
    static bool isBlank(const char* text);

    static bool startsWithCommand(
        const char* line,
        const char* command
    );

    static char* commandArguments(
        char* line,
        const char* command
    );

    static bool copyText(
        char* destination,
        size_t destinationSize,
        const char* source
    );

    static bool appendText(
        char* destination,
        size_t destinationSize,
        const char* source
    );

    static bool parseInteger(
        const char* text,
        int32_t* outValue
    );

    static bool parseVariableOperation(
        char* arguments,
        char** outVariable,
        int32_t* outValue
    );

    static bool parseChoiceLine(
        char* line,
        char** outText,
        char** outLabel
    );

    static bool parseCondition(
        char* arguments,
        char** outVariable,
        char** outOperator,
        int32_t* outValue,
        char** outLabel
    );

    static bool evaluateCondition(
        int32_t left,
        const char* operation,
        int32_t right
    );

    static bool isValidIdentifier(const char* identifier);

    // ============================================================
    // Error handling
    // ============================================================

    void setError(
        ErrorCode code,
        uint32_t lineNumber,
        const char* detail
    );

    const char* getErrorName() const;

    // ============================================================
    // Input handling
    // ============================================================

    void updateTitle(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio
    );

    void updateText(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio
    );

    void updateChoices(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio
    );

    void updateTimedWait();
    void updateKeyWait(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio
    );

    void updateFinished(
        PRUZEA::Input& input,
        PRUZEA::Audio& audio
    );

    // ============================================================
    // Drawing
    // ============================================================

    void drawTitle(PRUZEA::Graphics& graphics);
    void drawNovel(PRUZEA::Graphics& graphics);
    void drawFinished(PRUZEA::Graphics& graphics, uint32_t now);
    void drawError(PRUZEA::Graphics& graphics);

    void drawHeader(PRUZEA::Graphics& graphics);
    void drawTextWindow(PRUZEA::Graphics& graphics);
    void drawChoices(PRUZEA::Graphics& graphics);
    void drawFooter(PRUZEA::Graphics& graphics);

    void drawWrappedText(
        PRUZEA::Graphics& graphics,
        const char* text,
        int16_t x,
        int16_t y,
        uint16_t maxWidth,
        uint8_t maxLines,
        PRUZEA::Graphics::Color color,
        PRUZEA::Graphics::Font font
    );

    // ============================================================
    // Game state conversion
    // ============================================================

    PRUZEA::Game::GameState getPublicGameState() const;
};
