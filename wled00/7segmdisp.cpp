#include "7segmdisp.h"

static CRGB dummy;

void LedBasedDisplay::setRowColor(uint8_t row, bool state, CRGB color) {
    for (uint8_t c = 0, n = columnCount(); c < n; ++c) {
        setLedColor(row, c, state, color);
    }
}

void LedBasedDisplay::setColumnColor(uint8_t column, bool state, CRGB color) {
    for (uint8_t r = 0, n = rowCount(); r < n; ++r) {
        setLedColor(r, column, state, color);
    }
}

void LedBasedDisplay::setColor(bool state, CRGB color) {
    for (uint8_t r = 0, rn = rowCount(); r < rn; ++r) {
        for (uint8_t c = 0, cn = columnCount(); c < cn; ++c) {
            setLedColor(r, c, state, color);
        }
    }
}

SevenSegmentDisplay::SevenSegmentDisplay(LedBasedDisplayOutput output, uint8_t ledsPerSegment):
    _output(output),
    _ledsPerSegment(ledsPerSegment),
    _value(_7SEG_SYM_EMPTY),
    _mode(LedBasedDisplayMode::SET_ALL_LEDS) {

    _offColors = (CRGB*) malloc(7 * _ledsPerSegment * sizeof(CRGB));
    _onColors = (CRGB*) malloc(7 * _ledsPerSegment * sizeof(CRGB));
    _indices = (uint8_t*) malloc(7 * _ledsPerSegment * sizeof(uint8_t));

    _showZero = true;
}

SevenSegmentDisplay::~SevenSegmentDisplay() {
    delete _indices;
    delete _offColors;
    delete _onColors;
}

uint8_t SevenSegmentDisplay::rowCount() {
    return _ledsPerSegment * 2 + 3;
}

uint8_t SevenSegmentDisplay::columnCount() {
    return _ledsPerSegment + 2;
}

uint8_t SevenSegmentDisplay::internalIndex(uint8_t row, uint8_t column) {
    uint8_t lastRow = (_ledsPerSegment + 1) * 2;

    if (row < 0 || row > lastRow) {
        return _7SEG_INDEX_UNDEF;
    }

    uint8_t midRow = _ledsPerSegment + 1;

    bool top = row == 0;
    bool mid = row == midRow;
    bool bot = row == lastRow;

    if (top || mid || bot) {
        if (column >= 1 && column <= _ledsPerSegment) {
            if (top) {
                return _7SEG_IDX(_7SEG_SEG_A, column - 1);
            } else if (mid) {
                return _7SEG_IDX(_7SEG_SEG_G, column - 1);
            } else {
                return _7SEG_IDX(_7SEG_SEG_D, column - 1);
            }
        }
    } else if (column == 0) {
        if (row < midRow) {
            return _7SEG_IDX(_7SEG_SEG_F, row - 1);
        } else {
            return _7SEG_IDX(_7SEG_SEG_E, row - midRow - 1);
        }
    } else if (column == _ledsPerSegment + 1) {
        if (row < midRow) {
            return _7SEG_IDX(_7SEG_SEG_B, row - 1);
        } else {
            return _7SEG_IDX(_7SEG_SEG_C, row - midRow - 1);
        }
    }

    return _7SEG_INDEX_UNDEF;
}

uint8_t SevenSegmentDisplay::indexOfCoords(uint8_t row, uint8_t column) {
    uint8_t idx = internalIndex(row, column);
    return idx == _7SEG_INDEX_UNDEF ? idx : _indices[idx];
}

Coords SevenSegmentDisplay::coordsOfIndex(uint16_t index) {
    Coords coords;
    for (int i = 0, n = 7 * _ledsPerSegment; i < n; ++i) {
        if (_indices[i] == index) {
            uint8_t seg = i / _ledsPerSegment;
            uint8_t idx = i % _ledsPerSegment;
            switch (seg) {
            case _7SEG_SEG_A: coords.row = 0; break;
            case _7SEG_SEG_B:
            case _7SEG_SEG_F: coords.row = idx + 1; break;
            case _7SEG_SEG_C:
            case _7SEG_SEG_E: coords.row = _ledsPerSegment + 1 + idx + 1; break;
            case _7SEG_SEG_D: coords.row = _ledsPerSegment + 1 + _ledsPerSegment + 1; break;
            case _7SEG_SEG_G: coords.row = _ledsPerSegment + 1; break;
            }
            switch (seg) {
            case _7SEG_SEG_A:
            case _7SEG_SEG_D:
            case _7SEG_SEG_G: coords.col = idx + 1; break;
            case _7SEG_SEG_B:
            case _7SEG_SEG_C: coords.col = _ledsPerSegment + 1; break;
            case _7SEG_SEG_E:
            case _7SEG_SEG_F: coords.col = 0; break;
            }
            return coords;
        }
    }
    return coords;
}

CRGB* SevenSegmentDisplay::getLedColor(uint8_t row, uint8_t column, bool state) {
    uint8_t idx = internalIndex(row, column);

    if (idx == _7SEG_INDEX_UNDEF) {
        return &dummy;
    }

    return &(state ? _onColors : _offColors)[idx];
}

void SevenSegmentDisplay::setLedColor(uint8_t row, uint8_t column, bool state, CRGB color) {
    uint8_t idx = internalIndex(row, column);

    if (idx == _7SEG_INDEX_UNDEF) {
        return;
    }

    (state ? _onColors : _offColors)[idx] = color;
}

void SevenSegmentDisplay::update() {
    uint8_t mask = 0b00000001;

    for (uint8_t seg = _7SEG_SEG_A; seg <= _7SEG_SEG_G; ++seg) {
        bool on = _value & mask;

        if (_7SEG_MODE(_mode, on)) {
            for (uint8_t i = 0; i < _ledsPerSegment; ++i) {
                uint8_t segmIdx = _7SEG_IDX(seg, i);
                CRGB c = (on ? _onColors : _offColors)[segmIdx];
                (*_output)(_indices[segmIdx], c.red, c.green, c.blue);
            }
        }

        mask <<= 1;
    }
}

void SevenSegmentDisplay::setMode(LedBasedDisplayMode mode) {
    _mode = mode;
}

void SevenSegmentDisplay::mapSegment(uint8_t seg, ...) {
    va_list ap;
    va_start(ap, seg);

    for (uint8_t i = 0; i < _ledsPerSegment; i++) {
        _indices[_7SEG_IDX(seg, i)] = va_arg(ap, int);
    }

    va_end(ap);
}

void SevenSegmentDisplay::setSymbol(uint8_t symbol) {
    _value = symbol;
}

void SevenSegmentDisplay::setDigit(uint8_t digit) {
    switch (digit) {
        case 0:
            if (_showZero)
                setSymbol(_7SEG_SYM_0);
            else
                setSymbol(_7SEG_SYM_EMPTY);
            break;
        case 1: setSymbol(_7SEG_SYM_1); break;
        case 2: setSymbol(_7SEG_SYM_2); break;
        case 3: setSymbol(_7SEG_SYM_3); break;
        case 4: setSymbol(_7SEG_SYM_4); break;
        case 5: setSymbol(_7SEG_SYM_5); break;
        case 6: setSymbol(_7SEG_SYM_6); break;
        case 7: setSymbol(_7SEG_SYM_7); break;
        case 8: setSymbol(_7SEG_SYM_8); break;
        case 9: setSymbol(_7SEG_SYM_9); break;
        default: setSymbol(_7SEG_SYM_EMPTY);
    }
}

void SevenSegmentDisplay::setCharacter(char charcter) {
    switch (charcter) {
        case 'a': case 'A': setSymbol(_7SEG_SYM_A); break;
        case 'b': case 'B': setSymbol(_7SEG_SYM_B); break;
        case 'c': case 'C': setSymbol(_7SEG_SYM_C); break;
        case 'd': case 'D': setSymbol(_7SEG_SYM_D); break;
        case 'e': case 'E': setSymbol(_7SEG_SYM_E); break;
        case 'f': case 'F': setSymbol(_7SEG_SYM_F); break;
        case 'g': case 'G': setSymbol(_7SEG_SYM_G); break;
        case 'h': case 'H': setSymbol(_7SEG_SYM_H); break;
        case 'i': case 'I': setSymbol(_7SEG_SYM_I); break;
        case 'j': case 'J': setSymbol(_7SEG_SYM_J); break;
        case 'k': case 'K': setSymbol(_7SEG_SYM_K); break;
        case 'l': case 'L': setSymbol(_7SEG_SYM_L); break;
        case 'm': case 'M': setSymbol(_7SEG_SYM_M); break;
        case 'n': case 'N': setSymbol(_7SEG_SYM_N); break;
        case 'o': case 'O': setSymbol(_7SEG_SYM_O); break;
        case 'p': case 'P': setSymbol(_7SEG_SYM_P); break;
        case 'q': case 'Q': setSymbol(_7SEG_SYM_Q); break;
        case 'r': case 'R': setSymbol(_7SEG_SYM_R); break;
        case 's': case 'S': setSymbol(_7SEG_SYM_S); break;
        case 't': case 'T': setSymbol(_7SEG_SYM_T); break;
        case 'u': case 'U': setSymbol(_7SEG_SYM_U); break;
        case 'v': case 'V': setSymbol(_7SEG_SYM_V); break;
        case 'w': case 'W': setSymbol(_7SEG_SYM_W); break;
        case 'x': case 'X': setSymbol(_7SEG_SYM_X); break;
        case 'y': case 'Y': setSymbol(_7SEG_SYM_Y); break;
        case 'z': case 'Z': setSymbol(_7SEG_SYM_Z); break;
        case '-': setSymbol(_7SEG_SYM_DASH); break;
        case '_': setSymbol(_7SEG_SYM_UNDERSCORE); break;
        case ' ':
        default: setSymbol(_7SEG_SYM_EMPTY);
    }
}

void SevenSegmentDisplay::setShowZero(boolean showZero) {
    _showZero = showZero;
}

SeparatorDisplay::SeparatorDisplay(LedBasedDisplayOutput output):
    _output(output),
    _ledCount(0),
    _mappings(NULL),
    _state(false),
    _mode(LedBasedDisplayMode::SET_ALL_LEDS) {}

SeparatorDisplay::~SeparatorDisplay() {
    delete _mappings;
}

uint8_t SeparatorDisplay::rowCount() {
    uint8_t max = _mappings[0].row;
    for (uint8_t i = 1; i < _ledCount; ++i) {
        if (_mappings[i].row > max) {
            max = _mappings[i].row;
        }
    }
    return max + 1;
}

uint8_t SeparatorDisplay::columnCount() {
    uint8_t max = _mappings[0].column;
    for (uint8_t i = 1; i < _ledCount; ++i) {
        if (_mappings[i].column > max) {
            max = _mappings[i].column;
        }
    }
    return max + 1;
}

uint8_t SeparatorDisplay::indexOfCoords(uint8_t row, uint8_t column) {
    for (uint8_t i = 0; i < _ledCount; ++i) {
        if (_mappings[i].row == row && _mappings[i].column == column) {
            return _mappings[i].index;
        }
    }
    return _7SEG_INDEX_UNDEF;
}

Coords SeparatorDisplay::coordsOfIndex(uint16_t index) {
    for (uint8_t i = 0; i < _ledCount; ++i) {
        if (_mappings[i].index == index) {
            return Coords(_mappings[i].row, _mappings[i].column);
        }
    }
    return Coords();
}

CRGB* SeparatorDisplay::getLedColor(uint8_t row, uint8_t column, bool state) {
    for (uint8_t i = 0; i < _ledCount; ++i) {
        if (_mappings[i].row == row && _mappings[i].column == column) {
            if (state) {
                return &_mappings[i].onColor;
            } else {
                return &_mappings[i].offColor;
            }
        }
    }

    return &dummy;
}

void SeparatorDisplay::setLedColor(uint8_t row, uint8_t column, bool state, CRGB color) {
    for (uint8_t i = 0; i < _ledCount; ++i) {
        if (_mappings[i].row == row && _mappings[i].column == column) {
            if (state) {
                _mappings[i].onColor = color;
            } else {
                _mappings[i].offColor = color;
            }
        }
    }
}

void SeparatorDisplay::update() {
    for (uint8_t i = 0; i < _ledCount; ++i) {
        if (_7SEG_MODE(_mode, _state)) {
            CRGB c = _state ? _mappings[i].onColor : _mappings[i].offColor;
            (*_output)(_mappings[i].index, c.red, c.green, c.blue);
        }
    }
}

void SeparatorDisplay::setMode(LedBasedDisplayMode mode) {
    _mode = mode;
}

void SeparatorDisplay::map(uint8_t ledCount, ...) {
    if (_mappings != NULL) {
        delete _mappings;
    }

    _ledCount = ledCount;
    _mappings = (struct _Mapping*) malloc(ledCount * sizeof(struct _Mapping));

    va_list ap;
    va_start(ap, ledCount);

    for (uint8_t i = 0; i < ledCount; i++) {
        _mappings[i].row = va_arg(ap, int);
        _mappings[i].column = va_arg(ap, int);
        _mappings[i].index = va_arg(ap, int);
    }

    va_end(ap);
}

void SeparatorDisplay::setState(bool state) {
    _state = state;
}

LedBasedRowDisplay::LedBasedRowDisplay(uint8_t displayCount, ...):
    _displayCount(displayCount),
    _displays((LedBasedDisplay**) malloc(displayCount * sizeof(LedBasedDisplay*))) {

    va_list ap;
    va_start(ap, displayCount);

    for (uint8_t i = 0; i < displayCount; i++) {
        _displays[i] = va_arg(ap, LedBasedDisplay*);
    }

    va_end(ap);
}

LedBasedRowDisplay::~LedBasedRowDisplay() {
    delete _displays;
}

uint8_t LedBasedRowDisplay::rowCount() {
    uint8_t rowCount = 0;
    for (uint8_t i = 0; i < _displayCount; i++) {
        rowCount = max(rowCount, _displays[i]->rowCount());
    }
    return rowCount;
}

uint8_t LedBasedRowDisplay::columnCount() {
    uint8_t columnCount = 0;
    for (uint8_t i = 0; i < _displayCount; i++) {
        columnCount += _displays[i]->columnCount();
    }
    return columnCount;
}

uint8_t LedBasedRowDisplay::indexOfCoords(uint8_t row, uint8_t column) {
    uint8_t c = column;
    for (uint8_t i = 0; i < _displayCount; i++) {
        uint8_t cc = _displays[i]->columnCount();
        if (_displays[i]->columnCount() - 1 < c) {
            c -= cc;
        } else {
            return _displays[i]->indexOfCoords(row, c);
        }
    }
    return _7SEG_INDEX_UNDEF;
}

Coords LedBasedRowDisplay::coordsOfIndex(uint16_t index) {
    uint8_t columnCount = 0;
    for (uint8_t i = 0; i < _displayCount; i++) {
        Coords coords = _displays[i]->coordsOfIndex(index);
        if (coords.isValid()) {
            coords.col += columnCount;
            return coords;
        } else {
            columnCount += _displays[i]->columnCount();
        }
    }
    return Coords();
}

CRGB* LedBasedRowDisplay::getLedColor(uint8_t row, uint8_t column, bool state) {
    uint8_t c = column;
    for (uint8_t i = 0; i < _displayCount; i++) {
        uint8_t cc = _displays[i]->columnCount();
        if (_displays[i]->columnCount() - 1 < c) {
            c -= cc;
        } else {
            return _displays[i]->getLedColor(row, c, state);
        }
    }
    return &dummy;
}

void LedBasedRowDisplay::setLedColor(uint8_t row, uint8_t column, bool state, CRGB color) {
    uint8_t c = column;
    for (uint8_t i = 0; i < _displayCount; i++) {
        uint8_t cc = _displays[i]->columnCount();
        if (cc - 1 < c) {
            c -= cc;
        } else {
            _displays[i]->setLedColor(row, c, state, color);
            return;
        }
    }
}

void LedBasedRowDisplay::update() {
    for (uint8_t i = 0; i < _displayCount; i++) {
        _displays[i]->update();
    }
}

void LedBasedRowDisplay::setMode(LedBasedDisplayMode mode) {
    for (uint8_t i = 0; i < _displayCount; i++) {
        _displays[i]->setMode(mode);
    }
}
