#include "BSML/Macros/AsHostMacro.hpp"
#include "BSML/Parsing/BSMLDocParser.hpp"
#include "BSML/Parsing/BSMLParser.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "logging.hpp"

namespace BSML {
    static BSMLNodeParser<AsHostMacro> asHostMacroParser({"macro.as-host"});

    BSMLMacro::PropMap AsHostMacro::get_props() const {
        return {
            {"host", {"host"}},
        };
    }

    void AsHostMacro::Execute(UnityEngine::Transform* parent, const std::map<std::string, std::string>& data, BSMLParserParams& parserParams,  std::vector<std::unique_ptr<ComponentTypeWithData>>& componentInfo) const {
        INFO("Executing ashost macro");
        auto hostItr = data.find("host");
        if (hostItr != data.end()) {
            auto host = parserParams.TryGetValue(hostItr->second);
            if (!host) throw ParseException(fmt::format("Attribute 'host': could not find value '{}'", hostItr->second));
            // A present value may contain null. PC still parses the children,
            // using a fresh parser scope without a host in that case.
            BSMLParser::Construct(this, parent, host->GetValue());
        }
    }
}
