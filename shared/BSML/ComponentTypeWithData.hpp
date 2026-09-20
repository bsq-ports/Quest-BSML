#pragma once

#include "UnityEngine/Component.hpp"
#include "Parsing/BSMLParserParams.hpp"
#include <string>
#include <map>

namespace BSML {
    class TypeHandlerBase;
    struct BSML_EXPORT ComponentTypeWithData {
        public:
            TypeHandlerBase* typeHandler;
            UnityEngine::Component* component;

            /// @brief One raw string value per XML attribute this tag was
            /// given, keyed by the *canonical* property name a TypeHandler
            /// declares in get_props()
            ///
            /// E.g. an attribute written as `on-click="Foo"` in markup is
            /// looked up here as `data["onClick"]`, because
            /// ButtonHandler::get_props() maps the alias "on-click" to the
            /// canonical key "onClick" (see GetParameters() below, which builds
            /// this map from the tag's raw attributes + that alias table).
            /// 
            /// A value is not necessarily a literal setting: TypeHandlers that parse it directly
            /// (via StringParseHelper) treat it as one, but some (e.g. ButtonHandler's "onClick"/
            /// "clickEvent") instead treat the string as a *key* into BSMLParserParams — the real
            /// callback/event is looked up there via TryGetAction()/GetEvent(), not stored here.
            std::map<std::string, std::string> data;

        static std::map<std::string, std::string> GetParameters(const std::map<std::string, std::string>& allParams, const BSMLParserParams& parserParams, const std::map<std::string, std::vector<std::string>>& props);
    };
}
