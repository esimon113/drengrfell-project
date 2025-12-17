#include "resultError.h"

namespace df {
    ResultError::ResultError(const Kind kind, std::string text) : kind(kind), text(std::move(text)) {
    }

    std::ostream &operator<<(std::ostream &out, const ResultError &e) {
        std::string kind;
        switch (e.kind) {
            case ResultError::Kind::NullPointer:
                kind = "NullPointer";
                break;
            case ResultError::Kind::InvalidArgument:
                kind = "InvalidArgument";
                break;
            case ResultError::Kind::DomainError:
                kind = "DomainError";
                break;
            case ResultError::Kind::IOError:
                kind = "IOError";
                break;
            case ResultError::Kind::JsonParseError:
                kind = "JsonParseError";
                break;
        }

        out << kind << ": " << e.text;
        return out;
    }
}
