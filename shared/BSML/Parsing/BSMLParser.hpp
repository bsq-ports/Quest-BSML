#pragma once

#include "../../_config.h"
#include "../ComponentTypeWithData.hpp"
#include "BSMLParserParams.hpp"
#include "BSMLNode.hpp"

namespace BSML {
    class BSML_EXPORT BSMLParser {
        public:
            BSMLParser();
            ~BSMLParser();

            /// @throws ParseException If the XML is invalid or a tag is unknown.
            /// @note Exceptions propagate to the caller; this method does not render fallback content.
            static std::shared_ptr<BSMLParser> parse(std::string_view str);

            /// @throws ParseException If parsing, attribute conversion, or binding resolution fails.
            /// @note Construction and handler exceptions also propagate to the caller; no fallback content is rendered.
            static std::shared_ptr<BSMLParser> parse_and_construct(std::string_view str, UnityEngine::Transform* parent, System::Object* host);
            void Construct(UnityEngine::Transform* parent, System::Object* host);
            static std::shared_ptr<BSMLParserParams> Construct(const BSMLNode* root, UnityEngine::Transform* parent, System::Object* host);

            std::shared_ptr<BSMLParserParams> parserParams;
            BSMLNode* root;
    };
}
