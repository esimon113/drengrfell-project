#pragma once
#include "result.h"
#include <string>

namespace df {
	struct ResultError {
		enum class Kind {
			NullPointer,
			InvalidArgument,
			DomainError,
			IOError,
			JsonParseError,
		};

		ResultError(Kind kind, std::string text);

		Kind kind;
		std::string text;

		friend std::ostream& operator<<(std::ostream& out, const ResultError& e);
	};
} // namespace df
